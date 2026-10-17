#ifndef NEMU_AUDIO_SIMD_H
#define NEMU_AUDIO_SIMD_H

#include "AudioHLE.h"

extern short AudioResampleTable[64][4];

void ResampleAudioMMX(AudioCommand *command, AudioContext *context);
void MixAudioMMX(int input, int output, int gain, int sampleCount,
                 AudioContext *context);
void MixAudioSSE(int input, int output, int gain, int sampleCount,
                 AudioContext *context);
void InterleaveAudioMMX(int output, int left, int right, int sampleCount,
                        AudioContext *context);
void InterleaveAudioSSE(int output, int left, int right, int sampleCount,
                        AudioContext *context);
void MixAudioSign(int input, int output, int sampleCount, AudioContext *context);
void ABI2_AddMixer(AudioCommand *command, AudioContext *context);
void ABI2_EnvMixer(AudioCommand *command, AudioContext *context);
void ABI2_EnvMixerAlt(AudioCommand *command, AudioContext *context);

#endif
