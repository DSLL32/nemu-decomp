/* Self-contained XP-era crash reporting. No DbgHelp, PDB, symbol server or
   sidecar files. The normal user-build linker embeds a checked symbol index. */
#define _WIN32_WINNT 0x0501
#include "StaticCrash.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#pragma pack(push, 1)
struct SymbolHeader { char magic[8]; DWORD version, count, strings, stringSize; char build[16]; };
struct SymbolEntry { DWORD begin, end, name; };
#pragma pack(pop)
struct Capture {
    EXCEPTION_RECORD exception;
    CONTEXT context;
    DWORD thread, stackLow, stackHigh, savedStart, savedSize;
    bool snapshot;
    BYTE stack[16384];
};
static __declspec(thread) Capture firstChance;
static __declspec(thread) bool firstChanceValid;
static Capture crash;
static char report[65536], delphiType[128], delphiMessage[1024];
static unsigned reportLength;
static HINSTANCE application;
static char executableName[MAX_PATH];
static HANDLE requestEvent, reporterThread;
static DWORD reporterId;
static volatile LONG reporting, stopping;
static LPTOP_LEVEL_EXCEPTION_FILTER previousFilter;
static PVOID vectoredHandle;
typedef PVOID (WINAPI *AddHandler)(ULONG, PVECTORED_EXCEPTION_HANDLER);
typedef ULONG (WINAPI *RemoveHandler)(PVOID);
static RemoveHandler removeHandler;
static NemuStaticDescribeCode describeCode;
static const SymbolHeader *symbols;
static DWORD symbolSize;
static bool details;
static HWND detailEdit, detailButton, copyButton;
enum { ID_DETAILS = 1001, ID_COPY = 1002, ID_REPORT = 1003 };

static bool readMemory(DWORD address, void *output, unsigned size)
{
    SIZE_T read = 0;
    return address && size && ReadProcessMemory(GetCurrentProcess(), (void *)address, output, size, &read) && read == size;
}
static void bounds(Capture *capture)
{
    DWORD stackLower, stackUpper;
    __asm {
        mov eax, fs:[4]
        mov stackUpper, eax
        mov eax, fs:[8]
        mov stackLower, eax
    }
    capture->stackLow = stackLower; capture->stackHigh = stackUpper;
}
static void capture(Capture *target, EXCEPTION_POINTERS *exception, bool snapshot)
{
    target->exception = *exception->ExceptionRecord;
    target->exception.ExceptionRecord = NULL;
    target->context = *exception->ContextRecord;
    target->thread = GetCurrentThreadId();
    bounds(target); target->snapshot = snapshot;
    target->savedStart = target->context.Esp; target->savedSize = 0;
    if (snapshot && target->savedStart >= target->stackLow && target->savedStart < target->stackHigh) {
        DWORD size = target->stackHigh-target->savedStart;
        if (size > sizeof(target->stack)) size = sizeof(target->stack);
        SIZE_T read = 0;
        if (ReadProcessMemory(GetCurrentProcess(), (void *)target->savedStart, target->stack, size, &read))
            target->savedSize = (DWORD)read;
    }
}
static bool fatalCode(DWORD code)
{
    return code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_ILLEGAL_INSTRUCTION ||
        code == EXCEPTION_PRIV_INSTRUCTION || code == EXCEPTION_STACK_OVERFLOW ||
        code == EXCEPTION_INT_DIVIDE_BY_ZERO || code == EXCEPTION_INT_OVERFLOW ||
        (code >= EXCEPTION_FLT_DENORMAL_OPERAND && code <= EXCEPTION_FLT_UNDERFLOW) ||
        code == EXCEPTION_IN_PAGE_ERROR || code == EXCEPTION_ARRAY_BOUNDS_EXCEEDED;
}
static LONG CALLBACK observe(EXCEPTION_POINTERS *exception)
{
    // A first-chance observer records context only. It never diagnoses a
    // handled exception as a crash. Ordinary dynrec memory faults are frequent.
    if (!reporting && (fatalCode(exception->ExceptionRecord->ExceptionCode) ||
        exception->ExceptionRecord->ExceptionCode == 0x0eedfade)) {
        if (exception->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || !describeCode ||
            !describeCode((DWORD)exception->ExceptionRecord->ExceptionAddress, NULL, 0)) {
            capture(&firstChance, exception, true); firstChanceValid = true;
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
static bool readStack(DWORD address, void *output, unsigned size)
{
    if (address < crash.stackLow || address > crash.stackHigh || size > crash.stackHigh-address) return false;
    if (crash.snapshot) {
        if (address < crash.savedStart || address-crash.savedStart > crash.savedSize ||
            size > crash.savedSize-(address-crash.savedStart)) return false;
        memcpy(output, crash.stack+(address-crash.savedStart), size); return true;
    }
    return readMemory(address, output, size);
}
static void append(const char *format, ...)
{
    if (reportLength >= sizeof(report)-1) return;
    va_list args; va_start(args, format);
    unsigned available = sizeof(report)-reportLength;
    int n = _vsnprintf(report+reportLength, available-1, format, args);
    va_end(args);
    if (n < 0 || (unsigned)n >= available-1) reportLength = sizeof(report)-1;
    else reportLength += n;
    report[reportLength] = 0;
}
static bool executable(DWORD address)
{
    MEMORY_BASIC_INFORMATION info;
    if (!VirtualQuery((void *)address, &info, sizeof(info)) || info.State != MEM_COMMIT ||
        (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
    return (info.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
}
static const char *localSymbol(DWORD address, DWORD *displacement)
{
    DWORD base = (DWORD)application;
    if (!symbols || address < base) return NULL;
    DWORD rva = address-base, low = 0, high = symbols->count;
    const SymbolEntry *entries = (const SymbolEntry *)(symbols+1);
    while (low < high) { DWORD mid = low+(high-low)/2; if (entries[mid].begin <= rva) low = mid+1; else high = mid; }
    if (!low) return NULL;
    const SymbolEntry &entry = entries[low-1];
    if (rva >= entry.end || entry.name >= symbols->stringSize) return NULL;
    *displacement = rva-entry.begin;
    return (const char *)symbols+symbols->strings+entry.name;
}
static bool module(DWORD address, DWORD *base, char *name, unsigned size)
{
    MEMORY_BASIC_INFORMATION info;
    if (!VirtualQuery((void *)address, &info, sizeof(info)) || info.Type != MEM_IMAGE) return false;
    *base = (DWORD)info.AllocationBase;
    char path[MAX_PATH];
    if (!GetModuleFileNameA((HMODULE)*base, path, sizeof(path))) return false;
    path[sizeof(path)-1] = 0;
    const char *leaf = strrchr(path, '\\');
    lstrcpynA(name, leaf ? leaf+1 : path, size); return true;
}
static bool exported(DWORD base, DWORD address, char *name, unsigned size, DWORD *offset)
{
    // Export names are useful for loose plugins without debug files. An
    // interior address is explicitly described as a nearest-export estimate.
    IMAGE_DOS_HEADER dos; IMAGE_NT_HEADERS nt; IMAGE_EXPORT_DIRECTORY dir;
    if (!readMemory(base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
        dos.e_lfanew < 0 || dos.e_lfanew > 0x100000 ||
        !readMemory(base+dos.e_lfanew, &nt, sizeof(nt)) || nt.Signature != IMAGE_NT_SIGNATURE) return false;
    IMAGE_DATA_DIRECTORY data = nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (!data.VirtualAddress || data.VirtualAddress >= nt.OptionalHeader.SizeOfImage ||
        !readMemory(base+data.VirtualAddress, &dir, sizeof(dir)) || dir.NumberOfNames > 65536) return false;
    DWORD best = 0, bestName = 0;
    for (DWORD i = 0; i < dir.NumberOfNames; ++i) {
        DWORD text, function; WORD ordinal;
        if (!readMemory(base+dir.AddressOfNames+i*4, &text, 4) ||
            !readMemory(base+dir.AddressOfNameOrdinals+i*2, &ordinal, 2) || ordinal >= dir.NumberOfFunctions ||
            !readMemory(base+dir.AddressOfFunctions+ordinal*4, &function, 4)) break;
        if (function >= data.VirtualAddress && function-data.VirtualAddress < data.Size) continue;
        if (function <= address-base && function > best && executable(base+function)) { best = function; bestName = text; }
    }
    if (!bestName) return false;
    unsigned character;
    for (character = 0; character+1 < size; ++character) {
        if (!readMemory(base+bestName+character, name+character, 1)) return false;
        if (!name[character]) break;
    }
    name[character] = 0; *offset = address-base-best; return true;
}
static void location(DWORD address, bool returnAddress)
{
    // A return address can land exactly at the next function. Look up its
    // preceding call instruction, but always print the original raw address.
    DWORD lookup = address-(returnAddress && address ? 1 : 0), displacement = 0, base = 0;
    char generated[256], name[MAX_PATH];
    const char *symbol = localSymbol(lookup, &displacement);
    if (describeCode && describeCode(lookup, generated, sizeof(generated)))
        append("%08lX  %s", address, generated);
    else if (symbol)
        append("%08lX  %s!%s + 0x%lX", address, executableName, symbol, displacement+(returnAddress ? 1 : 0));
    else if (module(lookup, &base, name, sizeof(name))) {
        append("%08lX  %s + 0x%lX", address, name, address-base);
        if (base != (DWORD)application) {
            char exportName[256];
            if (exported(base, lookup, exportName, sizeof(exportName), &displacement))
                append("  [nearest export: %s + 0x%lX]", exportName, displacement+(returnAddress ? 1 : 0));
        }
    } else append("%08lX  [unknown module]", address);
}
static bool possibleReturn(DWORD address)
{
    if (!address || !executable(address-1)) return false;
    BYTE bytes[7];
    if (address >= 5 && readMemory(address-5, bytes, 5) && bytes[0] == 0xe8) return true;
    for (unsigned length = 2; length <= sizeof(bytes); ++length)
        if (address >= length && readMemory(address-length, bytes, length) && bytes[0] == 0xff &&
            ((bytes[1] >> 3) & 7) == 2) return true;
    return false;
}
#include "StaticCrashUnwind.inc"

static void buildReport(void)
{
    reportLength = 0; report[0] = 0;
    append("Nemu64 static crash report\r\nBuild: %.*s\r\nThread: %lu\r\n", 16,
        symbols ? symbols->build : "symbols unavailable", crash.thread);
    append("Exception: %08lX at %08lX\r\n", crash.exception.ExceptionCode, (DWORD)crash.exception.ExceptionAddress);
    if (crash.exception.ExceptionCode == EXCEPTION_ACCESS_VIOLATION && crash.exception.NumberParameters >= 2)
        append("Access: %s address %08lX\r\n", crash.exception.ExceptionInformation[0] == 1 ? "writing" :
            crash.exception.ExceptionInformation[0] == 8 ? "executing" : "reading", crash.exception.ExceptionInformation[1]);
    if (delphiType[0]) append("Delphi: %s: %s\r\n", delphiType, delphiMessage);
    const CONTEXT &c = crash.context;
    append("\r\nRegisters:\r\nEAX=%08lX EBX=%08lX ECX=%08lX EDX=%08lX\r\n"
        "ESI=%08lX EDI=%08lX EBP=%08lX ESP=%08lX\r\nEIP=%08lX EFLAGS=%08lX\r\n",
        c.Eax,c.Ebx,c.Ecx,c.Edx,c.Esi,c.Edi,c.Ebp,c.Esp,c.Eip,c.EFlags);
    append("\r\nFault location:\r\n"); location((DWORD)crash.exception.ExceptionAddress, false);
    append("\r\n\r\nStack (compiler FPO recipes / saved frame links):\r\n");
    if (crash.snapshot) append("Captured before exception handlers unwound the stack.\r\n");
    DWORD seen[96], frames = 0;
    CONTEXT cursor = c;
    location(cursor.Eip, false); append("\r\n");
    while (frames < sizeof(seen)/sizeof(seen[0])) {
        DWORD lookup = cursor.Eip-(frames && cursor.Eip ? 1 : 0);
        const FrameRecipe *recipe = findRecipe(lookup);
        const char *method;
        if (recipe) {
            BYTE instruction;
            if (!frames && readMemory(lookup,&instruction,1) &&
                ((instruction >= 0x58 && instruction <= 0x5f) || instruction == 0xc9 || instruction == 0xc3 || instruction == 0xc2)) {
                append("[Stopped: fault during possible stack teardown.]\r\n"); break;
            }
            bool searched = false;
            if (!applyRecipe(recipe,lookup,&cursor,&searched)) {
                append("[Stopped: unsupported/incomplete FPO state or unreadable stack.]\r\n"); break;
            }
            method = searched ? "FPO + bounded return search" : "FPO";
        } else {
            DWORD link[2], frame = cursor.Ebp;
            if ((frame & 3) || frame < cursor.Esp || !readStack(frame,link,sizeof(link)) ||
                !possibleReturn(link[1]) || (link[0] && link[0] <= frame)) break;
            cursor.Esp = frame+8; cursor.Ebp = link[0]; cursor.Eip = link[1];
            method = "frame link; frameless functions may be absent";
        }
        seen[frames++] = cursor.Eip; location(cursor.Eip,true); append("  [%s]\r\n",method);
    }
    append("\r\nStack candidates (heuristic call return addresses, NOT verified frames):\r\n");
    unsigned candidates = 0;
    for (DWORD slot = (c.Esp+3)&~3UL, scanned = 0; scanned < 4096 && candidates < 32 && slot >= c.Esp; slot += 4, ++scanned) {
        DWORD value;
        if (!readStack(slot, &value, 4)) break;
        bool duplicate = value == c.Eip;
        for (unsigned i = 0; i < frames; ++i) if (value == seen[i]) duplicate = true;
        if (duplicate || !possibleReturn(value)) continue;
        append("[ESP+%04lX] ", slot-c.Esp); location(value, true); append("\r\n"); ++candidates;
    }
    if (!candidates) append("None found.\r\n");
    append("\r\nRaw addresses are retained when symbols or reliable stack links are unavailable.\r\n");
}
static void copyReport(HWND window)
{
    HGLOBAL text = GlobalAlloc(GMEM_MOVEABLE, reportLength+1);
    if (!text) return;
    void *memory = GlobalLock(text);
    if (!memory) { GlobalFree(text); return; }
    memcpy(memory, report, reportLength+1); GlobalUnlock(text);
    if (OpenClipboard(window)) {
        if (EmptyClipboard() && SetClipboardData(CF_TEXT, text)) text = NULL;
        CloseClipboard();
    }
    if (text) GlobalFree(text);
}
static INT_PTR CALLBACK dialogProc(HWND window, UINT message, WPARAM w, LPARAM)
{
    if (message == WM_INITDIALOG) {
        SetWindowTextA(window, "Nemu64 - Application Error"); details = false;
        HFONT font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HWND icon = CreateWindowExA(0,"STATIC",NULL,WS_CHILD|WS_VISIBLE|SS_ICON,18,18,32,32,window,NULL,application,NULL);
        SendMessage(icon,STM_SETICON,(WPARAM)LoadIcon(NULL,IDI_ERROR),0);
        HWND text = CreateWindowExA(0,"STATIC","Nemu64 encountered an error and must close.\r\n\r\nPlease copy the details when reporting this problem.",
            WS_CHILD|WS_VISIBLE,64,18,345,80,window,NULL,application,NULL);
        HWND close = CreateWindowExA(0,"BUTTON","Close",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
            428,18,90,26,window,(HMENU)IDOK,application,NULL);
        detailButton = CreateWindowExA(0,"BUTTON","Details >>",WS_CHILD|WS_VISIBLE|WS_TABSTOP,428,58,90,26,window,(HMENU)ID_DETAILS,application,NULL);
        copyButton = CreateWindowExA(0,"BUTTON","Copy report",WS_CHILD|WS_VISIBLE|WS_TABSTOP,18,110,112,26,window,(HMENU)ID_COPY,application,NULL);
        detailEdit = CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT",report,WS_CHILD|WS_TABSTOP|WS_VSCROLL|WS_HSCROLL|
            ES_READONLY|ES_MULTILINE|ES_AUTOVSCROLL|ES_AUTOHSCROLL,18,150,500,270,window,(HMENU)ID_REPORT,application,NULL);
        HWND controls[] = {text,close,detailButton,copyButton};
        for (unsigned i = 0; i < sizeof(controls)/sizeof(controls[0]); ++i) SendMessage(controls[i],WM_SETFONT,(WPARAM)font,TRUE);
        SendMessage(detailEdit,WM_SETFONT,(WPARAM)GetStockObject(ANSI_FIXED_FONT),TRUE);
        SendMessage(detailEdit,EM_SETLIMITTEXT,sizeof(report),0);
        RECT rect = {0,0,536,150}; AdjustWindowRectEx(&rect,WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME,FALSE,WS_EX_DLGMODALFRAME);
        SetWindowPos(window,HWND_TOPMOST,(GetSystemMetrics(SM_CXSCREEN)-(rect.right-rect.left))/2,
            (GetSystemMetrics(SM_CYSCREEN)-480)/2,rect.right-rect.left,rect.bottom-rect.top,SWP_SHOWWINDOW);
        return TRUE;
    }
    if (message == WM_COMMAND) {
        if (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL) { EndDialog(window, IDOK); return TRUE; }
        if (LOWORD(w) == ID_COPY) { copyReport(window); return TRUE; }
        if (LOWORD(w) == ID_DETAILS) {
            details = !details;
            RECT rect = {0,0,536,details ? 438 : 150};
            AdjustWindowRectEx(&rect,WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME,FALSE,WS_EX_DLGMODALFRAME);
            SetWindowPos(window,NULL,0,0,rect.right-rect.left,rect.bottom-rect.top,SWP_NOMOVE|SWP_NOZORDER);
            ShowWindow(detailEdit,details ? SW_SHOW : SW_HIDE);
            SetWindowTextA(detailButton,details ? "Details <<" : "Details >>"); return TRUE;
        }
    }
    if (message == WM_CLOSE) { EndDialog(window, IDOK); return TRUE; }
    return FALSE;
}
static void showReport(void)
{
    __try { buildReport(); }
    __except(EXCEPTION_EXECUTE_HANDLER) { lstrcpyA(report,"Crash report could not be completed. The process must close."); reportLength=lstrlenA(report); }
    // No VCL/MFC, heap-based formatting or messages to the faulted UI thread.
    struct Template { DLGTEMPLATE dialog; WORD menu, cls, title; } layout;
    ZeroMemory(&layout,sizeof(layout));
    layout.dialog.style = WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;
    layout.dialog.dwExtendedStyle = WS_EX_DLGMODALFRAME;
    layout.dialog.cx = 340; layout.dialog.cy = 100;
    DialogBoxIndirectParamA(application,&layout.dialog,NULL,dialogProc,0);
    TerminateProcess(GetCurrentProcess(),crash.exception.ExceptionCode ? crash.exception.ExceptionCode : 1);
}
static DWORD WINAPI reporter(LPVOID)
{
    WaitForSingleObject(requestEvent,INFINITE);
    if (!stopping) showReport();
    return 0;
}
static LONG submit(void)
{
    if (reporterThread && GetCurrentThreadId() != reporterId) {
        SetEvent(requestEvent);
        WaitForSingleObject(reporterThread,INFINITE);
    } else showReport();
    return EXCEPTION_EXECUTE_HANDLER;
}
extern "C" LONG __cdecl NemuStaticCrashReport(EXCEPTION_POINTERS *exception)
{
    if (InterlockedCompareExchange(&reporting,1,0)) { TerminateProcess(GetCurrentProcess(),1); return EXCEPTION_EXECUTE_HANDLER; }
    __try { capture(&crash,exception,false); }
    __except(EXCEPTION_EXECUTE_HANDLER) { TerminateProcess(GetCurrentProcess(),1); return EXCEPTION_EXECUTE_HANDLER; }
    delphiType[0] = delphiMessage[0] = 0;
    return submit();
}
static LONG WINAPI unhandled(EXCEPTION_POINTERS *exception) { return NemuStaticCrashReport(exception); }
extern "C" void __cdecl NemuStaticCrashDelphi(void *address, const char *type, const char *message)
{
    // Only called for VCL's unhandled hardware exceptions, not validation
    // exceptions or EAbort. Restore the first-chance snapshot before unwinding.
    if (!firstChanceValid || firstChance.exception.ExceptionAddress != address || !fatalCode(firstChance.exception.ExceptionCode)) return;
    if (InterlockedCompareExchange(&reporting,1,0)) { TerminateProcess(GetCurrentProcess(),1); return; }
    crash = firstChance;
    lstrcpynA(delphiType,type ? type : "Exception",sizeof(delphiType));
    lstrcpynA(delphiMessage,message ? message : "",sizeof(delphiMessage));
    submit();
}
extern "C" BOOL __cdecl NemuStaticCrashIsGeneratedCode(DWORD address)
{ return describeCode && describeCode(address,NULL,0); }
extern "C" void __cdecl NemuStaticCrashSetCodeDescription(NemuStaticDescribeCode describe) { describeCode = describe; }
static void loadSymbols(void)
{
    HRSRC resource = FindResourceA(application,"NEMU_CRASH_SYMBOLS",RT_RCDATA);
    if (!resource) return;
    symbolSize = SizeofResource(application,resource);
    const SymbolHeader *header = (const SymbolHeader *)LockResource(LoadResource(application,resource));
    if (!header || symbolSize < sizeof(*header) || memcmp(header->magic,"NCRASH01",8) || header->version != 1 ||
        header->count > (symbolSize-sizeof(*header))/sizeof(SymbolEntry) ||
        header->strings != sizeof(*header)+header->count*sizeof(SymbolEntry) ||
        header->strings > symbolSize || header->stringSize != symbolSize-header->strings || !header->stringSize) return;
    const SymbolEntry *entries = (const SymbolEntry *)(header+1);
    DWORD previous = 0;
    for (DWORD i = 0; i < header->count; ++i) {
        if (entries[i].begin < previous || entries[i].begin >= entries[i].end || entries[i].name >= header->stringSize) return;
        previous = entries[i].begin;
        const char *name = (const char *)header+header->strings+entries[i].name;
        unsigned limit = header->stringSize-entries[i].name;
        if (limit > 1024) limit = 1024;
        if (!memchr(name,0,limit)) return;
    }
    symbols = header;
}
extern "C" void __cdecl NemuStaticCrashInitialize(HINSTANCE instance)
{
    if (application) return;
    application = instance;
    char path[MAX_PATH];
    if (!GetModuleFileNameA(instance,path,sizeof(path))) lstrcpyA(path,"nemu64-static.exe");
    path[sizeof(path)-1]=0;
    const char *leaf=strrchr(path,'\\');lstrcpynA(executableName,leaf ? leaf+1 : path,sizeof(executableName));
    loadSymbols(); loadFpo(); reporting = stopping = 0;
    requestEvent = CreateEventA(NULL,FALSE,FALSE,NULL);
    if (requestEvent) reporterThread = CreateThread(NULL,0,reporter,NULL,0,&reporterId);
    previousFilter = SetUnhandledExceptionFilter(unhandled);
    HMODULE kernel = GetModuleHandleA("kernel32.dll");
    AddHandler add = (AddHandler)GetProcAddress(kernel,"AddVectoredExceptionHandler");
    removeHandler = (RemoveHandler)GetProcAddress(kernel,"RemoveVectoredExceptionHandler");
    if (add && removeHandler) vectoredHandle = add(1,observe);
}
extern "C" void __cdecl NemuStaticCrashShutdown(void)
{
    if (!application || reporting) return;
    SetUnhandledExceptionFilter(previousFilter);
    if (vectoredHandle && removeHandler) removeHandler(vectoredHandle);
    vectoredHandle = NULL; stopping = 1;
    if (reporterThread) { SetEvent(requestEvent); WaitForSingleObject(reporterThread,INFINITE); CloseHandle(reporterThread); }
    if (requestEvent) CloseHandle(requestEvent);
    reporterThread = requestEvent = NULL; symbols = NULL; fpo = NULL; application = NULL; describeCode = NULL;
}
