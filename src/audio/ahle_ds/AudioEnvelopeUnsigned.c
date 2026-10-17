#include "AudioHLE.h"
#include <stdlib.h>

/* Unsigned envelope gains, applied to signed PCM. The loop structure follows
 * UltraHLE's envmix_zelda; the narrower gains, ramp saturation and dry-output
 * clamps are recovered from audio.dll. Private names and filename are inferred. */
void MixAudioEnvelopeUnsigned(int input, int output, int aux0, int aux1,
                             int aux2, int count, AudioContext *context)
{
    unsigned short wetLeft, wetRight, gainLeft, gainRight;
    unsigned short increaseLeft, decreaseLeft, increaseRight, decreaseRight;
    short rampLeft, rampRight;
    int i, j, jn, left, right;
    short mixedLeft, mixedRight;

    count >>= 1;
    input >>= 1;
    output >>= 1;
    aux0 >>= 1;
    aux1 >>= 1;
    aux2 >>= 1;
    wetLeft = (unsigned short)context->volumeLeft;
    wetRight = (unsigned short)context->volumeRight;
    gainLeft = (unsigned short)context->targetLeft;
    gainRight = (unsigned short)context->targetRight;

    rampLeft = (short)context->rampLeft;
    rampRight = (short)context->rampRight;
    if (rampRight >= 0) {
        increaseRight = abs(rampRight);
        decreaseRight = 0;
    } else {
        increaseRight = 0;
        decreaseRight = abs(rampRight);
    }
    if (rampLeft >= 0) {
        increaseLeft = abs(rampLeft);
        decreaseLeft = 0;
    } else {
        increaseLeft = 0;
        decreaseLeft = abs(rampLeft);
    }

    /* The original silence shortcut does not consult the right ramp. */
    if (wetLeft == 0 && wetRight == 0 && gainLeft == 0 &&
        rampLeft == 0 && gainRight == 0)
        return;
    if (wetLeft == 0 && wetRight == 0 && rampLeft == 0 && rampRight == 0) {
        for (j = 0; j < count; ++j) {
            mixedLeft = mixedRight = context->buffer[input + j];
            mixedLeft = (short)((mixedLeft * gainLeft) >> 16);
            mixedRight = (short)((mixedRight * gainRight) >> 16);
            left = mixedLeft + context->buffer[output + j];
            right = mixedRight + context->buffer[aux0 + j];
            if (left < -32767) left = -32767;
            else if (left > 32767) left = 32767;
            if (right < -32767) right = -32767;
            else if (right > 32767) right = 32767;
            context->buffer[output + j] = (short)left;
            context->buffer[aux0 + j] = (short)right;
        }
    } else if (rampLeft == 0 && rampRight == 0) {
        for (j = 0; j < count; ++j) {
            mixedLeft = mixedRight = context->buffer[input + j];
            mixedLeft = (short)((mixedLeft * gainLeft) >> 16);
            mixedRight = (short)((mixedRight * gainRight) >> 16);
            left = mixedLeft + context->buffer[output + j];
            right = mixedRight + context->buffer[aux0 + j];
            if (left < -32767) left = -32767;
            else if (left > 32767) left = 32767;
            if (right < -32767) right = -32767;
            else if (right > 32767) right = 32767;
            context->buffer[output + j] = (short)left;
            context->buffer[aux0 + j] = (short)right;
            mixedLeft = (mixedLeft * wetLeft) >> 16;
            mixedRight = (mixedRight * wetRight) >> 16;
            context->buffer[aux1 + j] += mixedLeft;
            context->buffer[aux2 + j] += mixedRight;
        }
    } else {
        for (i = 0; i < count; i += 8) {
            jn = i + 8;
            for (j = i; j < jn; ++j) {
                mixedLeft = mixedRight = context->buffer[input + j];
                mixedLeft = (short)((mixedLeft * gainLeft) >> 16);
                mixedRight = (short)((mixedRight * gainRight) >> 16);
                left = mixedLeft + context->buffer[output + j];
                right = mixedRight + context->buffer[aux0 + j];
                if (left < -32767) left = -32767;
                else if (left > 32767) left = 32767;
                if (right < -32767) right = -32767;
                else if (right > 32767) right = 32767;
                context->buffer[output + j] = (short)left;
                context->buffer[aux0 + j] = (short)right;
                mixedLeft = (mixedLeft * wetLeft) >> 16;
                mixedRight = (mixedRight * wetRight) >> 16;
                context->buffer[aux1 + j] += mixedLeft;
                context->buffer[aux2 + j] += mixedRight;
            }
            if (65535 - gainLeft < increaseLeft) gainLeft = 65535;
            else gainLeft += increaseLeft;
            if (gainLeft < decreaseLeft) gainLeft = 0;
            else gainLeft -= decreaseLeft;
            if (65535 - gainRight < increaseRight) gainRight = 65535;
            else gainRight += increaseRight;
            if (gainRight < decreaseRight) gainRight = 0;
            else gainRight -= decreaseRight;
        }
    }
}
