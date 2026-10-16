/* Differential fixture for gs_dll.dll, not reconstructed application source.
 * Uses only exported functions and normal Win32 dialog messages. The Python
 * runner gives both DLLs the same disposable INI file and Wine environment.
 */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NAMES 1006
#define VALUES 1025
#define ACTIVE 1033
#define NEW_CODE 1011
#define ADD_VALUE 1029
#define PROPERTIES 1010
#define ACTIVATE 1034
#define DEACTIVATE 1035

static unsigned char ram[8192], expected[8192], header[64];
static unsigned control_hash, children;
static HWND found;
static const char *wanted;
static UINT timer;
static int mode, step, ticks;
static void (__cdecl *configure)(void);

static void require(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "Probe assertion: %s\n", message); fflush(stderr); ExitProcess(5);
    }
}
static void hash_bytes(const void *data, unsigned size) {
    const unsigned char *p = (const unsigned char *)data;
    while (size--) { control_hash ^= *p++; control_hash *= 16777619u; }
}
static void hash_text(const char *text) { hash_bytes(text, (unsigned)strlen(text) + 1); }
static unsigned memory_hash(void) {
    control_hash = 2166136261u;
    hash_bytes(ram, sizeof(ram));
    return control_hash;
}
static BOOL CALLBACK find_dialog(HWND window, LPARAM unused) {
    char title[256], cls[128];
    (void)unused;
    GetWindowTextA(window, title, sizeof(title));
    GetClassNameA(window, cls, sizeof(cls));
    if (!strcmp(cls, "#32770") && !strcmp(title, wanted) && IsWindowVisible(window)) {
        found = window; return FALSE;
    }
    return TRUE;
}
static HWND dialog(const char *title) {
    wanted = title; found = NULL;
    EnumThreadWindows(GetCurrentThreadId(), find_dialog, 0);
    return found;
}
static BOOL CALLBACK inspect_child(HWND window, LPARAM unused) {
    char cls[128], text[4096];
    int id, enabled;
    LRESULT count, i, state;
    (void)unused;
    ++children;
    GetClassNameA(window, cls, sizeof(cls));
    GetWindowTextA(window, text, sizeof(text));
    id = GetDlgCtrlID(window); enabled = IsWindowEnabled(window);
    hash_bytes(&id, sizeof(id)); hash_text(cls); hash_text(text);
    hash_bytes(&enabled, sizeof(enabled));
    if (!strcmp(cls, "ListBox")) {
        count = SendMessage(window, LB_GETCOUNT, 0, 0);
        hash_bytes(&count, sizeof(count));
        state = SendMessage(window, LB_GETCURSEL, 0, 0);
        hash_bytes(&state, sizeof(state));
        for (i = 0; i < count; ++i) {
            require(SendMessage(window, LB_GETTEXTLEN, i, 0) < sizeof(text), "list text length");
            SendMessage(window, LB_GETTEXT, i, (LPARAM)text); hash_text(text);
        }
    }
    return TRUE;
}
static void snapshot(HWND window, const char *label) {
    char title[256];
    GetWindowTextA(window, title, sizeof(title));
    control_hash = 2166136261u; children = 0;
    EnumChildWindows(window, inspect_child, 0);
    printf("Dialog=%s title=%s children=%u controls=%08X\n", label, title, children, control_hash);
}
static int count(HWND window, int id) {
    return (int)SendDlgItemMessage(window, id, LB_GETCOUNT, 0, 0);
}
static void choose(HWND window, int id, int index) {
    require(SendDlgItemMessage(window, id, LB_SETCURSEL, index, 0) != LB_ERR, "list selection");
    SendMessage(window, WM_COMMAND, MAKEWPARAM(id, LBN_SELCHANGE), (LPARAM)GetDlgItem(window, id));
}
static void click(HWND window, int id) {
    SendMessage(window, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), (LPARAM)GetDlgItem(window, id));
}
static void post_click(HWND window, int id) {
    PostMessage(window, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), (LPARAM)GetDlgItem(window, id));
}
static void finish_dialog(HWND window, int result) {
    KillTimer(NULL, timer); timer = 0;
    post_click(window, result);
}
static void CALLBACK drive_dialog(HWND unused, UINT message, UINT id, DWORD time) {
    HWND window;
    int i;
    (void)unused; (void)message; (void)id; (void)time;
    require(++ticks < 100, "dialog automation timeout");
    window = dialog("Cheats");
    if (!window) return;
    if (mode == 1) {
        switch (step) {
        case 0:
            require(count(window, NAMES) == 3, "INI-loaded code names");
            require(count(window, ACTIVE) == 0, "initial active list is empty");
            snapshot(window, "loaded");
            for (i = 0; i < 3; ++i) { choose(window, NAMES, i); click(window, ACTIVATE); }
            require(count(window, ACTIVE) == 3, "activate seeded groups");
            choose(window, ACTIVE, 0); click(window, DEACTIVATE);
            require(count(window, ACTIVE) == 2, "deactivate one group");
            choose(window, NAMES, 0); click(window, ACTIVATE);
            require(count(window, ACTIVE) == 3, "reactivate first group");
            snapshot(window, "active");
            step = 1; post_click(window, NEW_CODE); break;
        case 1:
            window = dialog("New Code"); if (!window) return;
            snapshot(window, "new-code");
            SetDlgItemTextA(window, 1019, "Probe Edited");
            step = 2; post_click(window, IDOK); break;
        case 2:
            if (dialog("New Code")) return;
            require(count(window, NAMES) == 4, "new code creation");
            i = (int)SendDlgItemMessage(window, NAMES, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)"Probe Edited");
            require(i != LB_ERR, "new code appears in list");
            choose(window, NAMES, i);
            step = 3; post_click(window, ADD_VALUE); break;
        case 3:
            window = dialog("Add"); if (!window) return;
            snapshot(window, "add-value");
            SetDlgItemTextA(window, 1030, "80000320");
            SetDlgItemTextA(window, 1031, "00DD");
            step = 4; post_click(window, IDOK); break;
        case 4:
            if (dialog("Add")) return;
            require(count(window, VALUES) == 1, "new subcode creation");
            choose(window, VALUES, 0);
            step = 5; post_click(window, PROPERTIES); break;
        case 5:
            window = dialog("Code Properties"); if (!window) return;
            snapshot(window, "properties");
            SetDlgItemTextA(window, 1016, "00DE");
            step = 6; post_click(window, IDOK); break;
        case 6:
            if (dialog("Code Properties")) return;
            click(window, ACTIVATE);
            require(count(window, ACTIVE) == 4, "activate edited code");
            snapshot(window, "saved");
            step = 7; finish_dialog(window, IDOK); break;
        }
    } else if (mode == 2) {
        require(count(window, NAMES) == 4, "saved INI code names reload");
        require(count(window, ACTIVE) == 4, "active names survive dialog reopen");
        snapshot(window, "reopened");
        for (i = 0; i < 4; ++i) { choose(window, ACTIVE, 0); click(window, DEACTIVATE); }
        require(count(window, ACTIVE) == 0, "deactivate every group");
        snapshot(window, "inactive");
        step = 1; finish_dialog(window, IDCANCEL);
    } else if (mode == 3) {
        require(count(window, NAMES) == 0, "different byte-swapped ROM name selects empty section");
        require(count(window, ACTIVE) == 0, "reinitialize clears active names");
        snapshot(window, "different-rom");
        step = 1; finish_dialog(window, IDCANCEL);
    }
}
static void run_dialog(int scenario) {
    mode = scenario; step = ticks = 0;
    timer = SetTimer(NULL, 0, 100, drive_dialog);
    require(timer != 0, "create automation timer");
    configure();
    require(timer == 0 && step == (scenario == 1 ? 7 : 1), "modal dialog completed");
}
static void make_header(const char *name) {
    unsigned i;
    memset(header, 0, sizeof(header));
    for (i = 0; i < 24; ++i) header[32 + (i ^ 3)] = ' ';
    for (i = 0; name[i] && i < 24; ++i) header[32 + (i ^ 3)] = name[i];
}
static void byte_at(unsigned char *memory, unsigned address, unsigned value) {
    memory[address ^ 3] = (unsigned char)value;
}
static void word_at(unsigned char *memory, unsigned address, unsigned value) {
    *(unsigned short *)(memory + (address ^ 2)) = (unsigned short)value;
}
static void make_ram(void) {
    memset(ram, 0x5A, sizeof(ram));
    byte_at(ram, 0x200, 0x22); byte_at(ram, 0x201, 0x44);
    word_at(ram, 0x204, 0x6677); word_at(ram, 0x206, 0xAABB);
    byte_at(ram, 0x208, 0x55); byte_at(ram, 0x209, 0x77);
    word_at(ram, 0x20C, 0x1234); word_at(ram, 0x20E, 0x8765);
    memcpy(expected, ram, sizeof(ram));
}
static void expect_update(void) {
    byte_at(expected, 0x100, 0xAB); byte_at(expected, 0x101, 0xCD);
    word_at(expected, 0x104, 0x1234); word_at(expected, 0x106, 0xBEEF);
    byte_at(expected, 0x300, 0xA1); word_at(expected, 0x304, 0x5566);
    byte_at(expected, 0x309, 0xB2); word_at(expected, 0x30E, 0xABCD);
    byte_at(expected, 0x310, 0xC1); byte_at(expected, 0x320, 0xDE);
}
static void check_ram(const char *label) {
    unsigned i;
    printf("RAM=%s hash=%08X\n", label, memory_hash());
    for (i = 0; i < sizeof(ram); ++i) if (ram[i] != expected[i]) {
        fprintf(stderr, "RAM mismatch at physical offset %04X: %02X != %02X\n", i, ram[i], expected[i]);
        fflush(stderr);
        ExitProcess(6);
    }
}
int main(int argc, char **argv) {
    HMODULE library;
    void (__cdecl *initialize)(char *, char *, char *);
    void (__cdecl *update)(void);
    char ini[MAX_PATH];
    require(argc == 2, "DLL argument");
    setvbuf(stdout, NULL, _IONBF, 0);
    GetFullPathNameA("probe.ini", sizeof(ini), ini, NULL);
    library = LoadLibraryA(argv[1]);
    if (!library) { fprintf(stderr, "LoadLibrary failed: %lu\n", GetLastError()); return 2; }
    initialize = (void *)GetProcAddress(library, "?GSInitialize@@YAXPAD00@Z");
    configure = (void *)GetProcAddress(library, "?GSConfigure@@YAXXZ");
    update = (void *)GetProcAddress(library, "?GSUpdate@@YAXXZ");
    require(initialize && configure && update, "resolve all three GameShark exports");
    puts("Exports resolved");
    update(); configure();
    puts("Uninitialized exports return");
    make_header("GS PROBE"); make_ram();
    initialize((char *)header, (char *)ram, ini);
    update(); check_ram("initialized-inactive");
    run_dialog(1);
    expect_update(); update(); check_ram("all-code-types");
    byte_at(ram, 0x100, 0); byte_at(ram, 0x320, 0);
    update(); check_ram("reapplied");
    run_dialog(2);
    memset(ram, 0x69, sizeof(ram)); memcpy(expected, ram, sizeof(ram));
    update(); check_ram("deactivated");
    make_header("UNLISTED ROM"); initialize((char *)header, (char *)ram, ini);
    update(); check_ram("reinitialized");
    run_dialog(3);
    require(FreeLibrary(library), "DLL unload");
    puts("Probe completed");
    return 0;
}
