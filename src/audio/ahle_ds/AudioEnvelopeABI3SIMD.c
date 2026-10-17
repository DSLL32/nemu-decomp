/* ABI3 envelope mixers. The C setup and state serialization surround the
 * original style of hand-written 3DNow!/SSE sample kernels. Names are inferred.
 * These routines intentionally retain the original fixed buffer offsets. */
#include "AudioHLE.h"

#pragma optimize("", off)

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
