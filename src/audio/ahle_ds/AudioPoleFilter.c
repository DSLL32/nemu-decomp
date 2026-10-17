/* Eight-sample pole filter. Original source filename is unknown. */
#include "AudioProcessing.h"

void PoleFilterAudioCommand(AudioCommand *command, AudioContext *context)
{
    int output = context->outputResample >> 1;
    short *state = (short *)((command->words[1] & 0xffffff) + RDRAM);
    int input = context->inputResample >> 1;
    unsigned int instruction = command->words[0];
    int count = context->count >> 1;
    int gain = instruction & 0xffff;
    short history[4];
    short *coefficients;
    int scaled[8];
    int i, tap, row;
    __int64 accumulator[8];
    if (count == 0) return;
    if (!(instruction & 0x10000)) {
        for (i = 0; i < 4; ++i) history[i] = state[i];
    } else {
        for (i = 0; i < 4; ++i) history[i] = 0;
    }
    coefficients = &context->buffer[0x400];
    for (i = 0; i < 8; ++i)
        scaled[i] = (coefficients[i + 8] * gain) / 0x4000;
    for (i = 0; i < count; i += 8) {
        for (tap = 0; tap < 8; ++tap)
            accumulator[tap] = coefficients[tap] * history[2];
        for (tap = 0; tap < 8; ++tap)
            accumulator[tap] += coefficients[tap + 8] * history[3];
        for (tap = 0; tap < 8; ++tap)
            accumulator[tap] += context->buffer[input + tap] * gain;
        for (row = 0; row < 8; ++row)
            for (tap = row + 1; tap < 8; ++tap)
                accumulator[tap] += context->buffer[input + row] * scaled[tap - row - 1];
        for (tap = 0; tap < 8; ++tap) {
            __int64 sample = accumulator[tap] / 0x4000;
            if (accumulator[tap] - sample * 0x4000 < 0) --sample;
            if (sample > 32767) sample = 32767;
            else if (sample < -32767) sample = -32767;
            context->buffer[output++] = (short)sample;
        }
        history[2] = context->buffer[output - 2];
        history[3] = context->buffer[output - 1];
        input += 8;
    }
    state = (short *)(RDRAM + (command->words[1] & 0xffffff));
    for (i = 0; i < 4; ++i) state[i] = history[i];
}
