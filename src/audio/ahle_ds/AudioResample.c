/* Scalar polyphase resampling; original filename is unknown. */
#include "AudioProcessing.h"

extern short AudioResampleTable[64][4];

void ResampleAudioScalar(AudioCommand *command, AudioContext *context)
{
    int input = context->inputResample >> 1;
    int output = context->outputResample >> 1;
    int pitch = command->words[0] & 0xffff;
    int flags = command->words[0] >> 16;
    short *state = (short *)(RDRAM + (command->words[1] & 0xffffff));
    short extra = 0;
    int phase;
    int i;
    int samples;
    if (!(flags & 1)) {
        phase = state[4];
        if (flags & 2) {
            extra = state[5];
            for (i = 0; i < extra; ++i)
                context->buffer[input - extra + i] = state[16 - extra + i];
        }
        for (i = 0; i < 4; ++i)
            context->buffer[input - extra - 4 + i] = *state++;
    } else {
        phase = 0;
        for (i = 0; i < 4; ++i)
            context->buffer[input - 4 + i] = 0;
    }
    input -= extra + 4;
    samples = context->count >> 1;
    for (i = 0; i < samples; ++i) {
        int sample = 0;
        short tableOffset;
        int tap;
        int advance = phase >> 15;
        input += advance;
        phase -= advance << 15;
        tableOffset = ((phase << 6) >> 15) << 2;
        for (tap = 0; tap < 4; ++tap)
            sample += context->buffer[input + tap] *
                ((const short *)AudioResampleTable)[tableOffset + tap];
        sample >>= 15;
        if (sample > 32767) sample = 32767;
        else if (sample < -32767) sample = -32767;
        context->buffer[output++] = sample;
        phase += pitch;
    }
    input += phase >> 15;
    phase -= (phase >> 15) << 15;
    extra = (input - (context->inputResample >> 1) - 4) & 7;
    if (extra) extra = 8 - extra;
    state = (short *)(RDRAM + (command->words[1] & 0xffffff));
    state[4] = phase;
    state[5] = extra;
    for (i = 0; i < extra; ++i)
        state[16 - extra + i] = context->buffer[input + 4 + i];
    for (i = 0; i < 4; ++i)
        state[i] = context->buffer[input + i];
}
