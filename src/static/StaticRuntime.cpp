/* Bootstrap for the ordinary static user build. All components share the
   executable's real HINSTANCE and the native CRT; no DLL loader is involved. */
#include <afxwin.h>
#include "StaticRuntime.h"
#include "StaticTheme.h"

#pragma pack(push, 1)
struct DelphiInit { unsigned short priority; void (__cdecl *function)(void); };
#pragma pack(pop)
extern "C" void __cdecl StaticDelphiVclInit(unsigned char, unsigned char, int, unsigned char);
extern "C" void __cdecl StaticDelphiVclExit(void);
extern "C" unsigned int StaticDelphiTlsIndex;
extern "C" unsigned long _tls_index;
/* Forces the native CRT's standard TLS directory and loader allocation. */
__declspec(thread) volatile unsigned char NemuStaticNativeTls;

/* Ordinary function/data references, generated from compiler object symbols. */
#include "StaticBindings.inc"

static void *rdram;
static unsigned int initializedUnits;
static unsigned int attachedComponents;
static HINSTANCE executableInstance;
static bool delphiReady;
static bool mfcReady;

extern "C" void * __cdecl NemuStaticRdram(void) { return rdram; }
extern "C" const char * __cdecl NemuStaticPluginName(unsigned int index)
{
    return index < sizeof(plugins) / sizeof(plugins[0]) ? plugins[index].filename : NULL;
}
extern "C" int __cdecl NemuStaticPluginIndex(const char *name)
{
    for (unsigned int i = 0; i < sizeof(plugins) / sizeof(plugins[0]); ++i)
        if (!lstrcmpiA(name, plugins[i].filename)) return i;
    return -1;
}
extern "C" FARPROC __cdecl NemuStaticPluginProc(int index, const char *name)
{
    if (index < 0 || (unsigned int)index >= sizeof(plugins) / sizeof(plugins[0])) return NULL;
    const StaticPlugin &plugin = plugins[index];
    for (unsigned int i = 0; i < plugin.count; ++i)
        if (!strcmp(name, plugin.procedures[i].name)) return plugin.procedures[i].address;
    return NULL;
}
extern "C" bool __cdecl NemuStaticInitialize(HINSTANCE instance)
{
    // Reserve the emulated address space before VCL/MFC or graphics drivers.
    rdram = VirtualAlloc(NULL, 0x20000000, MEM_RESERVE, PAGE_READWRITE);
    if (!rdram) return false;
    executableInstance = instance;
    NemuStaticThemeInitialize();
    StaticDelphiVclInit(0, 0, (int)instance, 0);
    StaticDelphiTlsIndex = _tls_index;
    delphiReady = true;
    for (; initializedUnits < sizeof(delphiUnits) / sizeof(delphiUnits[0]); ++initializedUnits) {
        const StaticUnit &unit = delphiUnits[initializedUnits];
        for (unsigned int i = 0; i < unit.initCount; ++i)
            if (unit.init[i].function) unit.init[i].function();
    }
    if (!AfxWinInit(instance, NULL, GetCommandLineA(), SW_SHOWDEFAULT)) {
        NemuStaticShutdown();
        return false;
    }
    mfcReady = true;
    if (!AfxGetApp() || !AfxGetApp()->InitInstance()) {
        NemuStaticShutdown();
        return false;
    }
    for (; attachedComponents < sizeof(nativeEntries) / sizeof(nativeEntries[0]); ++attachedComponents)
        if (!nativeEntries[attachedComponents](instance, DLL_PROCESS_ATTACH, NULL)) {
            NemuStaticShutdown();
            return false;
        }
    return true;
}
extern "C" void __cdecl NemuStaticShutdown(void)
{
    while (attachedComponents)
        nativeEntries[--attachedComponents](executableInstance, DLL_PROCESS_DETACH, NULL);
    if (mfcReady) {
        if (AfxGetApp()) AfxGetApp()->ExitInstance();
        AfxWinTerm();
        mfcReady = false;
    }
    while (initializedUnits) {
        const StaticUnit &unit = delphiUnits[--initializedUnits];
        for (unsigned int i = unit.finiCount; i; --i)
            if (unit.fini[i - 1].function) unit.fini[i - 1].function();
    }
    if (delphiReady) { StaticDelphiVclExit(); delphiReady = false; }
    NemuStaticThemeShutdown();
}
