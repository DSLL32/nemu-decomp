/* Command-buffer envelope mixer; private filename and identifiers inferred. */
#include "AudioHLE.h"

__inline int ClampAudioSample(int sample)
{
    if (sample > 32767) sample = 32767;
    else if (sample < -32767) sample = -32767;
    return sample;
}

void MixAudioEnvelopeCommand(AudioCommand *command, AudioContext *context)
{
    unsigned int first = command->words[0];
    unsigned int second = command->words[1];
    unsigned short incrementLeft = context->envelope6c;
    unsigned short incrementRight = context->envelope6e;
    unsigned short gainLeft = context->envelope70;
    unsigned short gainRight = context->envelope72;
    unsigned short gainAux = context->envelope68;
    unsigned short incrementAux = context->envelope6a;
    int input = (first >> 12) & 0xff0;
    int count = (first >> 8) & 255;
    short signs[4];
    int outputLeft, outputRight, auxLeft, auxRight;
    short *buffer;
    int block, sample;
    signs[0] = -((first >> 1) & 1);
    signs[1] = -(first & 1);
    signs[2] = -((first >> 1) & 4);
    signs[3] = -((first >> 1) & 2);
    outputLeft = (second >> 20) & 0xff0;
    outputRight = (second >> 12) & 0xff0;
    if ((unsigned char)first & 16) {
        auxRight = (second >> 4) & 0xff0;
        auxLeft = (second & 255) << 4;
    } else {
        auxLeft = (second >> 4) & 0xff0;
        auxRight = (second & 255) << 4;
    }
    buffer = context->buffer;
    auxLeft >>= 1;
    input >>= 1;
    outputLeft >>= 1;
    outputRight >>= 1;
    auxRight >>= 1;
    for (block = 0; block < count; block += 8) {
        for (sample = 0; sample < 8; ++sample) {
            int in = buffer[input++];
            int left = ((in * gainLeft) >> 16) ^ signs[0];
            int right = ((in * gainRight) >> 16) ^ signs[1];
            int wetLeft = buffer[auxLeft];
            int wetRight = buffer[auxRight];
            int dryRight = buffer[outputRight] + right;
            int dryLeft = buffer[outputLeft] + left;
            buffer[outputLeft++] = ClampAudioSample(dryLeft);
            buffer[outputRight++] = ClampAudioSample(dryRight);
            wetRight += ((gainAux * right) >> 16) ^ signs[3];
            wetLeft += ((gainAux * left) >> 16) ^ signs[2];
            buffer[auxLeft++] = ClampAudioSample(wetLeft);
            buffer[auxRight++] = ClampAudioSample(wetRight);
        }
        gainAux += incrementAux;
        gainLeft += incrementLeft;
        gainRight += incrementRight;
    }
}
