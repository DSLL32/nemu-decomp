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
