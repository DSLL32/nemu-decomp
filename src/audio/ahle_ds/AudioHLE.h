#ifndef NEMU_AUDIO_HLE_H
#define NEMU_AUDIO_HLE_H

#include "AudioPlugin.h"

typedef union AudioCommand {
    unsigned int words[2];
    unsigned short halfwords[4];
    unsigned char bytes[8];
} AudioCommand;

/* This layout is recovered from native command handlers and envelope-state
 * save/load routines. Private field names are inferred from those consumers. */
typedef struct AudioContext {
    short *buffer;                         /* 00 */
    short *adpcmTable;                     /* 04 */
    unsigned int segment;                 /* 08 */
    int output;                           /* 0c: byte address */
    int input;                            /* 10: byte address */
    int count;                            /* 14: byte count */
    int aux0, aux1, aux2;                  /* 18..20 */
    short inputResample, outputResample;   /* 24..26 */
    short auxResample0, auxResample1, auxResample2; /* 28..2c */
    short reserved2e;
    unsigned int loopAddress;             /* 30 */
    int volumeLeft, volumeRight;           /* 34..38 */
    int targetLeft, targetRight;           /* 3c..40 */
    int rampLeft, rampRight;               /* 44..48 */
    int rateLeft, rateRight;               /* 4c..50 */
    short volumeLeft16, volumeRight16;     /* 54..56 */
    short targetLeft16, targetRight16;     /* 58..5a */
    int rateLeft16, rateRight16;           /* 5c..60 */
    short dry, wet;                        /* 64..66 */
    short envelope68, envelope6a;          /* 68..6a */
    short envelope6c, envelope6e;          /* 6c..6e */
    short envelope70, envelope72;          /* 70..72 */
    unsigned int mp3Address, mp3State;     /* 74..78 */
} AudioContext;

typedef void (*AudioCommandHandler)(AudioCommand *, AudioContext *);
extern AudioContext AudioState;
extern int Use3DNow, UseSSE, UseMMXExt;

#endif
