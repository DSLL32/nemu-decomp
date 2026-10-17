#include "AudioHLE.h"
#include <string.h>

extern void AudioError(const char *text, HRESULT result);

void LoadAudioBuffer(unsigned int destination, unsigned int address,
                     int byteCount, AudioContext *context)
{
    int i, sample;
    byteCount = (byteCount + 3) & ~3;
    if ((destination & 1) || (address & 3) || (byteCount & 1)) {
        AudioError("\n**alignment error in load\n", 0);
        return;
    }
    if (destination + byteCount > 0x1100)
        AudioError("\n**buffer overrun in abi loadmem() function\n", 0);
    destination >>= 1;
    for (i = 0; i < byteCount; i += 4) {
        sample = *(int *)(RDRAM + (address & 0xffffff));
        address += 4;
        context->buffer[destination++] = (short)(sample >> 16);
        context->buffer[destination++] = (short)sample;
    }
}

void SaveAudioBuffer(unsigned int source, unsigned int address,
                     int byteCount, AudioContext *context)
{
    int i, sample;
    byteCount = (byteCount + 3) & ~3;
    if ((source & 1) || (address & 3) || (byteCount & 1)) {
        AudioError("\n**alignment error in store\n", 0);
        return;
    }
    source >>= 1;
    for (i = 0; i < byteCount; i += 4) {
        sample = context->buffer[source++] << 16;
        sample |= context->buffer[source++] & 65535;
        *(int *)(RDRAM + ((i + address) & 0xffffff)) = sample;
    }
}

void MoveAudioBuffer(int destination, int source, int byteCount, AudioContext *context)
{
    if (source >= 0x1000 || destination >= 0x1000 ||
        source + byteCount > 0x1000 || destination + byteCount > 0x1000)
        return;
    memcpy(context->buffer + (destination >> 1), context->buffer + (source >> 1), byteCount);
}

void ClearAudioBuffer(int destination, int byteCount, AudioContext *context)
{
    if (destination >= 0x1000 || destination + byteCount > 0x1000)
        return;
    memset(context->buffer + (destination >> 1), 0, byteCount);
}

void DuplicateAudioBuffer(int destination, int source, int byteCount, AudioContext *context)
{
    int i, j;
    destination >>= 1;
    source >>= 1;
    byteCount >>= 1;
    for (i = 0; i < byteCount; i += 64) {
        for (j = 0; j < 64; j++)
            context->buffer[destination + j] = context->buffer[source + j];
        destination += 64;
    }
}

void ExtractAudioChannel(int destination, int source, int byteCount, AudioContext *context)
{
    int i;
    destination >>= 1;
    source >>= 1;
    byteCount >>= 1;
    for (i = 0; i < byteCount; i++) {
        context->buffer[destination] = context->buffer[source];
        source += 2;
        destination++;
    }
}
