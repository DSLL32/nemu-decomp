/* Floating envelope mixer. The scalar setup intentionally uses the original
 * unoptimized arithmetic around the hand-written SSE sample loop. */
#include "AudioHLE.h"
#include <math.h>

#define ALIGN16 __declspec(align(16))

/* The envelope mixer clips symmetrically at +/-32767. */
#define CLAMP_SAMPLE(tag) \
    __asm cmp ecx, 32767 \
    __asm jle tag##_below_max \
    __asm mov ecx, 32767 \
    __asm jmp tag##_clipped \
    __asm tag##_below_max: \
    __asm cmp ecx, -32767 \
    __asm jge tag##_clipped \
    __asm mov ecx, -32767 \
    __asm tag##_clipped:

#pragma optimize("", off)
void EnvMixerSSE(AudioCommand *command, AudioContext *context)
{
    ALIGN16 int leftMask[4] = {-1, 0, -1, 0};
    ALIGN16 int rightMask[4] = {0, -1, 0, -1};
    ALIGN16 float scale[4] = {1.0f/32768, 1.0f/32768, 1.0f/32768, 1.0f/32768};
    int bothDown = 0, bothUp = 0, leftUp = 0, rightUp = 0;
    unsigned short *state;
    int rateLeft, rateRight;
    ALIGN16 float volumeLeft, volumeRight, targetLeft, targetRight;
    int dry, wet;
    int flags, input, output, aux1, aux0, aux2;
    ALIGN16 float stepLeft, stepRight;
    short *buffer;
    int count;
    short *wetLeft, *wetRight, *dryLeft, *dryRight, *source;
    ALIGN16 float pair[4];
    int rightSample;

    if (!((command->words[0] >> 16 & 0xff) & 1)) {
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
    flags = command->words[0] >> 16 & 0xff;
    input = context->input >> 1;
    output = context->outputResample >> 1;
    aux1 = context->auxResample1 >> 1;
    aux0 = context->auxResample0 >> 1;
    aux2 = context->auxResample2 >> 1;
    stepLeft = rateLeft / 65536.0;
    stepRight = rateRight / 65536.0;
    buffer = context->buffer;
    count = context->count >> 1;
    if (!output && !aux0 && volumeLeft == 0 && stepLeft == 0 && volumeRight == 0)
        return;
    stepLeft = sqrt(sqrt(sqrt(stepLeft)));
    stepRight = sqrt(sqrt(sqrt(stepRight)));
    if (stepLeft > 1.0 && stepRight > 1.0)
        bothUp = 1;
    else if (stepLeft <= 1.0 && stepRight <= 1.0)
        bothDown = 1;
    else if (stepLeft > 1.0 && stepRight <= 1.0)
        leftUp = 1;
    else if (stepLeft <= 1.0 && stepRight > 1.0)
        rightUp = 1;
    wetLeft = buffer + aux1;
    wetRight = buffer + aux2;
    dryLeft = buffer + output;
    dryRight = buffer + aux0;
    source = buffer + input;
    pair[0] = volumeLeft;
    pair[1] = volumeRight;
    pair[2] = volumeLeft;
    pair[3] = volumeRight;
    __asm movaps xmm5, pair
    pair[0] = stepLeft;
    pair[1] = stepRight;
    pair[2] = stepLeft;
    pair[3] = stepRight;
    __asm movaps xmm2, pair
    pair[0] = targetLeft;
    pair[1] = targetRight;
    pair[2] = targetLeft;
    pair[3] = targetRight;
    __asm {
        movaps xmm1, pair
        movaps xmm3, scale
        mov esi, source
        emms
    }
    if (!count) goto done;
    __asm push ebx
    if (bothUp) {
        __asm {
        bothUp_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            add esi, 2
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothUp_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothUp_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothUp_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothUp_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            mulps xmm5, xmm2
            movaps xmm6, xmm1
            movaps xmm7, xmm1
            cmpleps xmm6, xmm5
            andps xmm7, xmm6
            andnps xmm6, xmm5
            orps xmm7, xmm6
            movaps xmm5, xmm7
            dec count
            jg bothUp_loop
        }
    }
    else if (bothDown) {
        __asm {
        bothDown_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            add esi, 2
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothDown_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothDown_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothDown_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothDown_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            mulps xmm5, xmm2
            movaps xmm7, xmm5
            movaps xmm6, xmm5
            cmpleps xmm7, xmm1
            movaps xmm0, xmm7
            andnps xmm7, xmm6
            andps xmm0, xmm1
            orps xmm7, xmm0
            movaps xmm5, xmm7
            dec count
            jg bothDown_loop
        }
    }
    else if (leftUp) {
        __asm movaps xmm4, rightMask
        __asm {
        leftUp_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            add esi, 2
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(leftUp_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(leftUp_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(leftUp_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(leftUp_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            mulps xmm5, xmm2
            movaps xmm7, xmm1
            movaps xmm6, xmm1
            cmpleps xmm6, xmm5
            xorps xmm6, xmm4
            andps xmm7, xmm6
            andnps xmm6, xmm5
            orps xmm7, xmm6
            movaps xmm5, xmm7
            dec count
            jg leftUp_loop
        }
    }
    else if (rightUp) {
        __asm movaps xmm4, leftMask
        __asm {
        rightUp_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            add esi, 2
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(rightUp_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(rightUp_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(rightUp_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(rightUp_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            mulps xmm5, xmm2
            movaps xmm7, xmm1
            movaps xmm6, xmm1
            cmpleps xmm6, xmm5
            xorps xmm6, xmm4
            andps xmm7, xmm6
            andnps xmm6, xmm5
            orps xmm7, xmm6
            movaps xmm5, xmm7
            dec count
            jg rightUp_loop
        }
    }
done:
    __asm {
        pop ebx
        movaps pair, xmm5
        emms
    }
    volumeLeft = pair[0];
    volumeRight = pair[1];
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = volumeLeft;
    state[1] = (volumeLeft - state[0]) * 65536.0f;
    state[2] = volumeRight;
    state[3] = (volumeRight - state[2]) * 65536.0f;
    state[6] = rateLeft >> 16;
    state[5] = rateLeft & 0xffff;
    state[9] = rateRight >> 16;
    state[8] = rateRight & 0xffff;
    state[4] = targetLeft;
    state[7] = targetRight;
    state[10] = dry;
    state[11] = wet;
}

/* 3DNow version uses the same state format and two-lane envelopes. */
void EnvMixer3DNow(AudioCommand *command, AudioContext *context)
{
    ALIGN16 int leftMask[2] = {-1, 0};
    ALIGN16 int rightMask[2] = {0, -1};
    ALIGN16 float scale[2] = {1.0f/32768, 1.0f/32768};
    int bothDown = 0, bothUp = 0, leftUp = 0, rightUp = 0;
    unsigned short *state;
    int rateLeft, rateRight;
    ALIGN16 float volumeLeft, volumeRight, targetLeft, targetRight;
    int dry, wet;
    int flags, input, output, aux1, aux0, aux2;
    ALIGN16 float stepLeft, stepRight;
    short *buffer;
    int count;
    short *wetLeft, *wetRight, *dryLeft, *dryRight, *source;
    ALIGN16 float pair[2];
    int rightSample;

    if (!((command->words[0] >> 16 & 0xff) & 1)) {
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
    flags = command->words[0] >> 16 & 0xff;
    input = context->input >> 1;
    output = context->outputResample >> 1;
    aux1 = context->auxResample1 >> 1;
    aux0 = context->auxResample0 >> 1;
    aux2 = context->auxResample2 >> 1;
    stepLeft = rateLeft / 65536.0;
    stepRight = rateRight / 65536.0;
    buffer = context->buffer;
    count = context->count >> 1;
    if (!output && !aux0 && volumeLeft == 0 && stepLeft == 0 && volumeRight == 0)
        return;
    stepLeft = sqrt(sqrt(sqrt(stepLeft)));
    stepRight = sqrt(sqrt(sqrt(stepRight)));
    if (stepLeft > 1.0 && stepRight > 1.0)
        bothUp = 1;
    else if (stepLeft <= 1.0 && stepRight <= 1.0)
        bothDown = 1;
    else if (stepLeft > 1.0 && stepRight <= 1.0)
        leftUp = 1;
    else if (stepLeft <= 1.0 && stepRight > 1.0)
        rightUp = 1;
    wetLeft = buffer + aux1;
    wetRight = buffer + aux2;
    dryLeft = buffer + output;
    dryRight = buffer + aux0;
    source = buffer + input;
    pair[0] = volumeLeft;
    pair[1] = volumeRight;
    __asm movq mm5, pair
    pair[0] = stepLeft;
    pair[1] = stepRight;
    __asm movq mm2, pair
    pair[0] = targetLeft;
    pair[1] = targetRight;
    __asm {
        movq mm1, pair
        movq mm3, scale
        mov esi, source
        femms
    }
    if (!count) goto done;
    __asm push ebx
    if (bothUp) {
        __asm {
        bothUp_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            add esi, 2
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothUp_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothUp_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothUp_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothUp_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            pfmul mm5, mm2
            movq mm6, mm5
            movq mm7, mm1
            pfcmpgt mm6, mm1
            pand mm7, mm6
            pandn mm6, mm5
            por mm7, mm6
            movq mm5, mm7
            dec count
            jg bothUp_loop
        }
    }
    else if (bothDown) {
        __asm {
        bothDown_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            add esi, 2
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothDown_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothDown_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothDown_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothDown_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            pfmul mm5, mm2
            movq mm7, mm1
            movq mm6, mm5
            pfcmpgt mm7, mm5
            movq mm0, mm7
            pandn mm7, mm6
            pand mm0, mm1
            por mm7, mm0
            movq mm5, mm7
            dec count
            jg bothDown_loop
        }
    }
    else if (leftUp) {
        __asm movq mm4, rightMask
        __asm {
        leftUp_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            add esi, 2
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(leftUp_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(leftUp_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(leftUp_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(leftUp_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            pfmul mm5, mm2
            movq mm7, mm1
            movq mm6, mm5
            pfcmpgt mm6, mm1
            pxor mm6, mm4
            pand mm7, mm6
            pandn mm6, mm5
            por mm7, mm6
            movq mm5, mm7
            dec count
            jg leftUp_loop
        }
    }
    else if (rightUp) {
        __asm movq mm4, leftMask
        __asm {
        rightUp_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            add esi, 2
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(rightUp_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(rightUp_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(rightUp_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(rightUp_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            pfmul mm5, mm2
            movq mm7, mm1
            movq mm6, mm5
            pfcmpgt mm6, mm1
            pxor mm6, mm4
            pand mm7, mm6
            pandn mm6, mm5
            por mm7, mm6
            movq mm5, mm7
            dec count
            jg rightUp_loop
        }
    }
done:
    __asm {
        pop ebx
        movq pair, mm5
        femms
    }
    volumeLeft = pair[0];
    volumeRight = pair[1];
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = volumeLeft;
    state[1] = (volumeLeft - state[0]) * 65536.0f;
    state[2] = volumeRight;
    state[3] = (volumeRight - state[2]) * 65536.0f;
    state[6] = rateLeft >> 16;
    state[5] = rateLeft & 0xffff;
    state[9] = rateRight >> 16;
    state[8] = rateRight & 0xffff;
    state[4] = targetLeft;
    state[7] = targetRight;
    state[10] = dry;
    state[11] = wet;
}
#pragma optimize("", on)
