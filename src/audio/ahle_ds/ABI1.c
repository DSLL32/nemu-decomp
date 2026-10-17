#include "transform.h"
#include "AudioSIMD.h"
#include "AudioCommands.h"
/* ABI command families. Names and source-unit boundaries are inferred. */
#include "AudioHLE.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

void AudioError(const char *, HRESULT);


extern int HighQuality, StereoReversed, FilterHLE;
extern int SpecialEnvelopeDetected;
extern int AudioFilterCount, AudioFilterAddress, AudioFilterOutput, AudioFilterState;
void LoadAudioBuffer(unsigned int, unsigned int, int, AudioContext *);
void SaveAudioBuffer(unsigned int, unsigned int, int, AudioContext *);
void MoveAudioBuffer(int, int, int, AudioContext *);
void ClearAudioBuffer(int, int, AudioContext *);
void DuplicateAudioBuffer(int, int, int, AudioContext *);
void ExtractAudioChannel(int, int, int, AudioContext *);
void DecodeAudioADPCM(unsigned int, int, int, int, int, AudioContext *);
void ResampleAudioMMX(AudioCommand *, AudioContext *);
void MixAudioMMX(int, int, int, int, AudioContext *);
void InterleaveAudioMMX(int, int, int, int, AudioContext *);
void ABI2_AddMixer(AudioCommand *, AudioContext *);
void EnvMixerSSE(AudioCommand *, AudioContext *);
void EnvMixer3DNow(AudioCommand *, AudioContext *);
void EnvMixerScalar(AudioCommand *, AudioContext *);
void EnvMixerSpecial(AudioCommand *, AudioContext *);
void EnvMixerABI3Scalar(AudioCommand *, AudioContext *);
void EnvMixerABI3_3DNow(AudioCommand *, AudioContext *);
void EnvMixerABI3SSE(AudioCommand *, AudioContext *);
void MixAudioEnvelope(int, int, int, int, int, int, AudioContext *);
void LoadAudioEnvelope(unsigned int, AudioContext *);
void SaveAudioEnvelope(unsigned int, AudioContext *);
void ResetAudioEnvelope(AudioContext *);
void PoleFilterAudioCommand(AudioCommand *, AudioContext *);
void ScaleAudioBufferCommand(AudioCommand *, AudioContext *);
void FilterAudioBuffer(int, int, unsigned int, unsigned int, int, AudioContext *);
void ChangeAudioABI(int);
void RecordHLEBuffer(const void *, int);
void RSP_PrepareAudioMicrocode(AudioCommand *, AudioContext *);
void RSP_RunAudioMicrocode(AudioCommand *, AudioContext *);

void ABI3_LoadBuffer(AudioCommand *, AudioContext *);
void ABI3_SaveBuffer(AudioCommand *, AudioContext *);
void ABI3_LoadADPCM(AudioCommand *, AudioContext *);


/* Explicit zero state belongs to this unit's strong BSS contribution. */
int SpecialEnvelopeDetected = 0;


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

void LoadAudioBuffer(unsigned int destination, unsigned int address,
                     int byteCount, AudioContext *context)
{
    int i, sample;
    byteCount = (byteCount + 3) & ~3;
    if ((destination & 1) || (address & 3) || (byteCount & 1)) {
        AudioError("\n**alignment error in load\n", 0);
        return;
    }
    if (destination + byteCount > 0x1100)
        AudioError("\n**buffer overrun in abi loadmem() function\n", 0);
    destination >>= 1;
    for (i = 0; i < byteCount; i += 4) {
        sample = *(int *)(RDRAM + (address & 0xffffff));
        address += 4;
        context->buffer[destination++] = (short)(sample >> 16);
        context->buffer[destination++] = (short)sample;
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

void SaveAudioBuffer(unsigned int source, unsigned int address,
                     int byteCount, AudioContext *context)
{
    int i, sample;
    byteCount = (byteCount + 3) & ~3;
    if ((source & 1) || (address & 3) || (byteCount & 1)) {
        AudioError("\n**alignment error in store\n", 0);
        return;
    }
    source >>= 1;
    for (i = 0; i < byteCount; i += 4) {
        sample = context->buffer[source++] << 16;
        sample |= context->buffer[source++] & 65535;
        *(int *)(RDRAM + ((i + address) & 0xffffff)) = sample;
    }
}

void MoveAudioBuffer(int destination, int source, int byteCount, AudioContext *context)
{
    if (source >= 0x1000 || destination >= 0x1000 ||
        source + byteCount > 0x1000 || destination + byteCount > 0x1000)
        return;
    memcpy(context->buffer + (destination >> 1), context->buffer + (source >> 1), byteCount);
}

__inline void ClearAudioBuffer(int destination, int byteCount, AudioContext *context)
{
    if (destination >= 0x1000 || destination + byteCount > 0x1000)
        return;
    memset(context->buffer + (destination >> 1), 0, byteCount);
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

void ABI1_NoOp(AudioCommand *command, AudioContext *context) {}

void ABI1_ADPCM(AudioCommand *command, AudioContext *context)
{
    DecodeAudioADPCM(command->words[1], context->input, context->output,
                    context->count, command->bytes[2], context);
}

void ABI1_ClearBuffer(AudioCommand *command, AudioContext *context)
{
    ClearAudioBuffer(command->words[0] & 0xffff, command->words[1] & 0xffff, context);
}

void ABI1_EnvMixer(AudioCommand *command, AudioContext *context)
{
    int flags = command->bytes[2];
    int input = context->input;
    int count = context->count;
    int output = context->output;
    int aux0 = context->aux0;
    int aux1 = context->aux1;
    int aux2 = context->aux2;
    unsigned int state;
    if (HighQuality) {
        if (*(unsigned int *)(RDRAM + AudioTask.ucode_data + 0x14) == 0x11ec1ba0) {
            if (!SpecialEnvelopeDetected)
                SpecialEnvelopeDetected = 1;
            EnvMixerSpecial(command, context);
        } else if (Use3DNow) {
            EnvMixer3DNow(command, context);
        } else if (UseSSE) {
            EnvMixerSSE(command, context);
        } else {
            EnvMixerScalar(command, context);
        }
        return;
    }
    state = command->words[1];
    if (!(flags & 1)) LoadAudioEnvelope(state, context);
    MixAudioEnvelope(input, output, aux0, aux1, aux2, count, context);
    SaveAudioEnvelope(state, context);
    ResetAudioEnvelope(context);
}

void ABI1_LoadBuffer(AudioCommand *command, AudioContext *context)
{
    LoadAudioBuffer(context->input, command->words[1] + context->segment, context->count, context);
}

void ABI1_Resample(AudioCommand *command, AudioContext *context)
{
    ResampleAudioMMX(command, context);
}

void ABI1_SaveBuffer(AudioCommand *command, AudioContext *context)
{
    SaveAudioBuffer(context->output, command->words[1] + context->segment, context->count, context);
}

void ABI1_Segment(AudioCommand *command, AudioContext *context) {}

void ABI1_SetBuffer(AudioCommand *command, AudioContext *context)
{
    int flags = command->bytes[2];
    if (flags & 8) {
        context->auxResample0 = command->halfwords[0];
        context->auxResample1 = command->halfwords[3];
        context->auxResample2 = command->halfwords[2];
    } else {
        context->inputResample = command->halfwords[0];
        context->outputResample = command->halfwords[3];
        context->count = command->words[1] & 0xffff;
    }
    if (flags & 8) {
        context->aux0 = command->words[0] & 0xffff;
        context->aux1 = command->words[1] >> 16;
        context->aux2 = command->words[1] & 0xffff;
    } else {
        context->input = command->words[0] & 0xffff;
        context->output = command->words[1] >> 16;
        context->count = command->words[1] & 0xffff;
    }
}

void ABI1_SetVolume(AudioCommand *command, AudioContext *context)
{
    int flags = command->bytes[2];
    int volume = command->words[0] & 0xffff;
    int fraction = command->words[1] & 0xffff;
    int rate = command->halfwords[3];
    if (flags & 8) {
        context->dry = (short)volume;
        context->wet = (short)fraction;
    } else if (flags & 4) {
        if (flags & 2) context->volumeLeft16 = (short)volume;
        else context->volumeRight16 = (short)volume;
    } else if (flags & 2) {
        context->targetLeft16 = (short)volume;
        context->rateLeft16 = command->words[1];
    } else {
        context->targetRight16 = (short)volume;
        context->rateRight16 = command->words[1];
    }
    switch (flags) {
    case 8: context->volumeLeft = fraction; context->volumeRight = fraction; break;
    case 6: context->targetLeft = volume; break;
    case 4: context->targetRight = volume; break;
    case 2:
        context->rateLeft = volume;
        if (rate) context->rampLeft = fraction & 0xffff;
        else context->rampLeft = (short)fraction;
        if (context->rampLeft > 65535) context->rampLeft = 65535;
        if (context->rampLeft < -65535) context->rampLeft = -65535;
        break;
    case 0:
        context->rateRight = volume;
        if (rate) context->rampRight = fraction & 0xffff;
        else context->rampRight = (short)fraction;
        if (context->rampRight > 65535) context->rampRight = 65535;
        if (context->rampRight < -65535) context->rampRight = -65535;
        break;
    }
}

void ABI1_DMEMMove(AudioCommand *command, AudioContext *context)
{
    MoveAudioBuffer(command->halfwords[3], command->words[0] & 0xffff,
                    command->words[1] & 0xffff, context);
}

void ABI1_LoadADPCM(AudioCommand *command, AudioContext *context)
{
    LoadAudioBuffer(0x1000, command->words[1] + context->segment,
                    command->words[0] & 0xfff, context);
}

void ABI1_Mixer(AudioCommand *command, AudioContext *context)
{
    int input = command->halfwords[3];
    int count = context->count;
    short gain = (short)command->words[0];
    int output = command->words[1] & 0xffff;
    if (!count) { ChangeAudioABI(2); return; }
    MixAudioMMX(input >> 1, output >> 1, gain, count >> 1, context);
}

void ABI1_Interleave(AudioCommand *command, AudioContext *context)
{
    unsigned int left = command->halfwords[3];
    int right = command->words[1] & 0xffff;
    int count = context->count;
    int output = context->output;
    count >>= 1;
    count = (count + 7) & ~7;
    left >>= 1;
    right >>= 1;
    output >>= 1;
    if (StereoReversed) { int temp = left; left = right; right = temp; }
    InterleaveAudioMMX(output, left, right, count, context);
    RecordHLEBuffer(context->buffer + output, count << 2);
}

void ABI1_PoleFilter(AudioCommand *command, AudioContext *context)
{
    int i, output, count;
    if (FilterHLE != 1) {
        output = context->outputResample >> 1;
        count = context->count >> 1;
        for (i = 0; i < count; i++) context->buffer[output + i] = 0;
    } else PoleFilterAudioCommand(command, context);
}

void ABI1_SetLoop(AudioCommand *command, AudioContext *context)
{
    context->loopAddress = command->words[1];
}

void MixAudioSign(int input, int output, int sampleCount,
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

void DuplicateAudioBuffer(int destination, int source, int byteCount, AudioContext *context)
{
    int i, j;
    destination >>= 1;
    source >>= 1;
    byteCount >>= 1;
    for (i = 0; i < byteCount; i += 64) {
        for (j = 0; j < 64; j++)
            context->buffer[destination + j] = context->buffer[source + j];
        destination += 64;
    }
}

void ExtractAudioChannel(int destination, int source, int byteCount, AudioContext *context)
{
    int i;
    destination >>= 1;
    source >>= 1;
    byteCount >>= 1;
    for (i = 0; i < byteCount; i++) {
        context->buffer[destination] = context->buffer[source];
        source += 2;
        destination++;
    }
}

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

