/* JPEG microcode reconstruction; original unit name inferred. */
#include <math.h>
#include <stdlib.h>
#include <string.h>

extern unsigned char *RDRAM;

int JpegZigZag[64] = {
    0, 1, 5, 6, 14, 15, 27, 28,
    2, 4, 7, 13, 16, 26, 29, 42,
    3, 8, 12, 17, 25, 30, 41, 43,
    9, 11, 18, 24, 31, 40, 44, 53,
    10, 19, 23, 32, 39, 45, 52, 54,
    20, 22, 33, 38, 46, 51, 55, 60,
    21, 34, 37, 47, 50, 56, 59, 61,
    35, 36, 48, 49, 57, 58, 62, 63
};

int JpegCosine[64] = {
    16, 16, 16, 16, 16, 16, 16, 16,
    15, 13, 8, 3, -3, -8, -13, -15,
    14, 6, -6, -14, -14, -6, 6, 14,
    13, -3, -15, -8, 8, 15, 3, -13,
    11, -11, -11, 11, 11, -11, -11, 11,
    8, -15, 3, 13, -13, -3, 15, -8,
    6, -14, 14, -6, -6, 14, -14, 6,
    3, -8, 13, -15, 15, -13, 8, -3
};

int JpegBlock[384];
int JpegOutput[256];
int JpegQuantizationCb[64];
int JpegPixels[16][16][4];
int JpegQuantizationCr[64];
int JpegQuantizationY[64];

void JpegReadBlock(int *destination, unsigned int address, int bytes)
{
    int i;
    for (i = 0; i < (bytes >> 1); i += 2) {
        int word = *(int *)(RDRAM + ((address + i * 2) & 0xffffff));
        destination[i] = (short)(word >> 16);
        destination[i + 1] = (short)word;
    }
}

void JpegWriteBlock(int *source, unsigned int address, int bytes)
{
    int i;
    for (i = 0; i < (bytes >> 1); i += 2) {
        int high = source[i] & 0xffff;
        int low = source[i + 1] & 0xffff;
        *(int *)(RDRAM + ((address + i * 2) & 0xffffff)) = (high << 16) + low;
    }
}

void JpegInitializeCosine(void)
{
    int i, j;
    for (i = 0; i < 8; ++i)
        for (j = 0; j < 8; ++j) {
            double coefficient = cos((2 * i + 1) * j * 3.14159265358979323846 / 16);
            if (j == 0)
                coefficient /= sqrt(2.0);
            JpegCosine[i * 8 + j] = (int)(coefficient * 200);
        }
}

unsigned short JpegSwapSample(unsigned short sample)
{
    return (unsigned short)_rotl(sample, 8);
}

void JpegCopyBlock(int *destination, int *source, int count)
{
    int i;
    for (i = 0; i < count; ++i)
        destination[i] = source[i];
}

void JpegInverseDCT(int *destination, int *source, int *quantization, int luminance)
{
    static int initialized;
    int block[64], sums[8];
    int i, j, k;
    if (!initialized) {
        JpegInitializeCosine();
        initialized = 1;
    }
    for (i = 0; i < 64; ++i) {
        int coefficient = JpegZigZag[i];
        block[i] = (source[coefficient] * quantization[coefficient]) >> 3;
    }
    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j)
            sums[j] = block[i] << 8;
        for (k = 1; k < 8; ++k)
            for (j = 0; j < 8; ++j)
                sums[j] += block[i + k * 8] * JpegCosine[k * 8 + j];
        for (j = 0; j < 8; ++j)
            block[i + j * 8] = sums[j] >> 8;
    }
    for (j = 0; j < 8; ++j) {
        for (i = 0; i < 8; ++i)
            sums[i] = block[j * 8] << 8;
        for (k = 1; k < 8; ++k)
            for (i = 0; i < 8; ++i)
                sums[i] += block[j * 8 + k] * JpegCosine[k * 8 + i];
        for (i = 0; i < 8; ++i)
            block[j * 8 + i] = sums[i] >> 8;
    }
    for (j = 0; j < 8; ++j)
        for (k = 0; k < 8; ++k)
            destination[j * 8 + k] = block[j * 8 + k];
}

void JpegInverseDCTAlternate(int *destination, int *source, int *quantization, int luminance)
{
    static int initialized;
    int block[64], sums[8];
    int i, j, k;
    if (!initialized) {
        JpegInitializeCosine();
        initialized = 1;
    }
    for (i = 0; i < 64; ++i) {
        int coefficient = JpegZigZag[i];
        block[i] = (source[coefficient] * quantization[coefficient]) >> 3;
    }
    for (i = 0; i < 8; ++i) {
        for (j = 0; j < 8; ++j)
            sums[j] = block[i] << 8;
        for (k = 1; k < 8; ++k)
            for (j = 0; j < 8; ++j)
                sums[j] += block[i + k * 8] * JpegCosine[k * 8 + j];
        for (j = 0; j < 8; ++j)
            block[i + j * 8] = sums[j] >> 8;
    }
    for (j = 0; j < 8; ++j) {
        for (i = 0; i < 8; ++i)
            sums[i] = block[j * 8] << 8;
        for (k = 1; k < 8; ++k)
            for (i = 0; i < 8; ++i)
                sums[i] += block[j * 8 + k] * JpegCosine[k * 8 + i];
        for (i = 0; i < 8; ++i)
            block[j * 8 + i] = sums[i] >> 8;
    }
    for (j = 0; j < 8; ++j)
        for (k = 0; k < 8; ++k)
            destination[j * 8 + k] = block[j * 8 + k];
}

void JpegDecodeMacroblock(void)
{
    int cb[64], cr[64];
    int x, y, bx, by;
    JpegInverseDCT(cb, JpegBlock + 256, JpegQuantizationCb, 0);
    JpegInverseDCT(cr, JpegBlock + 320, JpegQuantizationCr, 0);
    for (y = 0; y < 16; ++y)
        for (x = 0; x < 16; ++x) {
            int u = cb[(y >> 1) * 8 + (x >> 1)];
            int v = cr[(y >> 1) * 8 + (x >> 1)];
            JpegPixels[y][x][0] = (v * 291) >> 8;
            JpegPixels[y][x][1] = (-u * 101 - v * 148) >> 8;
            JpegPixels[y][x][2] = (u * 564) >> 8;
        }
    for (by = 0; by <= 8; by += 8)
        for (bx = 0; bx <= 8; bx += 8) {
            JpegInverseDCT(cr, JpegBlock + ((by / 8) * 2 + bx / 8) * 64,
                           JpegQuantizationY, 1);
            for (y = 0; y < 8; ++y)
                for (x = 0; x < 8; ++x) {
                    JpegPixels[by + y][bx + x][0] += cr[y * 8 + x] + 128;
                    JpegPixels[by + y][bx + x][1] += cr[y * 8 + x] + 128;
                    JpegPixels[by + y][bx + x][2] += cr[y * 8 + x] + 128;
                }
        }
    for (y = 0; y < 16; ++y)
        for (x = 0; x < 16; ++x) {
            int color, packed;
            color = JpegPixels[y][x][0] >> 3;
            if (color < 0) color = 0; else if (color > 31) color = 31;
            packed = (color << 11) + 1;
            color = JpegPixels[y][x][1] >> 3;
            if (color < 0) color = 0; else if (color > 31) color = 31;
            packed += color << 6;
            color = JpegPixels[y][x][2] >> 3;
            if (color < 0) color = 0; else if (color > 31) color = 31;
            packed += color << 1;
            JpegOutput[y * 16 + x] = packed;
        }
}

void DCTList(unsigned int address)
{
    unsigned int task[16];
    unsigned int data, cb, cr, y;
    int i;
    memcpy(task, RDRAM + address, sizeof(task));
    data = *(unsigned int *)(RDRAM + (task[12] & 0xffffff));
    y = *(unsigned int *)(RDRAM + ((task[12] + 12) & 0xffffff));
    cb = *(unsigned int *)(RDRAM + ((task[12] + 16) & 0xffffff));
    cr = *(unsigned int *)(RDRAM + ((task[12] + 20) & 0xffffff));
    if ((data & 0x7f000000) || (y & 0x7f000000) ||
        (cb & 0x7f000000) || (cr & 0x7f000000) || !data || !y || !cb || !cr)
        return;
    JpegReadBlock(JpegQuantizationY, y, 128);
    JpegReadBlock(JpegQuantizationCb, cb, 128);
    JpegReadBlock(JpegQuantizationCr, cr, 128);
    for (i = 1; i < 64; ++i) {
        JpegQuantizationY[i] <<= 1;
        JpegQuantizationCb[i] <<= 1;
        JpegQuantizationCr[i] <<= 1;
    }
    for (i = 0; i < 4; ++i) {
        JpegReadBlock(JpegBlock, data, 0x300);
        JpegDecodeMacroblock();
        JpegWriteBlock(JpegOutput, data, 0x200);
        data += 0x300;
    }
}
