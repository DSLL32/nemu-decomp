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


int AudioFilterCount, AudioFilterAddress, AudioFilterOutput, AudioFilterState;

void ABI2_NoOp(AudioCommand *command, AudioContext *context) {}

void ABI2_Unknown(AudioCommand *command, AudioContext *context) {}

void ABI2_ADPCM(AudioCommand *command, AudioContext *context) { ABI1_ADPCM(command, context); }

void ABI2_ClearBuffer(AudioCommand *command, AudioContext *context) { ABI1_ClearBuffer(command, context); }

void ABI2_AddMixer(AudioCommand *command, AudioContext *context)
{
    unsigned int w0 = command->words[0], w1 = command->words[1];
    int count = ((w0 >> 16) & 0xff) * 16;
    int input = w1 >> 16;
    int output = w1 & 0xffff;
    {
        int sampleCount = count >> 1;
        int inputSamples = input >> 1, outputSamples = output >> 1;

    short *destination, *source;
    sampleCount /= 4;
    destination = context->buffer + outputSamples;
    source = context->buffer + inputSamples;
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
}


void ABI2_Resample(AudioCommand *command, AudioContext *context) { ABI1_Resample(command, context); }

void ABI2_Filter(AudioCommand *command, AudioContext *context)
{
    if (FilterHLE == 1) {
        union { unsigned int word; unsigned char bytes[4]; } decoded;
        unsigned int address;
        unsigned int flags, count;
        decoded.word = command->words[0];
        address = command->words[1];
        flags = decoded.bytes[2];
        count = decoded.word & 0xffff;
        if (flags & 2) {
            AudioFilterCount = count;
            AudioFilterAddress = address;
        } else {
            AudioFilterOutput = count;
            AudioFilterState = address & 0xffffff;
            FilterAudioBuffer(count, AudioFilterCount, AudioFilterAddress,
                              AudioFilterState, flags, context);
        }
    }
}

void ABI2_SetBuffer(AudioCommand *command, AudioContext *context) { ABI1_SetBuffer(command, context); }

void ABI2_Duplicate(AudioCommand *command, AudioContext *context)
{
    DuplicateAudioBuffer(command->words[1] >> 16, command->words[0] & 0xffff,
                         command->bytes[2] << 7, context);
}

void ABI2_DuplicateAlt(AudioCommand *command, AudioContext *context)
{
    DuplicateAudioBuffer(command->words[1] >> 16, command->words[0] & 0xffff,
                         command->bytes[2] << 7, context);
}

void ABI2_DMEMMove(AudioCommand *command, AudioContext *context) { ABI1_DMEMMove(command, context); }

void ABI2_LoadADPCM(AudioCommand *command, AudioContext *context) { ABI3_LoadADPCM(command, context); }

void MixAudioScalar(int destination, int source, int byteCount, int gain, AudioContext *context)
{
    int i;
    destination >>= 1; source >>= 1; byteCount >>= 1;
    for (i = 0; i < byteCount; i++) {
        int value = (context->buffer[source + i] * gain) >> 15;
        value += context->buffer[destination + i];
        if (value < -32767) value = -32767;
        else if (value > 32767) value = 32767;
        context->buffer[destination + i] = (short)value;
    }
}

void ABI2_Mixer(AudioCommand *command, AudioContext *context)
{
    unsigned int first = command->words[0];
    unsigned int second = command->words[1];
    MixAudioMMX(second >> 17, ((int)second >> 1) & 0x7fff, (short)first,
                (int)(((first >> 16) & 0xff) << 4) >> 1, context);
}

void ABI2_Interleave(AudioCommand *command, AudioContext *context)
{
    int output = command->words[0] & 0xffff;
    int left = command->halfwords[3];
    int right = command->words[1] & 0xffff;
    int count = command->bytes[2] << 4;
    if (!count) { count = context->count; output = context->output; }
    count >>= 1; right >>= 1; left >>= 1; output >>= 1;
    if (StereoReversed) { int temp = right; right = left; left = temp; }
    InterleaveAudioMMX(output, left, right, count, context);
    RecordHLEBuffer(context->buffer + output, count << 2);
}

void ABI2_Scale(AudioCommand *command, AudioContext *context) { ScaleAudioBufferCommand(command, context); }

void ABI2_SetLoop(AudioCommand *command, AudioContext *context) { ABI1_SetLoop(command, context); }

void ABI2_ExtractChannel(AudioCommand *command, AudioContext *context)
{
    int destination = command->words[1] & 0xffff;
    int source = command->words[1] >> 16;
    int byteCount = (command->words[0] & 0xffff) * 2;

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


void ABI2_LoadBuffer(AudioCommand *command, AudioContext *context) { ABI3_LoadBuffer(command, context); }

void ABI2_SaveBuffer(AudioCommand *command, AudioContext *context) { ABI3_SaveBuffer(command, context); }

void ABI2_EnvSetup2(AudioCommand *command, AudioContext *context)
{
    unsigned int value = command->words[1];
    context->envelope70 = (short)(value >> 16);
    context->envelope72 = (short)value;
}

void ABI2_EnvSetup1(AudioCommand *command, AudioContext *context)
{
    unsigned int first = command->words[0], second = command->words[1];
    unsigned int volume = (first >> 8) & 0xff00;
    context->envelope68 = (short)volume;
    context->envelope6a = (short)first;
    context->envelope6c = (short)(second >> 16);
    context->envelope6e = (short)second;
    if (*(unsigned int *)(RDRAM + AudioTask.ucode_data + 0x34) == 0x1c281b38)
        context->envelope6a = 0;
}

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
