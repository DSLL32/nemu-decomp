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
