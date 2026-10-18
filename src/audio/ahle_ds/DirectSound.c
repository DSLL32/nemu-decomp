/* DirectSound output and the original Nemu audio-register interface.
   Filename and descriptive private identifiers are inferred from behavior. */
#include "transform.h"
#include "DirectSound.h"
#include <commctrl.h>
#include <stdlib.h>
#include <string.h>
#pragma intrinsic(_byteswap_ulong)

BYTE *RDRAM; /* 1016bae4; shared guest-memory pointer */
HINSTANCE AudioInstance; /* 1016baf0 */
HWND AudioWindow; /* 1016bafc */
LPDIRECTSOUND DirectSound; /* 1016c0e0 */
LPDIRECTSOUNDBUFFER SoundBuffer; /* 1016bae8 */
LPDIRECTSOUNDBUFFER PrimaryBuffer; /* 1016ba74 */
DSBUFFERDESC BufferDescription; /* 10169520 */
WAVEFORMATEX WaveFormat; /* 1016bfc0 */
CRITICAL_SECTION SoundCriticalSection; /* 1016c100 */
char *AudioDescription = "LaC's Nemu64 Audio DLL\nLittle Endian/MMX version 3.0"; /* 10039398 */
char *AudioCredits = "Greets to TNSe and Jabo for SIMD help."; /* 1003939c */
int RequestedBufferSize = 32768; /* 100393a0 */
int SoundBufferSize = 32768; /* 100393a4 */
int DirectHLEAudio = 1; /* 100393a8 */
int SyncAudio = 1; /* 100393ac */
DWORD SampleRate = 44100, PreviousSampleRate = 44100; /* 100393b0,b4 */
/* Retained initialized values; their original names and purpose are unknown. */
int AudioInitialValue544 = 544; /* 100393b8 */
DWORD AudioClock = 48681812; /* 100393bc */
int AudioInitialValue1 = 1; /* 100393c0 */
int IsNTSC; /* 1016c0e8 */
char AudioAboutText[256]; /* 1016bfe0 */
/* Both 2001 and 2003 builds reserve an 8192-byte interval for this buffer. */
char AudioErrorText[8192]; /* 10169560; capacity inferred from adjacent objects */
BYTE *QueuedBlocks[300]; /* 1016bb00 */
int QueuedBlockReady[300]; /* 1016b580 */
DWORD QueuedBlockLength[300]; /* 10169060 */
unsigned short QueueRead, QueueWrite; /* 10169040,1016ba78 */
int QueueCount, QueueBytes; /* 10169038,1016ba84 */
int BufferPosition; /* 1016bae0 */
DWORD CurrentPlayPosition; /* 1016b568 */
int BufferLimit; /* 1016ba30 */
volatile DWORD CurrentAudioLength; /* 1016ba60 */
HANDLE SoundEvents[16]; /* 1016baa0 */
DWORD TimerUser; /* 1016b564 */
struct AudioRegisters *AIRegisters; /* 10169510 */
struct AudioMIRegisters *MIRegisters; /* 1016ba7c */
struct AudioCPURegisters *CPURegisters; /* 1016b560 */
DWORD *AudioEvents; /* 1016baec */
FILE *HLEWaveFile, *AIWaveFile, *HLELogFile, *ABIFile; /* 1016ba64,1016903c,1016ba80,1016bfd8 */
extern DWORD AudioUCode; /* 1016c300 */
extern void SelectAudioABI(void); /* 10002a40 */
extern void DetectCPUFeatures(void);
extern void FreeAudioHLE(void);
extern void RaiseAudioInterrupt(void); /* 100137c0 */

/* Original strong BSS, in address order. Explicit zero initialization keeps
   external visibility. Later state has 48 unresolved bytes of original storage;
   no declarations have been invented to fill those gaps. */
BYTE AudioBuffer[0x80000] = {0}; /* 0x10040ed0 */
BYTE *AllocatedBuffer = 0; /* 0x100c0ed0 */
DWORD PALRates[0x4000] = {0}; /* 0x100c0ed8 */
DWORD TimerID = 0; /* 0x100d0ed8 */
DWORD NotificationPosition = 0; /* 0x100d0edc */
BYTE RegisterAudioBuffer[0x8000] = {0}; /* 0x100d0ee0 */
DWORD NTSCRates[0x4000] = {0}; /* 0x100d8ee0 */
int CriticalSectionReady = 0; /* 0x100e8ee4 */
int DebugMode = 0; /* 0x100e8ee8 */
int DummyMode = 0; /* 0x100e8ef0 */
int RecordHLE = 0; /* 0x100e8ef4 */
int RecordAI = 0; /* 0x100e8ef8 */
int DumpMicrocode = 0; /* 0x100e8efc */
int FilterHLE = 0; /* 0x100e8f00 */
int StereoReversed = 0; /* 0x100e8f04 */
int Prebuffer = 0; /* 0x100e8f08 */
int HighQuality = 0; /* 0x100e8f0c */
int RecordABI = 0; /* 0x100e8f10 */
int AIWritePending = 0; /* 0x100e8f14 */
int Use3DNow = 0; /* 0x100e8f18 */
int UseSSE = 0; /* 0x100e8f1c */
int UseMMXExt = 0; /* 0x100e8f20 */
int DroppedBlocks = 0; /* 0x100e8f28 */
int ClearCount = 0; /* 0x100e8f2c */
int TotalBytes = 0; /* 0x100e8f30 */
int TotalBlocks = 0; /* 0x100e8f34 */
int LegacyAudioState = 0; /* 0x100e8f40 */
BYTE *CurrentAudio = 0; /* 0x100e8f44 */
int QueueEnabled = 0; /* 0x100e8f48 */
int AudioRunning = 0; /* 0x100e8f4c */
DWORD AudioCount = 0; /* 0x100e8f50 */
int QueueAllocated = 0; /* 0x100e8f54 */
DWORD RequestedLength = 0; /* 0x100e8f58 */
volatile DWORD PlayPosition = 0; /* 0x100e8f5c */
int LastBlockLength = 0; /* 0x100e8f64 */
volatile int WritePosition = 0; /* 0x100e8f68 */
volatile int PreviousPosition = 0; /* 0x100e8f6c */
volatile int PositionState = 0; /* 0x100e8f70 */
int LastWritePosition = 0; /* 0x100e8f74 */
int DirectSoundReady = 0; /* 0x100e8f78 */
DWORD CopiedLength = 0; /* 0x100e8f7c */
DWORD RemainingLength = 0; /* 0x100e8f80 */
DWORD SubmittedLength = 0; /* 0x100e8f84 */
DWORD ReleasedPosition = 0; /* 0x100e8f8c */
volatile int BufferUnderrun = 0; /* 0x100e8f9c */
volatile int TimerIdleCount = 0; /* 0x100e8fa0 */
int ResetPosition = 0; /* 0x100e8fa4 */
int PendingBytes = 0; /* 0x100e8fa8 */
float BufferDuration = 0; /* 0x100e8fac */
int ScrollLockDown = 0; /* 0x100e8fb0 */
int ScrollLockReleased = 0; /* 0x100e8fb4 */
float LastBufferTime = 0; /* 0x100e8fb8 */
volatile int TimerActive = 0; /* 0x100e8fc4 */

/* 10003c40–10003ec0, including switch tables and alignment. */
void AudioError(const char *text, HRESULT result)
{
    const char *error;
    const char *message = text;
    if (text) MessageBox(AudioWindow, text, "Audio Plugin Error", MB_OK);
    switch (result) {
    case DSERR_NOAGGREGATION: error = "DSERR_NOAGGREGATION"; break;
    case DSERR_GENERIC: error = "DSERR_GENERIC"; break;
    case DSERR_UNSUPPORTED: error = "DSERR_UNSUPPORTED"; break;
    case DSERR_OUTOFMEMORY: error = "DSERR_OUTOFMEMORY"; break;
    case DSERR_CONTROLUNAVAIL: error = "DSERR_CONTROLUNAVAIL"; break;
    case DSERR_ALLOCATED: error = "DSerrtxt_ALLOCATED"; break;
    case DSERR_INVALIDPARAM: error = "DSERR_INVALIDPARAM"; break;
    case DSERR_INVALIDCALL: error = "DSERR_INVALIDCALL"; break;
    case DSERR_PRIOLEVELNEEDED: error = "DSERR_PRIOLEVELNEEDED"; break;
    case DSERR_BADFORMAT: error = "DSERR_BADFORMAT"; break;
    case DSERR_NODRIVER: error = "DSERR_NODRIVER"; break;
    case DSERR_ALREADYINITIALIZED: error = "DSERR_ALREADYINITIALIZED"; break;
    case DSERR_BUFFERLOST: error = "DSERR_BUFFERLOST"; break;
    case DSERR_OTHERAPPHASPRIO: error = "DSERR_OTHERAPPHASPRIO"; break;
    case DSERR_UNINITIALIZED: error = "DSERR_UNINITIALIZED"; break;
    default:
        sprintf(AudioErrorText, "unknown error %lu", result);
        error = text;
        message = AudioErrorText;
        break;
    }
    MessageBox(AudioWindow, message, error, MB_OK);
}
/* 10003ec0–10003f40 */
HRESULT InitializeDirectSound(HWND window)
{
    HRESULT result;
    if (CriticalSectionReady) {
        CriticalSectionReady = 0;
        DeleteCriticalSection(&SoundCriticalSection);
    }
    InitializeCriticalSection(&SoundCriticalSection);
    CriticalSectionReady = 1;
    result = DirectSoundCreate(NULL, &DirectSound, NULL);
    if (result) return result;
    result = IDirectSound_SetCooperativeLevel(DirectSound, window, DSSCL_PRIORITY);
    if (result) return result;
    if (DirectSound) DirectSoundReady = 1;
    return 0;
}
/* 10003f40–10003f50 */
void AIStop(void) {}
/* 10003f50–10004030 */
void AIKillSound(void)
{
    void *first, *second;
    DWORD firstLength, secondLength;
    HRESULT result = IDirectSoundBuffer_Lock(SoundBuffer, 0, SoundBufferSize,
                                            &first, &firstLength, &second, &secondLength, 0);
    if (result) { AudioError("ds lock error 1", result); return; }
    if (firstLength > 0) memset(first, 0, firstLength);
    if (secondLength > 0) memset(second, 0, secondLength);
    result = IDirectSoundBuffer_Unlock(SoundBuffer, first, firstLength, second, secondLength);
    if (result) { AudioError("ds unlock error 2", result); return; }
    result = IDirectSoundBuffer_Play(SoundBuffer, 0, 0, DSBPLAY_LOOPING);
    if (result) AudioError("DirectSoundBuffer_Play", result);
}
/* 10004030–10004060 */
void AISetUCodeID(unsigned int id)
{
    AudioUCode = id - 1;
    if (DummyMode) AudioUCode = -1;
    SelectAudioABI();
}
/* 10004060–10004070 */
void AudioReserved(void) {}
/* 10004070–10004140 */
void UpdateAudioFrequency(void)
{
    DWORD frequency;
    if (DummyMode) return;
    frequency = IsNTSC ? NTSCRates[AIRegisters->dacrate + 1] : PALRates[AIRegisters->dacrate + 1];
    if (frequency != SampleRate) {
        IsNTSC = *(DWORD *)(RDRAM + 0x300);
        if (IsNTSC) {
            AudioClock = 48681812;
            SampleRate = NTSCRates[AIRegisters->dacrate + 1];
        } else {
            AudioClock = 49656530;
            SampleRate = PALRates[AIRegisters->dacrate + 1];
        }
        if (CriticalSectionReady) EnterCriticalSection(&SoundCriticalSection);
        IDirectSoundBuffer_SetFrequency(SoundBuffer, SampleRate);
        if (CriticalSectionReady) LeaveCriticalSection(&SoundCriticalSection);
    }
}
/* 10004140–10004160. Retained sample pass. The minimal load/writeback source
   compiles to the observed empty loop; its compiler-eliminated sample
   expression is not uniquely recoverable. */
void AudioSampleLoop(void *data, unsigned int bytes)
{
    short *samples = (short *)data;
    int i;
    for (i = 0; i < bytes / 2; ++i) {
        short sample = samples[i];
        samples[i] = sample;
    }
}
/* 10004160–100041c0. Original explicit assembly, including four-sample unroll. */
void SwapSampleBytes(void *destination, const void *source, unsigned int bytes)
{
    __asm {
        mov edi, destination
        mov ecx, source
        mov edx, 0
    next:
        mov ax, [ecx+edx]
        rol ax, 8
        mov [edi+edx], ax
        add edx, 2
        mov ax, [ecx+edx]
        rol ax, 8
        mov [edi+edx], ax
        add edx, 2
        mov ax, [ecx+edx]
        rol ax, 8
        mov [edi+edx], ax
        add edx, 2
        mov ax, [ecx+edx]
        rol ax, 8
        mov [edi+edx], ax
        add edx, 2
        cmp edx, bytes
        jb next
    }
}
/* 100041c0–100041d0 */
void ConvertAudioSamples(void *destination, const void *source, unsigned int bytes)
{ SwapSampleBytes(destination, source, bytes); }
/* 100041d0–10004200 */
void CopyAudioSamples(void *destination, const void *source, unsigned int bytes)
{
    unsigned int i;
    for (i = 0; i < bytes / 2; ++i) ((short *)destination)[i] = ((const short *)source)[i];
}
/* 10004200–10004230 */
void CopyOutputSamples(void *destination, const void *source, unsigned int bytes)
{
    unsigned int i;
    for (i = 0; i < bytes / 2; ++i) ((short *)destination)[i] = ((const short *)source)[i];
}
/* 10004230–10004260 */
void ReadAudioMemory(void *destination, const void *source, unsigned int bytes)
{
    unsigned int i;
    for (i = 0; i < bytes / 4; ++i) {
        ((DWORD *)destination)[i] = _byteswap_ulong(((const DWORD *)source)[i]);
    }
}
/* 10004260–100042c0 */
void AllocateAudioQueue(void)
{
    unsigned short i;
    if (!QueueAllocated) {
        for (i = 0; i < 300; ++i) {
            QueuedBlocks[i] = malloc(0x500);
            QueuedBlockReady[i] = 0;
        }
        QueueRead = 0;
        QueueWrite = 0;
        QueueCount = 0;
        QueueBytes = 0;
        QueueAllocated = 1;
    }
}
/* 100042c0–10004300 */
void FreeAudioQueue(void)
{
    unsigned short i;
    if (QueueAllocated) {
        for (i = 0; i < 300; ++i) if (QueuedBlocks[i]) free(QueuedBlocks[i]);
        QueueAllocated = 0;
    }
}
/* 10004300–10004380 */
void ReadAudioQueue(void)
{
    DWORD length;
    if (!QueuedBlockReady[QueueRead]) { CurrentAudio = NULL; return; }
    QueuedBlockReady[QueueRead] = 0;
    length = QueuedBlockLength[QueueRead];
    memcpy(AudioBuffer, QueuedBlocks[QueueRead], length);
    CurrentAudio = AudioBuffer;
    CurrentAudioLength = length;
    QueuedBlockLength[QueueRead] = 0;
    if (++QueueRead >= 300) QueueRead = 0;
}
/* 10004380–100043e0 */
void AllocateSoundBuffer(void)
{
    AllocatedBuffer = malloc(SoundBufferSize);
    BufferPosition = SoundBufferSize / 2;
    CurrentPlayPosition = 0;
    CurrentAudioLength = 0;
    CurrentAudio = NULL;
    memset(AllocatedBuffer, 0, SoundBufferSize);
}
/* 100043e0–10004410 */
void ClearAudio(void *destination, DWORD bytes)
{
    memset(destination, 0, bytes);
    if (ClearCount) ++ClearCount;
}
/* 10004410–100044d0 */
void WaitForSoundBuffer(void)
{
    DWORD play, write;
    int position = BufferPosition;
    HRESULT result = IDirectSoundBuffer_GetCurrentPosition(SoundBuffer, &write, &play);
    if (result) { AudioError("ds get current postion", result); return; }
    BufferLimit = SoundBufferSize - 2 * LastBlockLength;
    if (SyncAudio) {
        while ((int)((position - play) & (SoundBufferSize - 1)) > BufferLimit) {
            result = IDirectSoundBuffer_GetCurrentPosition(SoundBuffer, &write, &play);
            if (result) { AudioError("ds get current postion", result); return; }
        }
    }
}
/* 100044d0–100045b0 */
void ClearSoundBuffer(DWORD position, DWORD bytes)
{
    void *first, *second;
    DWORD firstLength, secondLength;
    HRESULT result = IDirectSoundBuffer_Stop(SoundBuffer);
    if (result) { AudioError("DirectSoundBuffer_Play", result); return; }
    result = IDirectSoundBuffer_Lock(SoundBuffer, position, bytes, &first, &firstLength,
                                    &second, &secondLength, 0);
    if (result) { AudioError("ds lock error 4", result); return; }
    if (firstLength > 0) ClearAudio(first, firstLength);
    if (secondLength > 0) ClearAudio(second, secondLength);
    result = IDirectSoundBuffer_Unlock(SoundBuffer, first, firstLength, second, secondLength);
    if (result) { AudioError("ds unlock error", result); return; }
    result = IDirectSoundBuffer_Play(SoundBuffer, 0, 0, DSBPLAY_LOOPING);
    if (result) AudioError("DirectSoundBuffer_Play", result);
}
/* 100045b0–10004660 */
DWORD AudioNotificationThread(void)
{
    DWORD result, play, write;
    result = WaitForMultipleObjects(SoundBufferSize / 0x1000, SoundEvents, FALSE, INFINITE);
    while (result != WAIT_FAILED) {
        if (!result) {
            IDirectSoundBuffer_GetCurrentPosition(SoundBuffer, &play, &write);
            if ((int)((play - NotificationPosition) & (SoundBufferSize - 1)) >= SoundBufferSize - LastBlockLength)
                BufferUnderrun = 1;
            NotificationPosition = play;
        }
        result = WaitForMultipleObjects(SoundBufferSize / 0x1000, SoundEvents, FALSE, INFINITE);
    }
    return 0;
}
/* 10004660–100046b0 */
DWORD GetSoundPosition(void)
{
    DWORD play, write;
    HRESULT result;
    if (AudioRunning) {
        result = IDirectSoundBuffer_GetCurrentPosition(SoundBuffer, &play, &write);
        if (result) { AudioError("ds get current postion  in Get_Pos()", result); return 0; }
        return play & ~15;
    }
    return 0;
}
/* 100046b0–100046c0 */
void CALLBACK AudioTimerReserved(UINT id, UINT message, DWORD user, DWORD a, DWORD b) {}
/* 100046c0–10004a20 */
void ShowDriverInfo(void)
{
    DSCAPS caps;
    HRESULT result;
    if (!AudioRunning) return;
    if (DriverInfoVisible) { CloseAudioLog(); return; }
    OpenAudioLog();
    DebugPrint("Audio Driver Info:\n");
    caps.dwSize = sizeof(caps);
    result = IDirectSound_GetCaps(DirectSound, &caps);
    if (result) {
        DebugPrint("\nUnable to get info.\n");
        if (result == DSERR_UNINITIALIZED)
            DebugPrint("DirectSound not initialized. Try starting a rom first...");
        else if (result == DSERR_GENERIC) DebugPrint("DSERR_GENERIC");
        else if (result == DSERR_INVALIDPARAM) DebugPrint("DSERR_INVALIDPARAM");
        DebugPrint("\n\n");
    }
    DebugPrint("Flags:\n");
    DebugPrint("DSCAPS_CERTIFIED %s\n", caps.dwFlags & DSCAPS_CERTIFIED ? "YES" : "NO");
    DebugPrint("DSCAPS_CONTINUOUSRATE %s\n", caps.dwFlags & DSCAPS_CONTINUOUSRATE ? "YES" : "NO");
    DebugPrint("DSCAPS_EMULDRIVER %s\n", caps.dwFlags & DSCAPS_EMULDRIVER ? "YES" : "NO");
    DebugPrint("DSCAPS_PRIMARY16BIT %s\n", caps.dwFlags & DSCAPS_PRIMARY16BIT ? "YES" : "NO");
    DebugPrint("DSCAPS_PRIMARY8BIT %s\n", caps.dwFlags & DSCAPS_PRIMARY8BIT ? "YES" : "NO");
    DebugPrint("DSCAPS_PRIMARYMONO %s\n", caps.dwFlags & DSCAPS_PRIMARYMONO ? "YES" : "NO");
    DebugPrint("DSCAPS_PRIMARYSTEREO %s\n", caps.dwFlags & DSCAPS_PRIMARYSTEREO ? "YES" : "NO");
    DebugPrint("DSCAPS_SECONDARY16BIT %s\n", caps.dwFlags & DSCAPS_SECONDARY16BIT ? "YES" : "NO");
    DebugPrint("DSCAPS_SECONDARY8BIT %s\n", caps.dwFlags & DSCAPS_SECONDARY8BIT ? "YES" : "NO");
    DebugPrint("DSCAPS_SECONDARYMONO %s\n", caps.dwFlags & DSCAPS_SECONDARYMONO ? "YES" : "NO");
    DebugPrint("DSCAPS_SECONDARYSTEREO %s\n", caps.dwFlags & DSCAPS_SECONDARYSTEREO ? "YES" : "NO");
    DebugPrint("Hardware:\n");
    DebugPrint("dwMinSecondarySampleRate  0x%X\n", caps.dwMinSecondarySampleRate);
    DebugPrint("dwMaxSecondarySampleRate  0x%X\n", caps.dwMaxSecondarySampleRate);
    DebugPrint("dwPrimaryBuffers 0x%X\n", caps.dwPrimaryBuffers);
    DebugPrint("dwMaxHwMixingAllBuffers 0x%X\n", caps.dwMaxHwMixingAllBuffers);
    DebugPrint("dwMaxHwMixingStaticBuffers 0x%X\n", caps.dwMaxHwMixingStaticBuffers);
    DebugPrint("dwMaxHwMixingStreamingBuffers 0x%X\n", caps.dwMaxHwMixingStreamingBuffers);
    DebugPrint("dwFreeHwMixingAllBuffers 0x%X\n", caps.dwFreeHwMixingAllBuffers);
    DebugPrint("dwFreeHwMixingStaticBuffers 0x%X\n", caps.dwFreeHwMixingStaticBuffers);
    DebugPrint("dwFreeHwMixingStreamingBuffers 0x%X\n", caps.dwFreeHwMixingStreamingBuffers);
    DebugPrint("dwMaxHw3DAllBuffers 0x%X\n", caps.dwMaxHw3DAllBuffers);
    DebugPrint("dwMaxHw3DStaticBuffers 0x%X\n", caps.dwMaxHw3DStaticBuffers);
    DebugPrint("dwMaxHw3DStreamingBuffers 0x%X\n", caps.dwMaxHw3DStreamingBuffers);
    DebugPrint("dwFreeHw3DAllBuffers 0x%X\n", caps.dwFreeHw3DAllBuffers);
    DebugPrint("dwFreeHw3DStaticBuffers 0x%X\n", caps.dwFreeHw3DStaticBuffers);
    DebugPrint("dwFreeHw3DStreamingBuffers 0x%X\n", caps.dwFreeHw3DStreamingBuffers);
    DebugPrint("dwTotalHwMemBytes 0x%X\n", caps.dwTotalHwMemBytes);
    DebugPrint("dwFreeHwMemBytes 0x%X\n", caps.dwFreeHwMemBytes);
    DebugPrint("dwMaxContigFreeHwMemBytes 0x%X\n", caps.dwMaxContigFreeHwMemBytes);
    DebugPrint("dwUnlockTransferRateHwBuffers 0x%X\n", caps.dwUnlockTransferRateHwBuffers);
    DebugPrint("dwPlayCpuOverheadSwBuffers 0x%X\n", caps.dwPlayCpuOverheadSwBuffers);
}
struct WaveHeader {
    char riff[4]; DWORD size;
    char wave[4], fmt[4]; DWORD formatSize;
    WORD format, channels; DWORD rate, bytesPerSecond;
    WORD blockAlign, bits;
    char data[4]; DWORD dataSize;
};
/* 10004a20–10004b10 */
void WriteWaveHeader(FILE *file)
{
    struct WaveHeader header;
    long position;
    fflush(file);
    position = ftell(file);
    header.riff[0] = 'R'; header.riff[1] = 'I';
    header.riff[2] = 'F'; header.riff[3] = 'F';
    fseek(file, 0, SEEK_END);
    header.size = ftell(file) + 36;
    fseek(file, 0, SEEK_SET);
    header.wave[0] = 'W'; header.wave[1] = 'A';
    header.wave[2] = 'V'; header.wave[3] = 'E';
    header.fmt[0] = 'f'; header.fmt[1] = 'm';
    header.fmt[2] = 't'; header.fmt[3] = ' ';
    header.formatSize = 16;
    header.format = 1;
    header.channels = 2;
    header.rate = SampleRate & ~8;
    header.bytesPerSecond = SampleRate * 4;
    header.blockAlign = 4;
    header.bits = 16;
    header.data[0] = 'd'; header.data[1] = 'a';
    header.data[2] = 't'; header.data[3] = 'a';
    header.dataSize = position - 44;
    fwrite(&header, 1, 44, file);
    fflush(file);
    fseek(file, position, SEEK_SET);
}
/* 10004b10–10004b40 */
BOOL ReadIniString(char *section, char *key, char *fallback, char *value, char *file)
{
    GetPrivateProfileString(section, key, fallback, value, 256, file);
    return TRUE;
}
/* 10004b40–10004b90. The original passes 256 for a 32-byte value buffer. */
int ReadIniInt(char *section, char *key, int fallback, char *file)
{
    char defaultText[32], value[32];
    itoa(fallback, defaultText, 10);
    GetPrivateProfileString(section, key, defaultText, value, 256, file);
    return atoi(value);
}
/* 10004b90–10004bb0 */
BOOL WriteIniString(char *section, char *key, char *value, char *file)
{ return WritePrivateProfileString(section, key, value, file); }
/* 10004bb0–10004bf0 */
BOOL WriteIniInt(char *section, char *key, int value, char *file)
{
    char text[32];
    itoa(value, text, 10);
    return WritePrivateProfileString(section, key, text, file);
}
char AudioIniFile[MAX_PATH];
/* 64-byte interval before SeenCommands; exact original capacity is inferred. */
char AudioIniSection[64];
char AudioIniKey[32];
/* 10004bf0–10004db0 */
void ReadAudioSettings(void)
{
    lstrcpy(AudioIniFile, "");
    GetSystemDirectory(AudioIniFile, MAX_PATH);
    if (AudioIniFile[lstrlen(AudioIniFile) - 1] != '\\') lstrcat(AudioIniFile, "\\");
    lstrcat(AudioIniFile, "NemuAudio08.ini");
    lstrcpy(AudioIniKey, "DebugMode");
    DebugMode = ReadIniInt(AudioIniSection, AudioIniKey, 0, AudioIniFile);
    if (DebugMode) OpenAudioLog();
    lstrcpy(AudioIniKey, "ScrollBuffSize");
    RequestedBufferSize = ReadIniInt(AudioIniSection, AudioIniKey, 0xc000, AudioIniFile);
    lstrcpy(AudioIniKey, "DummyMode");
    DummyMode = ReadIniInt(AudioIniSection, AudioIniKey, 0, AudioIniFile);
    lstrcpy(AudioIniKey, "HLEAudio");
    DirectHLEAudio = ReadIniInt(AudioIniSection, AudioIniKey, 1, AudioIniFile);
    lstrcpy(AudioIniKey, "StereoReversed");
    StereoReversed = ReadIniInt(AudioIniSection, AudioIniKey, 0, AudioIniFile);
    lstrcpy(AudioIniKey, "HlePreBuffer");
    Prebuffer = ReadIniInt(AudioIniSection, AudioIniKey, 1, AudioIniFile);
    lstrcpy(AudioIniKey, "HighQuality");
    HighQuality = ReadIniInt(AudioIniSection, AudioIniKey, 1, AudioIniFile);
    lstrcpy(AudioIniKey, "SyncAudio");
    SyncAudio = ReadIniInt(AudioIniSection, AudioIniKey, 1, AudioIniFile);
}
/* 10004db0–10004de0 */
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        AudioInstance = instance;
        ReadAudioSettings();
        return TRUE;
    case DLL_PROCESS_DETACH:
        CloseAudioLog();
    }
    return TRUE;
}
/* 10004de0–10004ea0 */
void StopAudio(void)
{
    HRESULT result;
    Sleep(500);
    if (AudioRunning) {
        AudioRunning = 0;
        timeKillEvent(TimerID);
        timeEndPeriod(1);
        result = IDirectSoundBuffer_Stop(SoundBuffer);
        if (result) { AudioError("DirectSoundBuffer_Stop", result); return; }
        if (CriticalSectionReady) {
            CriticalSectionReady = 0;
            DeleteCriticalSection(&SoundCriticalSection);
        }
        if (SoundBuffer) IDirectSoundBuffer_Release(SoundBuffer);
        if (PrimaryBuffer) IDirectSoundBuffer_Release(PrimaryBuffer);
        if (DirectSound) IDirectSound_Release(DirectSound);
        if (AllocatedBuffer) free(AllocatedBuffer);
        FreeAudioQueue();
        CurrentAudio = NULL;
        CurrentAudioLength = 0;
    }
}
/* 10004ea0–10004ee0 */
void AIFree(void)
{
    StopAudio();
    AIRegisters->nextLength = 0;
    SampleRate = 0;
    FlushAudioLog();
    FreeAudioHLE();
    if (!TotalBlocks) TotalBlocks = 1;
}
/* 10004ee0–10004f00 */
void ReleaseAudioBlock(DWORD position)
{
    if (QueueEnabled) ReadAudioQueue();
    else ReleasedPosition = position;
}
/* 10004f00–10004f20 */
void WriteAudioSamples(void *destination, const void *source, unsigned int bytes)
{
    ClearCount = 1;
    CopiedLength = bytes;
    CopyOutputSamples(destination, source, bytes);
}
/* 10004f20–10004f50 */
void WriteAudioBytes(void *destination, const void *source, unsigned int bytes)
{
    ClearCount = 1;
    CopiedLength = bytes;
    memcpy(destination, source, bytes);
}
/* 10004f50–10005010 */
void WriteSoundBuffer(DWORD position, const void *source, DWORD bytes)
{
    void *first, *second;
    DWORD firstLength, secondLength;
    HRESULT result = IDirectSoundBuffer_Lock(SoundBuffer, position, bytes, &first,
                                            &firstLength, &second, &secondLength, 0);
    if (result) { AudioError("ds lock error 3", result); return; }
    if (firstLength > 0) WriteAudioBytes(first, source, firstLength);
    if (secondLength > 0) WriteAudioBytes(second, (const BYTE *)source + firstLength, secondLength);
    result = IDirectSoundBuffer_Unlock(SoundBuffer, first, firstLength, second, secondLength);
    if (result) AudioError("ds unlock error", result);
}
/* 10005010–10005470 */
void SubmitAudio(BYTE *source, int bytes)
{
    DWORD play, write;
    void *first, *second;
    DWORD firstLength, secondLength;
    __int64 counter, frequency;
    float now, samples;
    int distance, position;
    HRESULT result;
    if (ScrollLockDown && !GetAsyncKeyState(VK_SCROLL)) {
        ScrollLockDown = 0;
        ScrollLockReleased = 1;
    } else ScrollLockReleased = 0;
    if (GetAsyncKeyState(VK_SCROLL)) ScrollLockDown = 1;
    if (ScrollLockReleased) SyncAudio = !SyncAudio;
    TimerIdleCount = 0;
    if (!AudioRunning) return;
    if (!Prebuffer) {
        if (!CurrentAudio) {
            ReadAudioMemory(AudioBuffer, source, bytes);
            ConvertAudioSamples(AudioBuffer, AudioBuffer, bytes);
            CurrentAudioLength = bytes;
            CurrentAudio = AudioBuffer;
        } else ++DroppedBlocks;
        return;
    }
    ReadAudioMemory((DWORD *)AudioBuffer + PendingBytes / 4, source, bytes);
    ConvertAudioSamples((short *)AudioBuffer + PendingBytes / 2,
                        (short *)AudioBuffer + PendingBytes / 2, bytes);
    PendingBytes += bytes;
    if (PendingBytes < 0x1000) return;
    bytes = PendingBytes;
    PendingBytes = 0;
    if (ResetPosition) {
        IDirectSoundBuffer_Stop(SoundBuffer);
        IDirectSoundBuffer_GetCurrentPosition(SoundBuffer, &play, &write);
        WritePosition = 0;
        IDirectSoundBuffer_SetCurrentPosition(SoundBuffer, 0);
        IDirectSoundBuffer_Play(SoundBuffer, 0, 0, DSBPLAY_LOOPING);
        ResetPosition = 0;
    }
    IDirectSoundBuffer_GetCurrentPosition(SoundBuffer, &play, &write);
    BufferLimit = SoundBufferSize - 2 * LastBlockLength;
    distance = (WritePosition - write) & (SoundBufferSize - 1);
    QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now = (float)((double)counter / frequency);
    if (BufferUnderrun) BufferUnderrun = 1;
    if (BufferLimit != SoundBufferSize && SyncAudio && !ResetPosition) {
        if (!BufferUnderrun) {
            while (distance > BufferLimit) {
                result = IDirectSoundBuffer_GetCurrentPosition(SoundBuffer, &play, &write);
                if (result) { AudioError("ds get current postion", result); return; }
                distance = (WritePosition - write) & (SoundBufferSize - 1);
            }
        } else {
            IDirectSoundBuffer_GetCurrentPosition(SoundBuffer, &play, &write);
            distance = (WritePosition - write) & (SoundBufferSize - 1);
        }
    }
    position = WritePosition;
    LastBufferTime = now;
    LastWritePosition = WritePosition;
    samples = (float)(int)((WritePosition - play) & (SoundBufferSize - 1));
    BufferDuration = ((float)bytes * 0.25f / SampleRate) + (samples * 0.25f / SampleRate);
    LastBlockLength = bytes;
    WritePosition += bytes;
    if (WritePosition >= SoundBufferSize) WritePosition -= SoundBufferSize;
    BufferUnderrun = 0;
    if (CriticalSectionReady) EnterCriticalSection(&SoundCriticalSection);
    result = IDirectSoundBuffer_Lock(SoundBuffer, position, bytes, &first, &firstLength,
                                    &second, &secondLength, 0);
    if (result) {
        AudioError("ds lock error 6", result);
        if (CriticalSectionReady) LeaveCriticalSection(&SoundCriticalSection);
        return;
    }
    if (CriticalSectionReady) LeaveCriticalSection(&SoundCriticalSection);
    if (firstLength > 0) WriteAudioBytes(first, AudioBuffer, firstLength);
    if (secondLength > 0) WriteAudioBytes(second, AudioBuffer + firstLength, secondLength);
    if (CriticalSectionReady) EnterCriticalSection(&SoundCriticalSection);
    result = IDirectSoundBuffer_Unlock(SoundBuffer, first, firstLength, second, secondLength);
    if (result) { AudioError("ds unlock error", result); return; }
    if (CriticalSectionReady) LeaveCriticalSection(&SoundCriticalSection);
    ResetPosition = 0;
}
/* 10005470–10005760 */
void CALLBACK AudioTimer(UINT id, UINT message, DWORD user, DWORD a, DWORD b)
{
    int bytes;
    ++TimerActive;
    if (TimerActive == 1) {
    if (Prebuffer) {
        if (TimerIdleCount++ > 7) ClearSoundBuffer(0, SoundBufferSize);
        --TimerActive;
        return;
    }
    if (AudioRunning) {
        PlayPosition = GetSoundPosition();
        CurrentPlayPosition = PlayPosition;
        if (!CurrentAudio) {
            if (CurrentPlayPosition < (DWORD)BufferPosition) {
                bytes = SoundBufferSize - BufferPosition;
                if (bytes > 0) {
                    ClearSoundBuffer(BufferPosition, bytes);
                    BufferPosition += bytes;
                }
                bytes = CurrentPlayPosition;
                if (bytes > 0) {
                    ClearSoundBuffer(0, bytes);
                    BufferPosition = bytes;
                }
            } else if (CurrentPlayPosition > (DWORD)BufferPosition) {
                bytes = CurrentPlayPosition - BufferPosition;
                if (bytes > 0) {
                    ClearSoundBuffer(BufferPosition, bytes);
                    BufferPosition += bytes;
                }
            }
        } else {
        next:
            if (CurrentPlayPosition < (DWORD)BufferPosition) {
                bytes = SoundBufferSize - BufferPosition;
                if (bytes > 0 && CurrentAudioLength) {
                    if ((int)CurrentAudioLength - bytes < 1) {
                        bytes = CurrentAudioLength;
                        WriteSoundBuffer(BufferPosition, CurrentAudio, bytes);
                        BufferPosition += bytes;
                        CurrentAudioLength = 0;
                        CurrentAudio = NULL;
                        ReleaseAudioBlock(BufferPosition);
                        if (CurrentAudio) goto next;
                    } else {
                        CurrentAudioLength -= bytes;
                        WriteSoundBuffer(BufferPosition, CurrentAudio, bytes);
                        CurrentAudio += bytes;
                        BufferPosition += bytes;
                    }
                }
                bytes = CurrentPlayPosition;
                if (bytes > 0 && CurrentAudioLength) {
                    if ((int)CurrentAudioLength - bytes < 1) {
                        bytes = CurrentAudioLength;
                        WriteSoundBuffer(0, CurrentAudio, bytes);
                        BufferPosition = bytes;
                        CurrentAudioLength = 0;
                        CurrentAudio = NULL;
                        ReleaseAudioBlock(BufferPosition);
                        if (CurrentAudio) goto next;
                    } else {
                        CurrentAudioLength -= bytes;
                        WriteSoundBuffer(0, CurrentAudio, bytes);
                        CurrentAudio += bytes;
                        BufferPosition = bytes;
                    }
                }
            } else if (CurrentPlayPosition > (DWORD)BufferPosition) {
                bytes = CurrentPlayPosition - BufferPosition;
                if (bytes > 0 && CurrentAudioLength) {
                    if ((int)CurrentAudioLength - bytes < 1) {
                        bytes = CurrentAudioLength;
                        WriteSoundBuffer(BufferPosition, CurrentAudio, bytes);
                        BufferPosition += bytes;
                        CurrentAudioLength = 0;
                        CurrentAudio = NULL;
                        ReleaseAudioBlock(BufferPosition);
                        if (CurrentAudio) goto next;
                    } else {
                        CurrentAudioLength -= bytes;
                        WriteSoundBuffer(BufferPosition, CurrentAudio, bytes);
                        CurrentAudio += bytes;
                        BufferPosition += bytes;
                    }
                }
            }
        }
    }
    }
    --TimerActive;
}
/* 10005760–10005f70, including switch tables and alignment. */
BOOL CALLBACK AudioConfigDialog(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    int oldRecordHLE = RecordHLE;
    int oldRecordAI = RecordAI;
    int oldRecordABI = RecordABI;
    int oldDumpMicrocode = DumpMicrocode;
    int position;
    switch (message) {
    case WM_INITDIALOG:
        SendDlgItemMessage(window, 1036, TBM_SETRANGEMIN, TRUE, 0);
        SendDlgItemMessage(window, 1036, TBM_SETRANGEMAX, TRUE, 3);
        SendDlgItemMessage(window, 1036, TBM_SETTICFREQ, TRUE, 0);
        switch (RequestedBufferSize) {
        case 0x4000: position = 0; break;
        case 0x8000: position = 1; break;
        case 0xc000: position = 2; break;
        case 0x10000: position = 3; break;
        default: RequestedBufferSize = 0x8000; position = 1; break;
        }
        SendDlgItemMessage(window, 1036, TBM_SETPOS, TRUE, position);
        CheckDlgButton(window, 1005, DummyMode ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1006, DebugMode ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1015, RecordHLE ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1016, RecordAI ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1014, DumpMicrocode ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1019, FilterHLE ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1024, DirectHLEAudio ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1025, StereoReversed ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1026, Prebuffer ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1031, SyncAudio ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(window, 1029, HighQuality ? BST_CHECKED : BST_UNCHECKED);
        return TRUE;
    case WM_NOTIFY:
        switch (SendDlgItemMessage(window, 1036, TBM_GETPOS, 0, 0)) {
        case 0: RequestedBufferSize = 0x4000; return TRUE;
        case 1: RequestedBufferSize = 0x8000; return TRUE;
        case 2: RequestedBufferSize = 0xc000; return TRUE;
        case 3: RequestedBufferSize = 0x10000; return TRUE;
        }
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(wparam)) {
        case IDCANCEL:
            EndDialog(window, wparam);
            return TRUE;
        case IDOK:
            RecordHLE = IsDlgButtonChecked(window, 1015) ? 1 : 0;
            RecordAI = IsDlgButtonChecked(window, 1016) ? 1 : 0;
            DumpMicrocode = IsDlgButtonChecked(window, 1014) ? 1 : 0;
            FilterHLE = IsDlgButtonChecked(window, 1019) ? 1 : 0;
            if (RecordHLE != oldRecordHLE) {
                if (HLEWaveFile) fclose(HLEWaveFile);
                else {
                    HLEWaveFile = fopen("hle.wav", "wb");
                    if (!HLEWaveFile) RecordHLE = 0;
                    WriteWaveHeader(HLEWaveFile);
                }
            }
            if (RecordAI != oldRecordAI) {
                if (AIWaveFile) fclose(AIWaveFile);
                else {
                    AIWaveFile = fopen("ai.wav", "wb");
                    if (!AIWaveFile) RecordAI = 0;
                    WriteWaveHeader(AIWaveFile);
                }
            }
            if (DumpMicrocode != oldDumpMicrocode) {
                if (HLELogFile) fclose(HLELogFile);
                else {
                    HLELogFile = fopen("g:/roms/n64/hle.log", "wb");
                    if (!HLELogFile) DumpMicrocode = 0;
                }
            }
            if (RecordABI != oldRecordABI) {
                if (ABIFile) fclose(ABIFile);
                else {
                    ABIFile = fopen("Record.Abi", "wb");
                    if (!ABIFile) RecordAI = 0;
                }
            }
            lstrcpy(AudioIniKey, "ScrollBuffSize");
            WriteIniInt(AudioIniSection, AudioIniKey, RequestedBufferSize, AudioIniFile);
            DummyMode = IsDlgButtonChecked(window, 1005) ? 1 : 0;
            lstrcpy(AudioIniKey, "DummyMode");
            WriteIniInt(AudioIniSection, AudioIniKey, DummyMode, AudioIniFile);
            DirectHLEAudio = IsDlgButtonChecked(window, 1024) ? 1 : 0;
            lstrcpy(AudioIniKey, "HLEAudio");
            WriteIniInt(AudioIniSection, AudioIniKey, DirectHLEAudio, AudioIniFile);
            StereoReversed = IsDlgButtonChecked(window, 1025) ? 1 : 0;
            lstrcpy(AudioIniKey, "StereoReversed");
            WriteIniInt(AudioIniSection, AudioIniKey, StereoReversed, AudioIniFile);
            Prebuffer = IsDlgButtonChecked(window, 1026) ? 1 : 0;
            if (Prebuffer) ReleaseAudioBlock(0);
            lstrcpy(AudioIniKey, "HlePreBuffer");
            WriteIniInt(AudioIniSection, AudioIniKey, Prebuffer, AudioIniFile);
            HighQuality = IsDlgButtonChecked(window, 1029) ? 1 : 0;
            lstrcpy(AudioIniKey, "HighQuality");
            WriteIniInt(AudioIniSection, AudioIniKey, HighQuality, AudioIniFile);
            SyncAudio = IsDlgButtonChecked(window, 1031) ? 1 : 0;
            lstrcpy(AudioIniKey, "SyncAudio");
            WriteIniInt(AudioIniSection, AudioIniKey, SyncAudio, AudioIniFile);
            if (IsDlgButtonChecked(window, 1006)) {
                DebugMode = 1;
                OpenAudioLog();
            } else {
                DebugMode = 0;
                CloseAudioLog();
            }
            EndDialog(window, wparam);
            return TRUE;
        case 1033:
            ShowDriverInfo();
            return TRUE;
        case 1042:
            SendDlgItemMessage(window, 1036, TBM_SETPOS, TRUE, 1);
            CheckDlgButton(window, 1005, 0);
            CheckDlgButton(window, 1006, 0);
            CheckDlgButton(window, 1015, 0);
            CheckDlgButton(window, 1016, 0);
            CheckDlgButton(window, 1014, 0);
            CheckDlgButton(window, 1019, 0);
            CheckDlgButton(window, 1024, 1);
            CheckDlgButton(window, 1025, 0);
            CheckDlgButton(window, 1026, 1);
            CheckDlgButton(window, 1031, 1);
            CheckDlgButton(window, 1029, 1);
            return TRUE;
        case 1035:
            sprintf(AudioAboutText, "%s\nCompiled on:%s %5s\n\n%s",
                    AudioDescription, "Mar 22 2003", "23:20:37", AudioCredits);
            MessageBox(window, AudioAboutText, "About", MB_OK);
            return TRUE;
        }
        break;
    case WM_HSCROLL:
        return TRUE;
    }
    return FALSE;
}
/* 10005f70–10005fb0 */
void AIConfigure(HWND window)
{
    DialogBox(AudioInstance, MAKEINTRESOURCE(101), window, AudioConfigDialog);
    if (SoundBufferSize != RequestedBufferSize)
        MessageBox(NULL, "You have made changes that will not be effective until you reload a rom",
                   "Audio Warning", MB_OK);
}
/* 10005fb0–10006190 */
void StartAudio(void)
{
    HRESULT result;
    Sleep(500);
    if (AudioRunning) return;
    SoundBufferSize = RequestedBufferSize;
    SampleRate = 44100;
    AllocateSoundBuffer();
    AllocateAudioQueue();
    result = InitializeDirectSound(AudioWindow);
    if (result) { AudioError("Could not initialize direct sound.", result); return; }
    BufferDescription.dwReserved = 0;
    BufferDescription.dwSize = sizeof(BufferDescription);
    BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
    BufferDescription.dwBufferBytes = 0;
    BufferDescription.lpwfxFormat = NULL;
    result = IDirectSound_CreateSoundBuffer(DirectSound, &BufferDescription, &PrimaryBuffer, NULL);
    if (result) {
        AudioError("DirectSound_CreateSoundBuffer", result);
        PrimaryBuffer = NULL;
        return;
    }
    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormat.nChannels = 2;
    WaveFormat.nSamplesPerSec = SampleRate;
    WaveFormat.nAvgBytesPerSec = SampleRate * 4;
    WaveFormat.wBitsPerSample = 16;
    WaveFormat.nBlockAlign = 4;
    WaveFormat.cbSize = 0;
    result = IDirectSoundBuffer_SetFormat(PrimaryBuffer, &WaveFormat);
    if (result) {
        AudioError("DirectSoundBuffer_SetFormat", result);
        PrimaryBuffer = NULL;
        return;
    }
    memset(&BufferDescription, 0, sizeof(BufferDescription));
    BufferDescription.dwSize = sizeof(BufferDescription);
    BufferDescription.dwFlags = 0x18120;
    BufferDescription.dwBufferBytes = SoundBufferSize;
    BufferDescription.lpwfxFormat = &WaveFormat;
    result = IDirectSound_CreateSoundBuffer(DirectSound, &BufferDescription, &SoundBuffer, NULL);
    if (result) {
        AudioError("DirectSound_CreateSoundBuffer", result);
        SoundBuffer = NULL;
        return;
    }
    AIKillSound();
    timeBeginPeriod(1);
    TimerID = timeSetEvent(16, 1, AudioTimer, TimerUser, TIME_PERIODIC);
    WritePosition = 0;
    PreviousPosition = 0;
    PositionState = 0;
    LastWritePosition = 0;
    LastBlockLength = 0;
    PlayPosition = 0;
    AudioRunning = 1;
}
/* 10006190–100062c0 */
void AIInit(HWND window, BYTE *rdram, struct AudioCPURegisters *cpu,
                     struct AudioRegisters *ai, struct AudioMIRegisters *mi, DWORD *events)
{
    DWORD rate, divider;
    AudioWindow = window;
    RDRAM = rdram;
    AIRegisters = ai;
    MIRegisters = mi;
    CPURegisters = cpu;
    AudioEvents = events;
    LegacyAudioState = 0;
    DroppedBlocks = 0;
    ClearCount = 0;
    TotalBytes = 0;
    TotalBlocks = 0;
    if (DummyMode) { AudioRunning = 0; return; }
    StartAudio();
    DetectCPUFeatures();
    for (rate = 1; rate < 368000; ++rate) {
        divider = (DWORD)(48681812.0f / rate + 0.5f);
        if (divider > 0x3fff) divider = 0x3fff;
        else if (divider < 132) continue;
        if (rate > NTSCRates[divider]) NTSCRates[divider] = rate;
    }
    for (rate = 1; rate < 376000; ++rate) {
        divider = (DWORD)(49656530.0f / rate + 0.5f);
        if (divider > 0x3fff) divider = 0x3fff;
        else if (divider < 132) continue;
        if (rate > PALRates[divider]) PALRates[divider] = rate;
    }
}
/* 100062c0–100063a0 */
void ProcessAudioRegisters(void)
{
    AudioCount = CPURegisters->words[9];
    if (AIWaveFile) {
        fwrite(RDRAM + AIRegisters->address, AIRegisters->length, 1, AIWaveFile);
        WriteWaveHeader(AIWaveFile);
    }
    RequestedLength = AIRegisters->length;
    if (!AudioRunning) {
        AIRegisters->status &= ~0x40000000;
        AIRegisters->status &= ~0x80000000;
        if (MIRegisters->mask & 4) *AudioEvents |= 16;
        return;
    }
    if (AIRegisters->address < 0x7fffff && (AIRegisters->control & 1) && AIRegisters->length) {
        SubmitAudio(RDRAM + AIRegisters->address, AIRegisters->length);
        SubmittedLength = AIRegisters->length;
        RemainingLength = AIRegisters->length;
        TotalBytes += AIRegisters->length;
        ++TotalBlocks;
    }
}
/* 100063a0–100064c0 */
void AISetReg(DWORD address, DWORD value)
{
    DWORD oldControl;
    switch (address) {
    case 0x04500004:
        if (AIRegisters->length) {
            AIRegisters->nextLength = value & 0x3ffff;
            AIRegisters->status |= 0x80000000;
            return;
        }
        AIRegisters->length = value & 0x3ffff;
    case 0:
        ProcessAudioRegisters();
        AIWritePending = 1;
        return;
    case 0x04500000:
        if (AIRegisters->address) AIRegisters->nextAddress = value & 0xffffff;
        else AIRegisters->address = value & 0xffffff;
        return;
    case 0x04500008:
        oldControl = AIRegisters->control;
        AIRegisters->control = value & 1;
        if (!(oldControl & 1)) ProcessAudioRegisters();
        return;
    case 0x0450000c:
        MIRegisters->interrupt &= ~4;
        if (!MIRegisters->interrupt) CPURegisters->words[13] &= ~0x400;
        return;
    case 0x04500010:
        AIRegisters->dacrate = value & 0xffff;
        UpdateAudioFrequency();
        return;
    case 0x04500014:
        AIRegisters->bitrate = value & 15;
        return;
    default:
        AudioError("AI Reg - write word32 to unknown Register", address);
    }
}
/* 100064c0–10006530 */
void AppendRegisterAudio(BYTE *source, DWORD bytes)
{
    if (AudioRunning) {
        memcpy(RegisterAudioBuffer + CurrentAudioLength, source, bytes);
        CurrentAudioLength += bytes;
        if (CurrentAudioLength >= AIRegisters->length) {
            SubmitAudio(RegisterAudioBuffer, CurrentAudioLength);
            CurrentAudioLength = 0;
        }
    }
}
/* 10006530–100065c0 */
void CompleteAudioDMA(void)
{
    if (AudioRunning) {
        RaiseAudioInterrupt();
        if (AIRegisters->nextLength) {
            AIRegisters->address = AIRegisters->nextAddress;
            AIRegisters->nextAddress = 0;
            AIRegisters->length = AIRegisters->nextLength;
            AIRegisters->nextLength = 0;
            AIRegisters->status &= ~0x80000000;
            ProcessAudioRegisters();
        } else {
            AIRegisters->address = 0;
            AIRegisters->length = 0;
        }
        AIRegisters->status &= ~0x40000000;
        if (MIRegisters->mask & 4) *AudioEvents |= 16;
    }
}
/* 100065c0–100065d0 */
DWORD ReadAudioLength(void) { return 0; }
/* 100065d0–10006610 */
DWORD AIGetReg(DWORD address)
{
    switch (address) {
    case 0x04500004: return ReadAudioLength();
    case 0x0450000c: return AIRegisters->status;
    default:
        AudioError("AI Reg read word32 from unknown Register", address);
        return 0;
    }
}
