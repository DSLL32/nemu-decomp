/* Differential host for the original and reconstructed dialogs DLLs.
 * Compiles with the tracked VC7 toolchain. All INI writes belong in scratch.
 * Exercises the exported ABI, form creation, native controls, and Apply.
 * This is a test host, not recovered Nemu64 source.
 */
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    DWORD size;
    WORD version, type;
    char name[100];
    BOOL normal_memory, byteswapped_memory;
    char filename[MAX_PATH];
} Plugin;

static const char *expected_form;
static int edit_number, apply_changes, ticks, seen;
static HWND apply_button;

static void print_text(const char *text) {
    const unsigned char *p = (const unsigned char *)text;
    for (; *p; ++p) {
        if (*p == '\r') fputs("\\r", stdout);
        else if (*p == '\n') fputs("\\n", stdout);
        else putchar(*p);
    }
}

static BOOL CALLBACK visit_child(HWND hwnd, LPARAM unused) {
    char cls[128], value[2048], replacement[128];
    LRESULT count, i;
    (void)unused;
    GetClassNameA(hwnd, cls, sizeof(cls));
    GetWindowTextA(hwnd, value, sizeof(value));
    printf("  %s enabled=%d text=", cls, IsWindowEnabled(hwnd));
    print_text(value);
    putchar('\n');
    if (strcmp(cls, "TComboBox") == 0) {
        count = SendMessage(hwnd, CB_GETCOUNT, 0, 0);
        printf("    selected=%ld count=%ld\n", SendMessage(hwnd, CB_GETCURSEL, 0, 0), count);
        for (i = 0; i < count; ++i) {
            if (SendMessage(hwnd, CB_GETLBTEXTLEN, i, 0) < sizeof(value)) {
                SendMessage(hwnd, CB_GETLBTEXT, i, (LPARAM)value);
                fputs("    item=", stdout); print_text(value); putchar('\n');
            }
        }
        if (apply_changes) SendMessage(hwnd, CB_SETCURSEL, count > 2 ? 1 : 0, 0);
    }
    if (apply_changes && strcmp(cls, "TEdit") == 0) {
        sprintf(replacement, "C:\\probe\\folder%d\\", ++edit_number);
        SetWindowTextA(hwnd, replacement);
    }
    if (apply_changes && strcmp(cls, "TMemo") == 0) {
        sprintf(replacement, "Probe%d=changed\r\nignored line\r\n", ++edit_number);
        SetWindowTextA(hwnd, replacement);
    }
    if (strcmp(cls, "TButton") == 0 && (strcmp(value, "Apply") == 0 ||
                                       strcmp(value, "&Apply") == 0))
        apply_button = hwnd;
    return TRUE;
}

static BOOL CALLBACK activate_folders(HWND hwnd, LPARAM unused) {
    char cls[128];
    NMHDR notification;
    (void)unused;
    GetClassNameA(hwnd, cls, sizeof(cls));
    if (strcmp(cls, "TPageControl") != 0) return TRUE;
    SendMessage(hwnd, TCM_SETCURSEL, 1, 0);
    notification.hwndFrom = hwnd;
    notification.idFrom = GetDlgCtrlID(hwnd);
    notification.code = TCN_SELCHANGE;
    SendMessage(GetParent(hwnd), WM_NOTIFY, notification.idFrom, (LPARAM)&notification);
    return TRUE;
}

static BOOL CALLBACK visit_form(HWND hwnd, LPARAM unused) {
    char cls[128];
    (void)unused;
    GetClassNameA(hwnd, cls, sizeof(cls));
    if (strcmp(cls, expected_form) != 0) return TRUE;
    ++seen;
    if (apply_changes && strcmp(cls, "TFormNemuSettings") == 0)
        EnumChildWindows(hwnd, activate_folders, 0);
    EnumChildWindows(hwnd, visit_child, 0);
    if (apply_changes) {
        if (!apply_button) { fputs("Missing Apply button\n", stderr); ExitProcess(2); }
        SendMessage(apply_button, BM_CLICK, 0, 0);
    }
    PostMessage(hwnd, WM_CLOSE, 0, 0);
    return FALSE;
}

static VOID CALLBACK timer(HWND window, UINT message, UINT_PTR id, DWORD time) {
    (void)window; (void)message; (void)time;
    if (++ticks > 40) { fputs("Timed out waiting for form\n", stderr); ExitProcess(3); }
    EnumThreadWindows(GetCurrentThreadId(), visit_form, 0);
    if (seen) KillTimer(NULL, id);
}

static void prepare(const char *form, int write) {
    expected_form = form;
    apply_changes = write;
    edit_number = ticks = seen = 0;
    apply_button = NULL;
    if (!SetTimer(NULL, 0, 100, timer)) ExitProcess(4);
}

static void drain(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    if (!seen) ExitProcess(5);
}

static DWORD __stdcall read_word(DWORD address) {
    switch (address) {
    case 0x10000008: return 0x80000400;
    case 0x10000010: return 0x12345678;
    case 0x10000014: return 0xABCDEF01;
    case 0x10000038: return 'N';
    case 0x1000003C: return 0x41424500;
    default: return 0;
    }
}

int main(int argc, char **argv) {
    HMODULE dll;
    int (__cdecl *init)(void *);
    void (__cdecl *tool)(int, int);
    void (__cdecl *menu)(char *);
    void (__cdecl *settings)(int, Plugin *);
    void (__cdecl *done)(void);
    BYTE info[244], read_error = 0, show_errors = 1, running = 0;
    DWORD rom_size = 0, breakpoint_count = 0;
    HWND owner = NULL;
    Plugin plugins[3];
    char menu_text[256];
    int i;
    if (argc != 2) return 1;
    setvbuf(stdout, NULL, _IONBF, 0);
    puts("Loading DLL");
    dll = LoadLibraryA(argv[1]);
    if (!dll) { fprintf(stderr, "LoadLibrary: %lu\n", GetLastError()); return 1; }
    init = (void *)GetProcAddress(dll, "ToolInit");
    tool = (void *)GetProcAddress(dll, "ToolFunc");
    menu = (void *)GetProcAddress(dll, "ToolGetMenuText");
    settings = (void *)GetProcAddress(dll, "NemuSettings");
    done = (void *)GetProcAddress(dll, "ToolFree");
    if (!init || !tool || !menu || !settings || !done) return 1;
    memset(info, 0, sizeof(info));
#define SLOT(offset, value) (*(DWORD *)(info + (offset)) = (DWORD)(value))
    SLOT(0x00, &owner); SLOT(0x04, GetModuleHandle(NULL));
    SLOT(0x14, &rom_size); SLOT(0x1C, "Probe ROM");
    SLOT(0x94, read_word); SLOT(0x9C, &read_error); SLOT(0xA0, &show_errors);
    SLOT(0xA8, &breakpoint_count); SLOT(0xB4, "C:\\probe\\test.z64");
    SLOT(0xC4, &running); SLOT(0xC8, "12345678-ABCDEF01");
    printf("ToolInit=%d\n", init(info));
    menu(menu_text); fputs("Menu=", stdout); print_text(menu_text); putchar('\n');
    puts("Header without ROM"); prepare("TFormHeader", 0); tool(1, 0); drain();
    rom_size = 0x800000;
    puts("Header with ROM"); prepare("TFormHeader", 0); tool(1, 0); drain();
    memset(plugins, 0, sizeof(plugins));
    for (i = 0; i < 3; ++i) {
        plugins[i].size = sizeof(Plugin); plugins[i].version = 0x0103;
        plugins[i].type = i == 0 ? 3 : 2;
        sprintf(plugins[i].name, "Plugin %d", i);
        sprintf(plugins[i].filename, "plugin%d.dll", i);
    }
    puts("Settings while stopped"); prepare("TFormNemuSettings", 0); settings(3, plugins); drain();
    running = 1;
    puts("Settings while running, Apply"); prepare("TFormNemuSettings", 1); settings(3, plugins); drain();
    puts("ROM settings, Apply"); prepare("TFormRomSettings", 1); tool(1, 1); drain();
    done();
    FreeLibrary(dll);
    puts("Probe completed");
    return 0;
}
