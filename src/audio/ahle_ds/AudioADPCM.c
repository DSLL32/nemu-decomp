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

__forceinline void DecodeAudioADPCMBlock(short *output, short *history, int offset,
                                   short *book, int bitsPerSample,
                                   AudioContext *context)
{
    unsigned char data[16];
    int bytes = bitsPerSample == 4 ? 9 : 5;
    int i = 0;
    int index = offset >> 1;
    unsigned char *destination;
    short *encoded;
    short coefficient1, coefficient2, previous1, previous2;
    int shift;
    int packed;
    int header;
    int sampleIndex;
    int bookIndex;
    if (offset & 1) {
        data[i++] = ((unsigned char *)context->buffer)[index * 2];
        ++index;
    }
    encoded = &context->buffer[index];
    destination = &data[i];
    /* All three native expansions retain this assembly copy preparation,
     * followed by the C copy loop. The prepared EDI value is unused there;
     * this is an inferred original assembly remnant, not alignment padding. */
    __asm {
        mov esi, encoded
        mov edi, destination
    }
    while (i < bytes) {
        int pair = context->buffer[index++];
        data[i++] = pair >> 8;
        data[i++] = pair;
    }
    header = data[0];
    shift = (header >> 4) + 11;
    bookIndex = (header & 15) << 4;
    coefficient1 = book[bookIndex];
    coefficient2 = book[bookIndex + 8];
    previous1 = history[14];
    previous2 = history[15];
    if (bitsPerSample == 4) {
        short *decoded;
        int pairs;
        destination = data + 1;
        decoded = output;
        /* The original four-bit kernel uses integer assembly. Each signed
         * nibble is scaled, predicted from the two preceding samples, then
         * clipped before it becomes the next predictor input. */
        __asm {
            mov esi, destination
            mov edi, decoded
        }
        pairs = 8;
        __asm {
        next_pair:
            movzx eax, byte ptr [esi]
            shl eax, 24
            mov ebx, eax
            mov ecx, shift
            shl ebx, 4
            sar eax, 28
            shl eax, cl
            mov ecx, eax
            inc esi
            movsx eax, previous1
            movsx edx, coefficient1
            imul edx
            add ecx, eax
            movsx eax, previous2
            movsx edx, coefficient2
            imul edx
            add ecx, eax
            sar ecx, 11
            cmp ecx, 32767
            jle first_below_maximum
            mov ecx, 32767
            jmp first_clipped
        first_below_maximum:
            cmp ecx, -32767
            jge first_clipped
            mov ecx, -32767
        first_clipped:
            mov [edi], cx
            mov ax, previous2
            mov previous2, cx
            mov previous1, ax
            mov ecx, shift
            sar ebx, 28
            movsx eax, previous1
            shl ebx, cl
            movsx edx, coefficient1
            imul edx
            add ebx, eax
            movsx eax, previous2
            movsx edx, coefficient2
            imul edx
            add ebx, eax
            sar ebx, 11
            cmp ebx, 32767
            jle second_below_maximum
            mov ebx, 32767
            jmp second_clipped
        second_below_maximum:
            cmp ebx, -32767
            jge second_clipped
            mov ebx, -32767
        second_clipped:
            mov [edi+2], bx
            mov ax, previous2
            mov previous2, bx
            mov previous1, ax
            add edi, 4
            dec pairs
            jnz next_pair
        }
    } else if (bitsPerSample == 2) {
        sampleIndex = 1;
        for (i = 0; i < 16; ++i) {
            int sample;
            if (!(i & 3)) packed = data[sampleIndex++] << 24;
            else packed <<= 2;
            sample = (previous1 * coefficient1 + previous2 * coefficient2 +
                      ((packed >> 30) << shift)) >> 11;
            if (sample < -32767) sample = -32767;
            else if (sample > 32767) sample = 32767;
            output[i] = sample;
            previous1 = previous2;
            previous2 = sample;
        }
    } else {
        memset(output, 0, 32);
    }
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
            DecodeAudioADPCMBlock(destination, history, input,
                                 &context->buffer[0x800], 2, context);
            history = destination;
            input += 5;
            output += 16;
            --blocks;
        }
    } else {
        while (blocks) {
            short *destination = &context->buffer[output];
            DecodeAudioADPCMBlock(destination, history, input,
                                 &context->buffer[0x800], 4, context);
            history = destination;
            input += 9;
            output += 16;
            --blocks;
        }
    }
    SaveAudioHistory(output * 2 - 32, address, flags, context);
}
