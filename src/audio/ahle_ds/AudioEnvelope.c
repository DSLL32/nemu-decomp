/* Native audio envelope mixers. Private names and this file boundary are inferred. */
#include <math.h>
#include "AudioEnvelope.h"

extern void AudioError(const char *text, HRESULT result);
__inline int ClampAudioSample(int sample);

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

/* ECX is the signed sum. The original mixer saturates to +/-32767. */
#define CLIP_MIXED_SAMPLE(tag) \
    __asm cmp ecx, 32767 \
    __asm jle tag##_lower \
    __asm mov ecx, 32767 \
    __asm jmp tag##_done \
    __asm tag##_lower: \
    __asm cmp ecx, -32767 \
    __asm jge tag##_done \
    __asm mov ecx, -32767 \
    __asm tag##_done:

/* ESI addresses the input, EBX/EDI hold the two scaled channel samples.
 * Wet output addresses are 0x7c0/0x930; dry output is 0x4e0/0x650. */
#define MIX_ABI3_OUTPUTS(tag) \
    __asm mov eax, dry \
    __asm movsx ecx, word ptr [esi+0x4e0] \
    __asm imul eax, ebx \
    __asm sar eax, 15 \
    __asm add ecx, eax \
    CLIP_MIXED_SAMPLE(tag##_dryLeft) \
    __asm mov word ptr [esi+0x4e0], cx \
    __asm mov edx, wet \
    __asm mov ecx, ebx \
    __asm imul ecx, edx \
    __asm movsx eax, word ptr [esi+0x7c0] \
    __asm sar ecx, 15 \
    __asm add ecx, eax \
    CLIP_MIXED_SAMPLE(tag##_wetLeft) \
    __asm mov word ptr [esi+0x7c0], cx \
    __asm mov ecx, edi \
    __asm imul ecx, edx \
    __asm movsx eax, word ptr [esi+0x930] \
    __asm sar ecx, 15 \
    __asm add ecx, eax \
    CLIP_MIXED_SAMPLE(tag##_wetRight) \
    __asm mov word ptr [esi+0x930], cx \
    __asm mov eax, dry \
    __asm movsx ecx, word ptr [esi+0x650] \
    __asm imul eax, edi \
    __asm sar eax, 15 \
    __asm add ecx, eax \
    CLIP_MIXED_SAMPLE(tag##_dryRight) \
    __asm mov word ptr [esi+0x650], cx \
    __asm add esi, 2


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
        buffer[outputLeft] = ClampAudioSample(buffer[outputLeft] + sampleLeft * dry / 32768);
        ++outputLeft;
        buffer[wetLeft] = ClampAudioSample(buffer[wetLeft] + sampleLeft * wet / 32768);
        ++wetLeft;
        sampleRight = (int)(raw * volumeRight / 32768.f);
        buffer[outputRight] = ClampAudioSample(buffer[outputRight] + sampleRight * dry / 32768);
        ++outputRight;
        buffer[wetRight] = ClampAudioSample(buffer[wetRight] + sampleRight * wet / 32768);
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

#pragma optimize("", off)
void EnvMixerABI3_3DNow(AudioCommand *command, AudioContext *context)
{
    __declspec(align(16)) unsigned int leftMask[2] = {0xffffffff, 0};
    __declspec(align(16)) unsigned int rightMask[2] = {0, 0xffffffff};
    __declspec(align(16)) float scale[2] = {1.0f / 32768.0f, 1.0f / 32768.0f};
    int bothDown = 0, bothUp = 0, leftUp = 0, rightUp = 0;
    unsigned short *state;
    int rateLeft, rateRight;
    __declspec(align(16)) float volumeLeft, volumeRight, targetLeft, targetRight;
    int dry, wet, flags, input, outputLeft, auxLeft, outputRight, auxRight, count;
    __declspec(align(16)) float stepLeft, stepRight;
    short *buffer, *wetLeft, *wetRight, *dryLeft, *dryRight, *samples;
    __declspec(align(16)) float packed[2];

    if (!(((command->words[0] >> 16) & 0xff) & 1)) {
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
    flags = (command->words[0] >> 16) & 0xff;
    input = 0;
    outputLeft = 0x270;
    auxLeft = 0x3e0;
    outputRight = 0x328;
    auxRight = 0x498;
    count = 0xb8;
    stepLeft = (float)(rateLeft / 65536.0);
    stepRight = (float)(rateRight / 65536.0);
    buffer = context->buffer;
    count = context->count >> 1;
    if (outputLeft == 0 && outputRight == 0 && volumeLeft == 0 && stepLeft == 0 && volumeRight == 0)
        return;
    stepLeft /= 8;
    stepRight /= 8;
    if (stepLeft > 0 && stepRight > 0)
        bothUp = 1;
    else if (stepLeft <= 0 && stepRight <= 0)
        bothDown = 1;
    else if (stepLeft > 0 && stepRight <= 0)
        leftUp = 1;
    else if (stepLeft <= 0 && stepRight > 0)
        rightUp = 1;
    wetLeft = buffer + auxLeft;
    wetRight = buffer + auxRight;
    dryLeft = buffer + outputLeft;
    dryRight = buffer + outputRight;
    samples = buffer + input;
    packed[0] = volumeLeft;
    packed[1] = volumeRight;
    __asm movq mm5, packed
    packed[0] = stepLeft;
    packed[1] = stepRight;
    __asm movq mm2, packed
    packed[0] = targetLeft;
    packed[1] = targetRight;
    __asm movq mm1, packed
    __asm movq mm3, scale
    __asm mov esi, samples
    __asm femms
    /* Preserve the original zero-count branch: it reaches the EBX restore
     * before the matching save, as in both native SIMD implementations. */
    if (!count)
        __asm jmp finished
    __asm push ebx
    if (bothUp) {
        __asm {
up_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(up)
        __asm {
            pfadd mm5, mm2
            movq mm6, mm5
            movq mm7, mm1
            pfcmpgt mm6, mm1
            pand mm7, mm6
            pandn mm6, mm5
            por mm7, mm6
            movq mm5, mm7
            dec count
            jg up_loop
        }
    }
    else if (bothDown) {
        __asm {
down_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(down)
        __asm {
            pfadd mm5, mm2
            movq mm7, mm1
            movq mm6, mm5
            pfcmpgt mm7, mm5
            movq mm0, mm7
            pandn mm7, mm6
            pand mm0, mm1
            por mm7, mm0
            movq mm5, mm7
            dec count
            jg down_loop
        }
    }
    else if (leftUp) {
        __asm movq mm4, rightMask
        __asm {
left_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(left)
        __asm {
            pfadd mm5, mm2
            movq mm7, mm1
            movq mm6, mm5
            pfcmpgt mm6, mm1
            pxor mm6, mm4
            pand mm7, mm6
            pandn mm6, mm5
            por mm7, mm6
            movq mm5, mm7
            dec count
            jg left_loop
        }
    }
    else if (rightUp) {
        __asm movq mm4, leftMask
        __asm {
right_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(right)
        __asm {
            pfadd mm5, mm2
            movq mm7, mm1
            movq mm6, mm5
            pfcmpgt mm6, mm1
            pxor mm6, mm4
            pand mm7, mm6
            pandn mm6, mm5
            por mm7, mm6
            movq mm5, mm7
            dec count
            jg right_loop
        }
    }
    __asm {
finished:
        pop ebx
        movq packed, mm5
        femms
    }
    volumeLeft = packed[0];
    volumeRight = packed[1];
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = (unsigned short)volumeLeft;
    state[1] = (unsigned short)((volumeLeft - state[0]) * 65536.0f);
    state[2] = (unsigned short)volumeRight;
    state[3] = (unsigned short)((volumeRight - state[2]) * 65536.0f);
    state[6] = (unsigned short)(rateLeft >> 16);
    state[5] = (unsigned short)(rateLeft & 0xffff);
    state[9] = (unsigned short)(rateRight >> 16);
    state[8] = (unsigned short)(rateRight & 0xffff);
    state[4] = (unsigned short)targetLeft;
    state[7] = (unsigned short)targetRight;
    state[10] = (short)dry;
    state[11] = (short)wet;
}

void EnvMixerABI3SSE(AudioCommand *command, AudioContext *context)
{
    __declspec(align(16)) unsigned int leftMask[4] = {0xffffffff, 0, 0xffffffff, 0};
    __declspec(align(16)) unsigned int rightMask[4] = {0, 0xffffffff, 0, 0xffffffff};
    __declspec(align(16)) float scale[4] = {1.0f / 32768.0f, 1.0f / 32768.0f, 1.0f / 32768.0f, 1.0f / 32768.0f};
    int bothDown = 0, bothUp = 0, leftUp = 0, rightUp = 0;
    unsigned short *state;
    int rateLeft, rateRight;
    __declspec(align(16)) float volumeLeft, volumeRight, targetLeft, targetRight;
    int dry, wet, flags, input, outputLeft, auxLeft, outputRight, auxRight, count;
    __declspec(align(16)) float stepLeft, stepRight;
    short *buffer, *wetLeft, *wetRight, *dryLeft, *dryRight, *samples;
    __declspec(align(16)) float packed[4];

    if (!(((command->words[0] >> 16) & 0xff) & 1)) {
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
    flags = (command->words[0] >> 16) & 0xff;
    input = 0;
    outputLeft = 0x270;
    auxLeft = 0x3e0;
    outputRight = 0x328;
    auxRight = 0x498;
    count = 0xb8;
    stepLeft = (float)(rateLeft / 65536.0);
    stepRight = (float)(rateRight / 65536.0);
    buffer = context->buffer;
    count = context->count >> 1;
    if (outputLeft == 0 && outputRight == 0 && volumeLeft == 0 && stepLeft == 0 && volumeRight == 0)
        return;
    stepLeft /= 8;
    stepRight /= 8;
    if (stepLeft > 0 && stepRight > 0)
        bothUp = 1;
    else if (stepLeft <= 0 && stepRight <= 0)
        bothDown = 1;
    else if (stepLeft > 0 && stepRight <= 0)
        leftUp = 1;
    else if (stepLeft <= 0 && stepRight > 0)
        rightUp = 1;
    wetLeft = buffer + auxLeft;
    wetRight = buffer + auxRight;
    dryLeft = buffer + outputLeft;
    dryRight = buffer + outputRight;
    samples = buffer + input;
    packed[0] = volumeLeft;
    packed[1] = volumeRight;
    packed[2] = volumeLeft;
    packed[3] = volumeRight;
    __asm movaps xmm5, packed
    packed[0] = stepLeft;
    packed[1] = stepRight;
    packed[2] = stepLeft;
    packed[3] = stepRight;
    __asm movaps xmm2, packed
    packed[0] = targetLeft;
    packed[1] = targetRight;
    packed[2] = targetLeft;
    packed[3] = targetRight;
    __asm movaps xmm1, packed
    __asm movaps xmm3, scale
    __asm mov esi, samples
    __asm emms
    /* Preserve the original zero-count branch: it reaches the EBX restore
     * before the matching save, as in both native SIMD implementations. */
    if (!count)
        __asm jmp finished
    __asm push ebx
    if (bothUp) {
        __asm {
up_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(up)
        __asm {
            addps xmm5, xmm2
            movaps xmm6, xmm1
            movaps xmm7, xmm1
            cmpleps xmm6, xmm5
            andps xmm7, xmm6
            andnps xmm6, xmm5
            orps xmm7, xmm6
            movaps xmm5, xmm7
            dec count
            jg up_loop
        }
    }
    else if (bothDown) {
        __asm {
down_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(down)
        __asm {
            addps xmm5, xmm2
            movaps xmm7, xmm5
            movaps xmm6, xmm5
            cmpleps xmm7, xmm1
            movaps xmm0, xmm7
            andnps xmm7, xmm6
            andps xmm0, xmm1
            orps xmm7, xmm0
            movaps xmm5, xmm7
            dec count
            jg down_loop
        }
    }
    else if (leftUp) {
        __asm movaps xmm4, rightMask
        __asm {
left_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(left)
        __asm {
            addps xmm5, xmm2
            movaps xmm7, xmm1
            movaps xmm6, xmm1
            cmpleps xmm6, xmm5
            xorps xmm6, xmm4
            andps xmm7, xmm6
            andnps xmm6, xmm5
            orps xmm7, xmm6
            movaps xmm5, xmm7
            dec count
            jg left_loop
        }
    }
    else if (rightUp) {
        __asm movaps xmm4, leftMask
        __asm {
right_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(right)
        __asm {
            addps xmm5, xmm2
            movaps xmm7, xmm1
            movaps xmm6, xmm1
            cmpleps xmm6, xmm5
            xorps xmm6, xmm4
            andps xmm7, xmm6
            andnps xmm6, xmm5
            orps xmm7, xmm6
            movaps xmm5, xmm7
            dec count
            jg right_loop
        }
    }
    __asm {
finished:
        pop ebx
        movaps packed, xmm5
        emms
    }
    volumeLeft = packed[0];
    volumeRight = packed[1];
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = (unsigned short)volumeLeft;
    state[1] = (unsigned short)((volumeLeft - state[0]) * 65536.0f);
    state[2] = (unsigned short)volumeRight;
    state[3] = (unsigned short)((volumeRight - state[2]) * 65536.0f);
    state[6] = (unsigned short)(rateLeft >> 16);
    state[5] = (unsigned short)(rateLeft & 0xffff);
    state[9] = (unsigned short)(rateRight >> 16);
    state[8] = (unsigned short)(rateRight & 0xffff);
    state[4] = (unsigned short)targetLeft;
    state[7] = (unsigned short)targetRight;
    state[10] = (short)dry;
    state[11] = (short)wet;
}
#pragma optimize("", on)

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
        buffer[outputLeft] = ClampAudioSample(buffer[outputLeft] + sampleLeft * dry / 32768);
        ++outputLeft;
        buffer[wetLeft] = ClampAudioSample(buffer[wetLeft] + sampleLeft * wet / 32768);
        ++wetLeft;
        sampleRight = (int)(buffer[input++] * volumeRight / 32768.f);
        buffer[outputRight] = ClampAudioSample(buffer[outputRight] + sampleRight * dry / 32768);
        ++outputRight;
        buffer[wetRight] = ClampAudioSample(buffer[wetRight] + sampleRight * wet / 32768);
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

__inline int ClampAudioSample(int sample)
{
    if (sample > 32767) sample = 32767;
    else if (sample < -32767) sample = -32767;
    return sample;
}

void MixAudioEnvelopeCommand(AudioCommand *command, AudioContext *context)
{
    unsigned int first = command->words[0];
    unsigned int second = command->words[1];
    unsigned short incrementLeft = context->envelope6c;
    unsigned short incrementRight = context->envelope6e;
    unsigned short gainLeft = context->envelope70;
    unsigned short gainRight = context->envelope72;
    unsigned short gainAux = context->envelope68;
    unsigned short incrementAux = context->envelope6a;
    int input = (first >> 12) & 0xff0;
    int count = (first >> 8) & 255;
    short signs[4];
    int outputLeft, outputRight, auxLeft, auxRight;
    short *buffer;
    int block, sample;
    signs[0] = -((first >> 1) & 1);
    signs[1] = -(first & 1);
    signs[2] = -((first >> 1) & 4);
    signs[3] = -((first >> 1) & 2);
    outputLeft = (second >> 20) & 0xff0;
    outputRight = (second >> 12) & 0xff0;
    if ((unsigned char)first & 16) {
        auxRight = (second >> 4) & 0xff0;
        auxLeft = (second & 255) << 4;
    } else {
        auxLeft = (second >> 4) & 0xff0;
        auxRight = (second & 255) << 4;
    }
    buffer = context->buffer;
    auxLeft >>= 1;
    input >>= 1;
    outputLeft >>= 1;
    outputRight >>= 1;
    auxRight >>= 1;
    for (block = 0; block < count; block += 8) {
        for (sample = 0; sample < 8; ++sample) {
            int in = buffer[input++];
            int left = ((in * gainLeft) >> 16) ^ signs[0];
            int right = ((in * gainRight) >> 16) ^ signs[1];
            int wetLeft = buffer[auxLeft];
            int wetRight = buffer[auxRight];
            int dryRight = buffer[outputRight] + right;
            int dryLeft = buffer[outputLeft] + left;
            buffer[outputLeft++] = ClampAudioSample(dryLeft);
            buffer[outputRight++] = ClampAudioSample(dryRight);
            wetRight += ((gainAux * right) >> 16) ^ signs[3];
            wetLeft += ((gainAux * left) >> 16) ^ signs[2];
            buffer[auxLeft++] = ClampAudioSample(wetLeft);
            buffer[auxRight++] = ClampAudioSample(wetRight);
        }
        gainAux += incrementAux;
        gainLeft += incrementLeft;
        gainRight += incrementRight;
    }
}
