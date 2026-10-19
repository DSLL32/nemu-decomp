/* Support for the build-time PE linker. All component code and data are already
 * mapped by the Windows EXE loader; this does not unpack or load embedded DLLs.
 * Private component headers preserve resource namespaces and export tables.
 */
#define WINVER 0x0500
#define _WIN32_WINNT 0x0500
#include <windows.h>

typedef BOOL (WINAPI *EntryPoint)(HINSTANCE, DWORD, LPVOID);
typedef struct {
    char name[64];
    HMODULE base;
    DWORD size;
    EntryPoint entry;
    DWORD plugin;
    DWORD attached;
    DWORD noThreadCalls;
} StaticModule;

/* Filled by scripts/link_static.py, including the relocated entry addresses. */
__declspec(dllexport) StaticModule StaticModules[16] = {{"link-time module table"}};
__declspec(dllexport) DWORD StaticModuleCount = 0;
typedef struct {
    char library[64];
    char name[128];
    DWORD ordinal;
    FARPROC *slot;
} DelayedImport;
__declspec(dllexport) DelayedImport StaticDelayedImports[32] = {{"link-time imports"}};
__declspec(dllexport) DWORD StaticDelayedImportCount = 0;
static BOOL started;
static void *reservedRdram;

__declspec(dllexport) void *__cdecl NemuStaticRdram(void)
{
    return reservedRdram;
}

static StaticModule *byHandle(HMODULE base)
{
    DWORD i;
    for (i = 0; i < StaticModuleCount; ++i)
        if (StaticModules[i].base == base) return StaticModules + i;
    return NULL;
}

static StaticModule *byAddress(LPCVOID address)
{
    DWORD i;
    for (i = 0; i < StaticModuleCount; ++i) {
        StaticModule *m = StaticModules + i;
        if ((DWORD)address >= (DWORD)m->base &&
            (DWORD)address - (DWORD)m->base < m->size) return m;
    }
    return NULL;
}

static StaticModule *byName(LPCSTR name)
{
    const char *leaf = name, *p;
    DWORD i;
    if (!name) return NULL;
    for (p = name; *p; ++p) if (*p == '/' || *p == '\\') leaf = p + 1;
    for (i = 0; i < StaticModuleCount; ++i)
        if (!lstrcmpiA(leaf, StaticModules[i].name)) return StaticModules + i;
    return NULL;
}

static StaticModule *byWideName(LPCWSTR name)
{
    char narrow[MAX_PATH];
    if (!name || !WideCharToMultiByte(CP_ACP, 0, name, -1, narrow,
            sizeof(narrow), NULL, NULL)) return NULL;
    return byName(narrow);
}

__declspec(dllexport) const char *__cdecl NemuStaticPluginName(DWORD index)
{
    DWORD i;
    for (i = 0; i < StaticModuleCount; ++i)
        if (StaticModules[i].plugin && index-- == 0) return StaticModules[i].name;
    return NULL;
}

__declspec(dllexport) HMODULE WINAPI StaticGetModuleHandleA(LPCSTR name)
{
    StaticModule *m = byName(name);
    return m ? m->base : GetModuleHandleA(name);
}

__declspec(dllexport) HMODULE WINAPI StaticGetModuleHandleW(LPCWSTR name)
{
    StaticModule *m = byWideName(name);
    return m ? m->base : GetModuleHandleW(name);
}

__declspec(dllexport) HMODULE WINAPI StaticLoadLibraryA(LPCSTR name)
{
    StaticModule *m = byName(name);
    return m ? m->base : LoadLibraryA(name);
}

__declspec(dllexport) HMODULE WINAPI StaticLoadLibraryW(LPCWSTR name)
{
    StaticModule *m = byWideName(name);
    return m ? m->base : LoadLibraryW(name);
}

__declspec(dllexport) HMODULE WINAPI StaticLoadLibraryExA(LPCSTR name, HANDLE file, DWORD flags)
{
    StaticModule *m = byName(name);
    return m ? m->base : LoadLibraryExA(name, file, flags);
}

__declspec(dllexport) HMODULE WINAPI StaticLoadLibraryExW(LPCWSTR name, HANDLE file, DWORD flags)
{
    StaticModule *m = byWideName(name);
    return m ? m->base : LoadLibraryExW(name, file, flags);
}

__declspec(dllexport) BOOL WINAPI StaticFreeLibrary(HMODULE base)
{
    return byHandle(base) ? TRUE : FreeLibrary(base);
}

__declspec(dllexport) BOOL WINAPI StaticDisableThreadLibraryCalls(HMODULE base)
{
    StaticModule *m = byHandle(base);
    if (!m) return DisableThreadLibraryCalls(base);
    m->noThreadCalls = TRUE;
    return TRUE;
}

__declspec(dllexport) DWORD WINAPI StaticGetModuleFileNameA(HMODULE base, LPSTR path, DWORD size)
{
    /* A linked component lives in the executable, so its physical filename is
     * the executable's. GetModuleFileName(NULL) retains its usual semantics. */
    return GetModuleFileNameA(byHandle(base) ? NULL : base, path, size);
}

__declspec(dllexport) DWORD WINAPI StaticGetModuleFileNameW(HMODULE base, LPWSTR path, DWORD size)
{
    return GetModuleFileNameW(byHandle(base) ? NULL : base, path, size);
}

__declspec(dllexport) SIZE_T WINAPI StaticVirtualQuery(LPCVOID address,
    PMEMORY_BASIC_INFORMATION info, SIZE_T size)
{
    SIZE_T result = VirtualQuery(address, info, size);
    StaticModule *m = byAddress(address);
    /* Delphi's FindClassHInstance uses AllocationBase to locate DFM resources.
     * Report the component namespace when called through a component's IAT. */
    if (result && m) info->AllocationBase = m->base;
    return result;
}

__declspec(dllexport) FARPROC WINAPI StaticGetProcAddress(HMODULE base, LPCSTR name)
{
    StaticModule *m = byHandle(base);
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS *nt;
    IMAGE_EXPORT_DIRECTORY *exports;
    DWORD *names, *functions, index, i, rva;
    WORD *ordinals;
    if (!m) return GetProcAddress(base, name);
    dos = (IMAGE_DOS_HEADER *)base;
    nt = (IMAGE_NT_HEADERS *)((BYTE *)base + dos->e_lfanew);
    rva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
    if (!rva) return NULL;
    exports = (IMAGE_EXPORT_DIRECTORY *)((BYTE *)base + rva);
    functions = (DWORD *)((BYTE *)base + exports->AddressOfFunctions);
    if ((DWORD)name <= 0xffff) {
        index = (DWORD)name - exports->Base;
    } else {
        names = (DWORD *)((BYTE *)base + exports->AddressOfNames);
        ordinals = (WORD *)((BYTE *)base + exports->AddressOfNameOrdinals);
        index = exports->NumberOfFunctions;
        for (i = 0; i < exports->NumberOfNames; ++i)
            if (!lstrcmpA(name, (char *)base + names[i])) { index = ordinals[i]; break; }
    }
    if (index >= exports->NumberOfFunctions || !functions[index]) {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return NULL;
    }
    return (FARPROC)((BYTE *)base + functions[index]);
}

__declspec(dllexport) void WINAPI StaticTlsCallback(LPVOID instance, DWORD reason, LPVOID reserved)
{
    DWORD i;
    (void)instance;
    if (!started) return;
    if (reason == DLL_THREAD_ATTACH) {
        for (i = 0; i < StaticModuleCount; ++i) {
            StaticModule *m = StaticModules + i;
            if (m->attached && !m->noThreadCalls && m->entry)
                m->entry((HINSTANCE)m->base, reason, reserved);
        }
    } else if (reason == DLL_THREAD_DETACH || reason == DLL_PROCESS_DETACH) {
        for (i = StaticModuleCount; i; --i) {
            StaticModule *m = StaticModules + i - 1;
            if (m->attached && m->entry &&
                (reason == DLL_PROCESS_DETACH || !m->noThreadCalls))
                m->entry((HINSTANCE)m->base, reason, reserved);
            if (reason == DLL_PROCESS_DETACH) m->attached = FALSE;
        }
    }
}

__declspec(dllexport) void __cdecl StaticInitialize(void)
{
    DWORD i;
    /* The ordinary frontend reserves RAM before LoadPlugins. Delphi/VCL heaps
     * can fragment the 32-bit address space, so keep that ordering here too. */
    reservedRdram = VirtualAlloc(NULL, 0x20000000, MEM_RESERVE, PAGE_READWRITE);
    /* Match the normal frontend: graphics/sound system DLLs are loaded after
     * the RAM reservation. In Wine their drivers can reserve large regions. */
    for (i = 0; i < StaticDelayedImportCount; ++i) {
        DelayedImport *import = StaticDelayedImports + i;
        HMODULE library = LoadLibraryA(import->library);
        FARPROC function = library ? GetProcAddress(library,
            import->ordinal ? (LPCSTR)import->ordinal : import->name) : NULL;
        DWORD protection, ignored;
        if (!function || !VirtualProtect(import->slot, sizeof(FARPROC), PAGE_READWRITE, &protection)) {
            MessageBoxA(NULL, import->library, "Could not load system dependency", MB_ICONERROR);
            ExitProcess(ERROR_DLL_INIT_FAILED);
        }
        *import->slot = function;
        VirtualProtect(import->slot, sizeof(FARPROC), protection, &ignored);
    }
    started = TRUE;
    for (i = 0; i < StaticModuleCount; ++i) {
        StaticModule *m = StaticModules + i;
        if (m->entry && !m->entry((HINSTANCE)m->base, DLL_PROCESS_ATTACH, NULL)) {
            StaticTlsCallback(NULL, DLL_PROCESS_DETACH, NULL);
            MessageBoxA(NULL, m->name, "Could not initialize static component", MB_ICONERROR);
            ExitProcess(ERROR_DLL_INIT_FAILED);
        }
        m->attached = TRUE;
    }
}
