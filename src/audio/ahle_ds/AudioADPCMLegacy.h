#ifndef NEMU_AUDIO_ADPCM_LEGACY_H
#define NEMU_AUDIO_ADPCM_LEGACY_H

#include "AudioHLE.h"
void DecodeAudioADPCMWideCommand(AudioCommand *command, AudioContext *context);
void DecodeAudioADPCMLegacy(AudioCommand *command, AudioContext *context);

#endif
