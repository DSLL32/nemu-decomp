/* Native MMX/SSE audio command helpers. Private names are inferred. */
#include "AudioSIMD.h"

const short AudioResampleTable[64][4] = {
    {   3129,  26285,   3398,    -33 },
    {   2873,  26262,   3679,    -40 },
    {   2628,  26217,   3971,    -48 },
    {   2394,  26150,   4276,    -56 },
    {   2173,  26061,   4592,    -65 },
    {   1963,  25950,   4920,    -74 },
    {   1764,  25817,   5260,    -84 },
    {   1576,  25663,   5611,    -95 },
    {   1399,  25487,   5974,   -106 },
    {   1233,  25291,   6347,   -118 },
    {   1077,  25075,   6732,   -130 },
    {    932,  24838,   7127,   -143 },
    {    796,  24583,   7532,   -156 },
    {    671,  24309,   7947,   -170 },
    {    554,  24016,   8371,   -184 },
    {    446,  23706,   8804,   -198 },
    {    347,  23379,   9246,   -212 },
    {    257,  23036,   9696,   -226 },
    {    174,  22678,  10153,   -240 },
    {     99,  22304,  10618,   -254 },
    {     31,  21917,  11088,   -268 },
    {    -30,  21517,  11564,   -280 },
    {    -84,  21104,  12045,   -293 },
    {   -132,  20679,  12531,   -304 },
    {   -173,  20244,  13020,   -314 },
    {   -210,  19799,  13512,   -323 },
    {   -241,  19345,  14006,   -330 },
    {   -267,  18882,  14501,   -336 },
    {   -289,  18413,  14997,   -340 },
    {   -306,  17937,  15493,   -341 },
    {   -320,  17456,  15988,   -340 },
    {   -330,  16970,  16480,   -337 },
    {   -337,  16480,  16970,   -330 },
    {   -340,  15988,  17456,   -320 },
    {   -341,  15493,  17937,   -306 },
    {   -340,  14997,  18413,   -289 },
    {   -336,  14501,  18882,   -267 },
    {   -330,  14006,  19345,   -241 },
    {   -323,  13512,  19799,   -210 },
    {   -314,  13020,  20244,   -173 },
    {   -304,  12531,  20679,   -132 },
    {   -293,  12045,  21104,    -84 },
    {   -280,  11564,  21517,    -30 },
    {   -268,  11088,  21917,     31 },
    {   -254,  10618,  22304,     99 },
    {   -240,  10153,  22678,    174 },
    {   -226,   9696,  23036,    257 },
    {   -212,   9246,  23379,    347 },
    {   -198,   8804,  23706,    446 },
    {   -184,   8371,  24016,    554 },
    {   -170,   7947,  24309,    671 },
    {   -156,   7532,  24583,    796 },
    {   -143,   7127,  24838,    932 },
    {   -130,   6732,  25075,   1077 },
    {   -118,   6347,  25291,   1233 },
    {   -106,   5974,  25487,   1399 },
    {    -95,   5611,  25663,   1576 },
    {    -84,   5260,  25817,   1764 },
    {    -74,   4920,  25950,   1963 },
    {    -65,   4592,  26061,   2173 },
    {    -56,   4276,  26150,   2394 },
    {    -48,   3971,  26217,   2628 },
    {    -40,   3679,  26262,   2873 },
    {    -33,   3398,  26285,   3129 },
};

void ResampleAudioMMX(AudioCommand *command, AudioContext *context)
{
    unsigned int w0 = command->words[0], w1 = command->words[1];
    int pitch = w0 & 0xffff;
    int input = context->inputResample >> 1;
    short history = 0;
    int output = context->outputResample >> 1;
    short *state;
    unsigned char flags = (unsigned char)(w0 >> 16);
    int phase, i, count;
    short *source, *destination, *temporary;
    short products[4];
    state = (short *)((w1 & 0xffffff) + (unsigned int)RDRAM);

    if (!(flags & 1)) {
        phase = state[4];
        if (flags & 2) {
            history = state[5];
            for (i = 0; i < history; ++i)
                context->buffer[input - history + i] = state[16 - history + i];
        }
        context->buffer[input - history - 4] = *state++;
        context->buffer[input - history - 3] = *state++;
        context->buffer[input - history - 2] = *state++;
        context->buffer[input - history - 1] = *state++;
    } else {
        phase = 0;
        context->buffer[input - 4] = 0;
        context->buffer[input - 3] = 0;
        context->buffer[input - 2] = 0;
        context->buffer[input - 1] = 0;
    }
    count = context->count >> 1;
    input -= 4 + history;
    destination = context->buffer + output;
    source = context->buffer + input;
    temporary = products;
    __asm {
        mov esi, source
        xor ecx, ecx
        mov edi, phase
        mov ebx, temporary
    resample_loop:
        mov eax, edi
        shr eax, 15
        add ecx, eax
        shl eax, 15
        sub edi, eax
        mov phase, edi
        shr edi, 9
        movq mm0, qword ptr AudioResampleTable[edi*8]
        pmaddwd mm0, [esi+ecx*2]
        movq mm1, mm0
        psrlq mm1, 32
        paddd mm0, mm1
        movd eax, mm0
        sar eax, 15
        cmp eax, 32767
        jle below_maximum
        mov eax, 32767
        jmp clipped
    below_maximum:
        cmp eax, -32767
        jge clipped
        mov eax, -32767
    clipped:
        mov edx, destination
        mov edi, phase
        mov [edx], ax
        add edx, 2
        add edi, pitch
        dec count
        mov destination, edx
        jnz resample_loop
        mov phase, edi
        add input, ecx
        emms
    }
    i = phase >> 15;
    input += i;
    phase -= i << 15;
    history = (input - (context->inputResample >> 1) - 4) & 7;
    if (history)
        history = 8 - history;
    state = (short *)(RDRAM + (command->words[1] & 0xffffff));
    state[5] = history;
    state[4] = (short)phase;
    for (i = 0; i < history; ++i)
        state[16 - history + i] = context->buffer[input + 4 + i];
    for (i = 0; i < 4; ++i)
        state[i] = context->buffer[input + i];
}

void MixAudioMMX(int input, int output, int gain, int sampleCount,
                 AudioContext *context)
{
    short *destination, *source;
    __declspec(align(16)) short gains[8];
    sampleCount /= 8;
    destination = context->buffer + output;
    source = context->buffer + input;
    gains[0] = (short)gain;
    gains[1] = (short)gain;
    gains[2] = (short)gain;
    gains[3] = (short)gain;
    __asm {
        mov edi, destination
        mov esi, source
        mov ecx, sampleCount
        movq mm1, gains
    mix_loop:
        movq mm2, [esi]
        movq mm4, [edi]
        movq mm3, [esi+8]
        movq mm5, [edi+8]
        paddsw mm2, mm2
        paddsw mm3, mm3
        pmulhw mm2, mm1
        pmulhw mm3, mm1
        paddsw mm4, mm2
        paddsw mm5, mm3
        movq [edi], mm4
        movq [edi+8], mm5
        add edi, 16
        add esi, 16
        dec ecx
        jnz mix_loop
        emms
    }
}

void MixAudioSSE(int input, int output, int gain, int sampleCount,
                 AudioContext *context)
{
    short *destination, *source;
    __declspec(align(16)) short gains[8];
    sampleCount /= 8;
    destination = context->buffer + output;
    source = context->buffer + input;
    gains[0] = (short)gain;
    gains[1] = (short)gain;
    gains[2] = (short)gain;
    gains[3] = (short)gain;
    gains[4] = (short)gain;
    gains[5] = (short)gain;
    gains[6] = (short)gain;
    gains[7] = (short)gain;
    __asm {
        mov edi, destination
        mov esi, source
        mov ecx, sampleCount
        movdqa xmm1, gains
    mix_loop:
        movdqa xmm2, [esi]
        movdqa xmm4, [edi]
        paddsw xmm2, xmm2
        pmulhw xmm2, xmm1
        add edi, 16
        paddsw xmm4, xmm2
        add esi, 16
        movdqa [edi-16], xmm4
        dec ecx
        jnz mix_loop
        emms
    }
}

void InterleaveAudioMMX(int output, int left, int right, int sampleCount,
                        AudioContext *context)
{
    short *destination, *sourceLeft, *sourceRight;
    sampleCount /= 4;
    destination = context->buffer + output;
    sourceLeft = context->buffer + left;
    sourceRight = context->buffer + right;
    __asm {
        mov edi, destination
        mov esi, sourceLeft
        mov eax, sourceRight
        mov ecx, sampleCount
    interleave_loop:
        movq mm1, [esi]
        movq mm0, [eax]
        movq mm2, mm1
        punpcklwd mm1, mm0
        movq [edi], mm1
        punpckhwd mm2, mm0
        movq [edi+8], mm2
        add esi, 8
        add eax, 8
        add edi, 16
        dec ecx
        jnz interleave_loop
        emms
    }
}

void InterleaveAudioSSE(int output, int left, int right, int sampleCount,
                        AudioContext *context)
{
    short *destination, *sourceLeft, *sourceRight;
    sampleCount /= 8;
    destination = context->buffer + output;
    sourceLeft = context->buffer + left;
    sourceRight = context->buffer + right;
    __asm {
        mov edi, destination
        mov esi, sourceLeft
        mov eax, sourceRight
        mov ecx, sampleCount
    interleave_loop:
        movdqu xmm1, [esi]
        movdqu xmm0, [eax]
        movdqu xmm2, xmm1
        punpcklwd xmm1, xmm0
        movdqu [edi], xmm1
        punpckhwd xmm2, xmm0
        movdqu [edi+16], xmm2
        add esi, 16
        add eax, 16
        add edi, 32
        dec ecx
        jnz interleave_loop
        emms
    }
}

__inline void MixAudioSign(int input, int output, int sampleCount,
                          AudioContext *context)
{
    short *destination, *source;
    sampleCount /= 4;
    destination = context->buffer + output;
    source = context->buffer + input;
    __asm {
        mov edi, destination
        mov esi, source
        mov ecx, sampleCount
    mix_loop:
        movq mm2, [esi]
        movq mm4, [edi]
        psraw mm2, 15
        add edi, 8
        paddsw mm4, mm2
        add esi, 8
        movq [edi-8], mm4
        dec ecx
        jnz mix_loop
        emms
    }
}

void ABI2_AddMixer(AudioCommand *command, AudioContext *context)
{
    unsigned int w0 = command->words[0], w1 = command->words[1];
    int count = ((w0 >> 16) & 0xff) * 16;
    int input = w1 >> 16;
    int output = w1 & 0xffff;
    MixAudioSign(input >> 1, output >> 1, count >> 1, context);
}

/* The two ABI dispatch tables retain separate copies of this command handler. */
void ABI2_EnvMixer(AudioCommand *command, AudioContext *context)
{
    unsigned int w0 = command->words[0], w1 = command->words[1];
    short leftGain = context->envelope70;
    short rightGain = context->envelope72;
    short wetGain = context->envelope68;
    short wetDelta = context->envelope6a;
    short leftDelta = context->envelope6c;
    short rightDelta = context->envelope6e;
    int input = (w0 >> 12) & 0xff0;
    int count = (w0 >> 8) & 0xff;
    short invert[4];
    int left, right, wetLeft, wetRight;
    short leftVolume[4], rightVolume[4], wetVolume[4];
    short wetStep[4], leftStep[4], rightStep[4];
    short leftMask[4], rightMask[4], wetLeftMask[4], wetRightMask[4];
    short *source, *destinationLeft, *destinationRight, *destinationWetLeft, *destinationWetRight;
    invert[0] = -((w0 >> 1) & 1);
    invert[1] = -(w0 & 1);
    invert[2] = -((w0 >> 1) & 4);
    invert[3] = -((w0 >> 1) & 2);
    left = (w1 >> 20) & 0xff0;
    right = (w1 >> 12) & 0xff0;
    if (w0 & 0x10) {
        wetRight = (w1 >> 4) & 0xff0;
        wetLeft = (w1 & 0xff) << 4;
    } else {
        wetLeft = (w1 >> 4) & 0xff0;
        wetRight = (w1 & 0xff) << 4;
    }
    leftVolume[0] = leftGain;
    leftVolume[1] = leftGain;
    leftVolume[2] = leftGain;
    leftVolume[3] = leftGain;
    rightVolume[0] = rightGain;
    rightVolume[1] = rightGain;
    rightVolume[2] = rightGain;
    rightVolume[3] = rightGain;
    wetVolume[0] = wetGain;
    wetVolume[1] = wetGain;
    wetVolume[2] = wetGain;
    wetVolume[3] = wetGain;
    wetStep[0] = wetDelta;
    wetStep[1] = 0;
    wetStep[2] = wetDelta;
    wetStep[3] = 0;
    leftStep[0] = leftDelta;
    leftStep[1] = 0;
    leftStep[2] = leftDelta;
    leftStep[3] = 0;
    rightStep[0] = rightDelta;
    rightStep[1] = 0;
    rightStep[2] = rightDelta;
    rightStep[3] = 0;
    leftMask[0] = invert[0];
    leftMask[1] = invert[0];
    leftMask[2] = invert[0];
    leftMask[3] = invert[0];
    rightMask[0] = invert[1];
    rightMask[1] = invert[1];
    rightMask[2] = invert[1];
    rightMask[3] = invert[1];
    wetLeftMask[0] = invert[2];
    wetLeftMask[1] = invert[2];
    wetLeftMask[2] = invert[2];
    wetLeftMask[3] = invert[2];
    wetRightMask[0] = invert[3];
    wetRightMask[1] = invert[3];
    wetRightMask[2] = invert[3];
    wetRightMask[3] = invert[3];
    source = context->buffer + (input >> 1);
    destinationLeft = context->buffer + (left >> 1);
    destinationRight = context->buffer + (right >> 1);
    destinationWetLeft = context->buffer + (wetLeft >> 1);
    destinationWetRight = context->buffer + (wetRight >> 1);
    count /= 8;
    __asm {
        mov      ecx, count
        mov      esi, source
        mov      edi, destinationLeft
        mov      eax, destinationRight
        mov      ebx, destinationWetLeft
        mov      edx, destinationWetRight
        movq     mm2, leftVolume
        movq     mm3, rightVolume
        movq     mm5, wetVolume
        psrlw    mm2, 1
        psrlw    mm3, 1
        psrlw    mm5, 1
    envelope_loop:
        movq     mm1, [esi]
        movq     mm4, mm1
        pmulhw   mm1, mm2
        pmulhw   mm4, mm3
        psllw    mm1, 1
        psllw    mm4, 1
        pxor     mm1, leftMask
        pxor     mm4, rightMask
        movq     mm0, mm1
        movq     mm7, mm4
        paddsw   mm0, [edi]
        paddsw   mm7, [eax]
        movq     [edi], mm0
        movq     [eax], mm7
        pmulhw   mm1, mm5
        pmulhw   mm4, mm5
        psllw    mm1, 1
        psllw    mm4, 1
        pxor     mm1, wetLeftMask
        pxor     mm4, wetRightMask
        paddsw   mm1, [ebx]
        movq     [ebx], mm1
        paddsw   mm4, [edx]
        movq     [edx], mm4
        movq     mm1, [esi + 8]
        movq     mm4, mm1
        pmulhw   mm1, mm2
        pmulhw   mm4, mm3
        psllw    mm1, 1
        psllw    mm4, 1
        pxor     mm1, leftMask
        pxor     mm4, rightMask
        movq     mm0, mm1
        movq     mm7, mm4
        paddsw   mm0, [edi + 8]
        paddsw   mm7, [eax + 8]
        movq     [edi + 8], mm0
        movq     [eax + 8], mm7
        pmulhw   mm1, mm5
        pmulhw   mm4, mm5
        psllw    mm1, 1
        psllw    mm4, 1
        pxor     mm1, wetLeftMask
        pxor     mm4, wetRightMask
        paddsw   mm1, [ebx + 8]
        movq     [ebx + 8], mm1
        paddsw   mm4, [edx + 8]
        movq     [edx + 8], mm4
        add      esi, 0x10
        add      edi, 0x10
        add      eax, 0x10
        add      ebx, 0x10
        add      edx, 0x10
        movq     mm2, leftVolume
        movq     mm3, rightVolume
        movq     mm5, wetVolume
        paddd    mm2, leftStep
        paddd    mm3, rightStep
        paddd    mm5, wetStep
        pslld    mm2, 0x10
        pslld    mm3, 0x10
        pslld    mm5, 0x10
        movq     mm0, mm2
        movq     mm1, mm3
        movq     mm7, mm5
        psrld    mm2, 0x10
        psrld    mm3, 0x10
        psrld    mm5, 0x10
        movq     leftVolume, mm2
        movq     rightVolume, mm3
        movq     wetVolume, mm5
        por      mm2, mm0
        por      mm3, mm1
        por      mm5, mm7
        psrlw    mm2, 1
        psrlw    mm3, 1
        psrlw    mm5, 1
        dec      ecx
        jne      envelope_loop
        emms
    }
}

/* Separate native copy selected by the alternate command table. */
void ABI2_EnvMixerAlt(AudioCommand *command, AudioContext *context)
{
    unsigned int w0 = command->words[0], w1 = command->words[1];
    short leftGain = context->envelope70;
    short rightGain = context->envelope72;
    short wetGain = context->envelope68;
    short wetDelta = context->envelope6a;
    short leftDelta = context->envelope6c;
    short rightDelta = context->envelope6e;
    int input = (w0 >> 12) & 0xff0;
    int count = (w0 >> 8) & 0xff;
    short invert[4];
    int left, right, wetLeft, wetRight;
    short leftVolume[4], rightVolume[4], wetVolume[4];
    short wetStep[4], leftStep[4], rightStep[4];
    short leftMask[4], rightMask[4], wetLeftMask[4], wetRightMask[4];
    short *source, *destinationLeft, *destinationRight, *destinationWetLeft, *destinationWetRight;
    invert[0] = -((w0 >> 1) & 1);
    invert[1] = -(w0 & 1);
    invert[2] = -((w0 >> 1) & 4);
    invert[3] = -((w0 >> 1) & 2);
    left = (w1 >> 20) & 0xff0;
    right = (w1 >> 12) & 0xff0;
    if (w0 & 0x10) {
        wetRight = (w1 >> 4) & 0xff0;
        wetLeft = (w1 & 0xff) << 4;
    } else {
        wetLeft = (w1 >> 4) & 0xff0;
        wetRight = (w1 & 0xff) << 4;
    }
    leftVolume[0] = leftGain;
    leftVolume[1] = leftGain;
    leftVolume[2] = leftGain;
    leftVolume[3] = leftGain;
    rightVolume[0] = rightGain;
    rightVolume[1] = rightGain;
    rightVolume[2] = rightGain;
    rightVolume[3] = rightGain;
    wetVolume[0] = wetGain;
    wetVolume[1] = wetGain;
    wetVolume[2] = wetGain;
    wetVolume[3] = wetGain;
    wetStep[0] = wetDelta;
    wetStep[1] = 0;
    wetStep[2] = wetDelta;
    wetStep[3] = 0;
    leftStep[0] = leftDelta;
    leftStep[1] = 0;
    leftStep[2] = leftDelta;
    leftStep[3] = 0;
    rightStep[0] = rightDelta;
    rightStep[1] = 0;
    rightStep[2] = rightDelta;
    rightStep[3] = 0;
    leftMask[0] = invert[0];
    leftMask[1] = invert[0];
    leftMask[2] = invert[0];
    leftMask[3] = invert[0];
    rightMask[0] = invert[1];
    rightMask[1] = invert[1];
    rightMask[2] = invert[1];
    rightMask[3] = invert[1];
    wetLeftMask[0] = invert[2];
    wetLeftMask[1] = invert[2];
    wetLeftMask[2] = invert[2];
    wetLeftMask[3] = invert[2];
    wetRightMask[0] = invert[3];
    wetRightMask[1] = invert[3];
    wetRightMask[2] = invert[3];
    wetRightMask[3] = invert[3];
    source = context->buffer + (input >> 1);
    destinationLeft = context->buffer + (left >> 1);
    destinationRight = context->buffer + (right >> 1);
    destinationWetLeft = context->buffer + (wetLeft >> 1);
    destinationWetRight = context->buffer + (wetRight >> 1);
    count /= 8;
    __asm {
        mov      ecx, count
        mov      esi, source
        mov      edi, destinationLeft
        mov      eax, destinationRight
        mov      ebx, destinationWetLeft
        mov      edx, destinationWetRight
        movq     mm2, leftVolume
        movq     mm3, rightVolume
        movq     mm5, wetVolume
        psrlw    mm2, 1
        psrlw    mm3, 1
        psrlw    mm5, 1
    envelope_loop:
        movq     mm1, [esi]
        movq     mm4, mm1
        pmulhw   mm1, mm2
        pmulhw   mm4, mm3
        psllw    mm1, 1
        psllw    mm4, 1
        pxor     mm1, leftMask
        pxor     mm4, rightMask
        movq     mm0, mm1
        movq     mm7, mm4
        paddsw   mm0, [edi]
        paddsw   mm7, [eax]
        movq     [edi], mm0
        movq     [eax], mm7
        pmulhw   mm1, mm5
        pmulhw   mm4, mm5
        psllw    mm1, 1
        psllw    mm4, 1
        pxor     mm1, wetLeftMask
        pxor     mm4, wetRightMask
        paddsw   mm1, [ebx]
        movq     [ebx], mm1
        paddsw   mm4, [edx]
        movq     [edx], mm4
        movq     mm1, [esi + 8]
        movq     mm4, mm1
        pmulhw   mm1, mm2
        pmulhw   mm4, mm3
        psllw    mm1, 1
        psllw    mm4, 1
        pxor     mm1, leftMask
        pxor     mm4, rightMask
        movq     mm0, mm1
        movq     mm7, mm4
        paddsw   mm0, [edi + 8]
        paddsw   mm7, [eax + 8]
        movq     [edi + 8], mm0
        movq     [eax + 8], mm7
        pmulhw   mm1, mm5
        pmulhw   mm4, mm5
        psllw    mm1, 1
        psllw    mm4, 1
        pxor     mm1, wetLeftMask
        pxor     mm4, wetRightMask
        paddsw   mm1, [ebx + 8]
        movq     [ebx + 8], mm1
        paddsw   mm4, [edx + 8]
        movq     [edx + 8], mm4
        add      esi, 0x10
        add      edi, 0x10
        add      eax, 0x10
        add      ebx, 0x10
        add      edx, 0x10
        movq     mm2, leftVolume
        movq     mm3, rightVolume
        movq     mm5, wetVolume
        paddd    mm2, leftStep
        paddd    mm3, rightStep
        paddd    mm5, wetStep
        pslld    mm2, 0x10
        pslld    mm3, 0x10
        pslld    mm5, 0x10
        movq     mm0, mm2
        movq     mm1, mm3
        movq     mm7, mm5
        psrld    mm2, 0x10
        psrld    mm3, 0x10
        psrld    mm5, 0x10
        movq     leftVolume, mm2
        movq     rightVolume, mm3
        movq     wetVolume, mm5
        por      mm2, mm0
        por      mm3, mm1
        por      mm5, mm7
        psrlw    mm2, 1
        psrlw    mm3, 1
        psrlw    mm5, 1
        dec      ecx
        jne      envelope_loop
        emms
    }
}
