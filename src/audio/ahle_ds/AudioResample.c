/* Scalar polyphase resampling; original filename is inferred.
 * Byte flags and explicit history loads follow the matching MMX version.
 * The complete scalar body is still unmatched; see audio-processing-provenance.json.
 * Original unit12 retains the AMP table before the writable coefficient family. */
#include "AudioProcessing.h"
#include "transform.h"
#include "AudioResampleTables.h"

void ResampleAudioScalar(AudioCommand *command, AudioContext *context)
{
    unsigned int w0 = command->words[0], w1 = command->words[1];
    int pitch = w0 & 0xffff;
    int input = context->inputResample >> 1;
    short extra = 0;
    int output = context->outputResample >> 1;
    unsigned char flags = (unsigned char)(w0 >> 16);
    short *state = (short *)((w1 & 0xffffff) + (unsigned int)RDRAM);
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
        context->buffer[input - extra - 4] = *state++;
        context->buffer[input - extra - 3] = *state++;
        context->buffer[input - extra - 2] = *state++;
        context->buffer[input - extra - 1] = *state++;
    } else {
        phase = 0;
        context->buffer[input - 4] = 0;
        context->buffer[input - 3] = 0;
        context->buffer[input - 2] = 0;
        context->buffer[input - 1] = 0;
    }
    samples = context->count >> 1;
    input -= extra + 4;
    for (i = 0; i < samples; ++i) {
        int sample = 0;
        short tableOffset;
        int tap;
        int advance = phase >> 15;
        input += advance;
        phase -= advance << 15;
        tableOffset = (phase << 6) >> 15; tableOffset <<= 2;
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



