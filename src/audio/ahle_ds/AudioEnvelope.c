/* Floating point envelope mixers. Original private names are inferred. */
#include <math.h>
#include "AudioEnvelope.h"

extern void AudioError(char *message, int code);

__inline int ClampAudioSample(int sample)
{
    if (sample > 32767) sample = 32767;
    else if (sample < -32767) sample = -32767;
    return sample;
}

void EnvMixerScalar(AudioCommand *command, AudioContext *context)
{
    int rateLeft, rateRight, targetLeft, targetRight, dry, wet;
    float volumeLeft, volumeRight, stepLeft, stepRight;
    int input, outputLeft, outputRight, wetLeft, wetRight, count, flags;
    short *buffer;
    unsigned short *state;
    int sample, i;

    if ((command->words[0] >> 16) & 0x10)
        AudioError("A_MIX Set in envmixer?! heavens no. If you are a user, you shouldnt see this. report to lac@nemu.com you got this error.", 0);
    if (!((command->words[0] >> 16) & 1)) {
        state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
        rateLeft = (state[6] << 16) + state[5];
        rateRight = (state[9] << 16) + state[8];
        volumeLeft = ((state[0] << 16) + state[1]) / 65536.0f;
        volumeRight = ((state[2] << 16) + state[3]) / 65536.0f;
        targetLeft = state[4];
        targetRight = state[7];
        dry = state[10];
        wet = state[11];
    } else {
        rateLeft = context->rateLeft16;
        rateRight = context->rateRight16;
        targetLeft = context->targetLeft16;
        targetRight = context->targetRight16;
        volumeLeft = context->volumeLeft16;
        volumeRight = context->volumeRight16;
        wet = context->wet;
        dry = context->dry;
    }
    stepLeft = (float)(rateLeft / 65536.0);
    stepLeft = (float)sqrt(sqrt(sqrt(stepLeft)));
    stepRight = (float)(rateRight / 65536.0);
    stepRight = (float)sqrt(sqrt(sqrt(stepRight)));
    flags = (command->words[0] >> 16) & 0xff;
    input = context->input >> 1;
    outputLeft = context->outputResample >> 1;
    outputRight = context->auxResample0 >> 1;
    wetLeft = context->auxResample1 >> 1;
    wetRight = context->auxResample2 >> 1;
    buffer = context->buffer;
    count = context->count >> 1;
    if (!outputLeft && !outputRight && !volumeLeft && !stepLeft && !volumeRight)
        return;
    for (i = count + 1; i > 1; --i) {
        sample = (int)(buffer[input] * volumeLeft / 32768.0f);
        buffer[outputLeft] = ClampAudioSample(buffer[outputLeft] + sample * dry / 32768);
        ++outputLeft;
        if ((flags & 8) && wetLeft) {
            buffer[wetLeft] = ClampAudioSample(buffer[wetLeft] + sample * wet / 32768);
            ++wetLeft;
        }
        sample = (int)(buffer[input++] * volumeRight / 32768.0f);
        buffer[outputRight] = ClampAudioSample(buffer[outputRight] + sample * dry / 32768);
        ++outputRight;
        if ((flags & 8) && wetRight) {
            buffer[wetRight] = ClampAudioSample(buffer[wetRight] + sample * wet / 32768);
            ++wetRight;
        }
        volumeLeft *= stepLeft;
        if (stepLeft > 1.0) {
            if (volumeLeft > targetLeft) volumeLeft = (float)targetLeft;
        } else {
            if (volumeLeft < targetLeft) volumeLeft = (float)targetLeft;
        }
        volumeRight *= stepRight;
        if (stepRight > 1.0) {
            if (volumeRight > targetRight) volumeRight = (float)targetRight;
        } else {
            if (volumeRight < targetRight) volumeRight = (float)targetRight;
        }
    }
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = (unsigned short)volumeLeft;
    state[1] = (unsigned short)((volumeLeft - state[0]) * 65536.0f);
    state[2] = (unsigned short)volumeRight;
    state[3] = (unsigned short)((volumeRight - state[2]) * 65536.0f);
    state[6] = rateLeft >> 16;
    state[5] = rateLeft;
    state[9] = rateRight >> 16;
    state[8] = rateRight;
    state[4] = targetLeft;
    state[7] = targetRight;
    state[10] = dry;
    state[11] = wet;
}

void EnvMixerBlockScalar(AudioCommand *command, AudioContext *context)
{
    int rateLeft, rateRight, targetLeft, targetRight, dry, wet;
    float volumeLeft, volumeRight, stepLeft, stepRight;
    int input, outputLeft, outputRight, wetLeft, wetRight, count, flags;
    short *buffer;
    unsigned short *state;
    int sample;

    if ((command->words[0] >> 16) & 0x10)
        AudioError("A_MIX Set in envmixer?! heavens no. If you are a user, you shouldnt see this. report to lac@nemu.com you got this error.", 0);
    if (!((command->words[0] >> 16) & 1)) {
        state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
        rateLeft = (state[6] << 16) + state[5];
        rateRight = (state[9] << 16) + state[8];
        volumeLeft = ((state[0] << 16) + state[1]) / 65536.0f;
        volumeRight = ((state[2] << 16) + state[3]) / 65536.0f;
        targetLeft = state[4];
        targetRight = state[7];
        dry = state[10];
        wet = state[11];
    } else {
        rateLeft = context->rateLeft16;
        rateRight = context->rateRight16;
        targetLeft = context->targetLeft16;
        targetRight = context->targetRight16;
        volumeLeft = context->volumeLeft16;
        volumeRight = context->volumeRight16;
        wet = context->wet;
        dry = context->dry;
    }
    stepLeft = (float)(rateLeft / 65536.0);
    stepRight = (float)(rateRight / 65536.0);
    flags = (command->words[0] >> 16) & 0xff;
    input = context->input >> 1;
    outputLeft = context->outputResample >> 1;
    outputRight = context->auxResample0 >> 1;
    wetLeft = context->auxResample1 >> 1;
    wetRight = context->auxResample2 >> 1;
    buffer = context->buffer;
    count = context->count >> 1;
    if (!outputLeft && !outputRight && !volumeLeft && !stepLeft && !volumeRight)
        return;
    while (count > 0) {
        sample = (int)(buffer[input] * volumeLeft / 32768.0f);
        buffer[outputLeft] = ClampAudioSample(buffer[outputLeft] + sample);
        ++outputLeft;
        if ((flags & 8) && wetLeft) {
            buffer[wetLeft] = ClampAudioSample(buffer[wetLeft] + sample * wet / 32768);
            ++wetLeft;
        }
        sample = (int)(buffer[input++] * volumeRight / 32768.0f);
        buffer[outputRight] = ClampAudioSample(buffer[outputRight] + sample);
        ++outputRight;
        if ((flags & 8) && wetRight) {
            buffer[wetRight] = ClampAudioSample(buffer[wetRight] + sample * wet / 32768);
            ++wetRight;
        }
        if (!(count % 8) && (stepLeft != 0 || stepRight != 0)) {
            volumeLeft *= stepLeft;
            if (stepLeft > 1.0) {
                if (volumeLeft > targetLeft) volumeLeft = (float)targetLeft;
            } else {
                if (volumeLeft < targetLeft) volumeLeft = (float)targetLeft;
            }
            volumeRight *= stepRight;
            if (stepRight > 1.0) {
                if (volumeRight > targetRight) volumeRight = (float)targetRight;
            } else {
                if (volumeRight < targetRight) volumeRight = (float)targetRight;
            }
        }
        --count;
    }
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = (unsigned short)volumeLeft;
    state[1] = (unsigned short)((volumeLeft - state[0]) * 65536.0f);
    state[2] = (unsigned short)volumeRight;
    state[3] = (unsigned short)((volumeRight - state[2]) * 65536.0f);
    state[6] = rateLeft >> 16;
    state[5] = rateLeft;
    state[9] = rateRight >> 16;
    state[8] = rateRight;
    state[4] = targetLeft;
    state[7] = targetRight;
    state[10] = dry;
    state[11] = wet;
}
