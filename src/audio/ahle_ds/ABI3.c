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


/* The original ABI3 unit retains its own unused copy of the same guest
 * program and a pointer to it (10037290 / 10038290). */
#define RSP_AudioMicrocode ABI3AudioMicrocode
#include "RSPMicrocode.inc"
#undef RSP_AudioMicrocode
unsigned *ABI3AudioMicrocodeImage = ABI3AudioMicrocode;

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
        if (command->halfwords[1] & 1) context->volumeRight16 = command->halfwords[0];
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
