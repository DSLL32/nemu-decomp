#include "PIF.h"
#include "Memory.h"
#include "CPU.h"
#include <string.h>


inline unsigned ByteSwap(unsigned value)
{
    unsigned result;
    __asm { mov eax, value
            bswap eax
            mov result, eax }
    return result;
}

void FindCICInput(FILE *file, const unsigned char *data)
{
    unsigned char previous[16];
    while (fread(previous, 1, 16, file)) {
        if (memcmp(data, previous, 16) == 0) {
            fseek(file, -16, SEEK_CUR);
            return;
        }
    }
}

void SaveCICInput(const unsigned char *data)
{
    FILE *file = fopen("d:\\n64\\lastin.cic", "r+b");
    if (!file) {
        file = fopen("d:\\n64\\lastin.cic", "wb");
        if (!file) return;
    }
    fseek(file, 0, SEEK_SET);
    FindCICInput(file, data);
    fwrite(data, 1, 16, file);
    fclose(file);
}

int CICUseTable1(const unsigned char *response)
{
    unsigned swapped = ByteSwap(*(const unsigned *)response);
    unsigned key = (((swapped >> 4) & 0xf00000) | (swapped & 0xf0000)) >> 4;
    key |= (swapped & 0xf) << 4;
    key |= swapped & 0xf00;
    for (int i = 0; i < 32768; ++i) {
        unsigned inputWord = ByteSwap(PifResponseHistory[i]);
        if (key == (inputWord & 0xfffffff0)) return 1;
    }
    return 0;
}

void CIC6105Challenge(unsigned char *command)
{
    unsigned char response[32];
    unsigned char challenge[32];
    if (CIC6105Table) {
        unsigned char *data = command + 2;
        SaveCICInput(data);
        for (unsigned i = 0; i < 15; ++i) {
            challenge[i * 2] = data[i] >> 4;
            challenge[i * 2 + 1] = data[i] & 15;
        }
        int swapped = ByteSwap(*(unsigned *)data);
        unsigned inputWord = ByteSwap(CIC6105Table[(swapped >> 12) & 0xfffff]);
        *(int *)data = inputWord;
        response[0] = (inputWord >> 28) & 15;
        response[1] = (inputWord >> 24) & 15;
        response[2] = (inputWord >> 20) & 15;
        response[3] = (inputWord >> 16) & 15;
        response[4] = (inputWord >> 12) & 15;
        for (unsigned j = 0; j + 5 < 31; ++j) {
            response[j + 5] = PifResponseTable0[(challenge[j + 5] & 15) + ((response[j + 4] & 15) << 4)];
            if (response[j + 4] == 1 || response[j + 4] == 9 ||
                response[j + 4] == 11 || response[j + 4] == 14) {
                if (CICUseTable1(response + j))
                    response[j + 5] = PifResponseTable1[(challenge[j + 5] & 15) + ((response[j + 4] & 15) << 4)];
            }
        }
        for (unsigned k = 0; k < 15; ++k)
            data[k] = (response[k * 2 + 1] & 15) | (response[k * 2] << 4);
    }
}

