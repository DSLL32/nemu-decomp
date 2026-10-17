/* get_feature_flags follows AMD's Processor Recognition Application Note
 * 20734, code example cpuid_ex.c (June 2000 and January 2002 editions).
 * Native code, vendor literals, feature masks, and both SEH filters identify
 * this source independently. The enclosing Nemu unit filename is inferred.
 */
#include "transform.h"
#include <excpt.h>

#define FEATURE_CPUID          0x00000001
#define FEATURE_STD_FEATURES   0x00000002
#define FEATURE_EXT_FEATURES   0x00000004
#define FEATURE_TSC            0x00000010
#define FEATURE_MMX            0x00000020
#define FEATURE_CMOV           0x00000040
#define FEATURE_3DNOW          0x00000080
#define FEATURE_3DNOWEXT        0x00000100
#define FEATURE_MMXEXT         0x00000200
#define FEATURE_SSEFP          0x00000400
#define FEATURE_K6_MTRR        0x00000800
#define FEATURE_P6_MTRR        0x00001000

unsigned int get_feature_flags(void)
{
    unsigned int result = 0;
    unsigned int signature = 0;
    char vendor[13] = "UnknownVendr";
    char vendorAMD[13] = "AuthenticAMD";

    __try {
        __asm xor eax, eax
        __asm xor ebx, ebx
        __asm xor ecx, ecx
        __asm xor edx, edx
        __asm cpuid
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
    result |= FEATURE_CPUID;
    __asm {
        xor eax, eax
        cpuid
        mov dword ptr [vendor], ebx
        mov dword ptr [vendor+4], edx
        mov dword ptr [vendor+8], ecx
        test eax, eax
        jz no_standard_features
        or [result], FEATURE_STD_FEATURES
        mov eax, 1
        cpuid
        mov [signature], eax
        mov ecx, 0x00000010
        and ecx, edx
        neg ecx
        sbb ecx, ecx
        and ecx, FEATURE_TSC
        or [result], ecx
        mov ecx, 0x00800000
        and ecx, edx
        neg ecx
        sbb ecx, ecx
        and ecx, FEATURE_MMX
        or [result], ecx
        mov ecx, 0x00008000
        and ecx, edx
        neg ecx
        sbb ecx, ecx
        and ecx, FEATURE_CMOV
        or [result], ecx
        mov ecx, 0x00001000
        and ecx, edx
        neg ecx
        sbb ecx, ecx
        and ecx, FEATURE_P6_MTRR
        or [result], ecx
        mov ecx, 0x02000000
        and ecx, edx
        neg ecx
        sbb ecx, ecx
        and ecx, FEATURE_MMXEXT+FEATURE_SSEFP
        or [result], ecx
        mov eax, 0x80000000
        cpuid
        cmp eax, 0x80000000
        jbe no_extended_features
        or [result], FEATURE_EXT_FEATURES
        mov eax, 0x80000001
        cpuid
        mov ecx, 0x80000000
        and ecx, edx
        neg ecx
        sbb ecx, ecx
        and ecx, FEATURE_3DNOW
        or [result], ecx
        lea esi, vendorAMD
        lea edi, vendor
        mov ecx, 12
        cld
        repe cmpsb
        jnz not_AMD
        mov ecx, 0x40000000
        and ecx, edx
        neg ecx
        sbb ecx, ecx
        and ecx, FEATURE_3DNOWEXT
        or [result], ecx
        test [result], FEATURE_MMXEXT
        jnz has_mmxext
        mov ecx, 0x00400000
        and ecx, edx
        neg ecx
        sbb ecx, ecx
        and ecx, FEATURE_MMXEXT
        or [result], ecx
    has_mmxext:
        mov eax, [signature]
        and eax, 0xfff
        cmp eax, 0x588
        sbb edx, edx
        not edx
        cmp eax, 0x600
        sbb ecx, ecx
        and ecx, edx
        and ecx, FEATURE_K6_MTRR
        or [result], ecx
        jmp all_done
    not_AMD:
    no_extended_features:
    no_standard_features:
    all_done:
    }
    if (result & FEATURE_SSEFP) {
        __try {
            __asm orps xmm0, xmm0
            return result;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            return result & ~FEATURE_SSEFP;
        }
    } else {
        return result;
    }
}

extern int Use3DNow;
extern int UseSSE;
extern int UseMMXExt;

void DetectCPUFeatures(void)
{
    unsigned int features = get_feature_flags();
    Use3DNow = features & FEATURE_3DNOW;
    UseMMXExt = features & FEATURE_MMXEXT;
    UseSSE = features & FEATURE_SSEFP;
}
