/* Original 00416F00. The filename and separate native-C unit are inferred;
   see analysis/nemu-memory.json for the code, calling-convention and compiler
   record evidence. Compile with VC7 /O2 /ML, without /GL.

   The explicit EBX save is in addition to the compiler's register save. */
void CopyAndSwapWords(void *destination, const void *source, unsigned byteCount)
{
    __asm {
        push ebx
        mov eax, destination
        mov ecx, source
        mov edx, ecx
        add edx, byteCount
    copy_word:
        mov ebx, [ecx]
        bswap ebx
        mov [eax], ebx
        add eax, 4
        add ecx, 4
        cmp ecx, edx
        jb copy_word
        pop ebx
    }
}
