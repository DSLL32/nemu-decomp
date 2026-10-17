/* Audio sample helpers; source filename inferred from the native code group. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "AudioProcessing.h"

extern FILE *HLELogFile;
extern void LoadAudioBuffer(unsigned int offset, unsigned int address, int bytes,
                            AudioContext *context);
extern void SaveAudioBuffer(unsigned int offset, unsigned int address, int bytes,
                            AudioContext *context);

unsigned int AudioSqrtEvenMantissa[32768];
unsigned int AudioSqrtOddMantissa[32768];
int AudioEnvelopeWet;
int AudioEnvelopeDry;

void ReadAudioSamples(short *destination, unsigned int address, int bytes)
{
    int offset;
    for (offset = 0; offset < bytes; offset += 4) {
        int pair = *(int *)(RDRAM + ((address + offset) & 0xffffff));
        *destination++ = pair >> 16;
        *destination++ = pair;
    }
}

void InitializeAudioSqrt(void)
{
    unsigned int index;
    union {
        unsigned int bits;
        float value;
    } sample;
    for (index = 0; index <= 0x7fff; ++index) {
        sample.bits = (index | 0x3f8000) << 8;
        sample.value = (float)sqrt(sample.value);
        AudioSqrtEvenMantissa[index] = sample.bits & 0x7fffff;
        sample.bits = (index | 0x400000) << 8;
        sample.value = (float)sqrt(sample.value);
        AudioSqrtOddMantissa[index] = sample.bits & 0x7fffff;
    }
}

void LoadAudioEnvelope(unsigned int address, AudioContext *context)
{
    context->targetLeft  = *(int *)(RDRAM + ((address +  0) & 0xffffff));
    context->targetRight = *(int *)(RDRAM + ((address +  4) & 0xffffff));
    context->volumeLeft  = *(int *)(RDRAM + ((address +  8) & 0xffffff));
    context->volumeRight = *(int *)(RDRAM + ((address + 12) & 0xffffff));
    context->rampLeft    = *(int *)(RDRAM + ((address + 16) & 0xffffff));
    context->rampRight   = *(int *)(RDRAM + ((address + 20) & 0xffffff));
    context->rateLeft    = *(int *)(RDRAM + ((address + 24) & 0xffffff));
    context->rateRight   = *(int *)(RDRAM + ((address + 28) & 0xffffff));
    context->wet = AudioEnvelopeWet;
    context->dry = AudioEnvelopeDry;
}

void SaveAudioEnvelope(unsigned int address, AudioContext *context)
{
    *(int *)(RDRAM + ((address +  0) & 0xffffff)) = context->targetLeft;
    *(int *)(RDRAM + ((address +  4) & 0xffffff)) = context->targetRight;
    *(int *)(RDRAM + ((address +  8) & 0xffffff)) = context->volumeLeft;
    *(int *)(RDRAM + ((address + 12) & 0xffffff)) = context->volumeRight;
    *(int *)(RDRAM + ((address + 16) & 0xffffff)) = context->rampLeft;
    *(int *)(RDRAM + ((address + 20) & 0xffffff)) = context->rampRight;
    *(int *)(RDRAM + ((address + 24) & 0xffffff)) = context->rateLeft;
    *(int *)(RDRAM + ((address + 28) & 0xffffff)) = context->rateRight;
    AudioEnvelopeWet = context->wet;
    AudioEnvelopeDry = context->dry;
}

void ResetAudioEnvelope(AudioContext *context)
{
    context->volumeLeft = 0;
    context->volumeRight = 0;
    context->rampLeft = 0;
    context->rampRight = 0;
    context->rateLeft = 0;
    context->rateRight = 0;
    context->targetLeft = 0;
    context->targetRight = 0;
    context->wet = 0;
    context->dry = 0;
}

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

void LoadAudioHistory(int offset, unsigned int address, int flags,
                      AudioContext *context)
{
    if (flags & 1) {
        if (offset < 0x1000 && offset + 32 <= 0x1000)
            memset(&context->buffer[offset >> 1], 0, 32);
    } else {
        if (flags & 2)
            LoadAudioBuffer(offset, context->loopAddress, 32, context);
        else
            LoadAudioBuffer(offset, address, 32, context);
    }
}

void SaveAudioHistory(int offset, unsigned int address, int flags,
                      AudioContext *context)
{
    SaveAudioBuffer(offset, address, 32, context);
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

void DumpAudioCommand(AudioCommand *command)
{
    int i;
    if (HLELogFile) {
        fprintf(HLELogFile, "\n");
        for (i = 3; i >= 0; --i)
            fprintf(HLELogFile, " %02X", command->bytes[i]);
        for (i = 3; i >= 0; --i)
            fprintf(HLELogFile, " %02X", command->bytes[i + 4]);
        fprintf(HLELogFile, "\n\n");
    }
}
