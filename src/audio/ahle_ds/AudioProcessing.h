#ifndef NEMU_AUDIO_PROCESSING_H
#define NEMU_AUDIO_PROCESSING_H

#include "AudioHLE.h"

extern unsigned int AudioSqrtEvenMantissa[32768];
extern unsigned int AudioSqrtOddMantissa[32768];
extern int AudioEnvelopeWet, AudioEnvelopeDry;

void ReadAudioSamples(short *destination, unsigned int address, int bytes);
void InitializeAudioSqrt(void);
void LoadAudioEnvelope(unsigned int address, AudioContext *context);
void SaveAudioEnvelope(unsigned int address, AudioContext *context);
void ResetAudioEnvelope(AudioContext *context);
int ClampAudioSample(int sample);
void ScaleAudioBuffer(int offset, int bytes, int gain, AudioContext *context);
void ScaleAudioBufferCommand(AudioCommand *command, AudioContext *context);
void FilterAudioBlock(int offset, short *coefficients, AudioContext *context);
void FilterAudioBuffer(int offset, int bytes, unsigned int coefficientsAddress,
                       unsigned int stateAddress, int flags, AudioContext *context);
void LoadAudioHistory(int offset, unsigned int address, int flags, AudioContext *context);
void SaveAudioHistory(int offset, unsigned int address, int flags, AudioContext *context);
void DumpAudioCommand(AudioCommand *command);
void ResampleAudioScalar(AudioCommand *command, AudioContext *context);
void PoleFilterAudioCommand(AudioCommand *command, AudioContext *context);
void DecodeAudioADPCMUnsigned(unsigned short *output, unsigned short *history,
                              int offset, unsigned short *book, int bitsPerSample,
                              AudioContext *context);
void DecodeAudioADPCM(unsigned int address, int input, int output, int byteCount,
                      int flags, AudioContext *context);

int AudioCPUHasSSE(void);
int AudioOSHasSSE(void);
void AudioClampECX(void);

void ResetAudioProfile(void);
void AverageAudioProfile(unsigned int index);
void AdjustAudioProfileMaximum(unsigned int index);
void StartAudioProfile(unsigned int index);
void StopAudioProfile(unsigned int index);

#endif
