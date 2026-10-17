#ifndef NEMU_AUDIO_PLUGIN_H
#define NEMU_AUDIO_PLUGIN_H

#include <windows.h>

// Zilmar audio plugin ABI 1.1. The DLL uses the default C calling convention.
typedef struct PLUGIN_INFO {
    WORD Version;
    WORD Type;
    char Name[100];
    BOOL NormalMemory;
    BOOL MemoryBswaped;
} PLUGIN_INFO;

typedef struct AUDIO_INFO {
    HWND hWnd;
    HINSTANCE hinst;
    BOOL MemoryBswaped;
    BYTE *HEADER;
    BYTE *RDRAM;
    BYTE *DMEM;
    BYTE *IMEM;
    DWORD *MI_INTR_REG;
    DWORD *AI_DRAM_ADDR_REG;
    DWORD *AI_LEN_REG;
    DWORD *AI_CONTROL_REG;
    DWORD *AI_STATUS_REG;
    DWORD *AI_DACRATE_REG;
    DWORD *AI_BITRATE_REG;
    void (__cdecl *CheckInterrupts)();
} AUDIO_INFO;

typedef struct OSTask {
    DWORD type, flags;
    DWORD ucode_boot, ucode_boot_size;
    DWORD ucode, ucode_size;
    DWORD ucode_data, ucode_data_size;
    DWORD dram_stack, dram_stack_size;
    DWORD output_buff, output_buff_size;
    DWORD data_ptr, data_size;
    DWORD yield_data_ptr, yield_data_size;
} OSTask;

extern char PluginName[];
extern BYTE *RDRAM;
extern BOOL HLEAudio;
extern int DumpMicrocode;
extern OSTask AudioTask;

void DetectCPUFeatures();
void ProcessAudioTask(OSTask *task);
void FreeAudioHLE();
void AIConfigure(HWND window);
void AISetUCodeID(unsigned int id);

#endif
