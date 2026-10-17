/* Older audio ABI handlers retained in the original DLL. Private names and
 * this source filename are inferred; command layouts follow their consumers. */
#include "AudioLegacy.h"
#include <string.h>
#include <math.h>

extern void LoadAudioBuffer(unsigned int, unsigned int, int, AudioContext *);
extern void SaveAudioBuffer(unsigned int, unsigned int, int, AudioContext *);
extern void MoveAudioBuffer(int, int, int, AudioContext *);
extern void ClearAudioBuffer(int, int, AudioContext *);
extern void DecodeAudioADPCM(unsigned int, int, int, int, int, AudioContext *);
extern void ChangeAudioABI(int);
extern void RecordHLEBuffer(const void *, int);

void LegacyEnvelopeMixer(AudioCommand *command, AudioContext *context)
{
    int rateLeft, rateRight, targetLeft, targetRight, dry, wet;
    float rampLeft, rampRight, volumeLeft, volumeRight;
    int input, output, aux0, aux1, aux2, count, i;
    unsigned short *state;
    if (!(command->bytes[2] & 1)) {
        state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
        rateLeft = (state[6] << 16) + state[5];
        rateRight = (state[9] << 16) + state[8];
        rampLeft = sqrt(sqrt(sqrt(rateLeft / 65536.0)));
        rampRight = sqrt(sqrt(sqrt(rateRight / 65536.0)));
        volumeLeft = ((state[0] << 16) + state[1]) / 65536.0;
        volumeRight = ((state[2] << 16) + state[3]) / 65536.0;
        targetLeft = state[4];
        targetRight = state[7];
        dry = state[10];
        wet = state[11];
    } else {
        rateLeft = context->rateLeft16;
        rateRight = context->rateRight16;
        rampLeft = sqrt(sqrt(sqrt(rateLeft / 65536.0)));
        rampRight = sqrt(sqrt(sqrt(rateRight / 65536.0)));
        targetLeft = context->targetLeft16;
        targetRight = context->targetRight16;
        volumeLeft = context->volumeLeft16;
        volumeRight = context->volumeRight16;
        wet = context->wet;
        dry = context->dry;
    }
    input = context->input >> 1;
    output = context->outputResample >> 1;
    aux0 = context->auxResample0 >> 1;
    aux1 = context->auxResample1 >> 1;
    aux2 = context->auxResample2 >> 1;
    count = context->count >> 1;
    for (i = 0; i < count; ++i) {
        float sample = context->buffer[input] * volumeLeft / 32768.0;
        {
            float mixed = context->buffer[output] + sample * dry / 32768.0;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            context->buffer[output] = mixed;
        }
        ++output;
        {
            float mixed = context->buffer[aux1] + sample * wet / 32768.0;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            context->buffer[aux1] = mixed;
        }
        ++aux1;
        sample = context->buffer[input] * volumeRight / 32768.0;
        ++input;
        {
            float mixed = context->buffer[aux0] + sample * dry / 32768.0;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            context->buffer[aux0] = mixed;
        }
        ++aux0;
        {
            float mixed = context->buffer[aux2] + sample * wet / 32768.0;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            context->buffer[aux2] = mixed;
        }
        ++aux2;
        volumeLeft *= rampLeft;
        if (rampLeft > 1.0) {
            if (volumeLeft > targetLeft) volumeLeft = targetLeft;
        } else {
            if (volumeLeft < targetLeft) volumeLeft = targetLeft;
        }
        volumeRight *= rampRight;
        if (rampRight > 1.0) {
            if (volumeRight > targetRight) volumeRight = targetRight;
        } else {
            if (volumeRight < targetRight) volumeRight = targetRight;
        }
    }
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = (unsigned short)volumeLeft;
    state[1] = (volumeLeft - state[0]) * 65536.0f;
    state[2] = (unsigned short)volumeRight;
    state[3] = (volumeRight - state[2]) * 65536.0f;
    state[6] = rateLeft >> 16;
    state[5] = rateLeft;
    state[9] = rateRight >> 16;
    state[8] = rateRight;
    state[4] = targetLeft;
    state[7] = targetRight;
    state[10] = dry;
    state[11] = wet;
}

void LegacyNoOp(AudioCommand *command, AudioContext *context)
{
}

void LegacyADPCM(AudioCommand *command, AudioContext *context)
{
    DecodeAudioADPCM(command->words[1], context->input, context->output,
                     context->count, command->bytes[2], context);
}

void LegacyEnvelope(AudioCommand *command, AudioContext *context)
{
    LegacyEnvelopeMixer(command, context);
}

void LegacyLoadBuffer(AudioCommand *command, AudioContext *context)
{
    LoadAudioBuffer(context->input, context->segment + command->words[1],
                    context->count, context);
}

void LegacyResample(AudioCommand *command, AudioContext *context)
{
    int output = context->output;
    int count = context->count;
    unsigned int address = command->words[1];
    int pitch = command->words[0] & 0xffff;
    int input = context->input;
    int position, a, b, sample;
    int loadedstate[8];
    int savedstate[8];
    input -= 16;
    pitch <<= 1;
    count >>= 1;
    output >>= 1;
    input >>= 1;
    if (command->bytes[2] & 1) {
        position = 0;
        context->buffer[input + 0] = 0;
        context->buffer[input + 1] = 0;
        context->buffer[input + 2] = 0;
        context->buffer[input + 3] = 0;
    } else {
        loadedstate[0] = *(int *)(RDRAM + ((address + 0) & 0xffffff));
        loadedstate[1] = *(int *)(RDRAM + ((address + 4) & 0xffffff));
        loadedstate[2] = *(int *)(RDRAM + ((address + 8) & 0xffffff));
        loadedstate[3] = *(int *)(RDRAM + ((address + 12) & 0xffffff));
        loadedstate[4] = *(int *)(RDRAM + ((address + 16) & 0xffffff));
        context->buffer[input + 0] = loadedstate[0] >> 16;
        context->buffer[input + 1] = loadedstate[0];
        context->buffer[input + 2] = loadedstate[1] >> 16;
        context->buffer[input + 3] = loadedstate[1];
        context->buffer[input + 4] = loadedstate[2] >> 16;
        context->buffer[input + 5] = loadedstate[2];
        context->buffer[input + 6] = loadedstate[3] >> 16;
        context->buffer[input + 7] = loadedstate[3];
        position = loadedstate[4];
    }
    count += 16;
    position &= 0xffff;
    while (count--) {
        a = position >> 16;
        b = position & 0xffff;
        sample = context->buffer[input + a + 1] - context->buffer[input + a];
        sample = ((sample * b) >> 16) + context->buffer[input + a];
        context->buffer[output++] = sample;
        position += pitch;
    }
    position -= pitch * 16;
    input += position >> 16;
    a = context->buffer[input + 0] << 16;
    a |= context->buffer[input + 1] & 0xffff;
    savedstate[0] = a;
    a = context->buffer[input + 2] << 16;
    a |= context->buffer[input + 3] & 0xffff;
    savedstate[1] = a;
    a = context->buffer[input + 4] << 16;
    a |= context->buffer[input + 5] & 0xffff;
    savedstate[2] = a;
    a = context->buffer[input + 6] << 16;
    a |= context->buffer[input + 7] & 0xffff;
    savedstate[3] = a;
    savedstate[4] = position;
    *(int *)(RDRAM + ((address + 0) & 0xffffff)) = savedstate[0];
    *(int *)(RDRAM + ((address + 4) & 0xffffff)) = savedstate[1];
    *(int *)(RDRAM + ((address + 8) & 0xffffff)) = savedstate[2];
    *(int *)(RDRAM + ((address + 12) & 0xffffff)) = savedstate[3];
    *(int *)(RDRAM + ((address + 16) & 0xffffff)) = savedstate[4];
}

void LegacySaveBuffer(AudioCommand *command, AudioContext *context)
{
    SaveAudioBuffer(context->output, context->segment + command->words[1],
                    context->count, context);
}

void LegacySegment(AudioCommand *command, AudioContext *context)
{
}

void LegacySetBuffer(AudioCommand *command, AudioContext *context)
{
    int flags = command->bytes[2];
    if (flags & 8) {
        context->aux0 = command->words[0] & 0xffff;
        context->aux1 = command->halfwords[3];
        context->aux2 = command->words[1] & 0xffff;
    } else {
        context->input = command->words[0] & 0xffff;
        context->output = command->halfwords[3];
        context->count = command->words[1] & 0xffff;
    }
    if (flags & 8) {
        context->auxResample0 = command->halfwords[0];
        context->auxResample1 = command->halfwords[3];
        context->auxResample2 = command->halfwords[2];
    } else {
        context->inputResample = command->halfwords[0];
        context->outputResample = command->halfwords[3];
        context->count = command->words[1] & 0xffff;
    }
}

void LegacySetVolume(AudioCommand *command, AudioContext *context)
{
    int flags = command->bytes[2];
    if (flags & 8) {
        context->dry = command->halfwords[0];
        context->wet = command->halfwords[2];
    } else if (flags & 4) {
        if (flags & 2) context->volumeLeft16 = command->halfwords[0];
        else context->volumeRight16 = command->halfwords[0];
    } else {
        if (flags & 2) {
            context->targetLeft16 = command->halfwords[0];
            context->rateLeft16 = command->words[1];
        } else {
            context->targetRight16 = command->halfwords[0];
            context->rateRight16 = command->words[1];
        }
    }
}

void LegacyMoveBuffer(AudioCommand *command, AudioContext *context)
{
    MoveAudioBuffer(command->halfwords[3], command->words[0] & 0xffff,
                    command->words[1] & 0xffff, context);
}

void LegacyLoadADPCM(AudioCommand *command, AudioContext *context)
{
    LoadAudioBuffer(0x1000, context->segment + command->words[1],
                    command->words[0] & 0xfff, context);
}

void LegacyMix(AudioCommand *command, AudioContext *context)
{
    int count = context->count;
    int output = command->words[1] & 0xffff;
    int gain = (short)command->halfwords[0];
    int input = command->halfwords[3];
    int i;
    if (count == 0) {
        ChangeAudioABI(2);
        return;
    }
    output >>= 1;
    input >>= 1;
    count >>= 1;
    for (i = 0; i < count; ++i) {
        int sample = (context->buffer[input++] * gain +
                      context->buffer[output] * 32767) >> 15;
        if (sample > 32767) sample = 32767;
        else if (sample < -32767) sample = -32767;
        context->buffer[output++] = sample;
    }
}

void LegacyInterleave(AudioCommand *command, AudioContext *context)
{
    unsigned int left = command->halfwords[3];
    int right = command->words[1] & 0xffff;
    int output = context->output;
    int count = context->count;
    int i;
    count >>= 1;
    count = (count + 7) & ~7;
    left >>= 1;
    right >>= 1;
    output >>= 1;
    for (i = 0; i < count; ++i) {
        int sample = context->buffer[left + i];
        if (sample < -32767) sample = -32767;
        else if (sample > 32767) sample = 32767;
        context->buffer[output + i * 2] = sample;
        sample = context->buffer[right + i];
        if (sample < -32767) sample = -32767;
        else if (sample > 32767) sample = 32767;
        context->buffer[output + i * 2 + 1] = sample;
    }
    RecordHLEBuffer(context->buffer + output, count * 4);
}

void LegacyPoleFilter(AudioCommand *command, AudioContext *context)
{
}

void LegacySetLoop(AudioCommand *command, AudioContext *context)
{
    context->loopAddress = command->words[1];
}

void LegacyClearBuffer(AudioCommand *command, AudioContext *context)
{
    ClearAudioBuffer(command->words[0] & 0xffff, command->words[1] & 0xffff, context);
}
