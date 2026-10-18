/* Command diagnostics, ADPCM, pole filtering and scalar resampling.
 * This inferred unit begins at DumpAudioCommand (10AA0); its readonly constants
 * immediately precede the diagnostic strings. See audio-source-grouping.json. */
#include "transform.h"
#include "AudioADPCMBlock.h"
#include "AudioProcessing.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
extern FILE *HLELogFile;
void DumpAudioCommand(AudioCommand *command)
{
    int i;
    if (HLELogFile) {
        fprintf(HLELogFile, "\n");
        for (i = 3; i >= 0; --i)
            fprintf(HLELogFile, " %02X", command->bytes[i]);
        for (i = 3; i >= 0; --i)
            fprintf(HLELogFile, " %02X", command->bytes[i + 4]);
        fprintf(HLELogFile, "\n\n");
    }
}
/* Scalar ADPCM kernels. The original source filename is unknown.
 * The signed decoder shares its indexed packet-copy and two-bit decoding
 * skeleton with UltraHLE src/SLIST.C; target-specific assembly and arithmetic
 * are recovered from audio.dll. See audio-processing-provenance.json. */
#include "AudioProcessing.h"
#include <string.h>

void DecodeAudioADPCMUnsigned(unsigned short *output, unsigned short *history,
                             int offset, unsigned short *book,
                             int bitsPerSample, AudioContext *context)
{
    unsigned char data[16];
    int i = 0;
    int index = offset >> 1;
    short *input;
    unsigned int coefficient1, coefficient2, previous1, previous2, shift;
    unsigned int first, second;
    if (offset & 1) {
        data[i++] = ((unsigned char *)context->buffer)[index * 2];
        ++index;
    }
    input = &context->buffer[index];
    do {
        int pair = *input++;
        data[i++] = pair >> 8;
        data[i++] = pair;
    } while (i < 9);

    previous1 = history[14];
    previous2 = history[15];
    shift = (data[0] >> 4) + 11;
    coefficient1 = book[(data[0] & 15) * 2];
    coefficient2 = book[(data[0] & 15) * 2 + 1];

    /* Each packet contains eight packed pairs of four-bit samples. */
#define DECODE_PAIR(n) \
    first = (data[(n) + 1] >> 4) << shift; \
    second = (data[(n) + 1] & 15) << shift; \
    first = (first + previous1 * coefficient1 + previous2 * coefficient2) >> 11; \
    output[(n) * 2] = first; \
    previous1 = previous2; \
    previous2 = first; \
    second = (second + previous1 * coefficient1 + previous2 * coefficient2) >> 11; \
    output[(n) * 2 + 1] = second; \
    previous1 = previous2; \
    previous2 = second;
    DECODE_PAIR(0)
    DECODE_PAIR(1)
    DECODE_PAIR(2)
    DECODE_PAIR(3)
    DECODE_PAIR(4)
    DECODE_PAIR(5)
    DECODE_PAIR(6)
    DECODE_PAIR(7)
#undef DECODE_PAIR
}

void DecodeAudioADPCMBlock(short *output, short *history, int offset,
                                   short *book, int bitsPerSample,
                                   AudioContext *context)
{
    DECODE_AUDIO_ADPCM_BLOCK(output, history, offset, book, bitsPerSample, context, standalone)
}
#include "AudioProcessing.h"

void PoleFilterAudioCommand(AudioCommand *command, AudioContext *context)
{
    int output = context->outputResample >> 1;
    short *state = (short *)((command->words[1] & 0xffffff) + RDRAM);
    int input = context->inputResample >> 1;
    unsigned int instruction = command->words[0];
    int count = context->count >> 1;
    int gain = instruction & 0xffff;
    short history[4];
    short *coefficients;
    int scaled[8];
    int i, tap, row;
    __int64 accumulator[8];
    if (count == 0) return;
    if (!(instruction & 0x10000)) {
        for (i = 0; i < 4; ++i) history[i] = state[i];
    } else {
        for (i = 0; i < 4; ++i) history[i] = 0;
    }
    coefficients = &context->buffer[0x400];
    for (i = 0; i < 8; ++i)
        scaled[i] = (coefficients[i + 8] * gain) / 0x4000;
    for (i = 0; i < count; i += 8) {
        for (tap = 0; tap < 8; ++tap)
            accumulator[tap] = coefficients[tap] * history[2];
        for (tap = 0; tap < 8; ++tap)
            accumulator[tap] += coefficients[tap + 8] * history[3];
        for (tap = 0; tap < 8; ++tap)
            accumulator[tap] += context->buffer[input + tap] * gain;
        for (row = 0; row < 8; ++row)
            for (tap = row + 1; tap < 8; ++tap)
                accumulator[tap] += context->buffer[input + row] * scaled[tap - row - 1];
        for (tap = 0; tap < 8; ++tap) {
            __int64 sample = accumulator[tap] / 0x4000;
            if (accumulator[tap] - sample * 0x4000 < 0) --sample;
            if (sample > 32767) sample = 32767;
            else if (sample < -32767) sample = -32767;
            context->buffer[output++] = (short)sample;
        }
        history[2] = context->buffer[output - 2];
        history[3] = context->buffer[output - 1];
        input += 8;
    }
    state = (short *)(RDRAM + (command->words[1] & 0xffffff));
    for (i = 0; i < 4; ++i) state[i] = history[i];
}

/* Scalar resampling and ADPCM commands. Original unit boundary inferred from
 * native code order and this unit's retained coefficient data. */
#include "AudioADPCMBlock.h"
#include "AudioProcessing.h"
#include <string.h>
/* Scalar polyphase resampling; original filename is inferred.
 * Byte flags and explicit history loads follow the matching MMX version.
 * The complete scalar body is still unmatched; see audio-processing-provenance.json.
 * Original unit12 retains the AMP table before the writable coefficient family. */
#include "AudioProcessing.h"
#include "transform.h"
#include "AudioResampleTables.h"

void ResampleAudioScalar(AudioCommand *command, AudioContext *context)
{
    unsigned int w0 = command->words[0], w1 = command->words[1];
    int pitch = w0 & 0xffff;
    short extra = 0;
    int output = context->outputResample >> 1;
    int input = context->inputResample >> 1;
    unsigned char flags = (unsigned char)(w0 >> 16);
    short *state = (short *)((w1 & 0xffffff) + (unsigned int)RDRAM);
    int phase;
    int i;
    int samples;
    if (!(flags & 1)) {
        phase = state[4];
        if (flags & 2) {
            extra = state[5];
            for (i = 0; i < extra; ++i)
                context->buffer[input - extra + i] = state[16 - extra + i];
        }
        context->buffer[input - extra - 4] = *state++;
        context->buffer[input - extra - 3] = *state++;
        context->buffer[input - extra - 2] = *state++;
        context->buffer[input - extra - 1] = *state++;
    } else {
        phase = 0;
        context->buffer[input - 4] = 0;
        context->buffer[input - 3] = 0;
        context->buffer[input - 2] = 0;
        context->buffer[input - 1] = 0;
    }
    samples = context->count >> 1;
    input -= extra + 4;
    for (i = 0; i < samples; ++i) {
        short *destination = context->buffer;
        int sample = 0;
        short tableOffset;
        const short *coefficients;
        int tap;
        int advance = phase >> 15;
        input += advance;
        phase -= advance << 15;
        tableOffset = (phase * 64) >> 15; tableOffset <<= 2;
        coefficients = (const short *)AudioResampleTable;
        for (tap = 0; tap < 4; ++tap)
            sample += context->buffer[input + tap] *
                coefficients[tableOffset + tap];
        sample >>= 15;
        if (sample > 32767) sample = 32767;
        else if (sample < -32767) sample = -32767;
        destination[output++] = sample;
        phase += pitch;
    }
    i = phase >> 15;
    input += i;
    phase -= i << 15;
    extra = (input - (context->inputResample >> 1) - 4) & 7;
    if (extra) extra = 8 - extra;
    state = (short *)(RDRAM + (command->words[1] & 0xffffff));
    state[4] = phase;
    state[5] = extra;
    for (i = 0; i < extra; ++i)
        state[16 - extra + i] = context->buffer[input + 4 + i];
    for (i = 0; i < 4; ++i)
        state[i] = context->buffer[input + i];
}




/* Legacy ADPCM decoder with wide predictor accumulators.
 * The original private filename and identifiers are unknown. */
#include "AudioProcessing.h"
#include <string.h>

extern void LoadAudioBuffer(unsigned int, unsigned int, int, AudioContext *);
extern void SaveAudioBuffer(unsigned int, unsigned int, int, AudioContext *);
extern void ReadAudioMemory(void *, const void *, unsigned int);

void DecodeAudioADPCMWideCommand(AudioCommand *command, AudioContext *context)
{
    int residual[16];
    __int64 accumulator[8];
    __int64 sample;
    int output = context->outputResample >> 1;
    int count, i, j, coefficient, scale;
    unsigned char *input;
    short *book;
    unsigned int address = (unsigned int)(RDRAM + (command->words[1] & 0xffffff));
    int flags = command->bytes[2];
    if (!(flags & 1)) {
        if (flags & 2)
            LoadAudioBuffer(output * 2, (unsigned int)(RDRAM + context->loopAddress), 32, context);
        else
            LoadAudioBuffer(output * 2, address, 32, context);
    } else {
        int destination = output * 2;
        if (destination < 0x1000 && destination + 32 <= 0x1000)
            memset(context->buffer + (destination >> 1), 0, 32);
    }
    output += 16;
    ReadAudioMemory(context->buffer, context->buffer, 32);
    count = context->count >> 1;
    input = (unsigned char *)context->buffer + context->inputResample;
    while (count > 0) {
        unsigned char header = *input++;
        scale = 1 << (header >> 4);
        coefficient = header & 15;
        for (i = 0; i < 16; i += 2) {
            int first = *input >> 4;
            int second = *input++ & 15;
            if (first <= 7) residual[i] = first * scale;
            else residual[i] = -(16 - first) * scale;
            if (second <= 7) residual[i + 1] = second * scale;
            else residual[i + 1] = -(16 - second) * scale;
        }
        book = context->adpcmTable + coefficient * 16;
        for (i = 0; i < 8; ++i)
            accumulator[i] = book[i] * context->buffer[output - 2];
        for (i = 0; i < 8; ++i)
            accumulator[i] += book[i + 8] * context->buffer[output - 1];
        for (i = 0; i < 8; ++i)
            accumulator[i] += residual[i] << 11;
        for (i = 0; i < 8; ++i)
            for (j = i + 1; j < 8; ++j)
                accumulator[j] += book[j - i + 7] * residual[i];
        for (i = 0; i < 8; ++i) {
            sample = accumulator[i] / 2048;
            if (accumulator[i] - sample * 2048 < 0) --sample;
            if (sample > 32767) sample = 32767;
            else if (sample < -32767) sample = -32767;
            context->buffer[output++] = (short)sample;
        }
        for (i = 0; i < 8; ++i)
            accumulator[i] = book[i] * context->buffer[output - 2];
        for (i = 0; i < 8; ++i)
            accumulator[i] += book[i + 8] * context->buffer[output - 1];
        for (i = 0; i < 8; ++i)
            accumulator[i] += residual[i + 8] << 11;
        for (i = 0; i < 8; ++i)
            for (j = i + 1; j < 8; ++j)
                accumulator[j] += book[j - i + 7] * residual[i + 8];
        for (i = 0; i < 8; ++i) {
            sample = accumulator[i] / 2048;
            if (accumulator[i] - sample * 2048 < 0) --sample;
            if (sample > 32767) sample = 32767;
            else if (sample < -32767) sample = -32767;
            context->buffer[output++] = (short)sample;
        }
        count -= 16;
    }
    SaveAudioBuffer(output * 2 - 32,
                    (unsigned int)(RDRAM + (command->words[1] & 0xffffff)),
                    32, context);
}

/* Legacy 32-bit ADPCM decoder. Private filename and entry name are inferred.
 *
 * Adapted from the Azimer ADPCM formulation retained in Wii64 ucode1.cpp.
 * Azimer's December 2003 ABI1.cpp also preserves the unused Gain, in, and
 * catchme locals, which affect VC7's expression ordering. The historical
 * relationship and target-specific array bounds remain inferred. See
 * analysis/audio-adpcm-legacy-provenance.json and the accompanying
 * AudioADPCM-LICENSE.txt. The reconstruction retains the original DLL's
 * unusual short-buffer indexing and signed count behavior.
 */
#include "AudioADPCMLegacy.h"
#include <string.h>

void DecodeAudioADPCMLegacy(AudioCommand *command, AudioContext *context) {
    BYTE Flags=command->bytes[2];

    WORD Gain=command->halfwords[0];
    DWORD Address=command->words[1];
    WORD inPtr=0;

    short *out=context->buffer+(context->output >> 1);

    BYTE *in=(BYTE *)(context->buffer+(context->input >> 1));
    short count=(short)context->count;
    BYTE icode;
    BYTE code;
    int vscale;
    WORD index;
    WORD j;
    int a[8];
    short *book1,*book2;
    int l1,l2;
    /* Inferred ninth elements reproduce the two unused target stack slots. */
    int inp1[9];
    int inp2[9];
    count >>= 1;

    memset(out,0,32);

    if(!(Flags&0x1))
    {
        if(Flags&0x2) {
            WORD historyIndex;
            for(historyIndex=0;historyIndex<16;historyIndex++)
                out[historyIndex]=*(short *)&RDRAM[(context->loopAddress+historyIndex*2)^2];
        } else {
            memcpy(out,&RDRAM[Address],32);
        }
    }

    l1=out[14];
    l2=out[15];
    out+=16;
    while(count>0)
    {

        code=*(BYTE *)&context->buffer[((context->input >> 1)+inPtr)^3];
        index=code&0xf;
        index<<=4;
        book1=context->buffer+0x800+index;
        book2=book1+8;
        code>>=4;
        vscale=(0xffff8000>>((12-code)-1));

        inPtr++;
        j=0;
        while(j<8)

        {
            icode=*(BYTE *)&context->buffer[((context->input >> 1)+inPtr)^3];
            inPtr++;

            inp1[j]=(short)((icode&0xf0)<<8);
            if(code<12)
                inp1[j]=((int)((int)inp1[j]*(int)vscale)>>16);
            else { int catchme=1; }

            j++;

            inp1[j]=(short)((icode&0xf)<<12);
            if(code<12)
                inp1[j]=((int)((int)inp1[j]*(int)vscale)>>16);
            else { int catchme=1; }

            j++;
        }
        j=0;
        while(j<8)
        {
            icode=*(BYTE *)&context->buffer[((context->input >> 1)+inPtr)^3];
            inPtr++;

            inp2[j]=(short)((icode&0xf0)<<8);
            if(code<12)
                inp2[j]=((int)((int)inp2[j]*(int)vscale)>>16);
            else { int catchme=1; }

            j++;

            inp2[j]=(short)((icode&0xf)<<12);
            if(code<12)
                inp2[j]=((int)((int)inp2[j]*(int)vscale)>>16);
            else { int catchme=1; }

            j++;
        }

        a[0]= (int)book1[0]*(int)l1;
        a[0]+=(int)book2[0]*(int)l2;
        a[0]+=(int)inp1[0]*(int)2048;

        a[1] =(int)book1[1]*(int)l1;
        a[1]+=(int)book2[1]*(int)l2;
        a[1]+=(int)book2[0]*inp1[0];
        a[1]+=(int)inp1[1]*(int)2048;

        a[2] =(int)book1[2]*(int)l1;
        a[2]+=(int)book2[2]*(int)l2;
        a[2]+=(int)book2[1]*inp1[0];
        a[2]+=(int)book2[0]*inp1[1];
        a[2]+=(int)inp1[2]*(int)2048;

        a[3] =(int)book1[3]*(int)l1;
        a[3]+=(int)book2[3]*(int)l2;
        a[3]+=(int)book2[2]*inp1[0];
        a[3]+=(int)book2[1]*inp1[1];
        a[3]+=(int)book2[0]*inp1[2];
        a[3]+=(int)inp1[3]*(int)2048;

        a[4] =(int)book1[4]*(int)l1;
        a[4]+=(int)book2[4]*(int)l2;
        a[4]+=(int)book2[3]*inp1[0];
        a[4]+=(int)book2[2]*inp1[1];
        a[4]+=(int)book2[1]*inp1[2];
        a[4]+=(int)book2[0]*inp1[3];
        a[4]+=(int)inp1[4]*(int)2048;

        a[5] =(int)book1[5]*(int)l1;
        a[5]+=(int)book2[5]*(int)l2;
        a[5]+=(int)book2[4]*inp1[0];
        a[5]+=(int)book2[3]*inp1[1];
        a[5]+=(int)book2[2]*inp1[2];
        a[5]+=(int)book2[1]*inp1[3];
        a[5]+=(int)book2[0]*inp1[4];
        a[5]+=(int)inp1[5]*(int)2048;

        a[6] =(int)book1[6]*(int)l1;
        a[6]+=(int)book2[6]*(int)l2;
        a[6]+=(int)book2[5]*inp1[0];
        a[6]+=(int)book2[4]*inp1[1];
        a[6]+=(int)book2[3]*inp1[2];
        a[6]+=(int)book2[2]*inp1[3];
        a[6]+=(int)book2[1]*inp1[4];
        a[6]+=(int)book2[0]*inp1[5];
        a[6]+=(int)inp1[6]*(int)2048;

        a[7] =(int)book1[7]*(int)l1;
        a[7]+=(int)book2[7]*(int)l2;
        a[7]+=(int)book2[6]*inp1[0];
        a[7]+=(int)book2[5]*inp1[1];
        a[7]+=(int)book2[4]*inp1[2];
        a[7]+=(int)book2[3]*inp1[3];
        a[7]+=(int)book2[2]*inp1[4];
        a[7]+=(int)book2[1]*inp1[5];
        a[7]+=(int)book2[0]*inp1[6];
        a[7]+=(int)inp1[7]*(int)2048;

        for(j=0;j<8;j++)
        {
            a[j]>>=11;
            if(a[j]>32767) a[j]=32767;
            else if(a[j]<-32768) a[j]=-32768;
            *(out++)=a[j];
        }
        l1=a[6];
        l2=a[7];

        a[0]= (int)book1[0]*(int)l1;
        a[0]+=(int)book2[0]*(int)l2;
        a[0]+=(int)inp2[0]*(int)2048;

        a[1] =(int)book1[1]*(int)l1;
        a[1]+=(int)book2[1]*(int)l2;
        a[1]+=(int)book2[0]*inp2[0];
        a[1]+=(int)inp2[1]*(int)2048;

        a[2] =(int)book1[2]*(int)l1;
        a[2]+=(int)book2[2]*(int)l2;
        a[2]+=(int)book2[1]*inp2[0];
        a[2]+=(int)book2[0]*inp2[1];
        a[2]+=(int)inp2[2]*(int)2048;

        a[3] =(int)book1[3]*(int)l1;
        a[3]+=(int)book2[3]*(int)l2;
        a[3]+=(int)book2[2]*inp2[0];
        a[3]+=(int)book2[1]*inp2[1];
        a[3]+=(int)book2[0]*inp2[2];
        a[3]+=(int)inp2[3]*(int)2048;

        a[4] =(int)book1[4]*(int)l1;
        a[4]+=(int)book2[4]*(int)l2;
        a[4]+=(int)book2[3]*inp2[0];
        a[4]+=(int)book2[2]*inp2[1];
        a[4]+=(int)book2[1]*inp2[2];
        a[4]+=(int)book2[0]*inp2[3];
        a[4]+=(int)inp2[4]*(int)2048;

        a[5] =(int)book1[5]*(int)l1;
        a[5]+=(int)book2[5]*(int)l2;
        a[5]+=(int)book2[4]*inp2[0];
        a[5]+=(int)book2[3]*inp2[1];
        a[5]+=(int)book2[2]*inp2[2];
        a[5]+=(int)book2[1]*inp2[3];
        a[5]+=(int)book2[0]*inp2[4];
        a[5]+=(int)inp2[5]*(int)2048;

        a[6] =(int)book1[6]*(int)l1;
        a[6]+=(int)book2[6]*(int)l2;
        a[6]+=(int)book2[5]*inp2[0];
        a[6]+=(int)book2[4]*inp2[1];
        a[6]+=(int)book2[3]*inp2[2];
        a[6]+=(int)book2[2]*inp2[3];
        a[6]+=(int)book2[1]*inp2[4];
        a[6]+=(int)book2[0]*inp2[5];
        a[6]+=(int)inp2[6]*(int)2048;

        a[7] =(int)book1[7]*(int)l1;
        a[7]+=(int)book2[7]*(int)l2;
        a[7]+=(int)book2[6]*inp2[0];
        a[7]+=(int)book2[5]*inp2[1];
        a[7]+=(int)book2[4]*inp2[2];
        a[7]+=(int)book2[3]*inp2[3];
        a[7]+=(int)book2[2]*inp2[4];
        a[7]+=(int)book2[1]*inp2[5];
        a[7]+=(int)book2[0]*inp2[6];
        a[7]+=(int)inp2[7]*(int)2048;

        for(j=0;j<8;j++)
        {
            a[j]>>=11;
            if(a[j]>32767) a[j]=32767;
            else if(a[j]<-32768) a[j]=-32768;
            *(out++)=a[j];
        }
        l1=a[6];
        l2=a[7];

        count-=32;
    }
    out-=16;
    memcpy(&RDRAM[Address],out,32);
}


void LoadAudioHistory(int offset, unsigned int address, int flags,
                      AudioContext *context)
{
    if (flags & 1) {
        if (offset < 0x1000 && offset + 32 <= 0x1000)
            memset(&context->buffer[offset >> 1], 0, 32);
    } else {
        if (flags & 2)
            LoadAudioBuffer(offset, context->loopAddress, 32, context);
        else
            LoadAudioBuffer(offset, address, 32, context);
    }
}

void SaveAudioHistory(int offset, unsigned int address, int flags,
                      AudioContext *context)
{
    SaveAudioBuffer(offset, address, 32, context);
}



void DecodeAudioADPCM(unsigned int address, int input, int output, int byteCount,
                      int flags, AudioContext *context)
{
    short *history;
    int blocks = (byteCount + 31) >> 5;
    output >>= 1;
    LoadAudioHistory(output * 2, address, flags, context);
    history = &context->buffer[output];
    output += 16;
    if (flags & 4) {
        while (blocks) {
            short *destination = &context->buffer[output];
            {
                short *blockOutput = destination;
                short *blockHistory = history;
                int blockOffset = input;
                short *blockBook = &context->buffer[0x800];
                DECODE_AUDIO_ADPCM_BLOCK(blockOutput, blockHistory, blockOffset,
                    blockBook, 2, context, decode2)
            }
            history = destination;
            input += 5;
            output += 16;
            --blocks;
        }
    } else {
        while (blocks) {
            short *destination = &context->buffer[output];
            {
                short *blockOutput = destination;
                short *blockHistory = history;
                int blockOffset = input;
                short *blockBook = &context->buffer[0x800];
                DECODE_AUDIO_ADPCM_BLOCK(blockOutput, blockHistory, blockOffset,
                    blockBook, 4, context, decode4)
            }
            history = destination;
            input += 9;
            output += 16;
            --blocks;
        }
    }
    SaveAudioHistory(output * 2 - 32, address, flags, context);
}
