#include "RSP.h"
#include <stdio.h>
#include <math.h>

// Original function 00406820.
CpuU32 RSP_Reciprocal(CpuS32 value, CpuU32 squareRoot, CpuU32 singlePrecision)
{
    CpuU32 magnitude = value;
    if (singlePrecision) {
        if (value < 0) magnitude = -magnitude;
    } else if (value < 0) {
        if (value >= -32768) magnitude = -value;
        else magnitude = ~value;
    }
    int found = 0;
    int shift = 0, leading = 0;
    for (int i = 31; i > -1; i--) {
        if ((magnitude & (1 << i)) && !found) {
            shift = leading;
            found = 1;
        }
        leading++;
    }
    if (!magnitude) shift = singlePrecision ? 16 : 0;
    CpuS32 index = ((CpuS32)(magnitude << shift) >> 22) & 0x1ff;
    if (squareRoot) index = (index & 0x1fe) | (shift % 2) | 0x200;
    CpuU32 result = RSPReciprocalTable[index];
    if (squareRoot) shift = (~shift >> 1) & 15;
    else shift = ~shift & 31;
    result = (CpuS32)((result | 0x10000) << 14) >> shift;
    if (value < 0) result = ~result;
    if (!value) return 0x7fffffff;
    if (value == -32768) result = 0xffff0000;
    return result;
}

// Original function 00406900. The diagnostic path intentionally leaves the
// stream open, as in the original routine; normal execution selects the ROM.
void RSP_InitializeReciprocalTable()
{
    if (RSPUseReciprocalROM) {
        RSPReciprocalTable = RSPReciprocalROM;
        return;
    }
    RSPReciprocalTable = RSPComputedReciprocalROM;
    RSPComputedReciprocalROM[0] = 0x1ffff;
    int i, denominator;
    for (i = 1; i < 512; i++)
        RSPComputedReciprocalROM[i] = (int)(512.0f / (512 + i) * 131072.0f);
    RSPComputedReciprocalROM[513] = 0x1ffff;
    for (i = 1, denominator = 514; i < 256; i++, denominator += 2)
        RSPComputedReciprocalROM[513 + i * 2] = (int)((float)sqrt(512.0f / denominator) * 131072.0f);
    RSPComputedReciprocalROM[512] = (int)((float)sqrt(0.5) * 131072.0f);
    for (i = 1, denominator = 514; i < 256; i++, denominator += 2)
        RSPComputedReciprocalROM[512 + i * 2] = (int)((float)sqrt(512.0f / (denominator * 2.0f)) * 131072.0f);
    FILE *file = fopen("Test.txt", "wb");
    fprintf(file, "\n\n");
    for (i = 0; i < 1024; i++)
        fprintf(file, "divrom[%03x]=%04x \n", i, RSPReciprocalTable[i]);
    fprintf(file, "\n\n");
}
