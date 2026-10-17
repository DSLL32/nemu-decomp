/* Integer envelope mixers. Names and source grouping are inferred. */
#include "AudioHLE.h"

void MixAudioEnvelopeClamped(int input, int output, int aux0, int aux1,
                            int aux2, int count, AudioContext *context)
{
    int left;
    int right;
    int rampLeft;
    int rampRight;
    int limitLeft;
    int limitRight;
    int dry;
    int wet;
    int i, j;
    count >>= 1;
    input >>= 1;
    output >>= 1;
    aux0 >>= 1;
    aux1 >>= 1;
    aux2 >>= 1;
    dry = context->dry;
    left = context->targetLeft;
    right = context->targetRight;
    rampLeft = context->rampLeft;
    rampRight = context->rampRight;
    limitLeft = context->rateLeft;
    limitRight = context->rateRight;
    wet = context->wet;
    if (rampLeft == 0xffff) { left = limitLeft; rampLeft = 0; }
    if (rampRight == 0xffff) { right = limitRight; rampRight = 0; }
    if (wet == 0 && left == 0 && rampLeft == 0 && right == 0) return;
    for (i = 0; i < count; i += 8) {
        for (j = i; j < i + 8; ++j) {
            int sample = context->buffer[input + j];
            int l = (sample * left) >> 15;
            int r = (sample * right) >> 15;
            int outLeft = context->buffer[output + j] + l * dry / 32768;
            int outRight = context->buffer[aux0 + j] + r * dry / 32768;
            if (outLeft > 32767) outLeft = 32767;
            else if (outLeft < -32767) outLeft = -32767;
            if (outRight > 32767) outRight = 32767;
            else if (outRight < -32767) outRight = -32767;
            context->buffer[output + j] = outLeft;
            context->buffer[aux0 + j] = outRight;
            outLeft = context->buffer[aux1 + j] + ((l * wet) >> 15);
            outRight = context->buffer[aux2 + j] + ((r * wet) >> 15);
            if (outLeft > 32767) outLeft = 32767;
            else if (outLeft < -32767) outLeft = -32767;
            if (outRight > 32767) outRight = 32767;
            else if (outRight < -32767) outRight = -32767;
            context->buffer[aux1 + j] = outLeft;
            context->buffer[aux2 + j] = outRight;
        }
        left += rampLeft;
        if (rampLeft > 0) {
            if (left > limitLeft) { left = limitLeft; rampLeft = 0; }
        } else {
            if (left < limitLeft) { left = limitLeft; rampLeft = 0; }
        }
        right += rampRight;
        if (rampRight > 0) {
            if (right > limitRight) { right = limitRight; rampRight = 0; }
        } else {
            if (right < limitRight) { right = limitRight; rampRight = 0; }
        }
    }
    context->rampLeft = rampLeft;
    context->rampRight = rampRight;
    context->volumeLeft = wet;
    context->volumeRight = wet;
    context->targetLeft = left;
    context->targetRight = right;
    context->rateLeft = limitLeft;
    context->rateRight = limitRight;
}

void MixAudioEnvelope(int input, int output, int aux0, int aux1,
                      int aux2, int count, AudioContext *context)
{
    int wetLeft;
    int wetRight;
    int left;
    int right;
    int rampLeft;
    int rampRight;
    int limitLeft;
    int limitRight;
    int i, j;
    count >>= 1;
    input >>= 1;
    output >>= 1;
    aux0 >>= 1;
    aux1 >>= 1;
    aux2 >>= 1;
    wetLeft = context->volumeLeft << 1;
    wetRight = context->volumeRight << 1;
    left = context->targetLeft << 1;
    right = context->targetRight << 1;
    rampLeft = context->rampLeft << 1;
    rampRight = context->rampRight << 1;
    limitLeft = context->rateLeft << 1;
    limitRight = context->rateRight << 1;
    if (rampLeft == 0xffff) { left = limitLeft; rampLeft = 0; }
    if (rampRight == 0xffff) { right = limitRight; rampRight = 0; }
    if (wetLeft == 0 && wetRight == 0) {
        if (left == 0 && rampLeft == 0 && right == 0) return;
        if (rampLeft == 0 && rampRight == 0) {
            for (i = 0; i < count; ++i) {
                int sample = context->buffer[input + i];
                int l = (sample * left) >> 16;
                int r = (sample * right) >> 16;
                context->buffer[output + i] += l;
                context->buffer[aux0 + i] += r;
            }
        } else goto ramped;
    } else if (rampLeft == 0 && rampRight == 0) {
        for (i = 0; i < count; ++i) {
            int sample = context->buffer[input + i];
            int l = (sample * left) >> 16;
            int r = (sample * right) >> 16;
            context->buffer[output + i] += l;
            context->buffer[aux0 + i] += r;
            context->buffer[aux1 + i] += (l * wetLeft) >> 16;
            context->buffer[aux2 + i] += (r * wetRight) >> 16;
        }
    } else {
    ramped:
        for (i = 0; i < count; i += 8) {
            for (j = i; j < i + 8; ++j) {
                int sample = context->buffer[input + j];
                int l = (sample * left) >> 16;
                int r = (sample * right) >> 16;
                context->buffer[output + j] += l;
                context->buffer[aux0 + j] += r;
                context->buffer[aux1 + j] += (l * wetLeft) >> 16;
                context->buffer[aux2 + j] += (r * wetRight) >> 16;
            }
            left += rampLeft;
            if (rampLeft > 0) {
                if (left > limitLeft) { left = limitLeft; rampLeft = 0; }
            } else {
                if (left < limitLeft) { left = limitLeft; rampLeft = 0; }
            }
            right += rampRight;
            if (rampRight > 0) {
                if (right > limitRight) { right = limitRight; rampRight = 0; }
            } else {
                if (right < limitRight) { right = limitRight; rampRight = 0; }
            }
        }
    }
    context->volumeLeft = wetLeft >> 1;
    context->volumeRight = wetRight >> 1;
    context->rampLeft = rampLeft >> 1;
    context->rampRight = rampRight >> 1;
    context->targetLeft = left >> 1;
    context->targetRight = right >> 1;
    context->rateLeft = limitLeft >> 1;
    context->rateRight = limitRight >> 1;
}

void MixAudioEnvelopeLegacy(int input, int output, int aux0, int aux1,
                            int aux2, int count, AudioContext *context)
{
    int wetLeft;
    int wetRight;
    int left;
    int right;
    int rampLeft;
    int rampRight;
    int i, j;
    count >>= 1;
    input >>= 1;
    output >>= 1;
    aux0 >>= 1;
    aux1 >>= 1;
    aux2 >>= 1;
    wetLeft = context->volumeLeft;
    wetRight = context->volumeRight;
    left = context->targetLeft;
    right = context->targetRight;
    rampLeft = context->rampLeft;
    rampRight = context->rampRight;
    if (wetLeft == 0 && wetRight == 0) {
        if (left == 0 && rampLeft == 0 && right == 0) return;
        if (rampLeft == 0 && rampRight == 0) {
            for (i = 0; i < count; ++i) {
                int sample = context->buffer[input + i];
                int l = (sample * left) >> 16;
                int r = (sample * right) >> 16;
                int outRight = context->buffer[aux0 + i] + r;
                context->buffer[output + i] += l;
                context->buffer[aux0 + i] = outRight;
            }
        } else goto ramped;
    } else if (rampLeft == 0 && rampRight == 0) {
        for (i = 0; i < count; ++i) {
            int sample = context->buffer[input + i];
            int l = (sample * left) >> 16;
            int r = (sample * right) >> 16;
            int outRight = context->buffer[aux0 + i] + r;
            context->buffer[output + i] += l;
            context->buffer[aux0 + i] = outRight;
            context->buffer[aux1 + i] += (l * wetLeft) >> 16;
            context->buffer[aux2 + i] += (r * wetRight) >> 16;
        }
    } else {
    ramped:
        for (i = 0; i < count; i += 8) {
            for (j = i; j < i + 8; ++j) {
                int sample = context->buffer[input + j];
                int l = (sample * left) >> 16;
                int r = (sample * right) >> 16;
                int outRight = context->buffer[aux0 + j] + r;
                context->buffer[output + j] += l;
                context->buffer[aux0 + j] = outRight;
                context->buffer[aux1 + j] += (l * wetLeft) >> 16;
                context->buffer[aux2 + j] += (r * wetRight) >> 16;
            }
            left += rampLeft;
            right += rampRight;
            if (left > 0xffff) { left = 0xffff; rampLeft = 0; }
            else if (left < 0) { left = 0; rampLeft = 0; }
            if (right > 0xffff) { right = 0xffff; rampRight = 0; }
            else if (right < 0) { right = 0; rampRight = 0; }
        }
    }
}
