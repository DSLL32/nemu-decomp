#ifndef NEMU_AUDIO_ENVELOPE_H
#define NEMU_AUDIO_ENVELOPE_H

#include "AudioHLE.h"

void EnvMixerSSE(AudioCommand *command, AudioContext *context);
void EnvMixer3DNow(AudioCommand *command, AudioContext *context);
void EnvMixerScalar(AudioCommand *command, AudioContext *context);
void EnvMixerBlockScalar(AudioCommand *command, AudioContext *context);
void EnvMixerABI3Scalar(AudioCommand *command, AudioContext *context);
void EnvMixerABI3_3DNow(AudioCommand *command, AudioContext *context);
void EnvMixerABI3SSE(AudioCommand *command, AudioContext *context);
void EnvMixerSpecial(AudioCommand *command, AudioContext *context);
void MixAudioEnvelopeCommand(AudioCommand *command, AudioContext *context);
void MixAudioEnvelopeClamped(int input, int output, int aux0, int aux1,
                            int aux2, int count, AudioContext *context);
void MixAudioEnvelope(int input, int output, int aux0, int aux1,
                      int aux2, int count, AudioContext *context);
void MixAudioEnvelopeLegacy(int input, int output, int aux0, int aux1,
                            int aux2, int count, AudioContext *context);
void MixAudioEnvelopeUnsigned(int input, int output, int aux0, int aux1,
                              int aux2, int count, AudioContext *context);

#endif
