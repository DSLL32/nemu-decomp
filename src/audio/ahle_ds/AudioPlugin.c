#include "transform.h"
#include "AudioPlugin.h"
#include <stdio.h>
#include <stdlib.h>
#pragma intrinsic(_byteswap_ulong)
#include <string.h>

// Unit name inferred: export names, ABI types and literal strings survive;
// the original source filename is not present in the stripped DLL.
char PluginName[] = "LaC's Audio DLL - Little Endian";
BYTE *RDRAM;
int DumpMicrocode;
BYTE *PluginIMEM;
BYTE *PluginDMEM;
HMODULE AttachedAudio;
BOOL PluginInitialized;
void (__cdecl *AttachedAiDacrateChanged)(int);
void (__cdecl *AttachedAiLenChanged)();
DWORD (__cdecl *AttachedAiReadLength)();
void (__cdecl *AttachedCloseDLL)();
void (__cdecl *AttachedDllAbout)(HWND);
void (__cdecl *AttachedDllConfig)(HWND);
void (__cdecl *AttachedDllTest)(HWND);
void (__cdecl *AttachedGetDllInfo)(PLUGIN_INFO *);
BOOL (__cdecl *AttachedInitiateAudio)(AUDIO_INFO);
void (__cdecl *AttachedProcessAList)();
void (__cdecl *AttachedRomClosed)();
void (__cdecl *AttachedAiUpdate)(BOOL);

void RaiseAudioInterrupt(void) {}

void AiUpdate(BOOL wait) { AttachedAiUpdate(wait); }
void AiDacrateChanged(int system) { AttachedAiDacrateChanged(system); }
void AiLenChanged() { AttachedAiLenChanged(); }
DWORD AiReadLength() { return AttachedAiReadLength(); }

void CloseDLL()
{
    if (PluginInitialized) {
        PluginInitialized = FALSE;
        FreeAudioHLE();
        AttachedCloseDLL();
        FreeLibrary(AttachedAudio);
    }
}

void DllAbout(HWND window)
{
    MessageBox(window, PluginName, "About", MB_OK);
}

void DllConfig(HWND window) { AIConfigure(window); }
void DllTest(HWND window) { AttachedDllTest(window); }

void GetDllInfo(PLUGIN_INFO *info)
{
    info->Version = 0x0101;
    info->Type = 3;
    strcpy(info->Name, PluginName);
    info->NormalMemory = FALSE;
    info->MemoryBswaped = TRUE;
}

BOOL InitiateAudio(AUDIO_INFO info)
{
    if (PluginInitialized)
        return PluginInitialized;
    DetectCPUFeatures();
    AttachedAudio = LoadLibrary("plugin/attachedAudio.dll");
    AttachedAiDacrateChanged = (void (__cdecl *)(int))GetProcAddress(AttachedAudio, "AiDacrateChanged");
    AttachedAiLenChanged = (void (__cdecl *)())GetProcAddress(AttachedAudio, "AiLenChanged");
    AttachedAiReadLength = (DWORD (__cdecl *)())GetProcAddress(AttachedAudio, "AiReadLength");
    AttachedCloseDLL = (void (__cdecl *)())GetProcAddress(AttachedAudio, "CloseDLL");
    AttachedDllAbout = (void (__cdecl *)(HWND))GetProcAddress(AttachedAudio, "DllAbout");
    AttachedDllConfig = (void (__cdecl *)(HWND))GetProcAddress(AttachedAudio, "DllConfig");
    AttachedDllTest = (void (__cdecl *)(HWND))GetProcAddress(AttachedAudio, "DllTest");
    AttachedGetDllInfo = (void (__cdecl *)(PLUGIN_INFO *))GetProcAddress(AttachedAudio, "GetDllInfo");
    AttachedInitiateAudio = (BOOL (__cdecl *)(AUDIO_INFO))GetProcAddress(AttachedAudio, "InitiateAudio");
    AttachedProcessAList = (void (__cdecl *)())GetProcAddress(AttachedAudio, "ProcessAList");
    AttachedRomClosed = (void (__cdecl *)())GetProcAddress(AttachedAudio, "RomClosed");
    AttachedAiUpdate = (void (__cdecl *)(BOOL))GetProcAddress(AttachedAudio, "AiUpdate");
    AISetUCodeID(1);
    PluginInitialized = TRUE;
    RDRAM = info.RDRAM;
    PluginDMEM = info.DMEM;
    PluginIMEM = info.IMEM;
    AttachedInitiateAudio(info);
    return TRUE;
}

void ProcessAList()
{
    if (HLEAudio) {
        memcpy(&AudioTask, PluginDMEM + 0xfc0, sizeof(AudioTask));
        ProcessAudioTask(&AudioTask);
        if (DumpMicrocode) {
            FILE *file = fopen("abi_ucode_dmem.dmp", "wb");
            int i;
            for (i = 0; i < 0x1000; i += 4) {
                DWORD swapped = _byteswap_ulong(*(DWORD *)(PluginDMEM + i));
                fwrite(&swapped, 4, 1, file);
            }
            fclose(file);
            file = fopen("abi_ucode_imem.dmp", "wb");
            for (i = 0; i < 0x1000; i += 4) {
                DWORD swapped = _byteswap_ulong(*(DWORD *)(PluginIMEM + i));
                fwrite(&swapped, 4, 1, file);
            }
            fclose(file);
        }
    }
}

void RomClosed() { AttachedRomClosed(); }
