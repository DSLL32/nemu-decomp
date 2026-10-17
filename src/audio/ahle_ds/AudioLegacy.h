#ifndef NEMU_AUDIO_LEGACY_H
#define NEMU_AUDIO_LEGACY_H

#include "AudioHLE.h"

void LegacyEnvelopeMixer(AudioCommand *, AudioContext *);
void LegacyNoOp(AudioCommand *, AudioContext *);
void LegacyADPCM(AudioCommand *, AudioContext *);
void LegacyEnvelope(AudioCommand *, AudioContext *);
void LegacyLoadBuffer(AudioCommand *, AudioContext *);
void LegacyResample(AudioCommand *, AudioContext *);
void LegacySaveBuffer(AudioCommand *, AudioContext *);
void LegacySegment(AudioCommand *, AudioContext *);
void LegacySetBuffer(AudioCommand *, AudioContext *);
void LegacySetVolume(AudioCommand *, AudioContext *);
void LegacyMoveBuffer(AudioCommand *, AudioContext *);
void LegacyLoadADPCM(AudioCommand *, AudioContext *);
void LegacyMix(AudioCommand *, AudioContext *);
void LegacyInterleave(AudioCommand *, AudioContext *);
void LegacyPoleFilter(AudioCommand *, AudioContext *);
void LegacySetLoop(AudioCommand *, AudioContext *);
void LegacyClearBuffer(AudioCommand *, AudioContext *);

#endif
