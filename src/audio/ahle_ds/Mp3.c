/* Integer synthesis and envelope processing. The contiguous source grouping
 * is inferred from native function order and all thirteen repeated readonly
 * constant blocks; see analysis/audio-source-grouping.json. The private
 * filename remains inferred. */
/* Integer MP3 synthesis. Original unit name inferred from the contiguous band.
 * The primitives and butterfly have complete native-byte matches. Mp3DCT is
 * still a source candidate; analysis/audio-mp3-main-comparison.json records its
 * complete-byte differences and finite x86 behavior diagnostics.
 */
#include "transform.h"
#include <stdlib.h>

extern void ReadAudioMemory(void *buffer, const void *source, unsigned int bytes);

short Mp3Narrow(int value)
{
    return (short)value;
}

short Mp3Add(int left, short right)
{
    return left + right;
}

short Mp3Subtract(int left, int right)
{
    return left - right;
}

__inline int Mp3MultiplyFraction(short left, short right)
{
    return ((left * right) * 2 + 0x8000) >> 16;
}

short Mp3MultiplyUnsigned(short left, unsigned short right)
{
    return (left * right) >> 16;
}

short Mp3MultiplySplit(short value, unsigned short low, short high)
{
    return (value * (low + (high << 16))) >> 16;
}

__inline int Mp3AccumulateFraction(short first, short second,
                            unsigned short low, short high)
{
    __int64 accumulator = (second * high) * 2 + 0x8000;
    accumulator += ((__int64)(first * low)) & 0xffffff0000;
    return accumulator >> 16;
}

__inline int Mp3AccumulateUnsigned(short first, short second,
                            unsigned short low, unsigned short high)
{
    return (((first * low) & 0xffff0000) + second * high) >> 16;
}

void Mp3ReadSamples(unsigned short *buffer, const void *source, int bytes)
{
    int i;
    ReadAudioMemory(buffer, source, bytes);
    for (i = 0; i < bytes / 2; ++i)
        buffer[i] = (unsigned short)_rotl(buffer[i], 8);
}
/* Fixed-point counterpart of AMP's d/c butterfly workspace. The original
 * retains all 48 scalar declarations, including d16..d31, although those
 * sixteen intermediates are unused in this implementation. Their native
 * common-symbol order, together with res, reproduces the entire observed
 * storage interval; see analysis/audio-mp3-global-storage.json.
 */
short d0,d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,d11,d12,d13,d14,d15;
short d16,d17,d18,d19,d20,d21,d22,d23,d24,d25,d26,d27,d28,d29,d30,d31;
short c0,c1,c2,c3,c4,c5,c6,c7,c8,c9,c10,c11,c12,c13,c14,c15;
float res[32][18];

void Mp3Butterfly(unsigned short *coefficients)
{
    c0 = d8 + d0;
    c1 = d9 + d1;
    c2 = d10 + d2;
    c3 = d11 + d3;
    c4 = d12 + d4;
    c5 = d13 + d5;
    c6 = d14 + d6;
    c7 = d15 + d7;
    c8 = d0 - d8;
    c9 = d1 - d9;
    c10 = d2 - d10;
    c11 = d3 - d11;
    c12 = d4 - d12;
    c13 = d5 - d13;
    c14 = d6 - d14;
    c15 = d7 - d15;
    d0 = c0 + c4;
    d1 = c1 + c5;
    d2 = c2 + c6;
    d3 = c3 + c7;
    d4 = c0 - c4;
    d5 = c1 - c5;
    d6 = c2 - c6;
    d7 = c3 - c7;
    c8 = (c8 * coefficients[8]) >> 16;
    c9 = (c9 * coefficients[10]) >> 16;
    c10 = (c10 * coefficients[14]) >> 16;
    c11 = (c11 * coefficients[12]) >> 16;
    c12 = (c12 * coefficients[21]) >> 16;
    c13 = (c13 * coefficients[19]) >> 16;
    c14 = (c14 * coefficients[15]) >> 16;
    c15 = (c15 * coefficients[17]) >> 16;
    d4 = (d4 * coefficients[9]) >> 16;
    d5 = (d5 * coefficients[13]) >> 16;
    d6 = (d6 * coefficients[20]) >> 16;
    d7 = (d7 * coefficients[16]) >> 16;
    d8 = c8 + c12;
    d9 = c9 + c13;
    d10 = c10 + c14;
    d11 = c11 + c15;
    d12 = c8 - c12;
    d13 = c9 - c13;
    d14 = c10 - c14;
    d15 = c11 - c15;
    d12 = (d12 * coefficients[9]) >> 16;
    d13 = (d13 * coefficients[13]) >> 16;
    d14 = (d14 * coefficients[20]) >> 16;
    d15 = (d15 * coefficients[16]) >> 16;
    c0 = d0 + d2;
    c1 = d1 + d3;
    c2 = d0 - d2;
    c3 = d1 - d3;
    c4 = d4 + d6;
    c5 = d5 + d7;
    c6 = d4 - d6;
    c7 = d5 - d7;
    c8 = d8 + d10;
    c9 = d9 + d11;
    c10 = d8 - d10;
    c11 = d9 - d11;
    c12 = d12 + d14;
    c13 = d13 + d15;
    c14 = d12 - d14;
    c15 = d13 - d15;
    c2 = (c2 * coefficients[11]) >> 16;
    c3 = (c3 * coefficients[18]) >> 16;
    c6 = (c6 * coefficients[11]) >> 16;
    c7 = (c7 * coefficients[18]) >> 16;
    c10 = (c10 * coefficients[11]) >> 16;
    c11 = (c11 * coefficients[18]) >> 16;
    c14 = (c14 * coefficients[11]) >> 16;
    c15 = (c15 * coefficients[18]) >> 16;
}

#include <string.h>
#define MP3_ADD16(a, b) ((unsigned short)((int)(a) + (short)(b)))
#define MP3_SUB16(a, b) ((unsigned short)((int)(a) - (short)(b)))
#define MP3_SPLIT(value, factor) (((short)(value) * (int)(factor)) >> 16)
#define MP3_FRACTION Mp3MultiplyFraction
#define MP3_ACCUMULATE_FRACTION Mp3AccumulateFraction
#define MP3_ACCUMULATE_UNSIGNED Mp3AccumulateUnsigned

/* Buffer extents are inferred from the original stack partitions. The table
 * read begins at byte 16, and the input pointer skips an eight-byte header.
 * The original routine repeats each input block six times without advancing
 * its output base; it does not write the loaded synthesis state back.
 */
void Mp3DCT(unsigned char *input, const void *table, const void *state, unsigned int index)
{
    unsigned short coefficients[0x1120];
    short memory[0x800];
    unsigned char samples[0x300];
    short *output = (short *) input;
    unsigned int t6, t5, t4, t0, t2, t3;
    int outer, inner, x, i, offset;

    int v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15;
    input += 8;
    Mp3ReadSamples(coefficients + 8, table, 0x890);
    memset(memory, 0, sizeof(memory));
    t4 = index & 0x1e;
    t5 = 0xac0;
    Mp3ReadSamples(memory + 0x8a0 / 2, state, 0x440);
    t6 = 0x8a0;
    for (outer = 0; outer < 3; ++outer) {
        Mp3ReadSamples((unsigned short *) samples, input, 0x180);
        input += 0x180;
        for (inner = 0; inner < 6; ++inner) {
            d0 = Mp3Add(*(int *) (samples + 0), *(int *) (samples + 62));
            d1 = Mp3Add(*(int *) (samples + 2), *(int *) (samples + 60));
            d3 = Mp3Add(*(int *) (samples + 4), *(int *) (samples + 58));
            d2 = Mp3Add(*(int *) (samples + 6), *(int *) (samples + 56));
            d6 = Mp3Add(*(int *) (samples + 8), *(int *) (samples + 54));
            d7 = Mp3Add(*(int *) (samples + 10), *(int *) (samples + 52));
            d5 = Mp3Add(*(int *) (samples + 12), *(int *) (samples + 50));
            d4 = Mp3Add(*(int *) (samples + 14), *(int *) (samples + 48));
            d12 = Mp3Add(*(int *) (samples + 16), *(int *) (samples + 46));
            d13 = Mp3Add(*(int *) (samples + 18), *(int *) (samples + 44));
            d15 = Mp3Add(*(int *) (samples + 20), *(int *) (samples + 42));
            d14 = Mp3Add(*(int *) (samples + 22), *(int *) (samples + 40));
            d10 = Mp3Add(*(int *) (samples + 24), *(int *) (samples + 38));
            d11 = Mp3Add(*(int *) (samples + 26), *(int *) (samples + 36));
            d9 = Mp3Add(*(int *) (samples + 28), *(int *) (samples + 34));
            d8 = Mp3Add(*(int *) (samples + 30), *(int *) (samples + 32));
            t5 &= 0xffe0;
            t5 |= t4;
            t6 |= t4;
            Mp3Butterfly(coefficients);
            t0 = t6 + 0x100;
            t2 = t5 + 0x100;
            t3 = t5 + 0x200;
            v11 = MP3_SPLIT(MP3_SUB16(c0, c1), coefficients[40]);
            v0 = -MP3_ADD16(c0, c1);
            memory[t6 >> 1] = v11;
            memory[t3 >> 1] = v0;
            v2 = -MP3_ADD16(c2, c3);
            memory[t2 >> 1] = v2;
            v3 = MP3_SPLIT(MP3_SUB16(c2, c3), (coefficients[43] + ((short) coefficients[42] << 16))) - v2;
            memory[t0 >> 1] = v3;
            memory[t5 >> 1] = -v11;
            v4 = -MP3_ADD16(c4, c5);
            v5 = MP3_SPLIT(MP3_SUB16(c4, c5), (coefficients[43] + ((short) coefficients[42] << 16)));
            memory[(t3 - 0x80) >> 1] = v4;
            v7 = MP3_SPLIT(MP3_SUB16(c6, c7), (coefficients[45] + ((short) coefficients[44] << 16)));
            v5 -= v4;
            v7 -= v5;
            v6 = MP3_ADD16(c6, c7) * 2;
            v4 += v6;
            v5 -= v6;
            memory[(t6 + 0x180) >> 1] = v7;
            memory[(t2 - 0x80) >> 1] = -v4;
            memory[(t0 - 0x80) >> 1] = v5;
            v8 = MP3_ADD16(c8, c9);
            v9 = MP3_SPLIT(MP3_SUB16(c8, c9), (coefficients[43] + ((short) coefficients[42] << 16)));
            v11 = MP3_SUB16(c10, c11);
            d10 = MP3_ADD16(c10, c11) * 2;
            v2 = MP3_ADD16(v8, v9);
            d12 = MP3_ADD16(c12, c13) * 2;
            v13 = MP3_SPLIT(MP3_SUB16(c12, c13), (coefficients[45] + ((short) coefficients[44] << 16))) - v2 + d12;
            memory[(t0 - 0x40) >> 1] = v13 - d10;
            d14 = v8 -MP3_ADD16(c14, c15) * 4 + d10;
            v15 = MP3_SUB16(c14, c15);
            memory[(t6 + 0x40) >> 1] = v9 - d14;
            v11 = MP3_SPLIT(v11, (coefficients[45] + ((short) coefficients[44] << 16))) - v8 - v9 - v13 + v2;
            memory[(t0 + 0x40) >> 1] = v11;
            memory[(t6 + 0x1c0) >> 1] = -MP3_SPLIT(v15, (coefficients[47] + ((short) coefficients[46] << 16)));
            memory[(t2 + 0x40) >> 1] = v8 - d12;
            memory[(t3 - 0x40) >> 1] = -v8;
            memory[(t5 + 0x40) >> 1] = d14;
            memory[(t2 - 0x40) >> 1] = d10;
            d0 = (((short) Mp3Subtract(*(int *) (samples + 0), *(int *) (samples + 62)) * coefficients[24]) >> 16) * 2;
            d1 = (((short) Mp3Subtract(*(int *) (samples + 2), *(int *) (samples + 60)) * coefficients[25]) >> 16) * 2;
            d2 = (((short) Mp3Subtract(*(int *) (samples + 6), *(int *) (samples + 56)) * coefficients[27]) >> 16) * 2;
            d3 = (((short) Mp3Subtract(*(int *) (samples + 4), *(int *) (samples + 58)) * coefficients[26]) >> 16) * 2;
            d4 = (((short) Mp3Subtract(*(int *) (samples + 14), *(int *) (samples + 48)) * coefficients[31]) >> 16) * 2;
            d5 = (((short) Mp3Subtract(*(int *) (samples + 12), *(int *) (samples + 50)) * coefficients[30]) >> 16) * 2;
            d6 = (((short) Mp3Subtract(*(int *) (samples + 8), *(int *) (samples + 54)) * coefficients[28]) >> 16) * 2;
            d7 = (((short) Mp3Subtract(*(int *) (samples + 10), *(int *) (samples + 52)) * coefficients[29]) >> 16) * 2;
            d8 = (((short) Mp3Subtract(*(int *) (samples + 30), *(int *) (samples + 32)) * coefficients[39]) >> 16);
            d9 = (((short) Mp3Subtract(*(int *) (samples + 28), *(int *) (samples + 34)) * coefficients[38]) >> 16);
            d10 = (((short) Mp3Subtract(*(int *) (samples + 24), *(int *) (samples + 38)) * coefficients[36]) >> 16);
            d11 = (((short) Mp3Subtract(*(int *) (samples + 26), *(int *) (samples + 36)) * coefficients[37]) >> 16);
            d12 = (((short) Mp3Subtract(*(int *) (samples + 16), *(int *) (samples + 46)) * coefficients[32]) >> 16) * 2;
            d13 = (((short) Mp3Subtract(*(int *) (samples + 18), *(int *) (samples + 44)) * coefficients[33]) >> 16) * 2;
            d14 = (((short) Mp3Subtract(*(int *) (samples + 22), *(int *) (samples + 40)) * coefficients[35]) >> 16);
            d15 = (((short) Mp3Subtract(*(int *) (samples + 20), *(int *) (samples + 42)) * coefficients[34]) >> 16) * 2;
            Mp3Butterfly(coefficients);
            v1 = MP3_ACCUMULATE_FRACTION(c0, c1, coefficients[40], coefficients[41]);
            v0 = MP3_ACCUMULATE_UNSIGNED(c0, c1, coefficients[22], coefficients[22]);
            v2 = -MP3_ADD16(c2, c3);
            v3 = MP3_SPLIT(MP3_SUB16(c2, c3), (coefficients[43] + ((short) coefficients[42] << 16)));
            v4 = MP3_ADD16(v0, MP3_ADD16(c4, c5));
            v5 = MP3_SPLIT(MP3_SUB16(c4, c5), (coefficients[43] + ((short) coefficients[42] << 16))) + v1;
            v6 = MP3_ADD16(c6, c7) * 2 - v2 + v0;
            v7 = MP3_SPLIT(MP3_SUB16(c6, c7), (coefficients[45] + ((short) coefficients[44] << 16))) + v0 + v1 + v3;
            memory[(t3 - 0x20) >> 1] = -v0;
            v8 = MP3_ADD16(c8, c9);
            v9 = MP3_SPLIT(MP3_SUB16(c8, c9), (coefficients[43] + ((short) coefficients[42] << 16)));
            v10 = MP3_ADD16(c10, c11) * 2 + v8;
            v11 = MP3_SPLIT(MP3_SUB16(c10, c11), (coefficients[45] + ((short) coefficients[44] << 16))) + v9 + v8;
            v12 = v4 -MP3_ADD16(c12, c13) * 2;
            memory[(t2 + 0x20) >> 1] = v12;
            v13 = MP3_SPLIT(MP3_SUB16(c12, c13), (coefficients[45] + ((short) coefficients[44] << 16))) - v5 - v12;
            v14 = v6 -MP3_ADD16(c14, c15) * 4;
            memory[(t5 + 0x20) >> 1] = v14;
            memory[(t6 + 0x20) >> 1] = v14 + v1;
            v15 = MP3_SPLIT(MP3_SUB16(c14, c15), (coefficients[47] + ((short) coefficients[46] << 16))) - v7;
            memory[(t6 + 0x1e0) >> 1] = v15;
            v9 += v10;
            memory[(t5 + 0x60) >> 1] = v10 - v6;
            memory[(t2 - 0x60) >> 1] = v4 - v2 - v10;
            memory[(t2 - 0x20) >> 1] = v2 - v12;
            memory[(t2 + 0x60) >> 1] = v8 - v4;
            memory[(t3 - 0x60) >> 1] = v0 - v8;
            v5 += v4;
            memory[(t6 + 0x1a0) >> 1] = v7 - v11;
            memory[(t6 + 0x60) >> 1] = v9 - v6 - v1;
            memory[(t0 + 0x60) >> 1] = v11 - v5 - v3;
            memory[(t0 + 0x20) >> 1] = v3 - v13;
            memory[(t0 - 0x20) >> 1] = v13 + v2;
            memory[(t0 - 0x60) >> 1] = v5 - v9 - v2;
            t6 &= 0xffe0;
            offset = 0x80 - t4;
            {
                short *out = output;
                unsigned int address = t6, window = offset;
                for (x = 0; x < 8; ++x) {
                    int sum = 0;
                    for (i = 1; i <= 4; ++i) {
                        sum += MP3_FRACTION(memory[((address + 0x00) >> 1) + (i - 1) + x * 32],
                                            coefficients[((window + 0x00) >> 1) + (i - 1) + x * 32])
                             + MP3_FRACTION(memory[((address + 0x08) >> 1) + (i - 1) + x * 32],
                                            coefficients[((window + 0x08) >> 1) + (i - 1) + x * 32])
                             + MP3_FRACTION(memory[((address + 0x10) >> 1) + (i - 1) + x * 32],
                                            coefficients[((window + 0x10) >> 1) + (i - 1) + x * 32])
                             + MP3_FRACTION(memory[((address + 0x18) >> 1) + (i - 1) + x * 32],
                                            coefficients[((window + 0x18) >> 1) + (i - 1) + x * 32]);
                    }
                    *out++ = sum;
                    sum = 0;
                    for (i = 1; i <= 4; ++i) {
                        sum += MP3_FRACTION(memory[((address + 0x20) >> 1) + (i - 1) + x * 32],
                                            coefficients[((window + 0x40) >> 1) + (i - 1) + x * 32])
                             + MP3_FRACTION(memory[((address + 0x28) >> 1) + (i - 1) + x * 32],
                                            coefficients[((window + 0x48) >> 1) + (i - 1) + x * 32])
                             + MP3_FRACTION(memory[((address + 0x30) >> 1) + (i - 1) + x * 32],
                                            coefficients[((window + 0x50) >> 1) + (i - 1) + x * 32])
                             + MP3_FRACTION(memory[((address + 0x38) >> 1) + (i - 1) + x * 32],
                                            coefficients[((window + 0x58) >> 1) + (i - 1) + x * 32]);
                    }
                    *out++ = sum;
                }
            }
        }
    }
}

#undef MP3_ADD16
#undef MP3_SUB16
#undef MP3_SPLIT
#undef MP3_FRACTION
#undef MP3_ACCUMULATE_UNSIGNED
#undef MP3_ACCUMULATE_FRACTION

/* HLE processing, envelope mixing and predictor helpers.
 * Original translation-unit boundary inferred from native code and AMP data order. */
#include "transform.h"
#include "AudioADPCMBlock.h"
/* Audio sample helpers; source filename inferred from the native code group. */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "AudioProcessing.h"

extern FILE *HLELogFile;
extern void LoadAudioBuffer(unsigned int offset, unsigned int address, int bytes,
                            AudioContext *context);
extern void SaveAudioBuffer(unsigned int offset, unsigned int address, int bytes,
                            AudioContext *context);

/* The mantissa lookup uses the low exponent bit as its upper index bit.
 * Native initialization writes the upper half for [1,2), then the lower half
 * for [2,4). The contiguous private table precedes the envelope state. */
static unsigned int AudioSqrtMantissa[65536];
int AudioEnvelopeWet = 0;
int AudioEnvelopeDry = 0;

void ReadAudioSamples(short *destination, unsigned int address, int bytes)
{
    int offset;
    for (offset = 0; offset < bytes; offset += 4) {
        int pair = *(int *)(RDRAM + ((address + offset) & 0xffffff));
        *destination++ = pair >> 16;
        *destination++ = pair;
    }
}

void InitializeAudioSqrt(void)
{
    unsigned int index;
    union {
        unsigned int bits;
        float value;
    } sample;
    for (index = 0; index <= 0x7fff; ++index) {
        sample.bits = (index | 0x3f8000) << 8;
        sample.value = (float)sqrt(sample.value);
        AudioSqrtMantissa[index + 0x8000] = sample.bits & 0x7fffff;
        sample.bits = (index | 0x400000) << 8;
        sample.value = (float)sqrt(sample.value);
        AudioSqrtMantissa[index] = sample.bits & 0x7fffff;
    }
}

void LoadAudioEnvelope(unsigned int address, AudioContext *context)
{
    context->targetLeft  = *(int *)(RDRAM + ((address +  0) & 0xffffff));
    context->targetRight = *(int *)(RDRAM + ((address +  4) & 0xffffff));
    context->volumeLeft  = *(int *)(RDRAM + ((address +  8) & 0xffffff));
    context->volumeRight = *(int *)(RDRAM + ((address + 12) & 0xffffff));
    context->rampLeft    = *(int *)(RDRAM + ((address + 16) & 0xffffff));
    context->rampRight   = *(int *)(RDRAM + ((address + 20) & 0xffffff));
    context->rateLeft    = *(int *)(RDRAM + ((address + 24) & 0xffffff));
    context->rateRight   = *(int *)(RDRAM + ((address + 28) & 0xffffff));
    context->wet = AudioEnvelopeWet;
    context->dry = AudioEnvelopeDry;
}

void SaveAudioEnvelope(unsigned int address, AudioContext *context)
{
    *(int *)(RDRAM + ((address +  0) & 0xffffff)) = context->targetLeft;
    *(int *)(RDRAM + ((address +  4) & 0xffffff)) = context->targetRight;
    *(int *)(RDRAM + ((address +  8) & 0xffffff)) = context->volumeLeft;
    *(int *)(RDRAM + ((address + 12) & 0xffffff)) = context->volumeRight;
    *(int *)(RDRAM + ((address + 16) & 0xffffff)) = context->rampLeft;
    *(int *)(RDRAM + ((address + 20) & 0xffffff)) = context->rampRight;
    *(int *)(RDRAM + ((address + 24) & 0xffffff)) = context->rateLeft;
    *(int *)(RDRAM + ((address + 28) & 0xffffff)) = context->rateRight;
    AudioEnvelopeWet = context->wet;
    AudioEnvelopeDry = context->dry;
}

void ResetAudioEnvelope(AudioContext *context)
{
    context->volumeLeft = 0;
    context->volumeRight = 0;
    context->rampLeft = 0;
    context->rampRight = 0;
    context->rateLeft = 0;
    context->rateRight = 0;
    context->targetLeft = 0;
    context->targetRight = 0;
    context->wet = 0;
    context->dry = 0;
}

/* SSE availability checks used by the envelope mixer. */
#include <windows.h>

int AudioCPUHasSSE(void)
{
    int supported = 0;
    __asm {
        mov eax, 1
        cpuid
        shr edx, 26
        jnc no_sse
        mov supported, 1
    no_sse:
    }
    return supported;
}

int AudioOSHasSSE(void)
{
    float dividend[400];
    float divisor[400];
    int supported = 1;
    __try {
        __asm {
            movups xmm1, dividend
            divps xmm1, divisor
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        supported = 0;
    }
    return supported;
}

/* The SIMD mixer calls this small assembly helper with its value in ECX. */
__declspec(naked) void AudioClampECX(void)
{
    __asm {
        cmp ecx, 32767
        jle below_maximum
        mov ecx, 32767
        jmp clipped
    below_maximum:
        cmp ecx, -32767
        jge clipped
        mov ecx, -32767
    clipped:
        ret
    }
}

/* Native audio envelope mixers. Private names and this file boundary are inferred.
 * The integer mixer family derives from UltraHLE 1.0.0 src/SLIST.C:
 * Copyright (C) 1998 Epsilon & RealityMan.
 * See analysis/audio-envelope-ultrahle-provenance.json for archive identity,
 * original notices, and the target-specific adaptations. */
#include <math.h>
#include <stdlib.h>
#include "AudioEnvelope.h"

extern void AudioError(const char *text, HRESULT result);
__inline int ClampAudioSample(int sample);

#define ALIGN16 __declspec(align(16))

/* The envelope mixer clips symmetrically at +/-32767. */
#define CLAMP_SAMPLE(tag) \
    __asm cmp ecx, 32767 \
    __asm jle tag##_below_max \
    __asm mov ecx, 32767 \
    __asm jmp tag##_clipped \
    __asm tag##_below_max: \
    __asm cmp ecx, -32767 \
    __asm jge tag##_clipped \
    __asm mov ecx, -32767 \
    __asm tag##_clipped:

/* ECX is the signed sum. The original mixer saturates to +/-32767. */
#define CLIP_MIXED_SAMPLE(tag) \
    __asm cmp ecx, 32767 \
    __asm jle tag##_lower \
    __asm mov ecx, 32767 \
    __asm jmp tag##_done \
    __asm tag##_lower: \
    __asm cmp ecx, -32767 \
    __asm jge tag##_done \
    __asm mov ecx, -32767 \
    __asm tag##_done:

/* ESI addresses the input, EBX/EDI hold the two scaled channel samples.
 * Wet output addresses are 0x7c0/0x930; dry output is 0x4e0/0x650. */
#define MIX_ABI3_OUTPUTS(tag) \
    __asm mov eax, dry \
    __asm movsx ecx, word ptr [esi+0x4e0] \
    __asm imul eax, ebx \
    __asm sar eax, 15 \
    __asm add ecx, eax \
    CLIP_MIXED_SAMPLE(tag##_dryLeft) \
    __asm mov word ptr [esi+0x4e0], cx \
    __asm mov edx, wet \
    __asm mov ecx, ebx \
    __asm imul ecx, edx \
    __asm movsx eax, word ptr [esi+0x7c0] \
    __asm sar ecx, 15 \
    __asm add ecx, eax \
    CLIP_MIXED_SAMPLE(tag##_wetLeft) \
    __asm mov word ptr [esi+0x7c0], cx \
    __asm mov ecx, edi \
    __asm imul ecx, edx \
    __asm movsx eax, word ptr [esi+0x930] \
    __asm sar ecx, 15 \
    __asm add ecx, eax \
    CLIP_MIXED_SAMPLE(tag##_wetRight) \
    __asm mov word ptr [esi+0x930], cx \
    __asm mov eax, dry \
    __asm movsx ecx, word ptr [esi+0x650] \
    __asm imul eax, edi \
    __asm sar eax, 15 \
    __asm add ecx, eax \
    CLIP_MIXED_SAMPLE(tag##_dryRight) \
    __asm mov word ptr [esi+0x650], cx \
    __asm add esi, 2


#pragma optimize("", off)
void EnvMixerSSE(AudioCommand *command, AudioContext *context)
{
    ALIGN16 int leftMask[4] = {-1, 0, -1, 0};
    ALIGN16 int rightMask[4] = {0, -1, 0, -1};
    ALIGN16 float scale[4] = {1.0f/32768, 1.0f/32768, 1.0f/32768, 1.0f/32768};
    int bothDown = 0, bothUp = 0, leftUp = 0, rightUp = 0;
    unsigned short *state;
    int rateLeft, rateRight;
    ALIGN16 float volumeLeft, volumeRight, targetLeft, targetRight;
    int dry, wet;
    int flags, input, output, aux1, aux0, aux2;
    ALIGN16 float stepLeft, stepRight;
    short *buffer;
    int count;
    short *wetLeft, *wetRight, *dryLeft, *dryRight, *source;
    ALIGN16 float pair[4];
    int rightSample;

    if (!((command->words[0] >> 16 & 0xff) & 1)) {
        state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
        rateLeft = (state[6] << 16) + state[5];
        rateRight = (state[9] << 16) + state[8];
        volumeLeft = ((state[0] << 16) + state[1]) / 65536.0f;
        volumeRight = ((state[2] << 16) + state[3]) / 65536.0f;
        targetLeft = state[4];
        targetRight = state[7];
        dry = state[10];
        wet = state[11];
    } else {
        rateLeft = context->rateLeft16;
        rateRight = context->rateRight16;
        targetLeft = context->targetLeft16;
        targetRight = context->targetRight16;
        volumeLeft = context->volumeLeft16;
        volumeRight = context->volumeRight16;
        wet = context->wet;
        dry = context->dry;
    }
    flags = command->words[0] >> 16 & 0xff;
    input = context->input >> 1;
    output = context->outputResample >> 1;
    aux1 = context->auxResample1 >> 1;
    aux0 = context->auxResample0 >> 1;
    aux2 = context->auxResample2 >> 1;
    stepLeft = rateLeft / 65536.0;
    stepRight = rateRight / 65536.0;
    buffer = context->buffer;
    count = context->count >> 1;
    if (!output && !aux0 && volumeLeft == 0 && stepLeft == 0 && volumeRight == 0)
        return;
    stepLeft = sqrt(sqrt(sqrt(stepLeft)));
    stepRight = sqrt(sqrt(sqrt(stepRight)));
    if (stepLeft > 1.0 && stepRight > 1.0)
        bothUp = 1;
    else if (stepLeft <= 1.0 && stepRight <= 1.0)
        bothDown = 1;
    else if (stepLeft > 1.0 && stepRight <= 1.0)
        leftUp = 1;
    else if (stepLeft <= 1.0 && stepRight > 1.0)
        rightUp = 1;
    wetLeft = buffer + aux1;
    wetRight = buffer + aux2;
    dryLeft = buffer + output;
    dryRight = buffer + aux0;
    source = buffer + input;
    pair[0] = volumeLeft;
    pair[1] = volumeRight;
    pair[2] = volumeLeft;
    pair[3] = volumeRight;
    __asm movaps xmm5, pair
    pair[0] = stepLeft;
    pair[1] = stepRight;
    pair[2] = stepLeft;
    pair[3] = stepRight;
    __asm movaps xmm2, pair
    pair[0] = targetLeft;
    pair[1] = targetRight;
    pair[2] = targetLeft;
    pair[3] = targetRight;
    __asm {
        movaps xmm1, pair
        movaps xmm3, scale
        mov esi, source
        emms
    }
    if (!count) goto done;
    __asm push ebx
    if (bothUp) {
        __asm {
        bothUp_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            add esi, 2
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothUp_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothUp_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothUp_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothUp_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            mulps xmm5, xmm2
            movaps xmm6, xmm1
            movaps xmm7, xmm1
            cmpleps xmm6, xmm5
            andps xmm7, xmm6
            andnps xmm6, xmm5
            orps xmm7, xmm6
            movaps xmm5, xmm7
            dec count
            jg bothUp_loop
        }
    }
    else if (bothDown) {
        __asm {
        bothDown_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            add esi, 2
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothDown_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothDown_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothDown_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothDown_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            mulps xmm5, xmm2
            movaps xmm7, xmm5
            movaps xmm6, xmm5
            cmpleps xmm7, xmm1
            movaps xmm0, xmm7
            andnps xmm7, xmm6
            andps xmm0, xmm1
            orps xmm7, xmm0
            movaps xmm5, xmm7
            dec count
            jg bothDown_loop
        }
    }
    else if (leftUp) {
        __asm movaps xmm4, rightMask
        __asm {
        leftUp_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            add esi, 2
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(leftUp_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(leftUp_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(leftUp_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(leftUp_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            mulps xmm5, xmm2
            movaps xmm7, xmm1
            movaps xmm6, xmm1
            cmpleps xmm6, xmm5
            xorps xmm6, xmm4
            andps xmm7, xmm6
            andnps xmm6, xmm5
            orps xmm7, xmm6
            movaps xmm5, xmm7
            dec count
            jg leftUp_loop
        }
    }
    else if (rightUp) {
        __asm movaps xmm4, leftMask
        __asm {
        rightUp_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            add esi, 2
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(rightUp_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(rightUp_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(rightUp_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(rightUp_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            mulps xmm5, xmm2
            movaps xmm7, xmm1
            movaps xmm6, xmm1
            cmpleps xmm6, xmm5
            xorps xmm6, xmm4
            andps xmm7, xmm6
            andnps xmm6, xmm5
            orps xmm7, xmm6
            movaps xmm5, xmm7
            dec count
            jg rightUp_loop
        }
    }
done:
    __asm {
        pop ebx
        movaps pair, xmm5
        emms
    }
    volumeLeft = pair[0];
    volumeRight = pair[1];
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = volumeLeft;
    state[1] = (volumeLeft - state[0]) * 65536.0f;
    state[2] = volumeRight;
    state[3] = (volumeRight - state[2]) * 65536.0f;
    state[6] = rateLeft >> 16;
    state[5] = rateLeft & 0xffff;
    state[9] = rateRight >> 16;
    state[8] = rateRight & 0xffff;
    state[4] = targetLeft;
    state[7] = targetRight;
    state[10] = dry;
    state[11] = wet;
}

void EnvMixer3DNow(AudioCommand *command, AudioContext *context)
{
    ALIGN16 int leftMask[2] = {-1, 0};
    ALIGN16 int rightMask[2] = {0, -1};
    ALIGN16 float scale[2] = {1.0f/32768, 1.0f/32768};
    int bothDown = 0, bothUp = 0, leftUp = 0, rightUp = 0;
    unsigned short *state;
    int rateLeft, rateRight;
    ALIGN16 float volumeLeft, volumeRight, targetLeft, targetRight;
    int dry, wet;
    int flags, input, output, aux1, aux0, aux2;
    ALIGN16 float stepLeft, stepRight;
    short *buffer;
    int count;
    short *wetLeft, *wetRight, *dryLeft, *dryRight, *source;
    ALIGN16 float pair[2];
    int rightSample;

    if (!((command->words[0] >> 16 & 0xff) & 1)) {
        state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
        rateLeft = (state[6] << 16) + state[5];
        rateRight = (state[9] << 16) + state[8];
        volumeLeft = ((state[0] << 16) + state[1]) / 65536.0f;
        volumeRight = ((state[2] << 16) + state[3]) / 65536.0f;
        targetLeft = state[4];
        targetRight = state[7];
        dry = state[10];
        wet = state[11];
    } else {
        rateLeft = context->rateLeft16;
        rateRight = context->rateRight16;
        targetLeft = context->targetLeft16;
        targetRight = context->targetRight16;
        volumeLeft = context->volumeLeft16;
        volumeRight = context->volumeRight16;
        wet = context->wet;
        dry = context->dry;
    }
    flags = command->words[0] >> 16 & 0xff;
    input = context->input >> 1;
    output = context->outputResample >> 1;
    aux1 = context->auxResample1 >> 1;
    aux0 = context->auxResample0 >> 1;
    aux2 = context->auxResample2 >> 1;
    stepLeft = rateLeft / 65536.0;
    stepRight = rateRight / 65536.0;
    buffer = context->buffer;
    count = context->count >> 1;
    if (!output && !aux0 && volumeLeft == 0 && stepLeft == 0 && volumeRight == 0)
        return;
    stepLeft = sqrt(sqrt(sqrt(stepLeft)));
    stepRight = sqrt(sqrt(sqrt(stepRight)));
    if (stepLeft > 1.0 && stepRight > 1.0)
        bothUp = 1;
    else if (stepLeft <= 1.0 && stepRight <= 1.0)
        bothDown = 1;
    else if (stepLeft > 1.0 && stepRight <= 1.0)
        leftUp = 1;
    else if (stepLeft <= 1.0 && stepRight > 1.0)
        rightUp = 1;
    wetLeft = buffer + aux1;
    wetRight = buffer + aux2;
    dryLeft = buffer + output;
    dryRight = buffer + aux0;
    source = buffer + input;
    pair[0] = volumeLeft;
    pair[1] = volumeRight;
    __asm movq mm5, pair
    pair[0] = stepLeft;
    pair[1] = stepRight;
    __asm movq mm2, pair
    pair[0] = targetLeft;
    pair[1] = targetRight;
    __asm {
        movq mm1, pair
        movq mm3, scale
        mov esi, source
        femms
    }
    if (!count) goto done;
    __asm push ebx
    if (bothUp) {
        __asm {
        bothUp_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            add esi, 2
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothUp_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothUp_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothUp_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothUp_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            pfmul mm5, mm2
            movq mm6, mm5
            movq mm7, mm1
            pfcmpgt mm6, mm1
            pand mm7, mm6
            pandn mm6, mm5
            por mm7, mm6
            movq mm5, mm7
            dec count
            jg bothUp_loop
        }
    }
    else if (bothDown) {
        __asm {
        bothDown_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            add esi, 2
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothDown_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothDown_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(bothDown_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(bothDown_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            pfmul mm5, mm2
            movq mm7, mm1
            movq mm6, mm5
            pfcmpgt mm7, mm5
            movq mm0, mm7
            pandn mm7, mm6
            pand mm0, mm1
            por mm7, mm0
            movq mm5, mm7
            dec count
            jg bothDown_loop
        }
    }
    else if (leftUp) {
        __asm movq mm4, rightMask
        __asm {
        leftUp_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            add esi, 2
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(leftUp_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(leftUp_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(leftUp_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(leftUp_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            pfmul mm5, mm2
            movq mm7, mm1
            movq mm6, mm5
            pfcmpgt mm6, mm1
            pxor mm6, mm4
            pand mm7, mm6
            pandn mm6, mm5
            por mm7, mm6
            movq mm5, mm7
            dec count
            jg leftUp_loop
        }
    }
    else if (rightUp) {
        __asm movq mm4, leftMask
        __asm {
        rightUp_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            add esi, 2
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd rightSample, mm7
            mov edi, dryLeft
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, ebx
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(rightUp_dl)
        __asm {
            add dryLeft, 2
            mov [edi], cx
        }
        if (flags & 8) {
            __asm {
                mov edx, wet
                mov ecx, ebx
                mov edi, wetLeft
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(rightUp_wl)
            __asm {
                add wetLeft, 2
                mov [edi], cx
                mov ecx, rightSample
                mov edi, wetRight
                imul ecx, edx
                movsx eax, word ptr [edi]
                sar ecx, 15
                add ecx, eax
            }
            CLAMP_SAMPLE(rightUp_wr)
            __asm {
                add wetRight, 2
                mov [edi], cx
            }
        }
        __asm {
            mov edi, dryRight
            mov eax, dry
            movsx ecx, word ptr [edi]
            imul eax, rightSample
            sar eax, 15
            add ecx, eax
        }
        CLAMP_SAMPLE(rightUp_dr)
        __asm {
            add dryRight, 2
            mov [edi], cx
            pfmul mm5, mm2
            movq mm7, mm1
            movq mm6, mm5
            pfcmpgt mm6, mm1
            pxor mm6, mm4
            pand mm7, mm6
            pandn mm6, mm5
            por mm7, mm6
            movq mm5, mm7
            dec count
            jg rightUp_loop
        }
    }
done:
    __asm {
        pop ebx
        movq pair, mm5
        femms
    }
    volumeLeft = pair[0];
    volumeRight = pair[1];
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = volumeLeft;
    state[1] = (volumeLeft - state[0]) * 65536.0f;
    state[2] = volumeRight;
    state[3] = (volumeRight - state[2]) * 65536.0f;
    state[6] = rateLeft >> 16;
    state[5] = rateLeft & 0xffff;
    state[9] = rateRight >> 16;
    state[8] = rateRight & 0xffff;
    state[4] = targetLeft;
    state[7] = targetRight;
    state[10] = dry;
    state[11] = wet;
}
#pragma optimize("", on)

void EnvMixerScalar(AudioCommand *command, AudioContext *context)
{
    int rateLeft, rateRight, targetLeft, targetRight, dry, wet;
    float volumeLeft, volumeRight, stepLeft, stepRight;
    int input, outputLeft, outputRight, wetLeft, wetRight, count, flags;
    short *buffer;
    unsigned short *state;
    int sample, i;

    if ((command->words[0] >> 16) & 0x10)
        AudioError("A_MIX Set in envmixer?! heavens no. If you are a user, you shouldnt see this. report to lac@nemu.com you got this error.", 0);
    if (!((command->words[0] >> 16) & 1)) {
        state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
        rateLeft = (state[6] << 16) + state[5];
        rateRight = (state[9] << 16) + state[8];
        volumeLeft = ((state[0] << 16) + state[1]) / 65536.0f;
        volumeRight = ((state[2] << 16) + state[3]) / 65536.0f;
        targetLeft = state[4];
        targetRight = state[7];
        dry = state[10];
        wet = state[11];
    } else {
        rateLeft = context->rateLeft16;
        rateRight = context->rateRight16;
        targetLeft = context->targetLeft16;
        targetRight = context->targetRight16;
        volumeLeft = context->volumeLeft16;
        volumeRight = context->volumeRight16;
        wet = context->wet;
        dry = context->dry;
    }
    stepLeft = (float)(rateLeft / 65536.0);
    stepLeft = (float)sqrt(sqrt(sqrt(stepLeft)));
    stepRight = (float)(rateRight / 65536.0);
    stepRight = (float)sqrt(sqrt(sqrt(stepRight)));
    flags = (command->words[0] >> 16) & 0xff;
    input = context->input >> 1;
    outputLeft = context->outputResample >> 1;
    outputRight = context->auxResample0 >> 1;
    wetLeft = context->auxResample1 >> 1;
    wetRight = context->auxResample2 >> 1;
    buffer = context->buffer;
    count = context->count >> 1;
    if (!outputLeft && !outputRight && !volumeLeft && !stepLeft && !volumeRight)
        return;
    for (i = count + 1; i > 1; --i) {
        sample = (int)(buffer[input] * volumeLeft / 32768.0f);
        {
            int mixed = buffer[outputLeft] + sample * dry / 32768;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            buffer[outputLeft] = mixed;
        }
        ++outputLeft;
        if ((flags & 8) && wetLeft) {
            {
                int mixed = buffer[wetLeft] + sample * wet / 32768;
                if (mixed > 32767) mixed = 32767;
                else if (mixed < -32767) mixed = -32767;
                buffer[wetLeft] = mixed;
            }
            ++wetLeft;
        }
        sample = (int)(buffer[input++] * volumeRight / 32768.0f);
        {
            int mixed = buffer[outputRight] + sample * dry / 32768;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            buffer[outputRight] = mixed;
        }
        ++outputRight;
        if ((flags & 8) && wetRight) {
            {
                int mixed = buffer[wetRight] + sample * wet / 32768;
                if (mixed > 32767) mixed = 32767;
                else if (mixed < -32767) mixed = -32767;
                buffer[wetRight] = mixed;
            }
            ++wetRight;
        }
        volumeLeft *= stepLeft;
        if (stepLeft > 1.0) {
            if (volumeLeft > targetLeft) volumeLeft = (float)targetLeft;
        } else {
            if (volumeLeft < targetLeft) volumeLeft = (float)targetLeft;
        }
        volumeRight *= stepRight;
        if (stepRight > 1.0) {
            if (volumeRight > targetRight) volumeRight = (float)targetRight;
        } else {
            if (volumeRight < targetRight) volumeRight = (float)targetRight;
        }
    }
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = (unsigned short)volumeLeft;
    state[1] = (unsigned short)((volumeLeft - state[0]) * 65536.0f);
    state[2] = (unsigned short)volumeRight;
    state[3] = (unsigned short)((volumeRight - state[2]) * 65536.0f);
    state[6] = rateLeft >> 16;
    state[5] = rateLeft;
    state[9] = rateRight >> 16;
    state[8] = rateRight;
    state[4] = targetLeft;
    state[7] = targetRight;
    state[10] = dry;
    state[11] = wet;
}

void EnvMixerBlockScalar(AudioCommand *command, AudioContext *context)
{
    int rateLeft, rateRight, targetLeft, targetRight, dry, wet;
    float volumeLeft, volumeRight, stepLeft, stepRight;
    int input, outputLeft, outputRight, wetLeft, wetRight, count, flags;
    short *buffer;
    unsigned short *state;
    int sample;

    if ((command->words[0] >> 16) & 0x10)
        AudioError("A_MIX Set in envmixer?! heavens no. If you are a user, you shouldnt see this. report to lac@nemu.com you got this error.", 0);
    if (!((command->words[0] >> 16) & 1)) {
        state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
        rateLeft = (state[6] << 16) + state[5];
        rateRight = (state[9] << 16) + state[8];
        volumeLeft = ((state[0] << 16) + state[1]) / 65536.0f;
        volumeRight = ((state[2] << 16) + state[3]) / 65536.0f;
        targetLeft = state[4];
        targetRight = state[7];
        dry = state[10];
        wet = state[11];
    } else {
        rateLeft = context->rateLeft16;
        rateRight = context->rateRight16;
        targetLeft = context->targetLeft16;
        targetRight = context->targetRight16;
        volumeLeft = context->volumeLeft16;
        volumeRight = context->volumeRight16;
        wet = context->wet;
        dry = context->dry;
    }
    stepLeft = (float)(rateLeft / 65536.0);
    stepRight = (float)(rateRight / 65536.0);
    flags = (command->words[0] >> 16) & 0xff;
    input = context->input >> 1;
    outputLeft = context->outputResample >> 1;
    outputRight = context->auxResample0 >> 1;
    wetLeft = context->auxResample1 >> 1;
    wetRight = context->auxResample2 >> 1;
    buffer = context->buffer;
    count = context->count >> 1;
    if (!outputLeft && !outputRight && !volumeLeft && !stepLeft && !volumeRight)
        return;
    while (count > 0) {
        sample = (int)(buffer[input] * volumeLeft / 32768.0f);
        {
            int mixed = buffer[outputLeft] + sample;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            buffer[outputLeft] = mixed;
        }
        ++outputLeft;
        if ((flags & 8) && wetLeft) {
            {
                int mixed = buffer[wetLeft] + sample * wet / 32768;
                if (mixed > 32767) mixed = 32767;
                else if (mixed < -32767) mixed = -32767;
                buffer[wetLeft] = mixed;
            }
            ++wetLeft;
        }
        sample = (int)(buffer[input++] * volumeRight / 32768.0f);
        {
            int mixed = buffer[outputRight] + sample;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            buffer[outputRight] = mixed;
        }
        ++outputRight;
        if ((flags & 8) && wetRight) {
            {
                int mixed = buffer[wetRight] + sample * wet / 32768;
                if (mixed > 32767) mixed = 32767;
                else if (mixed < -32767) mixed = -32767;
                buffer[wetRight] = mixed;
            }
            ++wetRight;
        }
        if (!(count % 8) && (stepLeft != 0 || stepRight != 0)) {
            volumeLeft *= stepLeft;
            if (stepLeft > 1.0) {
                if (volumeLeft > targetLeft) volumeLeft = (float)targetLeft;
            } else {
                if (volumeLeft < targetLeft) volumeLeft = (float)targetLeft;
            }
            volumeRight *= stepRight;
            if (stepRight > 1.0) {
                if (volumeRight > targetRight) volumeRight = (float)targetRight;
            } else {
                if (volumeRight < targetRight) volumeRight = (float)targetRight;
            }
        }
        --count;
    }
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = (unsigned short)volumeLeft;
    state[1] = (unsigned short)((volumeLeft - state[0]) * 65536.0f);
    state[2] = (unsigned short)volumeRight;
    state[3] = (unsigned short)((volumeRight - state[2]) * 65536.0f);
    state[6] = rateLeft >> 16;
    state[5] = rateLeft;
    state[9] = rateRight >> 16;
    state[8] = rateRight;
    state[4] = targetLeft;
    state[7] = targetRight;
    state[10] = dry;
    state[11] = wet;
}

void EnvMixerABI3Scalar(AudioCommand *command, AudioContext *context)
{
    int rateLeft, rateRight;
    int targetLeft, targetRight;
    float volumeLeft, volumeRight;
    int dry, wet;
    float stepLeft, stepRight;
    unsigned short *state;
    int input, outputLeft, wetLeft, outputRight, wetRight;
    int i;
    short *buffer;
    int dryMask, wetMask;
    if (command->bytes[2] & 0x10)
        AudioError("A_MIX Set in envmixer?! heavens no. If you are a user, you shouldnt see this. report to lac@nemu.com you got this error.", 0);
    if (!(command->bytes[2] & 1)) {
        state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
        rateLeft = (state[6] << 16) + state[5];
        rateRight = (state[9] << 16) + state[8];
        volumeLeft = (float)((state[0] << 16) + state[1]) / 65536.f;
        volumeRight = (float)((state[2] << 16) + state[3]) / 65536.f;
        targetLeft = state[4];
        targetRight = state[7];
        dry = state[10];
        wet = state[11];
    } else {
        rateLeft = context->rateLeft16;
        rateRight = context->rateRight16;
        targetLeft = context->targetLeft16;
        targetRight = context->targetRight16;
        volumeLeft = (float)context->volumeLeft16;
        volumeRight = (float)context->volumeRight16;
        wet = context->wet;
        dry = context->dry;
    }
    stepLeft = (float)(rateLeft / 65536.0);
    stepLeft /= 8.f;
    stepRight = (float)(rateRight / 65536.0);
    stepRight /= 8.f;
    buffer = context->buffer;
    input = 0;
    outputLeft = 0x4e0 / 2;
    outputRight = 0x650 / 2;
    wetLeft = 0x7c0 / 2;
    wetRight = 0x930 / 2;
    dryMask = -(dry & 1);
    wetMask = -(wet & 1);
    /* Preserve the two input-polarity stages used by this ABI. */
    wetMask ^= dryMask;
    for (i = 0; i < 184; ++i) {
        int sampleLeft, sampleRight;
        short raw = buffer[input++];
        raw ^= dryMask;
        raw ^= wetMask;
        sampleLeft = (int)(raw * volumeLeft / 32768.f);
        {
            int mixed = buffer[outputLeft] + sampleLeft * dry / 32768;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            buffer[outputLeft] = mixed;
        }
        ++outputLeft;
        {
            int mixed = buffer[wetLeft] + sampleLeft * wet / 32768;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            buffer[wetLeft] = mixed;
        }
        ++wetLeft;
        sampleRight = (int)(raw * volumeRight / 32768.f);
        {
            int mixed = buffer[outputRight] + sampleRight * dry / 32768;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            buffer[outputRight] = mixed;
        }
        ++outputRight;
        {
            int mixed = buffer[wetRight] + sampleRight * wet / 32768;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            buffer[wetRight] = mixed;
        }
        ++wetRight;
        volumeLeft += stepLeft;
        volumeRight += stepRight;
        if (stepLeft > 0.f) {
            if (volumeLeft > targetLeft) volumeLeft = (float)targetLeft;
        } else {
            if (volumeLeft < targetLeft) volumeLeft = (float)targetLeft;
        }
        if (stepRight > 0.f) {
            if (volumeRight > targetRight) volumeRight = (float)targetRight;
        } else {
            if (volumeRight < targetRight) volumeRight = (float)targetRight;
        }
    }
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = (unsigned short)volumeLeft;
    state[1] = (unsigned short)((volumeLeft - state[0]) * 65536.f);
    state[2] = (unsigned short)volumeRight;
    state[3] = (unsigned short)((volumeRight - state[2]) * 65536.f);
    state[6] = (unsigned short)(rateLeft >> 16);
    state[5] = (unsigned short)rateLeft;
    state[9] = (unsigned short)(rateRight >> 16);
    state[8] = (unsigned short)rateRight;
    state[4] = (unsigned short)targetLeft;
    state[7] = (unsigned short)targetRight;
    state[10] = (unsigned short)dry;
    state[11] = (unsigned short)wet;
}

#pragma optimize("", off)
void EnvMixerABI3_3DNow(AudioCommand *command, AudioContext *context)
{
    __declspec(align(16)) unsigned int leftMask[2] = {0xffffffff, 0};
    __declspec(align(16)) unsigned int rightMask[2] = {0, 0xffffffff};
    __declspec(align(16)) float scale[2] = {1.0f / 32768.0f, 1.0f / 32768.0f};
    int bothDown = 0, bothUp = 0, leftUp = 0, rightUp = 0;
    unsigned short *state;
    int rateLeft, rateRight;
    __declspec(align(16)) float volumeLeft, volumeRight, targetLeft, targetRight;
    int dry, wet, flags, input, outputLeft, auxLeft, outputRight, auxRight, count;
    __declspec(align(16)) float stepLeft, stepRight;
    short *buffer, *wetLeft, *wetRight, *dryLeft, *dryRight, *samples;
    __declspec(align(16)) float packed[2];

    if (!(((command->words[0] >> 16) & 0xff) & 1)) {
        state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
        rateLeft = (state[6] << 16) + state[5];
        rateRight = (state[9] << 16) + state[8];
        volumeLeft = ((state[0] << 16) + state[1]) / 65536.0f;
        volumeRight = ((state[2] << 16) + state[3]) / 65536.0f;
        targetLeft = state[4];
        targetRight = state[7];
        dry = state[10];
        wet = state[11];
    } else {
        rateLeft = context->rateLeft16;
        rateRight = context->rateRight16;
        targetLeft = context->targetLeft16;
        targetRight = context->targetRight16;
        volumeLeft = context->volumeLeft16;
        volumeRight = context->volumeRight16;
        wet = context->wet;
        dry = context->dry;
    }
    flags = (command->words[0] >> 16) & 0xff;
    input = 0;
    outputLeft = 0x270;
    auxLeft = 0x3e0;
    outputRight = 0x328;
    auxRight = 0x498;
    count = 0xb8;
    stepLeft = (float)(rateLeft / 65536.0);
    stepRight = (float)(rateRight / 65536.0);
    buffer = context->buffer;
    count = context->count >> 1;
    if (outputLeft == 0 && outputRight == 0 && volumeLeft == 0 && stepLeft == 0 && volumeRight == 0)
        return;
    stepLeft /= 8;
    stepRight /= 8;
    if (stepLeft > 0 && stepRight > 0)
        bothUp = 1;
    else if (stepLeft <= 0 && stepRight <= 0)
        bothDown = 1;
    else if (stepLeft > 0 && stepRight <= 0)
        leftUp = 1;
    else if (stepLeft <= 0 && stepRight > 0)
        rightUp = 1;
    wetLeft = buffer + auxLeft;
    wetRight = buffer + auxRight;
    dryLeft = buffer + outputLeft;
    dryRight = buffer + outputRight;
    samples = buffer + input;
    packed[0] = volumeLeft;
    packed[1] = volumeRight;
    __asm movq mm5, packed
    packed[0] = stepLeft;
    packed[1] = stepRight;
    __asm movq mm2, packed
    packed[0] = targetLeft;
    packed[1] = targetRight;
    __asm movq mm1, packed
    __asm movq mm3, scale
    __asm mov esi, samples
    __asm femms
    /* Preserve the original zero-count branch: it reaches the EBX restore
     * before the matching save, as in both native SIMD implementations. */
    if (!count)
        __asm jmp finished
    __asm push ebx
    if (bothUp) {
        __asm {
up_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(up)
        __asm {
            pfadd mm5, mm2
            movq mm6, mm5
            movq mm7, mm1
            pfcmpgt mm6, mm1
            pand mm7, mm6
            pandn mm6, mm5
            por mm7, mm6
            movq mm5, mm7
            dec count
            jg up_loop
        }
    }
    else if (bothDown) {
        __asm {
down_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(down)
        __asm {
            pfadd mm5, mm2
            movq mm7, mm1
            movq mm6, mm5
            pfcmpgt mm7, mm5
            movq mm0, mm7
            pandn mm7, mm6
            pand mm0, mm1
            por mm7, mm0
            movq mm5, mm7
            dec count
            jg down_loop
        }
    }
    else if (leftUp) {
        __asm movq mm4, rightMask
        __asm {
left_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(left)
        __asm {
            pfadd mm5, mm2
            movq mm7, mm1
            movq mm6, mm5
            pfcmpgt mm6, mm1
            pxor mm6, mm4
            pand mm7, mm6
            pandn mm6, mm5
            por mm7, mm6
            movq mm5, mm7
            dec count
            jg left_loop
        }
    }
    else if (rightUp) {
        __asm movq mm4, leftMask
        __asm {
right_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movq mm0, mm3
            psrad mm6, 16
            pfmul mm0, mm5
            pi2fd mm7, mm6
            pfmul mm0, mm7
            pf2id mm7, mm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(right)
        __asm {
            pfadd mm5, mm2
            movq mm7, mm1
            movq mm6, mm5
            pfcmpgt mm6, mm1
            pxor mm6, mm4
            pand mm7, mm6
            pandn mm6, mm5
            por mm7, mm6
            movq mm5, mm7
            dec count
            jg right_loop
        }
    }
    __asm {
finished:
        pop ebx
        movq packed, mm5
        femms
    }
    volumeLeft = packed[0];
    volumeRight = packed[1];
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = (unsigned short)volumeLeft;
    state[1] = (unsigned short)((volumeLeft - state[0]) * 65536.0f);
    state[2] = (unsigned short)volumeRight;
    state[3] = (unsigned short)((volumeRight - state[2]) * 65536.0f);
    state[6] = (unsigned short)(rateLeft >> 16);
    state[5] = (unsigned short)(rateLeft & 0xffff);
    state[9] = (unsigned short)(rateRight >> 16);
    state[8] = (unsigned short)(rateRight & 0xffff);
    state[4] = (unsigned short)targetLeft;
    state[7] = (unsigned short)targetRight;
    state[10] = (short)dry;
    state[11] = (short)wet;
}

void EnvMixerABI3SSE(AudioCommand *command, AudioContext *context)
{
    __declspec(align(16)) unsigned int leftMask[4] = {0xffffffff, 0, 0xffffffff, 0};
    __declspec(align(16)) unsigned int rightMask[4] = {0, 0xffffffff, 0, 0xffffffff};
    __declspec(align(16)) float scale[4] = {1.0f / 32768.0f, 1.0f / 32768.0f, 1.0f / 32768.0f, 1.0f / 32768.0f};
    int bothDown = 0, bothUp = 0, leftUp = 0, rightUp = 0;
    unsigned short *state;
    int rateLeft, rateRight;
    __declspec(align(16)) float volumeLeft, volumeRight, targetLeft, targetRight;
    int dry, wet, flags, input, outputLeft, auxLeft, outputRight, auxRight, count;
    __declspec(align(16)) float stepLeft, stepRight;
    short *buffer, *wetLeft, *wetRight, *dryLeft, *dryRight, *samples;
    __declspec(align(16)) float packed[4];

    if (!(((command->words[0] >> 16) & 0xff) & 1)) {
        state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
        rateLeft = (state[6] << 16) + state[5];
        rateRight = (state[9] << 16) + state[8];
        volumeLeft = ((state[0] << 16) + state[1]) / 65536.0f;
        volumeRight = ((state[2] << 16) + state[3]) / 65536.0f;
        targetLeft = state[4];
        targetRight = state[7];
        dry = state[10];
        wet = state[11];
    } else {
        rateLeft = context->rateLeft16;
        rateRight = context->rateRight16;
        targetLeft = context->targetLeft16;
        targetRight = context->targetRight16;
        volumeLeft = context->volumeLeft16;
        volumeRight = context->volumeRight16;
        wet = context->wet;
        dry = context->dry;
    }
    flags = (command->words[0] >> 16) & 0xff;
    input = 0;
    outputLeft = 0x270;
    auxLeft = 0x3e0;
    outputRight = 0x328;
    auxRight = 0x498;
    count = 0xb8;
    stepLeft = (float)(rateLeft / 65536.0);
    stepRight = (float)(rateRight / 65536.0);
    buffer = context->buffer;
    count = context->count >> 1;
    if (outputLeft == 0 && outputRight == 0 && volumeLeft == 0 && stepLeft == 0 && volumeRight == 0)
        return;
    stepLeft /= 8;
    stepRight /= 8;
    if (stepLeft > 0 && stepRight > 0)
        bothUp = 1;
    else if (stepLeft <= 0 && stepRight <= 0)
        bothDown = 1;
    else if (stepLeft > 0 && stepRight <= 0)
        leftUp = 1;
    else if (stepLeft <= 0 && stepRight > 0)
        rightUp = 1;
    wetLeft = buffer + auxLeft;
    wetRight = buffer + auxRight;
    dryLeft = buffer + outputLeft;
    dryRight = buffer + outputRight;
    samples = buffer + input;
    packed[0] = volumeLeft;
    packed[1] = volumeRight;
    packed[2] = volumeLeft;
    packed[3] = volumeRight;
    __asm movaps xmm5, packed
    packed[0] = stepLeft;
    packed[1] = stepRight;
    packed[2] = stepLeft;
    packed[3] = stepRight;
    __asm movaps xmm2, packed
    packed[0] = targetLeft;
    packed[1] = targetRight;
    packed[2] = targetLeft;
    packed[3] = targetRight;
    __asm movaps xmm1, packed
    __asm movaps xmm3, scale
    __asm mov esi, samples
    __asm emms
    /* Preserve the original zero-count branch: it reaches the EBX restore
     * before the matching save, as in both native SIMD implementations. */
    if (!count)
        __asm jmp finished
    __asm push ebx
    if (bothUp) {
        __asm {
up_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(up)
        __asm {
            addps xmm5, xmm2
            movaps xmm6, xmm1
            movaps xmm7, xmm1
            cmpleps xmm6, xmm5
            andps xmm7, xmm6
            andnps xmm6, xmm5
            orps xmm7, xmm6
            movaps xmm5, xmm7
            dec count
            jg up_loop
        }
    }
    else if (bothDown) {
        __asm {
down_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(down)
        __asm {
            addps xmm5, xmm2
            movaps xmm7, xmm5
            movaps xmm6, xmm5
            cmpleps xmm7, xmm1
            movaps xmm0, xmm7
            andnps xmm7, xmm6
            andps xmm0, xmm1
            orps xmm7, xmm0
            movaps xmm5, xmm7
            dec count
            jg down_loop
        }
    }
    else if (leftUp) {
        __asm movaps xmm4, rightMask
        __asm {
left_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(left)
        __asm {
            addps xmm5, xmm2
            movaps xmm7, xmm1
            movaps xmm6, xmm1
            cmpleps xmm6, xmm5
            xorps xmm6, xmm4
            andps xmm7, xmm6
            andnps xmm6, xmm5
            orps xmm7, xmm6
            movaps xmm5, xmm7
            dec count
            jg left_loop
        }
    }
    else if (rightUp) {
        __asm movaps xmm4, leftMask
        __asm {
right_loop:
            movd mm6, [esi]
            pshufw mm6, mm6, 0
            movaps xmm0, xmm3
            psrad mm6, 16
            mulps xmm0, xmm5
            cvtpi2ps xmm7, mm6
            mulps xmm0, xmm7
            cvtps2pi mm7, xmm0
            movd ebx, mm7
            movq mm0, mm7
            punpckldq mm0, mm0
            punpckhdq mm7, mm0
            movd edi, mm7
        }
        MIX_ABI3_OUTPUTS(right)
        __asm {
            addps xmm5, xmm2
            movaps xmm7, xmm1
            movaps xmm6, xmm1
            cmpleps xmm6, xmm5
            xorps xmm6, xmm4
            andps xmm7, xmm6
            andnps xmm6, xmm5
            orps xmm7, xmm6
            movaps xmm5, xmm7
            dec count
            jg right_loop
        }
    }
    __asm {
finished:
        pop ebx
        movaps packed, xmm5
        emms
    }
    volumeLeft = packed[0];
    volumeRight = packed[1];
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = (unsigned short)volumeLeft;
    state[1] = (unsigned short)((volumeLeft - state[0]) * 65536.0f);
    state[2] = (unsigned short)volumeRight;
    state[3] = (unsigned short)((volumeRight - state[2]) * 65536.0f);
    state[6] = (unsigned short)(rateLeft >> 16);
    state[5] = (unsigned short)(rateLeft & 0xffff);
    state[9] = (unsigned short)(rateRight >> 16);
    state[8] = (unsigned short)(rateRight & 0xffff);
    state[4] = (unsigned short)targetLeft;
    state[7] = (unsigned short)targetRight;
    state[10] = (short)dry;
    state[11] = (short)wet;
}
#pragma optimize("", on)

void EnvMixerSpecial(AudioCommand *command, AudioContext *context)
{
    int rateLeft, rateRight;
    int targetLeft, targetRight;
    float volumeLeft, volumeRight;
    int dry, wet;
    float stepLeft, stepRight;
    unsigned short *state;
    int input, outputLeft, wetLeft, outputRight, wetRight;
    int count, i;
    short *buffer;
    if (command->bytes[2] & 0x10)
        AudioError("A_MIX Set in envmixer?! heavens no. If you are a user, you shouldnt see this. report to lac@nemu.com you got this error.", 0);
    if (!(command->bytes[2] & 1)) {
        state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
        rateLeft = (state[6] << 16) + state[5];
        rateRight = (state[9] << 16) + state[8];
        volumeLeft = (float)((state[0] << 16) + state[1]) / 65536.f;
        volumeRight = (float)((state[2] << 16) + state[3]) / 65536.f;
        targetLeft = state[4];
        targetRight = state[7];
        dry = state[10];
        wet = state[11];
    } else {
        rateLeft = context->rateLeft16;
        rateRight = context->rateRight16;
        targetLeft = context->targetLeft16;
        targetRight = context->targetRight16;
        volumeLeft = (float)context->volumeLeft16;
        volumeRight = (float)context->volumeRight16;
        wet = context->wet;
        dry = context->dry;
    }
    stepLeft = (float)(rateLeft / 65536.0);
    stepLeft /= 8.f;
    stepRight = (float)(rateRight / 65536.0);
    stepRight /= 8.f;
    input = context->input >> 1;
    outputLeft = context->outputResample >> 1;
    wetLeft = context->auxResample1 >> 1;
    outputRight = context->auxResample0 >> 1;
    wetRight = context->auxResample2 >> 1;
    buffer = context->buffer;
    count = context->count >> 1;
    for (i = 0; i < count; ++i) {
        int sampleLeft, sampleRight;
        sampleLeft = (int)(buffer[input] * volumeLeft / 32768.f);
        {
            int mixed = buffer[outputLeft] + sampleLeft * dry / 32768;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            buffer[outputLeft] = mixed;
        }
        ++outputLeft;
        {
            int mixed = buffer[wetLeft] + sampleLeft * wet / 32768;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            buffer[wetLeft] = mixed;
        }
        ++wetLeft;
        sampleRight = (int)(buffer[input++] * volumeRight / 32768.f);
        {
            int mixed = buffer[outputRight] + sampleRight * dry / 32768;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            buffer[outputRight] = mixed;
        }
        ++outputRight;
        {
            int mixed = buffer[wetRight] + sampleRight * wet / 32768;
            if (mixed > 32767) mixed = 32767;
            else if (mixed < -32767) mixed = -32767;
            buffer[wetRight] = mixed;
        }
        ++wetRight;
        volumeLeft += stepLeft;
        if (stepLeft > 0.f) {
            if (volumeLeft > targetLeft) volumeLeft = (float)targetLeft;
        } else {
            if (volumeLeft < targetLeft) volumeLeft = (float)targetLeft;
        }
        volumeRight += stepRight;
        if (stepRight > 0.f) {
            if (volumeRight > targetRight) volumeRight = (float)targetRight;
        } else {
            if (volumeRight < targetRight) volumeRight = (float)targetRight;
        }
    }
    state = (unsigned short *)(RDRAM + (command->words[1] & 0xffffff));
    state[0] = (unsigned short)volumeLeft;
    state[1] = (unsigned short)((volumeLeft - state[0]) * 65536.f);
    state[2] = (unsigned short)volumeRight;
    state[3] = (unsigned short)((volumeRight - state[2]) * 65536.f);
    state[6] = (unsigned short)(rateLeft >> 16);
    state[5] = (unsigned short)rateLeft;
    state[9] = (unsigned short)(rateRight >> 16);
    state[8] = (unsigned short)rateRight;
    state[4] = (unsigned short)targetLeft;
    state[7] = (unsigned short)targetRight;
    state[10] = (unsigned short)dry;
    state[11] = (unsigned short)wet;
}

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

/* Integer envelope mixers. Nemu source grouping and private function names are
 * inferred. MixAudioEnvelope and MixAudioEnvelopeLegacy retain the source
 * structure of UltraHLE's archived SLIST.C envmix_mario/envmix_zelda routines;
 * globals are adapted to the context argument observed in the Nemu binary. */

void MixAudioEnvelopeClamped(int input, int output, int aux0, int aux1,
                            int aux2, int count, AudioContext *context)
{
    int left;
    int right;
    int rampLeft;
    int rampRight;
    int limitLeft;
    int limitRight;
    int dry;
    int wet;
    int i, j;
    count >>= 1;
    input >>= 1;
    output >>= 1;
    aux0 >>= 1;
    aux1 >>= 1;
    aux2 >>= 1;
    dry = context->dry;
    left = context->targetLeft;
    right = context->targetRight;
    rampLeft = context->rampLeft;
    rampRight = context->rampRight;
    limitLeft = context->rateLeft;
    limitRight = context->rateRight;
    wet = context->wet;
    if (rampLeft == 0xffff) { left = limitLeft; rampLeft = 0; }
    if (rampRight == 0xffff) { right = limitRight; rampRight = 0; }
    if (wet == 0 && left == 0 && rampLeft == 0 && right == 0) return;
    for (i = 0; i < count; i += 8) {
        for (j = i; j < i + 8; ++j) {
            int sample = context->buffer[input + j];
            int l = (sample * left) >> 15;
            int r = (sample * right) >> 15;
            int outLeft = context->buffer[output + j] + l * dry / 32768;
            int outRight = context->buffer[aux0 + j] + r * dry / 32768;
            if (outLeft > 32767) outLeft = 32767;
            else if (outLeft < -32767) outLeft = -32767;
            if (outRight > 32767) outRight = 32767;
            else if (outRight < -32767) outRight = -32767;
            context->buffer[output + j] = outLeft;
            context->buffer[aux0 + j] = outRight;
            outLeft = context->buffer[aux1 + j] + ((l * wet) >> 15);
            outRight = context->buffer[aux2 + j] + ((r * wet) >> 15);
            if (outLeft > 32767) outLeft = 32767;
            else if (outLeft < -32767) outLeft = -32767;
            if (outRight > 32767) outRight = 32767;
            else if (outRight < -32767) outRight = -32767;
            context->buffer[aux1 + j] = outLeft;
            context->buffer[aux2 + j] = outRight;
        }
        left += rampLeft;
        if (rampLeft > 0) {
            if (left > limitLeft) { left = limitLeft; rampLeft = 0; }
        } else {
            if (left < limitLeft) { left = limitLeft; rampLeft = 0; }
        }
        right += rampRight;
        if (rampRight > 0) {
            if (right > limitRight) { right = limitRight; rampRight = 0; }
        } else {
            if (right < limitRight) { right = limitRight; rampRight = 0; }
        }
    }
    context->rampLeft = rampLeft;
    context->rampRight = rampRight;
    context->volumeLeft = wet;
    context->volumeRight = wet;
    context->targetLeft = left;
    context->targetRight = right;
    context->rateLeft = limitLeft;
    context->rateRight = limitRight;
}

void MixAudioEnvelope(int src,int dst1,int dst2,int eff1,int eff2,int cnt,AudioContext *context)
{
    int i,j,jn;
    int lteff,rteff;
    int ltmul,ltadd,lttar;
    int rtmul,rtadd,rttar;
    int lt,rt;
    cnt>>=1;
    src>>=1;
    dst1>>=1;
    dst2>>=1;
    eff1>>=1;
    eff2>>=1;

    lteff=context->volumeLeft<<1;
    rteff=context->volumeRight<<1;
    ltmul=context->targetLeft<<1;
    rtmul=context->targetRight<<1;
    ltadd=context->rampLeft<<1;
    rtadd=context->rampRight<<1;
    lttar=context->rateLeft<<1;
    rttar=context->rateRight<<1;

    if(ltadd==65535 && ltadd>0)
    {
        ltmul=lttar;
        ltadd=0;
    }
    if(rtadd==65535 && rtadd>0)
    {
        rtmul=rttar;
        rtadd=0;
    }

    if(!lteff && !rteff && !ltmul && !ltadd && !rtmul && !rtmul)
    {
        // no need to mix, volumes zero
        return;
    }


    if(!lteff && !rteff && !ltadd && !rtadd)
    {
        // no effect mix
        for(j=0;j<cnt;j++)
        {
            lt=rt=context->buffer[src+j];
            lt=(lt*ltmul)>>16;
            rt=(rt*rtmul)>>16;
            context->buffer[dst1+j]+=lt;
            context->buffer[dst2+j]+=rt;
        }
    }
    else if(!ltadd && !rtadd)
    {
        // no volume change
        for(j=0;j<cnt;j++)
        {
            lt=rt=context->buffer[src+j];
            lt=(lt*ltmul)>>16;
            rt=(rt*rtmul)>>16;
            context->buffer[dst1+j]+=lt;
            context->buffer[dst2+j]+=rt;
            lt=(lt*lteff)>>16;
            rt=(rt*rteff)>>16;
            context->buffer[eff1+j]+=lt;
            context->buffer[eff2+j]+=rt;
        }
    }
    else
    {
        // full mix (no target)
        for(i=0;i<cnt;i+=8)
        {
            jn=i+8;
            for(j=i;j<jn;j++)
            {
                lt=rt=context->buffer[src+j];
                lt=(lt*ltmul)>>16;
                rt=(rt*rtmul)>>16;
                context->buffer[dst1+j]+=lt;
                context->buffer[dst2+j]+=rt;
                lt=(lt*lteff)>>16;
                rt=(rt*rteff)>>16;
                context->buffer[eff1+j]+=lt;
                context->buffer[eff2+j]+=rt;
            }
            ltmul+=ltadd;
            if(ltadd>0)
            {
                if(ltmul>lttar)
                {
                    ltmul=lttar;
                    ltadd=0;
                }
            }
            else
            {
                if(ltmul<lttar)
                {
                    ltmul=lttar;
                    ltadd=0;
                }
            }
            rtmul+=rtadd;
            if(rtadd>0)
            {
                if(rtmul>rttar)
                {
                    rtmul=rttar;
                    rtadd=0;
                }
            }
            else
            {
                if(rtmul<rttar)
                {
                    rtmul=rttar;
                    rtadd=0;
                }
            }
        }
    }

    context->volumeLeft=lteff>>1;
    context->volumeRight=rteff>>1;
    context->rampLeft=ltadd>>1;
    context->rampRight=rtadd>>1;
    context->targetLeft=ltmul>>1;
    context->targetRight=rtmul>>1;
    context->rateLeft=lttar>>1;
    context->rateRight=rttar>>1;
}

void MixAudioEnvelopeLegacy(int src,int dst1,int dst2,int eff1,int eff2,int cnt,AudioContext *context)
{
    int i,j,jn,lx,rx;
    int lteff,rteff;
    int ltmul,ltadd;
    int rtmul,rtadd;
    int lt,rt;
    cnt>>=1;
    src>>=1;
    dst1>>=1;
    dst2>>=1;
    eff1>>=1;
    eff2>>=1;

    lteff=context->volumeLeft;
    rteff=context->volumeRight;
    ltmul=context->targetLeft;
    rtmul=context->targetRight;
    ltadd=context->rampLeft;
    rtadd=context->rampRight;

    if(!lteff && !rteff && !ltmul && !ltadd && !rtmul && !rtmul)
    {
        // no need to mix, volumes zero
        return;
    }


    if(!lteff && !rteff && !ltadd && !rtadd)
    {
        // no effect mix
        for(j=0;j<cnt;j++)
        {
            lt=rt=context->buffer[src+j];
            lt=(lt*ltmul)>>16;
            rt=(rt*rtmul)>>16;
            lx=lt+context->buffer[dst1+j];
            rx=rt+context->buffer[dst2+j];
            /*
            if(lx<-32767) lx=-32767;
            if(lx> 32767) lx= 32767;
            if(rx<-32767) rx=-32767;
            if(rx> 32767) rx= 32767;
            */
            context->buffer[dst1+j]=lx;
            context->buffer[dst2+j]=rx;
        }
    }
    else if(!ltadd && !rtadd)
    {
        // no volume change
        for(j=0;j<cnt;j++)
        {
            lt=rt=context->buffer[src+j];
            lt=(lt*ltmul)>>16;
            rt=(rt*rtmul)>>16;
            lx=lt+context->buffer[dst1+j];
            rx=rt+context->buffer[dst2+j];
            /*
            if(lx<-32767) lx=-32767;
            if(lx> 32767) lx= 32767;
            if(rx<-32767) rx=-32767;
            if(rx> 32767) rx= 32767;
            */
            context->buffer[dst1+j]=lx;
            context->buffer[dst2+j]=rx;
            lt=(lt*lteff)>>16;
            rt=(rt*rteff)>>16;
            context->buffer[eff1+j]+=lt;
            context->buffer[eff2+j]+=rt;
        }
    }
    else
    {
        for(i=0;i<cnt;i+=8)
        {
            jn=i+8;
            for(j=i;j<jn;j++)
            {
                lt=rt=context->buffer[src+j];
                lt=(lt*ltmul)>>16;
                rt=(rt*rtmul)>>16;
                lx=lt+context->buffer[dst1+j];
                rx=rt+context->buffer[dst2+j];
                /*
                if(lx<-32767) lx=-32767;
                if(lx> 32767) lx= 32767;
                if(rx<-32767) rx=-32767;
                if(rx> 32767) rx= 32767;
                */
                context->buffer[dst1+j]=lx;
                context->buffer[dst2+j]=rx;
                lt=(lt*lteff)>>16;
                rt=(rt*rteff)>>16;
                context->buffer[eff1+j]+=lt;
                context->buffer[eff2+j]+=rt;
            }
            ltmul+=ltadd;
            rtmul+=rtadd;
            if(ltmul>65535)
            {
                ltmul=65535;
                ltadd=0;
            }
            else if(ltmul<0)
            {
                ltmul=0;
                ltadd=0;
            }
            if(rtmul>65535)
            {
                rtmul=65535;
                rtadd=0;
            }
            else if(rtmul<0)
            {
                rtmul=0;
                rtadd=0;
            }
        }
    }
}

/* Unsigned envelope gains, applied to signed PCM. The loop structure follows
 * UltraHLE's envmix_zelda; the narrower gains, ramp saturation and dry-output
 * clamps are recovered from audio.dll. Private names and filename are inferred. */
void MixAudioEnvelopeUnsigned(int input, int output, int aux0, int aux1,
                             int aux2, int count, AudioContext *context)
{
    unsigned short wetLeft, wetRight, gainLeft, gainRight;
    unsigned short increaseLeft, decreaseLeft, increaseRight, decreaseRight;
    short rampLeft, rampRight;
    int i, j, jn, left, right;
    short mixedLeft, mixedRight;

    count >>= 1;
    input >>= 1;
    output >>= 1;
    aux0 >>= 1;
    aux1 >>= 1;
    aux2 >>= 1;
    wetLeft = (unsigned short)context->volumeLeft;
    wetRight = (unsigned short)context->volumeRight;
    gainLeft = (unsigned short)context->targetLeft;
    gainRight = (unsigned short)context->targetRight;

    rampLeft = (short)context->rampLeft;
    rampRight = (short)context->rampRight;
    if (rampRight >= 0) {
        increaseRight = abs(rampRight);
        decreaseRight = 0;
    } else {
        increaseRight = 0;
        decreaseRight = abs(rampRight);
    }
    if (rampLeft >= 0) {
        increaseLeft = abs(rampLeft);
        decreaseLeft = 0;
    } else {
        increaseLeft = 0;
        decreaseLeft = abs(rampLeft);
    }

    /* The original silence shortcut does not consult the right ramp. */
    if (wetLeft == 0 && wetRight == 0 && gainLeft == 0 &&
        rampLeft == 0 && gainRight == 0)
        return;
    if (wetLeft == 0 && wetRight == 0 && rampLeft == 0 && rampRight == 0) {
        for (j = 0; j < count; ++j) {
            mixedLeft = mixedRight = context->buffer[input + j];
            mixedLeft = (short)((mixedLeft * gainLeft) >> 16);
            mixedRight = (short)((mixedRight * gainRight) >> 16);
            left = mixedLeft + context->buffer[output + j];
            right = mixedRight + context->buffer[aux0 + j];
            if (left < -32767) left = -32767;
            else if (left > 32767) left = 32767;
            if (right < -32767) right = -32767;
            else if (right > 32767) right = 32767;
            context->buffer[output + j] = (short)left;
            context->buffer[aux0 + j] = (short)right;
        }
    } else if (rampLeft == 0 && rampRight == 0) {
        for (j = 0; j < count; ++j) {
            mixedLeft = mixedRight = context->buffer[input + j];
            mixedLeft = (short)((mixedLeft * gainLeft) >> 16);
            mixedRight = (short)((mixedRight * gainRight) >> 16);
            left = mixedLeft + context->buffer[output + j];
            right = mixedRight + context->buffer[aux0 + j];
            if (left < -32767) left = -32767;
            else if (left > 32767) left = 32767;
            if (right < -32767) right = -32767;
            else if (right > 32767) right = 32767;
            context->buffer[output + j] = (short)left;
            context->buffer[aux0 + j] = (short)right;
            mixedLeft = (mixedLeft * wetLeft) >> 16;
            mixedRight = (mixedRight * wetRight) >> 16;
            context->buffer[aux1 + j] += mixedLeft;
            context->buffer[aux2 + j] += mixedRight;
        }
    } else {
        for (i = 0; i < count; i += 8) {
            jn = i + 8;
            for (j = i; j < jn; ++j) {
                mixedLeft = mixedRight = context->buffer[input + j];
                mixedLeft = (short)((mixedLeft * gainLeft) >> 16);
                mixedRight = (short)((mixedRight * gainRight) >> 16);
                left = mixedLeft + context->buffer[output + j];
                right = mixedRight + context->buffer[aux0 + j];
                if (left < -32767) left = -32767;
                else if (left > 32767) left = 32767;
                if (right < -32767) right = -32767;
                else if (right > 32767) right = 32767;
                context->buffer[output + j] = (short)left;
                context->buffer[aux0 + j] = (short)right;
                mixedLeft = (mixedLeft * wetLeft) >> 16;
                mixedRight = (mixedRight * wetRight) >> 16;
                context->buffer[aux1 + j] += mixedLeft;
                context->buffer[aux2 + j] += mixedRight;
            }
            if (65535 - gainLeft < increaseLeft) gainLeft = 65535;
            else gainLeft += increaseLeft;
            if (gainLeft < decreaseLeft) gainLeft = 0;
            else gainLeft -= decreaseLeft;
            if (65535 - gainRight < increaseRight) gainRight = 65535;
            else gainRight += increaseRight;
            if (gainRight < decreaseRight) gainRight = 0;
            else gainRight -= decreaseRight;
        }
    }
}
