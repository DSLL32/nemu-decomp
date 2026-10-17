/* Packet decoder source shared by the standalone entry and its command paths.
 * The macro boundary and names are inferred from their repeated native code.
 * The initial ESI/EDI preparation is retained in all three original expansions,
 * followed by the indexed C copy loop. It is an inferred assembly remnant.
 * The four-bit path uses signed integer assembly; the two-bit path uses C.
 * See audio-processing-provenance.json for the exact standalone-body proof. */
#ifndef NEMU_AUDIO_ADPCM_BLOCK_H
#define NEMU_AUDIO_ADPCM_BLOCK_H

#define DECODE_AUDIO_ADPCM_BLOCK(output, history, offset, book, bitsPerSample, context, tag) \
{ \
    unsigned char data[16]; \
    int bytes = bitsPerSample == 4 ? 9 : 5; \
    int i = 0; \
    int index = offset >> 1; \
    unsigned char *destination; \
    short *encoded; \
    short coefficient1, coefficient2, previous1, previous2; \
    int shift; \
    int packed; \
    int header; \
    int sampleIndex; \
    int bookIndex; \
    if (offset & 1) { \
        data[i++] = ((unsigned char *)context->buffer)[index * 2]; \
        ++index; \
    } \
    encoded = &context->buffer[index]; \
    destination = &data[i]; \
 \
    __asm { \
        __asm mov esi, encoded \
        __asm mov edi, destination \
    } \
    while (i < bytes) { \
        int pair = context->buffer[index++]; \
        data[i++] = pair >> 8; \
        data[i++] = pair; \
    } \
    header = data[0]; \
    shift = (header >> 4) + 11; \
    bookIndex = (header & 15) << 4; \
    coefficient1 = book[bookIndex]; \
    coefficient2 = book[bookIndex + 8]; \
    previous1 = history[14]; \
    previous2 = history[15]; \
    if (bitsPerSample == 4) { \
        short *decoded; \
        int pairs; \
        destination = data + 1; \
        decoded = output; \
 \
        __asm { \
        __asm mov esi, destination \
        __asm mov edi, decoded \
    } \
        pairs = 8; \
        __asm { \
        __asm tag##_next_pair: \
        __asm movzx eax, byte ptr [esi] \
        __asm shl eax, 24 \
        __asm mov ebx, eax \
        __asm mov ecx, shift \
        __asm shl ebx, 4 \
        __asm sar eax, 28 \
        __asm shl eax, cl \
        __asm mov ecx, eax \
        __asm inc esi \
        __asm movsx eax, previous1 \
        __asm movsx edx, coefficient1 \
        __asm imul edx \
        __asm add ecx, eax \
        __asm movsx eax, previous2 \
        __asm movsx edx, coefficient2 \
        __asm imul edx \
        __asm add ecx, eax \
        __asm sar ecx, 11 \
        __asm cmp ecx, 32767 \
        __asm jle tag##_first_below_maximum \
        __asm mov ecx, 32767 \
        __asm jmp tag##_first_clipped \
        __asm tag##_first_below_maximum: \
        __asm cmp ecx, -32767 \
        __asm jge tag##_first_clipped \
        __asm mov ecx, -32767 \
        __asm tag##_first_clipped: \
        __asm mov [edi], cx \
        __asm mov ax, previous2 \
        __asm mov previous2, cx \
        __asm mov previous1, ax \
        __asm mov ecx, shift \
        __asm sar ebx, 28 \
        __asm movsx eax, previous1 \
        __asm shl ebx, cl \
        __asm movsx edx, coefficient1 \
        __asm imul edx \
        __asm add ebx, eax \
        __asm movsx eax, previous2 \
        __asm movsx edx, coefficient2 \
        __asm imul edx \
        __asm add ebx, eax \
        __asm sar ebx, 11 \
        __asm cmp ebx, 32767 \
        __asm jle tag##_second_below_maximum \
        __asm mov ebx, 32767 \
        __asm jmp tag##_second_clipped \
        __asm tag##_second_below_maximum: \
        __asm cmp ebx, -32767 \
        __asm jge tag##_second_clipped \
        __asm mov ebx, -32767 \
        __asm tag##_second_clipped: \
        __asm mov [edi+2], bx \
        __asm mov ax, previous2 \
        __asm mov previous2, bx \
        __asm mov previous1, ax \
        __asm add edi, 4 \
        __asm dec pairs \
        __asm jnz tag##_next_pair \
    } \
    } else if (bitsPerSample == 2) { \
        sampleIndex = 1; \
        for (i = 0; i < 16; ++i) { \
            int sample; \
            if (!(i & 3)) packed = data[sampleIndex++] << 24; \
            else packed <<= 2; \
            sample = (previous1 * coefficient1 + previous2 * coefficient2 + \
                      ((packed >> 30) << shift)) >> 11; \
            if (sample < -32767) sample = -32767; \
            else if (sample > 32767) sample = 32767; \
            output[i] = sample; \
            previous1 = previous2; \
            previous2 = sample; \
        } \
    } else { \
        memset(output, 0, 32); \
    } \
}

#endif
