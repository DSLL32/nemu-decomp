/* MP3 fixed-point primitives. Filename inferred from the contiguous code group. */
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

short Mp3MultiplyFraction(short left, short right)
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

short Mp3AccumulateFraction(short first, short second,
                            unsigned short low, short high)
{
    __int64 accumulator = (second * high) * 2 + 0x8000;
    accumulator += ((__int64)(first * low)) & 0xffffff0000;
    return accumulator >> 16;
}

short Mp3AccumulateUnsigned(short first, short second,
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
