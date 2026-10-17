/* Linear volume-envelope mixers. Names and file boundaries are inferred. */
#include "AudioEnvelope.h"

extern void AudioError(const char *text, HRESULT result);

static __inline int ClampEnvelope(int sample)
{
    if (sample > 32767) sample = 32767;
    else if (sample < -32767) sample = -32767;
    return sample;
}

void EnvMixerABI3Scalar(AudioCommand *command, AudioContext *context)
{
    int rateLeft, rateRight;
    int targetLeft, targetRight;
    float volumeLeft, volumeRight;
    int dry, wet;
    float stepLeft, stepRight;
    unsigned short *state;
    int input, outputLeft, wetLeft, outputRight, wetRight;
    int i;
    short *buffer;
    int dryMask, wetMask;
    if (command->bytes[2] & 0x10)
        AudioError("A_MIX Set in envmixer?! heavens no. If you are a user, you shouldnt see this. report to lac@nemu.com you got this error.", 0);
    if (!(command->bytes[2] & 1)) {
        state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
        rateLeft = (state[6] << 16) + state[5];
        rateRight = (state[9] << 16) + state[8];
        volumeLeft = (float)((state[0] << 16) + state[1]) / 65536.f;
        volumeRight = (float)((state[2] << 16) + state[3]) / 65536.f;
        targetLeft = state[4];
        targetRight = state[7];
        dry = state[10];
        wet = state[11];
    } else {
        rateLeft = context->rateLeft16;
        rateRight = context->rateRight16;
        targetLeft = context->targetLeft16;
        targetRight = context->targetRight16;
        volumeLeft = (float)context->volumeLeft16;
        volumeRight = (float)context->volumeRight16;
        wet = context->wet;
        dry = context->dry;
    }
    stepLeft = (float)(rateLeft / 65536.0);
    stepLeft /= 8.f;
    stepRight = (float)(rateRight / 65536.0);
    stepRight /= 8.f;
    buffer = context->buffer;
    input = 0;
    outputLeft = 0x4e0 / 2;
    outputRight = 0x650 / 2;
    wetLeft = 0x7c0 / 2;
    wetRight = 0x930 / 2;
    dryMask = -(dry & 1);
    wetMask = -(wet & 1);
    /* Preserve the two input-polarity stages used by this ABI. */
    wetMask ^= dryMask;
    for (i = 0; i < 184; ++i) {
        int sampleLeft, sampleRight;
        short raw = buffer[input++];
        raw ^= dryMask;
        raw ^= wetMask;
        sampleLeft = (int)(raw * volumeLeft / 32768.f);
        buffer[outputLeft] = ClampEnvelope(buffer[outputLeft] + sampleLeft * dry / 32768);
        ++outputLeft;
        buffer[wetLeft] = ClampEnvelope(buffer[wetLeft] + sampleLeft * wet / 32768);
        ++wetLeft;
        sampleRight = (int)(raw * volumeRight / 32768.f);
        buffer[outputRight] = ClampEnvelope(buffer[outputRight] + sampleRight * dry / 32768);
        ++outputRight;
        buffer[wetRight] = ClampEnvelope(buffer[wetRight] + sampleRight * wet / 32768);
        ++wetRight;
        volumeLeft += stepLeft;
        volumeRight += stepRight;
        if (stepLeft > 0.f) {
            if (volumeLeft > targetLeft) volumeLeft = (float)targetLeft;
        } else {
            if (volumeLeft < targetLeft) volumeLeft = (float)targetLeft;
        }
        if (stepRight > 0.f) {
            if (volumeRight > targetRight) volumeRight = (float)targetRight;
        } else {
            if (volumeRight < targetRight) volumeRight = (float)targetRight;
        }
    }
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = (unsigned short)volumeLeft;
    state[1] = (unsigned short)((volumeLeft - state[0]) * 65536.f);
    state[2] = (unsigned short)volumeRight;
    state[3] = (unsigned short)((volumeRight - state[2]) * 65536.f);
    state[6] = (unsigned short)(rateLeft >> 16);
    state[5] = (unsigned short)rateLeft;
    state[9] = (unsigned short)(rateRight >> 16);
    state[8] = (unsigned short)rateRight;
    state[4] = (unsigned short)targetLeft;
    state[7] = (unsigned short)targetRight;
    state[10] = (unsigned short)dry;
    state[11] = (unsigned short)wet;
}

void EnvMixerSpecial(AudioCommand *command, AudioContext *context)
{
    int rateLeft, rateRight;
    int targetLeft, targetRight;
    float volumeLeft, volumeRight;
    int dry, wet;
    float stepLeft, stepRight;
    unsigned short *state;
    int input, outputLeft, wetLeft, outputRight, wetRight;
    int count, i;
    short *buffer;
    if (command->bytes[2] & 0x10)
        AudioError("A_MIX Set in envmixer?! heavens no. If you are a user, you shouldnt see this. report to lac@nemu.com you got this error.", 0);
    if (!(command->bytes[2] & 1)) {
        state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
        rateLeft = (state[6] << 16) + state[5];
        rateRight = (state[9] << 16) + state[8];
        volumeLeft = (float)((state[0] << 16) + state[1]) / 65536.f;
        volumeRight = (float)((state[2] << 16) + state[3]) / 65536.f;
        targetLeft = state[4];
        targetRight = state[7];
        dry = state[10];
        wet = state[11];
    } else {
        rateLeft = context->rateLeft16;
        rateRight = context->rateRight16;
        targetLeft = context->targetLeft16;
        targetRight = context->targetRight16;
        volumeLeft = (float)context->volumeLeft16;
        volumeRight = (float)context->volumeRight16;
        wet = context->wet;
        dry = context->dry;
    }
    stepLeft = (float)(rateLeft / 65536.0);
    stepLeft /= 8.f;
    stepRight = (float)(rateRight / 65536.0);
    stepRight /= 8.f;
    input = context->input >> 1;
    outputLeft = context->outputResample >> 1;
    wetLeft = context->auxResample1 >> 1;
    outputRight = context->auxResample0 >> 1;
    wetRight = context->auxResample2 >> 1;
    buffer = context->buffer;
    count = context->count >> 1;
    for (i = 0; i < count; ++i) {
        int sampleLeft, sampleRight;
        sampleLeft = (int)(buffer[input] * volumeLeft / 32768.f);
        buffer[outputLeft] = ClampEnvelope(buffer[outputLeft] + sampleLeft * dry / 32768);
        ++outputLeft;
        buffer[wetLeft] = ClampEnvelope(buffer[wetLeft] + sampleLeft * wet / 32768);
        ++wetLeft;
        sampleRight = (int)(buffer[input++] * volumeRight / 32768.f);
        buffer[outputRight] = ClampEnvelope(buffer[outputRight] + sampleRight * dry / 32768);
        ++outputRight;
        buffer[wetRight] = ClampEnvelope(buffer[wetRight] + sampleRight * wet / 32768);
        ++wetRight;
        volumeLeft += stepLeft;
        if (stepLeft > 0.f) {
            if (volumeLeft > targetLeft) volumeLeft = (float)targetLeft;
        } else {
            if (volumeLeft < targetLeft) volumeLeft = (float)targetLeft;
        }
        volumeRight += stepRight;
        if (stepRight > 0.f) {
            if (volumeRight > targetRight) volumeRight = (float)targetRight;
        } else {
            if (volumeRight < targetRight) volumeRight = (float)targetRight;
        }
    }
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = (unsigned short)volumeLeft;
    state[1] = (unsigned short)((volumeLeft - state[0]) * 65536.f);
    state[2] = (unsigned short)volumeRight;
    state[3] = (unsigned short)((volumeRight - state[2]) * 65536.f);
    state[6] = (unsigned short)(rateLeft >> 16);
    state[5] = (unsigned short)rateLeft;
    state[9] = (unsigned short)(rateRight >> 16);
    state[8] = (unsigned short)rateRight;
    state[4] = (unsigned short)targetLeft;
    state[7] = (unsigned short)targetRight;
    state[10] = (unsigned short)dry;
    state[11] = (unsigned short)wet;
}

