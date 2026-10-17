/* External UI smoke fixture, not reconstructed emulator source.
 * Launches one frozen executable, inspects ordinary Win32 controls, and closes
 * dialogs through their normal commands. The separate read-only thread hook
 * inspects Wine's process-local menu state. No ROM or application patch.
 */
#define WINVER 0x0500
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

static PROCESS_INFORMATION process;
static const char *wanted_class, *wanted_title;
static HWND found;
static unsigned child_count;
#define SNAPSHOT_MESSAGE (WM_APP + 0x317)

static void text(const char *s) {
    const unsigned char *p = (const unsigned char *)s;
    putchar('"');
    while (*p) {
        if (*p == '\\' || *p == '"') printf("\\%c", *p);
        else if (*p < 32 || *p >= 127) printf("\\x%02X", *p);
        else putchar(*p);
        ++p;
    }
    putchar('"');
}

static BOOL belongs(HWND window) {
    DWORD id;
    GetWindowThreadProcessId(window, &id);
    return id == process.dwProcessId;
}

static BOOL CALLBACK find_window(HWND window, LPARAM unused) {
    char cls[128], title[1024];
    (void)unused;
    if (!belongs(window) || !IsWindowVisible(window)) return TRUE;
    GetClassNameA(window, cls, sizeof(cls));
    GetWindowTextA(window, title, sizeof(title));
    if ((!wanted_class || !strcmp(wanted_class, cls)) &&
        (!wanted_title || !strcmp(wanted_title, title))) {
        found = window;
        return FALSE;
    }
    return TRUE;
}

static HWND find(const char *cls, const char *title) {
    wanted_class = cls; wanted_title = title; found = NULL;
    EnumWindows(find_window, 0);
    return found;
}

static BOOL CALLBACK dump_window(HWND window, LPARAM child) {
    char cls[128], title[4096];
    if (!child && !belongs(window)) return TRUE;
    GetClassNameA(window, cls, sizeof(cls));
    GetWindowTextA(window, title, sizeof(title));
    printf("Diagnostic %s visible=%d enabled=%d class=", child ? "child" : "window",
           IsWindowVisible(window), IsWindowEnabled(window));
    text(cls); printf(" title="); text(title); putchar('\n');
    if (!child) EnumChildWindows(window, dump_window, 1);
    return TRUE;
}

static void fail(const char *message) {
    DWORD code;
    fprintf(stderr, "Probe assertion: %s (Win32 error %lu)\n", message, GetLastError());
    EnumWindows(dump_window, 0);
    if (process.hProcess) {
        GetExitCodeProcess(process.hProcess, &code);
        printf("Application exit status at failure=%lu\n", code);
        if (code == STILL_ACTIVE) {
            TerminateProcess(process.hProcess, 91);
            WaitForSingleObject(process.hProcess, 3000);
        }
    }
    fflush(stdout); fflush(stderr);
    ExitProcess(5);
}

static void require(int condition, const char *message) {
    if (!condition) fail(message);
}

static LRESULT message(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
    DWORD result = 0;
    require(SendMessageTimeoutA(window, msg, wparam, lparam,
                SMTO_ABORTIFHUNG | SMTO_BLOCK, 2000, &result) != 0,
            "responsive window message");
    return (LRESULT)result;
}

static HWND await_window(const char *cls, const char *title) {
    DWORD deadline = GetTickCount() + 20000;
    HWND window;
    while ((LONG)(deadline - GetTickCount()) > 0) {
        window = find(cls, title);
        if (window) {
            message(window, WM_NULL, 0, 0);
            return window;
        }
        require(WaitForSingleObject(process.hProcess, 0) == WAIT_TIMEOUT,
                "application stays running until the requested window opens");
        Sleep(100);
    }
    fprintf(stderr, "Missing window class=%s title=%s\n",
            cls ? cls : "*", title ? title : "*");
    fail("window appeared before timeout");
    return NULL;
}

static void await_closed(HWND window) {
    DWORD deadline = GetTickCount() + 10000;
    while (IsWindow(window) && IsWindowVisible(window) &&
           (LONG)(deadline - GetTickCount()) > 0) Sleep(50);
    require(!IsWindow(window) || !IsWindowVisible(window), "dialog closed normally");
}

static void inspect_menu(HWND window) {
    FILE *file;
    char line[4096];
    int completed = 0;
    DeleteFileA("nemu-menu.txt");
    message(window, SNAPSHOT_MESSAGE, 0, 0);
    file = fopen("nemu-menu.txt", "r");
    require(file != NULL, "menu hook wrote snapshot");
    while (fgets(line, sizeof(line), file)) {
        if (!strcmp(line, "Menu snapshot completed\n")) completed = 1;
        fputs(line, stdout);
    }
    fclose(file);
    require(completed, "read runtime menu items");
}

static BOOL CALLBACK inspect_child(HWND window, LPARAM unused) {
    char cls[128], value[8192];
    LRESULT count, i, length;
    RECT bounds;
    (void)unused;
    GetClassNameA(window, cls, sizeof(cls));
    memset(value, 0, sizeof(value));
    message(window, WM_GETTEXT, sizeof(value), (LPARAM)value);
    GetClientRect(window, &bounds);
    printf("Control index=%u class=", child_count++); text(cls);
    printf(" visible=%d enabled=%d size=%ldx%ld text=", IsWindowVisible(window),
           IsWindowEnabled(window), bounds.right, bounds.bottom);
    text(value);
    /* VCL control IDs can be HWNDs, so they are not comparison inputs. */
    if (!strcmp(cls, "Button") || !strcmp(cls, "TCheckBox") || !strcmp(cls, "TRadioButton"))
        printf(" checked=%ld", message(window, BM_GETCHECK, 0, 0));
    if (!strcmp(cls, "SysListView32"))
        printf(" items=%ld", message(window, LVM_GETITEMCOUNT, 0, 0));
    if (!strcmp(cls, "ToolbarWindow32"))
        printf(" buttons=%ld", message(window, TB_BUTTONCOUNT, 0, 0));
    if (!strcmp(cls, "TPageControl"))
        printf(" tabs=%ld selected=%ld", message(window, TCM_GETITEMCOUNT, 0, 0),
               message(window, TCM_GETCURSEL, 0, 0));
    putchar('\n');
    if (!strcmp(cls, "TComboBox") || !strcmp(cls, "ComboBox")) {
        WCHAR wide[8192];
        DWORD copied;
        count = message(window, CB_GETCOUNT, 0, 0);
        printf("Combo count=%ld selected=%ld\n", count, message(window, CB_GETCURSEL, 0, 0));
        require(count >= 0 && count < 100, "bounded combo contents");
        for (i = 0; i < count; ++i) {
            length = message(window, CB_GETLBTEXTLEN, i, 0);
            require(length >= 0 && length < sizeof(value), "bounded combo text");
            /* Wine marshals cross-process list text as UTF-16. Use its Unicode
             * interface explicitly instead of treating that buffer as ANSI. */
            memset(wide, 0, sizeof(wide));
            require(SendMessageTimeoutW(window, CB_GETLBTEXT, i, (LPARAM)wide,
                        SMTO_ABORTIFHUNG | SMTO_BLOCK, 2000, &copied) != 0,
                    "read complete combo item text");
            require(copied == (DWORD)length, "complete combo item length");
            require(WideCharToMultiByte(CP_UTF8, 0, wide, -1, value, sizeof(value),
                        NULL, NULL) != 0, "convert combo item text");
            printf("Combo item=%ld text=", i); text(value); putchar('\n');
        }
    }
    return TRUE;
}

static void snapshot(HWND window, const char *label, int menu) {
    char title[1024], cls[128];
    RECT bounds;
    message(window, WM_NULL, 0, 0);
    GetClassNameA(window, cls, sizeof(cls));
    GetWindowTextA(window, title, sizeof(title));
    GetClientRect(window, &bounds);
    printf("Snapshot=%s class=", label); text(cls); printf(" title="); text(title);
    printf(" size=%ldx%ld enabled=%d\n", bounds.right, bounds.bottom, IsWindowEnabled(window));
    child_count = 0;
    EnumChildWindows(window, inspect_child, 0);
    printf("Children=%u\n", child_count);
    if (menu) inspect_menu(window);
    fflush(stdout);
}

static void click(HWND window, int id) {
    require(PostMessageA(window, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED),
                         (LPARAM)GetDlgItem(window, id)), "post dialog button command");
}

int main(int argc, char **argv) {
    STARTUPINFOA startup;
    HWND main_window, about, disclaimer, settings;
    DWORD exit_code;
    char command[MAX_PATH + 4], hook_path[MAX_PATH];
    HMODULE hook_module;
    HHOOK hook;
    char *filename;
    require(argc == 2 && strlen(argv[1]) < MAX_PATH, "one executable path argument");
    memset(&startup, 0, sizeof(startup)); startup.cb = sizeof(startup);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    sprintf(command, "\"%s\"", argv[1]);
    require(CreateProcessA(argv[1], command, NULL, NULL, FALSE, 0, NULL, NULL,
                           &startup, &process), "launch application");
    WaitForInputIdle(process.hProcess, 20000);
    main_window = await_window("Nemu64", NULL);
    Sleep(500);
    require(GetModuleFileNameA(NULL, hook_path, sizeof(hook_path)), "locate menu probe DLL");
    filename = strrchr(hook_path, '\\'); require(filename != NULL, "probe executable path");
    strcpy(filename + 1, "nemu_menu_probe.dll");
    hook_module = LoadLibraryA(hook_path); require(hook_module != NULL, "load menu probe DLL");
    hook = SetWindowsHookExA(WH_CALLWNDPROC, (HOOKPROC)GetProcAddress(hook_module, "MenuProbeHook"),
                hook_module, GetWindowThreadProcessId(main_window, NULL));
    require(hook != NULL, "install read-only menu probe hook");
    snapshot(main_window, "main-initial", 1);
    require(IsWindowEnabled(main_window), "startup has no blocking dialog");

    require(PostMessageA(main_window, WM_COMMAND, 104, 0), "open About from main menu");
    about = await_window("#32770", "About");
    snapshot(about, "about", 0);
    click(about, 1005);
    disclaimer = await_window("#32770", "Disclaimer");
    snapshot(disclaimer, "disclaimer", 0);
    click(disclaimer, IDOK); await_closed(disclaimer);
    click(about, IDOK); await_closed(about);

    require(PostMessageA(main_window, WM_COMMAND, 32780, 0), "open Settings from main menu");
    settings = await_window("TFormNemuSettings", "Nemu Settings");
    snapshot(settings, "settings", 0);
    require(PostMessageA(settings, WM_CLOSE, 0, 0), "cancel Settings normally");
    await_closed(settings);
    message(main_window, WM_NULL, 0, 0);
    snapshot(main_window, "main-after-dialogs", 1);
    require(IsWindowEnabled(main_window), "main window enabled after closing dialogs");
    require(UnhookWindowsHookEx(hook), "remove menu probe hook");
    FreeLibrary(hook_module);
    require(PostMessageA(main_window, WM_CLOSE, 0, 0), "request normal shutdown");
    require(WaitForSingleObject(process.hProcess, 15000) == WAIT_OBJECT_0,
            "application terminates normally");
    require(GetExitCodeProcess(process.hProcess, &exit_code), "read application exit code");
    printf("Application normal exit=%lu\n", exit_code);
    require(exit_code == 0, "normal application exit status");
    CloseHandle(process.hThread); CloseHandle(process.hProcess);
    puts("Probe completed");
    return 0;
}
