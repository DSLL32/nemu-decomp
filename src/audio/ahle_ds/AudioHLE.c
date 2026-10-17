/* Audio task dispatch and ABI selection. Original private names inferred. */
#include "transform.h"
#include "AudioHLE.h"
#include "AudioCommands.h"
#include "AudioProcessing.h"
#include "DirectSound.h"
#include <stdlib.h>
#include <string.h>
#pragma intrinsic(_byteswap_ulong)

AudioCommand *CurrentCommand;
AudioCommandHandler AudioCommandTable[32];
BOOL HLEAudio;
int AudioTaskSeen;
int AudioBuffersAllocated;
int AudioTaskRunning = 1;
/* Retained diagnostic names and initialized state. Private identifiers are
 * inferred from the original initialized-data contribution. */
const char *AudioCommandNames[16] = {
    "A_SPNOOP",
    "A_ADPCM",
    "A_CLEARBUFF",
    "A_ENVMIXER",
    "A_LOADBUFF",
    "A_RESAMPLE",
    "A_SAVEBUFF",
    "A_SEGMENT",
    "A_SETBUFF",
    "A_SETVOL",
    "A_DMEMMOVE",
    "A_LOADADPCM",
    "A_MIXER",
    "A_INTERLEAVE",
    "A_POLEF",
    "A_SETLOOP",
};
int AudioInitialState = 1; /* Retained unreferenced four-byte value at 10036a08. */

int SeenCommands[32];
DWORD AudioUCode;
AudioContext AudioState;
unsigned int AudioTaskCount;
OSTask AudioTask;
extern int QueueEnabled;

void SelectAudioABI(void);

void RecordHLEBuffer(const void *samples, int bytes)
{
    QueueEnabled = 0;
    if (HLEWaveFile) {
        fwrite(samples, 1, bytes, HLEWaveFile);
        WriteWaveHeader(HLEWaveFile);
    }
}

static __inline unsigned int ResolveAudioAddress(unsigned int address, AudioContext *context)
{
    return (address & 0xffffff) + context->segment;
}

void RunAudioTask(OSTask *task)
{
    unsigned int count;
    AudioState.segment = 0;
    count = task->data_size >> 3;
    CurrentCommand = (AudioCommand *)(RDRAM + ResolveAudioAddress(task->data_ptr, &AudioState));
    AudioTaskCount++;
    for (; count; count--) {
        int command = CurrentCommand->bytes[3];
        if (command < 32) {
            AudioCommandTable[command](CurrentCommand, &AudioState);
            if (DebugMode) SeenCommands[command] = 1;
        }
        CurrentCommand++;
    }
    if (DebugMode) SeenCommands[31] = 1;
}

unsigned int AudioChecksum(unsigned int *words, unsigned int count)
{
    unsigned int sum = 0, i;
    for (i = 0; i < count; i++) sum += words[i];
    return sum;
}

void ProcessAudioTask(OSTask *task)
{
    if (!AudioTaskSeen) AudioTaskSeen = 1;
    if ((task->data_size & ~7u) > 80000) return;
    AudioState.segment = 0;
    AudioTaskRunning = 0;
    if (DirectHLEAudio) RunAudioTask(task);
    AudioTaskRunning = 1;
}

unsigned long ByteSwapAudioWord(unsigned long value)
{
    return _byteswap_ulong(value);
}

void AUDList(unsigned int address)
{
    FILE *file;
    int i;
    unsigned int word;
    if (HLEAudio) {
        memcpy(&AudioTask, RDRAM + address, sizeof(AudioTask));
        ProcessAudioTask(&AudioTask);
        if (DumpMicrocode) {
            file = fopen("abi_ucode_dmem.dmp", "wb");
            for (i = 0; i < 0x1000; i += 4) {
                word = _byteswap_ulong(*(unsigned int *)(RDRAM + AudioTask.ucode_data + i));
                fwrite(&word, 4, 1, file);
            }
            fclose(file);
            if (AudioTask.ucode != 0xffffffff && AudioTask.ucode > 0) {
                file = fopen("abi_ucode_imem.dmp", "wb");
                for (i = 0; i < 0x80; i += 4) {
                    word = _byteswap_ulong(*(unsigned int *)(RDRAM + AudioTask.ucode_boot + i));
                    fwrite(&word, 4, 1, file);
                }
                for (i = 0; i < 0xf80; i += 4) {
                    word = _byteswap_ulong(*(unsigned int *)(RDRAM + AudioTask.ucode + i));
                    fwrite(&word, 4, 1, file);
                }
            }
            fclose(file);
        }
    }
}

void FreeAudioHLE(void)
{
    if (HLEAudio) {
        HLEAudio = 0;
        QueueEnabled = 0;
        if (AudioBuffersAllocated) {
            free(AudioState.buffer);
            free(AudioState.adpcmTable);
            AudioBuffersAllocated = 0;
        }
    }
}

void ChangeAudioABI(int abi)
{
    FreeAudioHLE();
    AudioUCode = abi;
    SelectAudioABI();
    CurrentCommand--;
}

void ResetAudioABI(void)
{
    ChangeAudioABI(1);
}

void SelectAudioABI(void)
{
    int i;
    if (HLEAudio) return;
    if (!AudioBuffersAllocated) {
        AudioState.buffer = (short *)malloc(0x3100);
        AudioState.adpcmTable = (short *)malloc(0x80);
        AudioBuffersAllocated = 1;
    }
    memset(AudioState.buffer, 0, 0x3100);
    InitializeAudioSqrt();
    memset(SeenCommands, 0, sizeof(SeenCommands));
    for (i = 0; i < 32; i++) AudioCommandTable[i] = (AudioCommandHandler)ResetAudioABI;
    AudioTaskCount = 0;
    ResetAudioProfile();
    if (AudioUCode == 2) {
        AudioCommandTable[0] = ABI3_NoOp;
        AudioCommandTable[1] = ABI3_ADPCM;
        AudioCommandTable[2] = ABI3_ClearBuffer;
        AudioCommandTable[3] = ABI3_EnvMixer;
        AudioCommandTable[4] = ABI3_LoadBuffer;
        AudioCommandTable[5] = ABI3_Resample;
        AudioCommandTable[6] = ABI3_SaveBuffer;
        AudioCommandTable[7] = ABI3_MP3;
        AudioCommandTable[8] = ABI3_MP3Address;
        AudioCommandTable[9] = ABI3_SetVolume;
        AudioCommandTable[10] = ABI3_DMEMMove;
        AudioCommandTable[11] = ABI3_LoadADPCM;
        AudioCommandTable[12] = ABI3_Mixer;
        AudioCommandTable[13] = ABI3_Interleave;
        AudioCommandTable[14] = ABI3_PoleFilter;
        AudioCommandTable[15] = ABI3_SetLoop;
        AudioCommandTable[16] = ABI3_Unknown16;
        AudioCommandTable[17] = ABI3_Unknown17;
    } else if (AudioUCode == 1) {
        AudioCommandTable[0] = ABI2_NoOp;
        AudioCommandTable[1] = ABI2_ADPCM;
        AudioCommandTable[2] = ABI2_ClearBuffer;
        AudioCommandTable[3] = ABI2_NoOp;
        AudioCommandTable[4] = ABI2_AddMixer;
        AudioCommandTable[5] = ABI2_Resample;
        AudioCommandTable[6] = ABI2_NoOp;
        AudioCommandTable[7] = ABI2_Filter;
        AudioCommandTable[8] = ABI2_SetBuffer;
        AudioCommandTable[9] = ABI2_Duplicate;
        AudioCommandTable[10] = ABI2_DMEMMove;
        AudioCommandTable[11] = ABI2_LoadADPCM;
        AudioCommandTable[12] = ABI2_Mixer;
        AudioCommandTable[13] = ABI2_Interleave;
        AudioCommandTable[14] = ABI2_Scale;
        AudioCommandTable[15] = ABI2_SetLoop;
        AudioCommandTable[16] = ABI2_NoOp;
        AudioCommandTable[17] = ABI2_ExtractChannel;
        AudioCommandTable[18] = ABI2_EnvSetup1;
        AudioCommandTable[19] = ABI2_EnvMixerAlt;
        AudioCommandTable[20] = ABI2_LoadBuffer;
        AudioCommandTable[21] = ABI2_SaveBuffer;
        AudioCommandTable[22] = ABI2_EnvSetup2;
        AudioCommandTable[23] = ABI2_NoOp;
        AudioCommandTable[24] = ABI2_Scale;
        AudioCommandTable[25] = ABI2_NoOp;
        AudioCommandTable[26] = ABI2_DuplicateAlt;
        AudioCommandTable[27] = ABI2_NoOp;
        AudioCommandTable[28] = ABI2_NoOp;
        AudioCommandTable[29] = ABI2_NoOp;
        AudioCommandTable[30] = ABI2_NoOp;
        AudioCommandTable[31] = ABI2_NoOp;
    } else if (AudioUCode == 0) {
        AudioCommandTable[0] = ABI1_NoOp;
        AudioCommandTable[1] = ABI1_ADPCM;
        AudioCommandTable[2] = ABI1_ClearBuffer;
        AudioCommandTable[3] = ABI1_EnvMixer;
        AudioCommandTable[4] = ABI1_LoadBuffer;
        AudioCommandTable[5] = ABI1_Resample;
        AudioCommandTable[6] = ABI1_SaveBuffer;
        AudioCommandTable[7] = ABI1_NoOp;
        AudioCommandTable[8] = ABI1_SetBuffer;
        AudioCommandTable[9] = ABI1_SetVolume;
        AudioCommandTable[10] = ABI1_DMEMMove;
        AudioCommandTable[11] = ABI1_LoadADPCM;
        AudioCommandTable[12] = ABI1_Mixer;
        AudioCommandTable[13] = ABI1_Interleave;
        AudioCommandTable[14] = ABI1_PoleFilter;
        AudioCommandTable[15] = ABI1_SetLoop;
    }
    HLEAudio = 1;
    AudioTaskRunning = 1;
}
