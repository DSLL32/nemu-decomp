/* ABI command families. Names and source-unit boundaries are inferred. */
#include "AudioHLE.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

void AudioError(const char *, HRESULT);
static char AudioErrorMessage[1024];

extern int HighQuality, StereoReversed, FilterHLE;
int SpecialEnvelopeDetected;
int AudioFilterCount, AudioFilterAddress, AudioFilterOutput, AudioFilterState;
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
    int auxiliary = command->bytes[2] & 8;
    if (auxiliary) {
        context->auxResample0 = command->halfwords[0];
        context->auxResample1 = command->halfwords[3];
        context->auxResample2 = command->halfwords[2];
    } else {
        context->inputResample = command->halfwords[0];
        context->outputResample = command->halfwords[3];
        context->count = command->words[1] & 0xffff;
    }
    if (auxiliary) {
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

/* Retained variadic diagnostic entry; private name inferred. */
void AudioWarning(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vsprintf(AudioErrorMessage, format, arguments);
    va_end(arguments);
    AudioError(AudioErrorMessage, 0);
}

void ABI2_NoOp(AudioCommand *command, AudioContext *context) {}
void ABI2_Unknown(AudioCommand *command, AudioContext *context) {}
void ABI2_ADPCM(AudioCommand *command, AudioContext *context) { ABI1_ADPCM(command, context); }
void ABI2_ClearBuffer(AudioCommand *command, AudioContext *context) { ABI1_ClearBuffer(command, context); }
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
    int count = (command->words[0] & 0xffff) << 1;
    int source = command->words[1] >> 16;
    int destination = command->words[1] & 0xffff;
    int i;
    count >>= 1; source >>= 1; destination >>= 1;
    for (i = 0; i < count; i++) {
        context->buffer[destination] = context->buffer[source];
        source += 2; destination++;
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
    context->envelope6a = (short)first;
    context->envelope68 = (short)volume;
    context->envelope6c = (short)(second >> 16);
    context->envelope6e = (short)second;
    if (*(unsigned int *)(RDRAM + AudioTask.ucode_data + 0x34) == 0x1c281b38)
        context->envelope6a = 0;
}

void ABI3_NoOp(AudioCommand *command, AudioContext *context) {}
void ABI3_ADPCM(AudioCommand *command, AudioContext *context)
{
    int second = command->words[1], first = command->words[0];
    int flags = (second >> 28) & 0xf;
    context->input = (second >> 12) & 0xf; context->output = second & 0xfff; context->count = (second >> 16) & 0xfff;
    DecodeAudioADPCM(first & 0xffffff, context->input, context->output, context->count, flags, context);
}
void ABI3_ClearBuffer(AudioCommand *command, AudioContext *context) { ABI1_ClearBuffer(command, context); }
void ABI3_EnvMixer(AudioCommand *command, AudioContext *context)
{
    unsigned int state = command->words[1] & 0xffffff;
    int flags = (command->words[0] >> 16) & 0xff;
    if (HighQuality) {
        if (command->bytes[2] & 1) context->volumeRight16 = command->halfwords[0];
        if (Use3DNow) EnvMixerABI3_3DNow(command, context);
        else if (UseSSE) EnvMixerABI3SSE(command, context);
        else EnvMixerABI3Scalar(command, context);
        return;
    }
    if (context->rampLeft > 65535) context->rampLeft = 65535;
    if (context->rampLeft < -65535) context->rampLeft = -65535;
    if (context->rampRight > 65535) context->rampRight = 65535;
    if (context->rampRight < -65535) context->rampRight = -65535;
    context->targetRight = command->words[0] & 0xffff;
    if (!(flags & 1)) LoadAudioEnvelope(state, context);
    MixAudioEnvelope(0, 0x4e0, 0x650, 0x7c0, 0x930, 0x170, context);
    SaveAudioEnvelope(state, context);
    ResetAudioEnvelope(context);
}
void ABI3_LoadBuffer(AudioCommand *command, AudioContext *context)
{
    int first = command->words[0];
    LoadAudioBuffer(first & 0xfff, command->words[1] & 0xffffff, (first >> 12) & 0xfff, context);
}
void ABI3_Resample(AudioCommand *command, AudioContext *context)
{
    int second = command->words[1];
    int flags = (second >> 30) & 3;
    int speed = (second >> 14) & 0xffff;
    unsigned int state = command->words[0] & 0xffffff;
    context->input = (second >> 2) & 0xfff;
    context->inputResample = (short)context->input;
    context->count = 0x170;
    context->output = (second & 3) ? 0x170 : 0;
    context->outputResample = (short)context->output;
    command->bytes[2] = flags;
    command->halfwords[0] = (short)speed;
    command->words[1] = state;
    ABI1_Resample(command, context);
}
void ABI3_SaveBuffer(AudioCommand *command, AudioContext *context)
{
    int first = command->words[0];
    SaveAudioBuffer(first & 0xfff, command->words[1] & 0xffffff, (first >> 12) & 0xfff, context);
}
void ABI3_Unknown16(AudioCommand *command, AudioContext *context) {}
void ABI3_Unknown17(AudioCommand *command, AudioContext *context) {}
void ABI3_Unknown18(AudioCommand *command, AudioContext *context) {}
void ABI3_MP3(AudioCommand *command, AudioContext *context)
{
    RSP_PrepareAudioMicrocode(command, context);
    RSP_RunAudioMicrocode(command, context);
}
void ABI3_MP3Address(AudioCommand *command, AudioContext *context)
{
    unsigned int address = command->words[0], state = command->words[1];
    context->mp3State = state & 0xffffff;
    context->mp3Address = address & 0xffffff;
}
void ABI3_SetVolume(AudioCommand *command, AudioContext *context)
{
    int flags = command->bytes[2];
    int volume = command->words[0] & 0xffff;
    int fraction = command->words[1] & 0xffff;
    int rate = command->halfwords[3];
    if (flags & 4) {
        if (flags & 2) {
            context->volumeLeft16 = (short)volume;
            context->dry = (short)rate;
            context->wet = (short)fraction;
        } else {
            context->targetRight16 = (short)volume;
            context->rateRight16 = command->words[1];
        }
    } else {
        context->targetLeft16 = (short)volume;
        context->rateLeft16 = command->words[1];
    }
    switch (flags) {
    case 6: context->volumeLeft=fraction; context->volumeRight=fraction; context->targetLeft=volume; break;
    case 4: context->rampRight=(short)rate << 3; context->rateRight=volume; break;
    case 0: context->rampLeft=(short)rate << 3; context->rateLeft=volume; break;
    }
}
void ABI3_DMEMMove(AudioCommand *command, AudioContext *context) { ABI1_DMEMMove(command, context); }
void ABI3_LoadADPCM(AudioCommand *command, AudioContext *context)
{
    LoadAudioBuffer(0x1000, command->words[1], command->words[0] & 0xfff, context);
}
void ABI3_Mixer(AudioCommand *command, AudioContext *context)
{
    MixAudioMMX(command->halfwords[3] >> 1, ((int)command->words[1] >> 1) & 0x7fff,
                (short)command->words[0], 0xb8, context);
}
void ABI3_Interleave(AudioCommand *command, AudioContext *context)
{
    int left=0x270,right=0x328;
    if (StereoReversed) { left=0x328; right=0x270; }
    InterleaveAudioMMX(0, left, right, 0xb8, context);
    RecordHLEBuffer(context->buffer, 0x2e0);
}
void ABI3_PoleFilter(AudioCommand *command, AudioContext *context) {}
void ABI3_SetLoop(AudioCommand *command, AudioContext *context) { context->loopAddress=command->words[1]; }
