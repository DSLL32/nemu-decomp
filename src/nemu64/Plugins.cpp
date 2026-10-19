#include "Plugins.h"
#include "CPU.h"
#include "Memory.h"
#include "RSP.h"
#include "RomBrowser.h"
#include "WinMain.h"
#include "FrontendIni.h"
#include <stdio.h>
#include <string.h>
#ifdef NEMU_STATIC
#include "../static/StaticRuntime.h"
#endif

char PluginDirectory[MAX_PATH];
PluginDescriptor PluginCatalog[40];
unsigned int PluginCount;
ToolDescriptor ToolCatalog[40];
unsigned int ToolCount;
HMODULE PluginModules[30];
unsigned int PluginModuleCount;
PluginDescriptor *SelectedGraphicsPlugin;
PluginDescriptor *PendingGraphicsPlugin;
void *GraphicsCombiner;
void (__cdecl *CIInit)(HWND, void *);
void (__cdecl *CIFree)();
void (__cdecl *CIGetJoyData)();
void (__cdecl *CIRumble)(unsigned int);
void (__cdecl *CIConfigure)(HWND);
void (__cdecl *AIInit)(HWND, unsigned char *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
void (__cdecl *AIFree)();
void (__cdecl *AISetReg)(unsigned int, unsigned int);
unsigned int (__cdecl *AIGetReg)(unsigned int);
void (__cdecl *AIConfigure)(HWND);
void (__cdecl *AUDList)(unsigned int);
void (__cdecl *AISetUCodeID)(unsigned int);
void (__cdecl *NemuSettings)(unsigned int, PluginDescriptor *);
void (__cdecl *DCTList)(unsigned int);

bool LoadPlugin(char *filename)
{
#ifdef NEMU_STATIC
    if (PluginModuleCount >= 30 || PluginCount >= 40 || ToolCount >= 40)
        return false;
#endif
    char path[MAX_PATH];
    char error[200];
    strcpy(path, PluginDirectory);
    strcat(path, filename);
#ifdef NEMU_STATIC
    int builtin = NemuStaticPluginIndex(filename);
    HMODULE module = builtin < 0 ? LoadLibraryA(path) : NULL;
#define PluginProc(module, name) (builtin >= 0 ? NemuStaticPluginProc(builtin, name) : GetProcAddress(module, name))
    if (builtin >= 0 || module) {
#else
#define PluginProc(module, name) GetProcAddress(module, name)
    HMODULE module = LoadLibraryA(path);
    if (module) {
#endif
        if (PluginProc(module, "GetDllInfo")) {
            PluginDescriptor *plugin = &PluginCatalog[PluginCount];
            plugin->Size = sizeof(PluginDescriptor);
            plugin->AiDacrateChanged = (void (__cdecl *)(int))PluginProc(module, "AiDacrateChanged");
            plugin->AiLenChanged = (void (__cdecl *)())PluginProc(module, "AiLenChanged");
            plugin->AiReadLength = (unsigned int (__cdecl *)())PluginProc(module, "AiReadLength");
            plugin->AiUpdate = (void (__cdecl *)(BOOL))PluginProc(module, "AiUpdate");
            plugin->ChangeWindow = (void (__cdecl *)())PluginProc(module, "ChangeWindow");
            plugin->CloseDLL = (void (__cdecl *)())PluginProc(module, "CloseDLL");
            plugin->DllAbout = (void (__cdecl *)(HWND))PluginProc(module, "DllAbout");
            plugin->DllConfig = (void (__cdecl *)(HWND))PluginProc(module, "DllConfig");
            plugin->DllTest = (void (__cdecl *)(HWND))PluginProc(module, "DllTest");
            plugin->DrawScreen = (void (__cdecl *)())PluginProc(module, "DrawScreen");
            plugin->FillRectHack = (void (__cdecl *)())PluginProc(module, "FillRectHack");
            plugin->GetCombinerPointer = (void (__cdecl *)(void **))PluginProc(module, "GetCombinerPointer");
            plugin->GetDllInfo = (void (__cdecl *)(PluginInfo *))PluginProc(module, "GetDllInfo");
            plugin->InitiateAudio = (BOOL (__cdecl *)(AudioInfo))PluginProc(module, "InitiateAudio");
            plugin->InitiateGFX = (BOOL (__cdecl *)(GraphicsInfo))PluginProc(module, "InitiateGFX");
            plugin->MoveScreen = (void (__cdecl *)(int, int))PluginProc(module, "MoveScreen");
            plugin->ProcessAList = (void (__cdecl *)())PluginProc(module, "ProcessAList");
            plugin->ProcessDList = (void (__cdecl *)())PluginProc(module, "ProcessDList");
            plugin->RomClosed = (void (__cdecl *)())PluginProc(module, "RomClosed");
            plugin->RomOpen = (void (__cdecl *)())PluginProc(module, "RomOpen");
            plugin->UpdateScreen = (void (__cdecl *)())PluginProc(module, "UpdateScreen");
            plugin->ViStatusChanged = (void (__cdecl *)())PluginProc(module, "ViStatusChanged");
            plugin->ViWidthChanged = (void (__cdecl *)())PluginProc(module, "ViWidthChanged");
            plugin->VISetUCodeID = (void (__cdecl *)(unsigned int))PluginProc(module, "VISetUCodeID");
            plugin->SetResolution = (void (__cdecl *)(int, int))PluginProc(module, "SetResolution");
            plugin->GetDllInfo(&plugin->Info);
            strcpy(plugin->Filename, filename);
            ++PluginCount;
        }
        if (PluginProc(module, "CIInit")) {
            CIInit = (void (__cdecl *)(HWND, void *))PluginProc(module, "CIInit");
            CIFree = (void (__cdecl *)())PluginProc(module, "CIFree");
            CIGetJoyData = (void (__cdecl *)())PluginProc(module, "CIGetJoyData");
            CIRumble = (void (__cdecl *)(unsigned int))PluginProc(module, "CIRumble");
            CIConfigure = (void (__cdecl *)(HWND))PluginProc(module, "CIConfigure");
        }
        if (PluginProc(module, "AIInit")) {
            AIInit = (void (__cdecl *)(HWND, unsigned char *, unsigned int *, unsigned int *, unsigned int *, unsigned int *))PluginProc(module, "AIInit");
            AIFree = (void (__cdecl *)())PluginProc(module, "AIFree");
            AISetReg = (void (__cdecl *)(unsigned int, unsigned int))PluginProc(module, "AISetReg");
            AIGetReg = (unsigned int (__cdecl *)(unsigned int))PluginProc(module, "AIGetReg");
            AIConfigure = (void (__cdecl *)(HWND))PluginProc(module, "AIConfigure");
            AUDList = (void (__cdecl *)(unsigned int))PluginProc(module, "AUDList");
            AISetUCodeID = (void (__cdecl *)(unsigned int))PluginProc(module, "AISetUCodeID");
        }
        if (PluginProc(module, "ToolInit")) {
            ToolCatalog[ToolCount].Init = (unsigned int (__cdecl *)(NemuHostInterface *))PluginProc(module, "ToolInit");
            ToolCatalog[ToolCount].Free = (void (__cdecl *)())PluginProc(module, "ToolFree");
            ToolCatalog[ToolCount].Invoke = (void (__cdecl *)(unsigned int, unsigned int))PluginProc(module, "ToolFunc");
            ToolCatalog[ToolCount].GetMenuText = (void (__cdecl *)(char *))PluginProc(module, "ToolGetMenuText");
            sprintf(ToolCatalog[ToolCount].MenuText, "%s...", filename);
            ++ToolCount;
        }
        if (PluginProc(module, "NemuSettings"))
            NemuSettings = (void (__cdecl *)(unsigned int, PluginDescriptor *))PluginProc(module, "NemuSettings");
        if (PluginProc(module, "DCTList"))
            DCTList = (void (__cdecl *)(unsigned int))PluginProc(module, "DCTList");
#ifdef NEMU_STATIC
        if (module)
#endif
            PluginModules[PluginModuleCount++] = module;
        return true;
    }
    else {
        sprintf(error, "Error loading library '%s'.\n\nExecution will continue but you won't be able to use this plugin", filename);
        MessageBoxA(MainWindow, error, "Error", MB_ICONERROR);
        return false;
    }
}
#undef PluginProc

bool LoadPlugins()
{
    char search[300];
    WIN32_FIND_DATAA data;
    UnloadPlugins();
    PluginModuleCount = 0;
#ifdef NEMU_STATIC
    PluginCount = 0;
#endif
    strcpy(PluginDirectory, ApplicationDirectory);
    strcat(PluginDirectory, "Plugin\\");
#ifdef NEMU_STATIC
    // Register bundled implementations even when no Plugin directory exists.
    for (unsigned int i = 0; NemuStaticPluginName(i); ++i)
        if (!LoadPlugin((char *)NemuStaticPluginName(i))) return false;
#endif
    strcpy(search, PluginDirectory);
    strcat(search, "*.dll");
    HANDLE handle = FindFirstFileA(search, &data);
    if (handle != INVALID_HANDLE_VALUE) {
        do {
#ifdef NEMU_STATIC
            bool bundled = false;
            for (unsigned int i = 0; NemuStaticPluginName(i); ++i)
                if (!lstrcmpiA(data.cFileName, NemuStaticPluginName(i))) bundled = true;
            if (bundled) continue;
#endif
            LoadPlugin(data.cFileName);
        } while (FindNextFileA(handle, &data));
        FindClose(handle);
        return true;
    }
#ifdef NEMU_STATIC
    return PluginCount != 0;
#else
    return false;
#endif
}

void UnloadPlugins()
{
    ToolCount = 0;
    if (PluginModuleCount) do {
        --PluginModuleCount;
        if (PluginModules[PluginModuleCount])
            FreeLibrary(PluginModules[PluginModuleCount]);
    } while (PluginModuleCount > 0);
}



unsigned int NetPlayActive;
int NetPlayPlayer;
int NetPlayPlayerCount;
unsigned char RomHeader[64];
unsigned int GraphicsInterruptRegister;
int ShowErrorMessages;
char SaveStateDirectory[MAX_PATH];
extern char SelectedRomFilename[256];


void NotifyTools(unsigned int mask, unsigned int reason)
{
    for (unsigned int i = 0; i < ToolCount; ++i)
        if ((ToolCatalog[i].Flags & mask) && ToolCatalog[i].Invoke)
            ToolCatalog[i].Invoke(mask, reason);
}

// Separate callback identities are inferred from the two immediate addresses
// stored by the original caller. Their identical bodies can be folded by ICF.
void __stdcall NetplayChatReceived(char *, char *)
{
}



void CheckGraphicsInterrupts()
{
}
