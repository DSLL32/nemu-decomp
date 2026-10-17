#include "Plugins.h"
#include "CPU.h"
#include "Memory.h"
#include "RSP.h"
#include "RomBrowser.h"
#include "WinMain.h"
#include "FrontendIni.h"
#include <stdio.h>
#include <string.h>

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
    char path[MAX_PATH];
    char error[200];
    strcpy(path, PluginDirectory);
    strcat(path, filename);
    HMODULE module = LoadLibraryA(path);
    if (module) {
        if (GetProcAddress(module, "GetDllInfo")) {
            PluginDescriptor *plugin = &PluginCatalog[PluginCount];
            plugin->Size = sizeof(PluginDescriptor);
            plugin->AiDacrateChanged = (void (__cdecl *)(int))GetProcAddress(module, "AiDacrateChanged");
            plugin->AiLenChanged = (void (__cdecl *)())GetProcAddress(module, "AiLenChanged");
            plugin->AiReadLength = (unsigned int (__cdecl *)())GetProcAddress(module, "AiReadLength");
            plugin->AiUpdate = (void (__cdecl *)(BOOL))GetProcAddress(module, "AiUpdate");
            plugin->ChangeWindow = (void (__cdecl *)())GetProcAddress(module, "ChangeWindow");
            plugin->CloseDLL = (void (__cdecl *)())GetProcAddress(module, "CloseDLL");
            plugin->DllAbout = (void (__cdecl *)(HWND))GetProcAddress(module, "DllAbout");
            plugin->DllConfig = (void (__cdecl *)(HWND))GetProcAddress(module, "DllConfig");
            plugin->DllTest = (void (__cdecl *)(HWND))GetProcAddress(module, "DllTest");
            plugin->DrawScreen = (void (__cdecl *)())GetProcAddress(module, "DrawScreen");
            plugin->FillRectHack = (void (__cdecl *)())GetProcAddress(module, "FillRectHack");
            plugin->GetCombinerPointer = (void (__cdecl *)(void **))GetProcAddress(module, "GetCombinerPointer");
            plugin->GetDllInfo = (void (__cdecl *)(PluginInfo *))GetProcAddress(module, "GetDllInfo");
            plugin->InitiateAudio = (BOOL (__cdecl *)(AudioInfo))GetProcAddress(module, "InitiateAudio");
            plugin->InitiateGFX = (BOOL (__cdecl *)(GraphicsInfo))GetProcAddress(module, "InitiateGFX");
            plugin->MoveScreen = (void (__cdecl *)(int, int))GetProcAddress(module, "MoveScreen");
            plugin->ProcessAList = (void (__cdecl *)())GetProcAddress(module, "ProcessAList");
            plugin->ProcessDList = (void (__cdecl *)())GetProcAddress(module, "ProcessDList");
            plugin->RomClosed = (void (__cdecl *)())GetProcAddress(module, "RomClosed");
            plugin->RomOpen = (void (__cdecl *)())GetProcAddress(module, "RomOpen");
            plugin->UpdateScreen = (void (__cdecl *)())GetProcAddress(module, "UpdateScreen");
            plugin->ViStatusChanged = (void (__cdecl *)())GetProcAddress(module, "ViStatusChanged");
            plugin->ViWidthChanged = (void (__cdecl *)())GetProcAddress(module, "ViWidthChanged");
            plugin->VISetUCodeID = (void (__cdecl *)(unsigned int))GetProcAddress(module, "VISetUCodeID");
            plugin->SetResolution = (void (__cdecl *)(int, int))GetProcAddress(module, "SetResolution");
            plugin->GetDllInfo(&plugin->Info);
            strcpy(plugin->Filename, filename);
            ++PluginCount;
        }
        if (GetProcAddress(module, "CIInit")) {
            CIInit = (void (__cdecl *)(HWND, void *))GetProcAddress(module, "CIInit");
            CIFree = (void (__cdecl *)())GetProcAddress(module, "CIFree");
            CIGetJoyData = (void (__cdecl *)())GetProcAddress(module, "CIGetJoyData");
            CIRumble = (void (__cdecl *)(unsigned int))GetProcAddress(module, "CIRumble");
            CIConfigure = (void (__cdecl *)(HWND))GetProcAddress(module, "CIConfigure");
        }
        if (GetProcAddress(module, "AIInit")) {
            AIInit = (void (__cdecl *)(HWND, unsigned char *, unsigned int *, unsigned int *, unsigned int *, unsigned int *))GetProcAddress(module, "AIInit");
            AIFree = (void (__cdecl *)())GetProcAddress(module, "AIFree");
            AISetReg = (void (__cdecl *)(unsigned int, unsigned int))GetProcAddress(module, "AISetReg");
            AIGetReg = (unsigned int (__cdecl *)(unsigned int))GetProcAddress(module, "AIGetReg");
            AIConfigure = (void (__cdecl *)(HWND))GetProcAddress(module, "AIConfigure");
            AUDList = (void (__cdecl *)(unsigned int))GetProcAddress(module, "AUDList");
            AISetUCodeID = (void (__cdecl *)(unsigned int))GetProcAddress(module, "AISetUCodeID");
        }
        if (GetProcAddress(module, "ToolInit")) {
            ToolCatalog[ToolCount].Init = (unsigned int (__cdecl *)(NemuHostInterface *))GetProcAddress(module, "ToolInit");
            ToolCatalog[ToolCount].Free = (void (__cdecl *)())GetProcAddress(module, "ToolFree");
            ToolCatalog[ToolCount].Invoke = (void (__cdecl *)(unsigned int, unsigned int))GetProcAddress(module, "ToolFunc");
            ToolCatalog[ToolCount].GetMenuText = (void (__cdecl *)(char *))GetProcAddress(module, "ToolGetMenuText");
            sprintf(ToolCatalog[ToolCount].MenuText, "%s...", filename);
            ++ToolCount;
        }
        if (GetProcAddress(module, "NemuSettings"))
            NemuSettings = (void (__cdecl *)(unsigned int, PluginDescriptor *))GetProcAddress(module, "NemuSettings");
        if (GetProcAddress(module, "DCTList"))
            DCTList = (void (__cdecl *)(unsigned int))GetProcAddress(module, "DCTList");
        PluginModules[PluginModuleCount++] = module;
        return true;
    }
    else {
        sprintf(error, "Error loading library '%s'.\n\nExecution will continue but you won't be able to use this plugin", filename);
        MessageBoxA(MainWindow, error, "Error", MB_ICONERROR);
        return false;
    }
}

bool LoadPlugins()
{
    char search[300];
    WIN32_FIND_DATAA data;
    UnloadPlugins();
    PluginModuleCount = 0;
    strcpy(PluginDirectory, ApplicationDirectory);
    strcat(PluginDirectory, "Plugin\\");
    strcpy(search, PluginDirectory);
    strcat(search, "*.dll");
    HANDLE handle = FindFirstFileA(search, &data);
    if (handle != INVALID_HANDLE_VALUE) {
        do {
            LoadPlugin(data.cFileName);
        } while (FindNextFileA(handle, &data));
        FindClose(handle);
        return true;
    }
    return false;
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
