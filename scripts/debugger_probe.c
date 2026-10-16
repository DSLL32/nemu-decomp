/* Differential host for debugger.dll's public ABI, not recovered Nemu64 code.
 * Build with the tracked VC7 toolchain. The runner supplies a disposable CWD
 * and Wine prefix, so bookmark/configuration and registry writes are isolated.
 */
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>

static BYTE info[244], ram[0x800000], spmem[8192], rom[0x800000];
static DWORD ram_size = sizeof(ram), rom_size = sizeof(rom), pc = 0x80000400;
static DWORD rsp_regs[32], cop0[32], fpu32[32], fcr31, rsp_control[16], mi[16];
static __int64 cpu_regs[32], fpu64[32], accumulator[32], mult_lo, mult_hi;
static WORD rsp_flags[3], vectors[32][8];
static DWORD flags, register_stats[32], opcode_stats[0x220], vi_count = 1;
static DWORD opcode_breaks[2048][2], opcode_break_count, rsp_breaks[1024], rsp_break_count;
static DWORD watches[1024][2], watch_count, breaks[1024][3], break_count;
static DWORD last_pc = 0x80000400, task, undo_position, undo_data[16384];
static DWORD execution_counts[0x200000], cycle_counts[0x200000], rdp_history[2048], rdp_position;
static BYTE translate_error, read_error, show_errors = 1, running;
static BYTE register_stats_enabled, opcode_stats_enabled, undo_enabled, block_stats_enabled;
static HANDLE emulation_thread, thread_stop;
static DWORD WINAPI emulation_stub(void *unused) {
    (void)unused;
    WaitForSingleObject(thread_stop, INFINITE);
    return 0;
}
static HWND owner, found_form, first_checklist;
static const char *wanted_class;
static unsigned child_count, control_hash;

static void hash_bytes(const void *data, unsigned size) {
    const BYTE *p = (const BYTE *)data;
    while (size--) { control_hash ^= *p++; control_hash *= 16777619u; }
}
static void hash_string(const char *s) { hash_bytes(s, (unsigned)strlen(s) + 1); }
static void print_text(const char *s) {
    while (*s) {
        if (*s == '\r') fputs("\\r", stdout);
        else if (*s == '\n') fputs("\\n", stdout);
        else putchar(*s);
        ++s;
    }
}
static DWORD __stdcall translate(DWORD address, BOOL write_access) {
    (void)write_access;
    translate_error = 0;
    return address & 0x1fffffff;
}
static DWORD __stdcall read_word(DWORD address) {
    DWORD physical = address & 0x1fffffff;
    read_error = 0;
    if (physical <= sizeof(ram) - 4) return *(DWORD *)(ram + (physical & ~3u));
    if (physical >= 0x04000000 && physical <= 0x04001ffc)
        return *(DWORD *)(spmem + ((physical - 0x04000000) & ~3u));
    if (physical >= 0x10000000 && physical <= 0x107ffffc)
        return *(DWORD *)(rom + ((physical - 0x10000000) & ~3u));
    return 0; /* Readable synthetic peripheral registers. */
}
static void __stdcall write_word(DWORD address, DWORD value) {
    DWORD physical = address & 0x1fffffff;
    read_error = 0;
    if (physical <= sizeof(ram) - 4) *(DWORD *)(ram + (physical & ~3u)) = value;
    else if (physical >= 0x04000000 && physical <= 0x04001ffc)
        *(DWORD *)(spmem + ((physical - 0x04000000) & ~3u)) = value;
}
static BOOL CALLBACK find_form(HWND window, LPARAM unused) {
    char cls[128];
    (void)unused;
    GetClassNameA(window, cls, sizeof(cls));
    if (strcmp(cls, wanted_class) == 0) { found_form = window; return FALSE; }
    return TRUE;
}
static HWND form(const char *cls) {
    wanted_class = cls; found_form = NULL;
    EnumThreadWindows(GetCurrentThreadId(), find_form, 0);
    return found_form;
}
static BOOL CALLBACK inspect_child(HWND window, LPARAM unused) {
    char cls[128], text[4096];
    LRESULT count, i, state;
    (void)unused;
    ++child_count;
    GetClassNameA(window, cls, sizeof(cls));
    GetWindowTextA(window, text, sizeof(text));
    hash_string(cls); hash_string(text);
    state = IsWindowEnabled(window); hash_bytes(&state, sizeof(state));
    if (strcmp(cls, "TCheckBox") == 0 || strcmp(cls, "TRadioButton") == 0) {
        state = SendMessage(window, BM_GETCHECK, 0, 0); hash_bytes(&state, sizeof(state));
    }
    if (strcmp(cls, "TComboBox") == 0) {
        state = SendMessage(window, CB_GETCURSEL, 0, 0); hash_bytes(&state, sizeof(state));
        count = SendMessage(window, CB_GETCOUNT, 0, 0); hash_bytes(&count, sizeof(count));
        for (i = 0; i < count; ++i)
            if (SendMessage(window, CB_GETLBTEXTLEN, i, 0) < sizeof(text)) {
                SendMessage(window, CB_GETLBTEXT, i, (LPARAM)text); hash_string(text);
            }
    }
    if (strcmp(cls, "TListBox") == 0 || strcmp(cls, "TCheckListBox") == 0) {
        if (!first_checklist && strcmp(cls, "TCheckListBox") == 0) first_checklist = window;
        count = SendMessage(window, LB_GETCOUNT, 0, 0); hash_bytes(&count, sizeof(count));
        for (i = 0; i < count; ++i)
            if (SendMessage(window, LB_GETTEXTLEN, i, 0) < sizeof(text)) {
                SendMessage(window, LB_GETTEXT, i, (LPARAM)text); hash_string(text);
            }
    }
    return TRUE;
}
static void drain(void) {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg); DispatchMessage(&msg);
    }
}
static HWND snapshot(const char *cls) {
    HWND window = form(cls);
    RECT bounds;
    char title[256];
    if (!window) { fprintf(stderr, "Missing form: %s\n", cls); ExitProcess(3); }
    GetWindowTextA(window, title, sizeof(title));
    GetWindowRect(window, &bounds);
    child_count = 0; control_hash = 2166136261u; first_checklist = NULL;
    EnumChildWindows(window, inspect_child, 0);
    printf("Form=%s visible=%d size=%ldx%ld children=%u controls=%08X title=",
           cls, IsWindowVisible(window), bounds.right - bounds.left,
           bounds.bottom - bounds.top, child_count, control_hash);
    print_text(title); putchar('\n');
    return window;
}
static void init_abi(void) {
    unsigned i, j;
#define SLOT(offset, value) (*(DWORD *)(info + (offset)) = (DWORD)(value))
    memset(info, 0, sizeof(info));
    thread_stop = CreateEvent(NULL, TRUE, FALSE, NULL);
    emulation_thread = CreateThread(NULL, 0, emulation_stub, NULL, CREATE_SUSPENDED, NULL);
    owner = CreateWindowA("STATIC", "Debugger probe host", WS_OVERLAPPEDWINDOW,
                         0, 0, 100, 100, NULL, NULL, GetModuleHandle(NULL), NULL);
    for (i = 0; i < 32; ++i) {
        cpu_regs[i] = ((__int64)0x12340000 << 32) | i;
        rsp_regs[i] = 0x1000 + i; cop0[i] = 0x2000 + i;
        *(float *)&fpu32[i] = (float)i * 1.25f;
        *(double *)&fpu64[i] = (double)i * 0.5;
        accumulator[i] = i * 0x123456;
        for (j = 0; j < 8; ++j) vectors[i][j] = (WORD)(i * 8 + j);
    }
    cpu_regs[0] = 0; cpu_regs[29] = 0x80001000;
    rsp_regs[0] = 0; rsp_control[6] = 0x1000;
    *(DWORD *)(ram + 0x400) = 0x01084025; /* or t0,t0,t0 */
    *(DWORD *)(ram + 0x404) = 0x21090001; /* addi t1,t0,1 */
    *(DWORD *)(spmem + 0x1000) = 0x21090001;
    SLOT(0x00, &owner); SLOT(0x04, ram); SLOT(0x08, &ram_size);
    SLOT(0x0C, spmem); SLOT(0x10, rom); SLOT(0x14, &rom_size);
    SLOT(0x1C, "Probe ROM"); SLOT(0x20, "PROBE ROM INTERNAL");
    SLOT(0x24, &pc); SLOT(0x28, cpu_regs); SLOT(0x2C, cop0);
    SLOT(0x30, fpu32); SLOT(0x34, &fcr31); SLOT(0x38, rsp_regs);
    SLOT(0x3C, rsp_flags); SLOT(0x40, accumulator); SLOT(0x44, vectors);
    SLOT(0x48, &mult_lo); SLOT(0x4C, &mult_hi); SLOT(0x54, &flags);
    SLOT(0x58, &emulation_thread); SLOT(0x5C, &register_stats_enabled);
    SLOT(0x60, &opcode_stats_enabled); SLOT(0x64, register_stats);
    SLOT(0x68, opcode_stats); SLOT(0x6C, &vi_count);
    SLOT(0x70, opcode_breaks); SLOT(0x74, &opcode_break_count);
    SLOT(0x78, rsp_breaks); SLOT(0x7C, &rsp_break_count);
    SLOT(0x80, watches); SLOT(0x84, &watch_count); SLOT(0x88, &last_pc);
    SLOT(0x8C, translate); SLOT(0x90, &translate_error); SLOT(0x94, read_word);
    SLOT(0x98, write_word); SLOT(0x9C, &read_error); SLOT(0xA0, &show_errors);
    SLOT(0xA4, breaks); SLOT(0xA8, &break_count); SLOT(0xAC, rsp_control);
    SLOT(0xB0, mi); SLOT(0xB4, "C:\\probe\\test.z64"); SLOT(0xBC, fpu64);
    SLOT(0xC0, &task); SLOT(0xC4, &running); SLOT(0xC8, "12345678-ABCDEF01");
    SLOT(0xD0, &undo_enabled); SLOT(0xD4, undo_data); SLOT(0xD8, &undo_position);
    SLOT(0xDC, execution_counts); SLOT(0xE0, cycle_counts);
    SLOT(0xE4, &block_stats_enabled); SLOT(0xEC, rdp_history); SLOT(0xF0, &rdp_position);
#undef SLOT
}
int main(int argc, char **argv) {
    static const char *forms[] = {"TFormDebug", "TFormMemory", "TFormRegisters",
        "TFormBookmarks", "TFormDebugBreakOpcode", "TFormDebugRegisterChange",
        "TFormDump", "TFormSearch", "TFormStats", "TFormImageView"};
    HMODULE dll;
    int (__cdecl *init)(void *);
    void (__cdecl *tool)(int, int);
    void (__cdecl *menu)(char *);
    void (__cdecl *done)(void);
    char menu_text[1024];
    HWND window, list;
    int i, result;
    if (argc != 2) return 1;
    setvbuf(stdout, NULL, _IONBF, 0);
    init_abi();
    puts("Loading DLL");
    dll = LoadLibraryA(argv[1]);
    if (!dll) { fprintf(stderr, "LoadLibrary: %lu\n", GetLastError()); return 1; }
    init = (void *)GetProcAddress(dll, "ToolInit");
    tool = (void *)GetProcAddress(dll, "ToolFunc");
    menu = (void *)GetProcAddress(dll, "ToolGetMenuText");
    done = (void *)GetProcAddress(dll, "ToolFree");
    if (!init || !tool || !menu || !done) return 2;
    result = init(info); printf("ToolInit=%d\n", result);
    if (result != 399) return 4;
    menu(menu_text); fputs("Menu=", stdout); print_text(menu_text); putchar('\n');
    for (i = 0; i < 10; ++i) {
        printf("ToolFunc(1,%d)\n", i);
        tool(1, i); drain(); window = snapshot(forms[i]);
        SendMessage(window, WM_CLOSE, 0, 0); drain();
    }
    puts("Notifications");
    tool(2, 0); drain(); snapshot(forms[0]);
    tool(2, 1); drain(); snapshot(forms[0]);
    tool(0x80, 0); drain();
    tool(0x100, 0x01084025); tool(4, 0); tool(8, 0); tool(0, 0);
    printf("InitialFlags=%08lX\n", flags);
    puts("Register access breakpoint");
    tool(1, 5); drain(); snapshot(forms[5]); list = first_checklist;
    if (!list) return 5;
    SendMessage(list, LB_SETCURSEL, 8, 0);
    SendMessage(list, WM_CHAR, ' ', 0);
    flags = 0;
    tool(0x100, 0x01084025);
    printf("RegisterBreakFlags=%08lX\n", flags);
    if (!(flags & 0x100000)) return 6;
    SendMessage(list, WM_CHAR, ' ', 0);
    flags = 0; tool(0x100, 0x01084025);
    printf("RegisterClearFlags=%08lX\n", flags);
    if (flags) return 7;
    puts("Opcode breakpoint");
    tool(1, 4); drain(); snapshot(forms[4]); list = first_checklist;
    if (!list) return 8;
    SendMessage(list, LB_SETCURSEL, 0, 0);
    SendMessage(list, WM_CHAR, ' ', 0);
    printf("OpcodeBreakCount=%lu mask=%08lX opcode=%08lX flags=%08lX\n",
           opcode_break_count, opcode_breaks[0][0], opcode_breaks[0][1], flags);
    if (opcode_break_count != 1) return 9;
    SendMessage(list, WM_CHAR, ' ', 0);
    printf("OpcodeClearCount=%lu\n", opcode_break_count);
    if (opcode_break_count != 0) return 10;
    for (i = 0; i < 10; ++i) {
        window = form(forms[i]); if (window) SendMessage(window, WM_CLOSE, 0, 0);
    }
    drain();
    printf("HostState pc=%08lX read_error=%u translate_error=%u show_errors=%u\n",
           pc, read_error, translate_error, show_errors);
    done(); puts("ToolFree completed");
    FreeLibrary(dll);
    SetEvent(thread_stop); ResumeThread(emulation_thread);
    WaitForSingleObject(emulation_thread, 1000);
    CloseHandle(emulation_thread); CloseHandle(thread_stop); DestroyWindow(owner);
    puts("Probe completed");
    return 0;
}
