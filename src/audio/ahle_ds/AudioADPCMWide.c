/* Legacy ADPCM decoder with wide predictor accumulators.
 * The original private filename and identifiers are unknown. */
#include "AudioProcessing.h"
#include <string.h>

extern void LoadAudioBuffer(unsigned int, unsigned int, int, AudioContext *);
extern void SaveAudioBuffer(unsigned int, unsigned int, int, AudioContext *);
extern void ReadAudioMemory(void *, const void *, unsigned int);

void DecodeAudioADPCMWideCommand(AudioCommand *command, AudioContext *context)
{
    int residual[16];
    __int64 accumulator[8];
    __int64 sample;
    int output = context->outputResample >> 1;
    int count, i, j, coefficient, scale;
    unsigned char *input;
    short *book;
    unsigned int address = (unsigned int)(RDRAM + (command->words[1] & 0xffffff));
    int flags = command->bytes[2];
    if (!(flags & 1)) {
        if (flags & 2)
            LoadAudioBuffer(output * 2, (unsigned int)(RDRAM + context->loopAddress), 32, context);
        else
            LoadAudioBuffer(output * 2, address, 32, context);
    } else {
        int destination = output * 2;
        if (destination < 0x1000 && destination + 32 <= 0x1000)
            memset(context->buffer + (destination >> 1), 0, 32);
    }
    output += 16;
    ReadAudioMemory(context->buffer, context->buffer, 32);
    count = context->count >> 1;
    input = (unsigned char *)context->buffer + context->inputResample;
    while (count > 0) {
        unsigned char header = *input++;
        scale = 1 << (header >> 4);
        coefficient = header & 15;
        for (i = 0; i < 16; i += 2) {
            int first = *input >> 4;
            int second = *input++ & 15;
            if (first <= 7) residual[i] = first * scale;
            else residual[i] = -(16 - first) * scale;
            if (second <= 7) residual[i + 1] = second * scale;
            else residual[i + 1] = -(16 - second) * scale;
        }
        book = context->adpcmTable + coefficient * 16;
        for (i = 0; i < 8; ++i)
            accumulator[i] = book[i] * context->buffer[output - 2];
        for (i = 0; i < 8; ++i)
            accumulator[i] += book[i + 8] * context->buffer[output - 1];
        for (i = 0; i < 8; ++i)
            accumulator[i] += residual[i] << 11;
        for (i = 0; i < 8; ++i)
            for (j = i + 1; j < 8; ++j)
                accumulator[j] += book[j - i + 7] * residual[i];
        for (i = 0; i < 8; ++i) {
            sample = accumulator[i] / 2048;
            if (accumulator[i] - sample * 2048 < 0) --sample;
            if (sample > 32767) sample = 32767;
            else if (sample < -32767) sample = -32767;
            context->buffer[output++] = (short)sample;
        }
        for (i = 0; i < 8; ++i)
            accumulator[i] = book[i] * context->buffer[output - 2];
        for (i = 0; i < 8; ++i)
            accumulator[i] += book[i + 8] * context->buffer[output - 1];
        for (i = 0; i < 8; ++i)
            accumulator[i] += residual[i + 8] << 11;
        for (i = 0; i < 8; ++i)
            for (j = i + 1; j < 8; ++j)
                accumulator[j] += book[j - i + 7] * residual[i + 8];
        for (i = 0; i < 8; ++i) {
            sample = accumulator[i] / 2048;
            if (accumulator[i] - sample * 2048 < 0) --sample;
            if (sample > 32767) sample = 32767;
            else if (sample < -32767) sample = -32767;
            context->buffer[output++] = (short)sample;
        }
        count -= 16;
    }
    SaveAudioBuffer(output * 2 - 32,
                    (unsigned int)(RDRAM + (command->words[1] & 0xffffff)),
                    32, context);
}
