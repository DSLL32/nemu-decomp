/* Exercise the plugin ABI without creating a Direct3D device or using a ROM. */
#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    WORD Version, Type;
    char Name[100];
    BOOL NormalMemory, MemoryBswaped;
} PluginInfo;

/* Version 1.2 graphics plugin ABI: 124 bytes on Win32. */
typedef struct {
    HWND window, status;
    BOOL byteswapped;
    BYTE *header, *ram, *dmem, *imem;
    DWORD *registers[23];
    void (__cdecl *interrupts)(void);
} GraphicsInfo;

static void __cdecl interrupts(void) {}

int main(int argc, char **argv)
{
    static const char *names[] = {
        "ChangeWindow", "CloseDLL", "DllAbout", "DllConfig", "FillRectHack",
        "GetCombinerPointer", "GetDllInfo", "InitiateGFX", "ProcessDList",
        "RomClosed", "RomOpen", "SetResolution", "UpdateScreen"
    };
    HMODULE dll;
    PluginInfo info;
    GraphicsInfo gfx;
    BYTE header[64] = {0}, dmem[4096] = {0}, imem[4096] = {0};
    DWORD registers[23] = {0};
    BYTE *ram;
    void *combiner = NULL;
    unsigned i;
    if (argc != 2 || sizeof(gfx) != 124 || sizeof(info) != 112) return 2;
    dll = LoadLibraryA(argv[1]);
    if (!dll) { printf("LoadLibrary failed: %lu\n", GetLastError()); return 3; }
    for (i = 0; i < sizeof(names)/sizeof(names[0]); ++i) {
        if (!GetProcAddress(dll, names[i])) {
            printf("Missing export: %s\n", names[i]); return 4;
        }
    }
    puts("Exports: 13");
    memset(&info, 0xa5, sizeof(info));
    ((void (__cdecl *)(PluginInfo *))GetProcAddress(dll, "GetDllInfo"))(&info);
    if (info.Version != 0x102 || info.Type != 2 || info.NormalMemory ||
        !info.MemoryBswaped || strcmp(info.Name, "Nemu64 Graphics (combine debug)")) return 5;
    printf("Info: %04x %u %s %d %d\n", info.Version, info.Type, info.Name,
           info.NormalMemory, info.MemoryBswaped);
    ram = (BYTE *)VirtualAlloc(NULL, 8*1024*1024, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    if (!ram) return 6;
    memset(&gfx, 0, sizeof(gfx));
    gfx.byteswapped = TRUE;
    gfx.header = header; gfx.ram = ram; gfx.dmem = dmem; gfx.imem = imem;
    gfx.interrupts = interrupts;
    for (i = 0; i < 23; ++i) gfx.registers[i] = &registers[i];
    if (!((BOOL (__cdecl *)(GraphicsInfo))GetProcAddress(dll, "InitiateGFX"))(gfx)) return 7;
    puts("InitiateGFX: true");
    ((void (__cdecl *)(void **))GetProcAddress(dll, "GetCombinerPointer"))(&combiner);
    if (!combiner || IsBadReadPtr(combiner, 32)) return 8;
    for (i = 0; i < 32; ++i) if (((BYTE *)combiner)[i]) return 9;
    puts("Combiner header: accessible, zero initialized");
    ((void (__cdecl *)(long, long))GetProcAddress(dll, "SetResolution"))(800, 600);
    ((void (__cdecl *)(void))GetProcAddress(dll, "ChangeWindow"))();
    ((void (__cdecl *)(void))GetProcAddress(dll, "UpdateScreen"))();
    ((void (__cdecl *)(void))GetProcAddress(dll, "RomClosed"))();
    ((void (__cdecl *)(void))GetProcAddress(dll, "CloseDLL"))();
    ((void (__cdecl *)(void))GetProcAddress(dll, "CloseDLL"))();
    if (!FreeLibrary(dll)) return 10;
    VirtualFree(ram, 0, MEM_RELEASE);
    puts("Probe completed");
    return 0;
}
