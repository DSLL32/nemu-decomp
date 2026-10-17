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
/* Fixed-point MP3 butterfly stages. */
short Mp3V0;
short Mp3V1;
short Mp3V2;
short Mp3V3;
short Mp3V4;
short Mp3V5;
short Mp3V6;
short Mp3V7;
short Mp3V8;
short Mp3V9;
short Mp3V10;
short Mp3V11;
short Mp3V12;
short Mp3V13;
short Mp3V14;
short Mp3V15;
short Mp3V16;
short Mp3V17;
short Mp3V18;
short Mp3V19;
short Mp3V20;
short Mp3V21;
short Mp3V22;
short Mp3V23;
short Mp3V24;
short Mp3V25;
short Mp3V26;
short Mp3V27;
short Mp3V28;
short Mp3V29;
short Mp3V30;
short Mp3V31;

void Mp3Butterfly(unsigned short *coefficients)
{
    Mp3V16 = Mp3V8 + Mp3V0;
    Mp3V17 = Mp3V9 + Mp3V1;
    Mp3V18 = Mp3V10 + Mp3V2;
    Mp3V19 = Mp3V11 + Mp3V3;
    Mp3V20 = Mp3V12 + Mp3V4;
    Mp3V21 = Mp3V13 + Mp3V5;
    Mp3V22 = Mp3V14 + Mp3V6;
    Mp3V23 = Mp3V15 + Mp3V7;
    Mp3V24 = Mp3V0 - Mp3V8;
    Mp3V25 = Mp3V1 - Mp3V9;
    Mp3V26 = Mp3V2 - Mp3V10;
    Mp3V27 = Mp3V3 - Mp3V11;
    Mp3V28 = Mp3V4 - Mp3V12;
    Mp3V29 = Mp3V5 - Mp3V13;
    Mp3V30 = Mp3V6 - Mp3V14;
    Mp3V31 = Mp3V7 - Mp3V15;
    Mp3V0 = Mp3V16 + Mp3V20;
    Mp3V1 = Mp3V17 + Mp3V21;
    Mp3V2 = Mp3V18 + Mp3V22;
    Mp3V3 = Mp3V19 + Mp3V23;
    Mp3V4 = Mp3V16 - Mp3V20;
    Mp3V5 = Mp3V17 - Mp3V21;
    Mp3V6 = Mp3V18 - Mp3V22;
    Mp3V7 = Mp3V19 - Mp3V23;
    Mp3V24 = (Mp3V24 * coefficients[8]) >> 16;
    Mp3V25 = (Mp3V25 * coefficients[10]) >> 16;
    Mp3V26 = (Mp3V26 * coefficients[14]) >> 16;
    Mp3V27 = (Mp3V27 * coefficients[12]) >> 16;
    Mp3V28 = (Mp3V28 * coefficients[21]) >> 16;
    Mp3V29 = (Mp3V29 * coefficients[19]) >> 16;
    Mp3V30 = (Mp3V30 * coefficients[15]) >> 16;
    Mp3V31 = (Mp3V31 * coefficients[17]) >> 16;
    Mp3V4 = (Mp3V4 * coefficients[9]) >> 16;
    Mp3V5 = (Mp3V5 * coefficients[13]) >> 16;
    Mp3V6 = (Mp3V6 * coefficients[20]) >> 16;
    Mp3V7 = (Mp3V7 * coefficients[16]) >> 16;
    Mp3V8 = Mp3V24 + Mp3V28;
    Mp3V9 = Mp3V25 + Mp3V29;
    Mp3V10 = Mp3V26 + Mp3V30;
    Mp3V11 = Mp3V27 + Mp3V31;
    Mp3V12 = Mp3V24 - Mp3V28;
    Mp3V13 = Mp3V25 - Mp3V29;
    Mp3V14 = Mp3V26 - Mp3V30;
    Mp3V15 = Mp3V27 - Mp3V31;
    Mp3V12 = (Mp3V12 * coefficients[9]) >> 16;
    Mp3V13 = (Mp3V13 * coefficients[13]) >> 16;
    Mp3V14 = (Mp3V14 * coefficients[20]) >> 16;
    Mp3V15 = (Mp3V15 * coefficients[16]) >> 16;
    Mp3V16 = Mp3V0 + Mp3V2;
    Mp3V17 = Mp3V1 + Mp3V3;
    Mp3V18 = Mp3V0 - Mp3V2;
    Mp3V19 = Mp3V1 - Mp3V3;
    Mp3V20 = Mp3V4 + Mp3V6;
    Mp3V21 = Mp3V5 + Mp3V7;
    Mp3V22 = Mp3V4 - Mp3V6;
    Mp3V23 = Mp3V5 - Mp3V7;
    Mp3V24 = Mp3V8 + Mp3V10;
    Mp3V25 = Mp3V9 + Mp3V11;
    Mp3V26 = Mp3V8 - Mp3V10;
    Mp3V27 = Mp3V9 - Mp3V11;
    Mp3V28 = Mp3V12 + Mp3V14;
    Mp3V29 = Mp3V13 + Mp3V15;
    Mp3V30 = Mp3V12 - Mp3V14;
    Mp3V31 = Mp3V13 - Mp3V15;
    Mp3V18 = (Mp3V18 * coefficients[11]) >> 16;
    Mp3V19 = (Mp3V19 * coefficients[18]) >> 16;
    Mp3V22 = (Mp3V22 * coefficients[11]) >> 16;
    Mp3V23 = (Mp3V23 * coefficients[18]) >> 16;
    Mp3V26 = (Mp3V26 * coefficients[11]) >> 16;
    Mp3V27 = (Mp3V27 * coefficients[18]) >> 16;
    Mp3V30 = (Mp3V30 * coefficients[11]) >> 16;
    Mp3V31 = (Mp3V31 * coefficients[18]) >> 16;
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
    offset = 0x80 - t4;
    for (outer = 0; outer < 3; ++outer) {
        Mp3ReadSamples((unsigned short *) samples, input, 0x180);
        input += 0x180;
        for (inner = 0; inner < 6; ++inner) {
            Mp3V0 = Mp3Add(*(int *) (samples + 0), *(int *) (samples + 62));
            Mp3V1 = Mp3Add(*(int *) (samples + 2), *(int *) (samples + 60));
            Mp3V3 = Mp3Add(*(int *) (samples + 4), *(int *) (samples + 58));
            Mp3V2 = Mp3Add(*(int *) (samples + 6), *(int *) (samples + 56));
            Mp3V6 = Mp3Add(*(int *) (samples + 8), *(int *) (samples + 54));
            Mp3V7 = Mp3Add(*(int *) (samples + 10), *(int *) (samples + 52));
            Mp3V5 = Mp3Add(*(int *) (samples + 12), *(int *) (samples + 50));
            Mp3V4 = Mp3Add(*(int *) (samples + 14), *(int *) (samples + 48));
            Mp3V12 = Mp3Add(*(int *) (samples + 16), *(int *) (samples + 46));
            Mp3V13 = Mp3Add(*(int *) (samples + 18), *(int *) (samples + 44));
            Mp3V15 = Mp3Add(*(int *) (samples + 20), *(int *) (samples + 42));
            Mp3V14 = Mp3Add(*(int *) (samples + 22), *(int *) (samples + 40));
            Mp3V10 = Mp3Add(*(int *) (samples + 24), *(int *) (samples + 38));
            Mp3V11 = Mp3Add(*(int *) (samples + 26), *(int *) (samples + 36));
            Mp3V9 = Mp3Add(*(int *) (samples + 28), *(int *) (samples + 34));
            Mp3V8 = Mp3Add(*(int *) (samples + 30), *(int *) (samples + 32));
            t5 &= 0xffe0;
            t5 |= t4;
            t6 |= t4;
            Mp3Butterfly(coefficients);
            t0 = t6 + 0x100;
            t2 = t5 + 0x100;
            t3 = t5 + 0x200;
            v11 = MP3_SPLIT(MP3_SUB16(Mp3V16, Mp3V17), coefficients[40]);
            v0 = -MP3_ADD16(Mp3V16, Mp3V17);
            memory[t6 >> 1] = v11;
            memory[t3 >> 1] = v0;
            v2 = -MP3_ADD16(Mp3V18, Mp3V19);
            memory[t2 >> 1] = v2;
            v3 = MP3_SPLIT(MP3_SUB16(Mp3V18, Mp3V19), (coefficients[43] + ((short) coefficients[42] << 16))) - v2;
            memory[t0 >> 1] = v3;
            memory[t5 >> 1] = -v11;
            v4 = -MP3_ADD16(Mp3V20, Mp3V21);
            v5 = MP3_SPLIT(MP3_SUB16(Mp3V20, Mp3V21), (coefficients[43] + ((short) coefficients[42] << 16)));
            memory[(t3 - 0x80) >> 1] = v4;
            v7 = MP3_SPLIT(MP3_SUB16(Mp3V22, Mp3V23), (coefficients[45] + ((short) coefficients[44] << 16)));
            v5 -= v4;
            v7 -= v5;
            v6 = MP3_ADD16(Mp3V22, Mp3V23) * 2;
            v4 += v6;
            v5 -= v6;
            memory[(t6 + 0x180) >> 1] = v7;
            memory[(t2 - 0x80) >> 1] = -v4;
            memory[(t0 - 0x80) >> 1] = v5;
            v8 = MP3_ADD16(Mp3V24, Mp3V25);
            v9 = MP3_SPLIT(MP3_SUB16(Mp3V24, Mp3V25), (coefficients[43] + ((short) coefficients[42] << 16)));
            v11 = MP3_SUB16(Mp3V26, Mp3V27);
            Mp3V10 = MP3_ADD16(Mp3V26, Mp3V27) * 2;
            v2 = MP3_ADD16(v8, v9);
            Mp3V12 = MP3_ADD16(Mp3V28, Mp3V29) * 2;
            v13 = MP3_SPLIT(MP3_SUB16(Mp3V28, Mp3V29), (coefficients[45] + ((short) coefficients[44] << 16))) - v2 + Mp3V12;
            memory[(t0 - 0x40) >> 1] = v13 - Mp3V10;
            Mp3V14 = v8 -MP3_ADD16(Mp3V30, Mp3V31) * 4 + Mp3V10;
            v15 = MP3_SUB16(Mp3V30, Mp3V31);
            memory[(t6 + 0x40) >> 1] = v9 - Mp3V14;
            v11 = MP3_SPLIT(v11, (coefficients[45] + ((short) coefficients[44] << 16))) - v8 - v9 - v13 + v2;
            memory[(t0 + 0x40) >> 1] = v11;
            memory[(t6 + 0x1c0) >> 1] = -MP3_SPLIT(v15, (coefficients[47] + ((short) coefficients[46] << 16)));
            memory[(t2 + 0x40) >> 1] = v8 - Mp3V12;
            memory[(t3 - 0x40) >> 1] = -v8;
            memory[(t5 + 0x40) >> 1] = Mp3V14;
            memory[(t2 - 0x40) >> 1] = Mp3V10;
            Mp3V0 = (((short) Mp3Subtract(*(int *) (samples + 0), *(int *) (samples + 62)) * coefficients[24]) >> 16) * 2;
            Mp3V1 = (((short) Mp3Subtract(*(int *) (samples + 2), *(int *) (samples + 60)) * coefficients[25]) >> 16) * 2;
            Mp3V2 = (((short) Mp3Subtract(*(int *) (samples + 6), *(int *) (samples + 56)) * coefficients[27]) >> 16) * 2;
            Mp3V3 = (((short) Mp3Subtract(*(int *) (samples + 4), *(int *) (samples + 58)) * coefficients[26]) >> 16) * 2;
            Mp3V4 = (((short) Mp3Subtract(*(int *) (samples + 14), *(int *) (samples + 48)) * coefficients[31]) >> 16) * 2;
            Mp3V5 = (((short) Mp3Subtract(*(int *) (samples + 12), *(int *) (samples + 50)) * coefficients[30]) >> 16) * 2;
            Mp3V6 = (((short) Mp3Subtract(*(int *) (samples + 8), *(int *) (samples + 54)) * coefficients[28]) >> 16) * 2;
            Mp3V7 = (((short) Mp3Subtract(*(int *) (samples + 10), *(int *) (samples + 52)) * coefficients[29]) >> 16) * 2;
            Mp3V8 = (((short) Mp3Subtract(*(int *) (samples + 30), *(int *) (samples + 32)) * coefficients[39]) >> 16);
            Mp3V9 = (((short) Mp3Subtract(*(int *) (samples + 28), *(int *) (samples + 34)) * coefficients[38]) >> 16);
            Mp3V10 = (((short) Mp3Subtract(*(int *) (samples + 24), *(int *) (samples + 38)) * coefficients[36]) >> 16);
            Mp3V11 = (((short) Mp3Subtract(*(int *) (samples + 26), *(int *) (samples + 36)) * coefficients[37]) >> 16);
            Mp3V12 = (((short) Mp3Subtract(*(int *) (samples + 16), *(int *) (samples + 46)) * coefficients[32]) >> 16) * 2;
            Mp3V13 = (((short) Mp3Subtract(*(int *) (samples + 18), *(int *) (samples + 44)) * coefficients[33]) >> 16) * 2;
            Mp3V14 = (((short) Mp3Subtract(*(int *) (samples + 22), *(int *) (samples + 40)) * coefficients[35]) >> 16);
            Mp3V15 = (((short) Mp3Subtract(*(int *) (samples + 20), *(int *) (samples + 42)) * coefficients[34]) >> 16) * 2;
            Mp3Butterfly(coefficients);
            v1 = MP3_ACCUMULATE_FRACTION(Mp3V16, Mp3V17, coefficients[40], coefficients[41]);
            v0 = MP3_ACCUMULATE_UNSIGNED(Mp3V16, Mp3V17, coefficients[22], coefficients[22]);
            v2 = -MP3_ADD16(Mp3V18, Mp3V19);
            v3 = MP3_SPLIT(MP3_SUB16(Mp3V18, Mp3V19), (coefficients[43] + ((short) coefficients[42] << 16)));
            v4 = MP3_ADD16(v0, MP3_ADD16(Mp3V20, Mp3V21));
            v5 = MP3_SPLIT(MP3_SUB16(Mp3V20, Mp3V21), (coefficients[43] + ((short) coefficients[42] << 16))) + v1;
            v6 = MP3_ADD16(Mp3V22, Mp3V23) * 2 - v2 + v0;
            v7 = MP3_SPLIT(MP3_SUB16(Mp3V22, Mp3V23), (coefficients[45] + ((short) coefficients[44] << 16))) + v0 + v1 + v3;
            memory[(t3 - 0x20) >> 1] = -v0;
            v8 = MP3_ADD16(Mp3V24, Mp3V25);
            v9 = MP3_SPLIT(MP3_SUB16(Mp3V24, Mp3V25), (coefficients[43] + ((short) coefficients[42] << 16)));
            v10 = MP3_ADD16(Mp3V26, Mp3V27) * 2 + v8;
            v11 = MP3_SPLIT(MP3_SUB16(Mp3V26, Mp3V27), (coefficients[45] + ((short) coefficients[44] << 16))) + v9 + v8;
            v12 = v4 -MP3_ADD16(Mp3V28, Mp3V29) * 2;
            memory[(t2 + 0x20) >> 1] = v12;
            v13 = MP3_SPLIT(MP3_SUB16(Mp3V28, Mp3V29), (coefficients[45] + ((short) coefficients[44] << 16))) - v5 - v12;
            v14 = v6 -MP3_ADD16(Mp3V30, Mp3V31) * 4;
            memory[(t5 + 0x20) >> 1] = v14;
            memory[(t6 + 0x20) >> 1] = v14 + v1;
            v15 = MP3_SPLIT(MP3_SUB16(Mp3V30, Mp3V31), (coefficients[47] + ((short) coefficients[46] << 16))) - v7;
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
            {
                short *out = output;
                unsigned int address = t6, window = offset;
                for (x = 0; x < 8; ++x) {
                    int a = 0, b = 0, c = 0, d = 0;
                    for (i = 0; i < 4; ++i) {
                        a += MP3_FRACTION(memory[((address + 0x00) >> 1) + i], coefficients[((window + 0x00) >> 1) + i]);
                        b += MP3_FRACTION(memory[((address + 0x08) >> 1) + i], coefficients[((window + 0x08) >> 1) + i]);
                        c += MP3_FRACTION(memory[((address + 0x10) >> 1) + i], coefficients[((window + 0x10) >> 1) + i]);
                        d += MP3_FRACTION(memory[((address + 0x18) >> 1) + i], coefficients[((window + 0x18) >> 1) + i]);
                    }
                    *out++ = a + b + c + d;
                    a = b = c = d = 0;
                    for (i = 0; i < 4; ++i) {
                        a += MP3_FRACTION(memory[((address + 0x20) >> 1) + i], coefficients[((window + 0x40) >> 1) + i]);
                        b += MP3_FRACTION(memory[((address + 0x28) >> 1) + i], coefficients[((window + 0x48) >> 1) + i]);
                        c += MP3_FRACTION(memory[((address + 0x30) >> 1) + i], coefficients[((window + 0x50) >> 1) + i]);
                        d += MP3_FRACTION(memory[((address + 0x38) >> 1) + i], coefficients[((window + 0x58) >> 1) + i]);
                    }
                    *out++ = a + b + c + d;
                    address += 0x40;
                    window += 0x40;
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
