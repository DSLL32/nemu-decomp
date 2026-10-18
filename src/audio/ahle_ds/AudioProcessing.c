/* Buffer filtering and profiling. This inferred unit spans 10590..10AA0.
 * Its sole readonly constant contribution lies between the envelope literals
 * and the following decoder unit; see analysis/audio-source-grouping.json. */
#include "transform.h"
#include "AudioADPCMBlock.h"
#include "AudioProcessing.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
extern FILE *HLELogFile;
void ScaleAudioBuffer(int offset, int bytes, int gain, AudioContext *context)
{
    int i;
    offset >>= 1;
    bytes >>= 1;
    for (i = 0; i < bytes; ++i) {
        int sample = (context->buffer[offset + i] * gain) >> 8;
        if (sample < -32767) sample = -32767;
        else if (sample > 32767) sample = 32767;
        context->buffer[offset + i] = sample;
    }
}

void ScaleAudioBufferCommand(AudioCommand *command, AudioContext *context)
{
    unsigned short gain = (command->words[0] >> 4) & 0xf000;
    int offset = command->halfwords[3] >> 1;
    int samples = ((int)command->words[0] >> 1) & 0x7fff;
    int i;
    for (i = 0; i < samples; ++i) {
        int sample = (context->buffer[offset + i] * gain) >> 16;
        if (sample < -32767) sample = -32767;
        else if (sample > 32767) sample = 32767;
        context->buffer[offset + i] = sample;
    }
}

void FilterAudioBlock(int offset, short *coefficients, AudioContext *context)
{
    int i, tap;
    for (i = 0; i < 8; ++i)
        context->buffer[0x808 + i] = context->buffer[offset + i];
    for (i = 0; i < 8; ++i) {
        int sample = 0;
        for (tap = 0; tap < 8; ++tap)
            sample += context->buffer[0x800 + i + tap] * coefficients[tap];
        context->buffer[offset + i] = sample >> 16;
    }
    for (i = 0; i < 8; ++i)
        context->buffer[0x800 + i] = context->buffer[0x808 + i];
}

void FilterAudioBuffer(int offset, int bytes, unsigned int coefficientsAddress,
                      unsigned int stateAddress, int flags, AudioContext *context)
{
    short coefficients[8];
    offset >>= 1;
    bytes >>= 4;
    ReadAudioSamples(coefficients, coefficientsAddress, 16);
    if (coefficients[3] == 32767 && coefficients[2] == 0 && coefficients[4] == 0 &&
        coefficients[1] == 0 && coefficients[5] == 0)
        return;
    flags &= ~2;
    LoadAudioHistory(0x800, stateAddress, flags & 1, context);
    offset += 8;
    while (bytes) {
        FilterAudioBlock(offset, coefficients, context);
        offset += 8;
        --bytes;
    }
    SaveAudioHistory(0x800, stateAddress, flags & 1, context);
}

/* Internal HLE timing counters. Original source filename is unknown. */
#include <string.h>

typedef unsigned __int64 AudioClock;

typedef struct AudioProfileCounter {
    AudioClock start;
    AudioClock end;
    AudioClock total;
    AudioClock count;
    AudioClock average;
    AudioClock maximum;
    int enabled;
} AudioProfileCounter;

AudioProfileCounter AudioProfileCounters[4096];

#define READ_AUDIO_CLOCK(destination) \
    { AudioClock *clock = (destination); \
      __asm mov edi, clock \
      __asm pushad \
      __asm rdtsc \
      __asm mov [edi+4], edx \
      __asm mov [edi], eax \
      __asm popad }

void ResetAudioProfile(void)
{
    memset(AudioProfileCounters, 0, sizeof(AudioProfileCounters));
}

void AverageAudioProfile(unsigned int index)
{
    AudioProfileCounters[index].average = AudioProfileCounters[index].total /
        AudioProfileCounters[index].count;
}

void AdjustAudioProfileMaximum(unsigned int index)
{
    AudioProfileCounters[index].maximum += AudioProfileCounters[index].maximum /
        AudioProfileCounters[index].count;
}

void StartAudioProfile(unsigned int index)
{
    if (AudioProfileCounters[index].count <= 16) {
        AudioProfileCounters[index].enabled = 1;
        READ_AUDIO_CLOCK(&AudioProfileCounters[index].start);
    }
}

void StopAudioProfile(unsigned int index)
{
    if (AudioProfileCounters[index].count <= 16 &&
        AudioProfileCounters[index].start != 0) {
        READ_AUDIO_CLOCK(&AudioProfileCounters[index].end);
        if (AudioProfileCounters[index].end >= AudioProfileCounters[index].start) {
            AudioProfileCounters[index].total += AudioProfileCounters[index].end -
                AudioProfileCounters[index].start;
            ++AudioProfileCounters[index].count;
            if (AudioProfileCounters[index].maximum < AudioProfileCounters[index].end -
                AudioProfileCounters[index].start)
                AudioProfileCounters[index].maximum = AudioProfileCounters[index].end -
                    AudioProfileCounters[index].start;
        }
    }
}
