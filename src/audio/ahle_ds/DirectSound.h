#ifndef NEMU_DIRECT_SOUND_H
#define NEMU_DIRECT_SOUND_H
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
/* The SDK's legacy COM interfaces are used in C form, as in the input DLL. */
#include <prerelease/dsound.h>

struct AudioRegisters {
    DWORD address, nextAddress, length, nextLength;
    DWORD control, status, dacrate, bitrate;
};
struct AudioMIRegisters { DWORD mode, interrupt, mask; };
struct AudioCPURegisters { DWORD words[14]; };

extern BYTE *RDRAM;
extern int DumpMicrocode;
extern HINSTANCE AudioInstance;
extern HWND AudioWindow;
extern int DebugMode, DummyMode, RecordHLE, RecordAI, FilterHLE;
extern int StereoReversed, Prebuffer, HighQuality, RecordABI;
extern int RequestedBufferSize, SoundBufferSize, DirectHLEAudio, SyncAudio;
extern DWORD SampleRate, PreviousSampleRate, AudioClock;
extern FILE *HLEWaveFile, *AIWaveFile, *HLELogFile, *ABIFile;
extern char AudioIniFile[MAX_PATH], AudioIniSection[128], AudioIniKey[32];
extern int DriverInfoVisible;
extern char *AudioDescription, *AudioCredits;

void DebugPrint(char *format, ...);
void FilePrint(FILE *file, char *format, ...);
void OpenAudioLog(void);
void CloseAudioLog(void);
void FlushAudioLog(void);
void ShowDriverInfo(void);
void WriteWaveHeader(FILE *file);
void ReleaseAudioBlock(DWORD position);
int ReadIniInt(char *section, char *key, int fallback, char *file);
BOOL WriteIniInt(char *section, char *key, int value, char *file);
void ReadAudioMemory(void *destination, const void *source, unsigned int bytes);
void AIStop(void);
void AIKillSound(void);
void AIFree(void);
void AIInit(HWND window, BYTE *rdram, struct AudioCPURegisters *cpu,
            struct AudioRegisters *ai, struct AudioMIRegisters *mi, DWORD *events);
void AISetReg(DWORD address, DWORD value);
DWORD AIGetReg(DWORD address);
void AISetUCodeID(unsigned int id);
void AIConfigure(HWND window);
#endif
