/******************************************************************************

 Copyright (c) 1999 Advanced Micro Devices, Inc.

 LIMITATION OF LIABILITY:  THE MATERIALS ARE PROVIDED *AS IS* WITHOUT ANY
 EXPRESS OR IMPLIED WARRANTY OF ANY KIND INCLUDING WARRANTIES OF MERCHANTABILITY,
 NONINFRINGEMENT OF THIRD-PARTY INTELLECTUAL PROPERTY, OR FITNESS FOR ANY
 PARTICULAR PURPOSE.  IN NO EVENT SHALL AMD OR ITS SUPPLIERS BE LIABLE FOR ANY
 DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS OF PROFITS,
 BUSINESS INTERRUPTION, LOSS OF INFORMATION) ARISING OUT OF THE USE OF OR
 INABILITY TO USE THE MATERIALS, EVEN IF AMD HAS BEEN ADVISED OF THE POSSIBILITY
 OF SUCH DAMAGES.  BECAUSE SOME JURISDICTIONS PROHIBIT THE EXCLUSION OR LIMITATION
 OF LIABILITY FOR CONSEQUENTIAL OR INCIDENTAL DAMAGES, THE ABOVE LIMITATION MAY
 NOT APPLY TO YOU.

 AMD does not assume any responsibility for any errors which may appear in the
 Materials nor any responsibility to support or update the Materials.  AMD retains
 the right to make changes to its test specifications at any time, without notice.

 NO SUPPORT OBLIGATION: AMD is not obligated to furnish, support, or make any
 further information, software, technical information, know-how, or show-how
 available to you.

 So that all may benefit from your experience, please report  any  problems
 or  suggestions about this software to 3dsdk.support@amd.com

 AMD Developer Technologies, M/S 585
 Advanced Micro Devices, Inc.
 5900 E. Ben White Blvd.
 Austin, TX 78741
 3dsdk.support@amd.com

*******************************************************************************

 DETECT.C

 AMD3D 3D library code: Code to detect for 3DNow! capability.

*******************************************************************************/

#include "amdlib.h"

/* Unreferenced capability masks retained in the original native object. */
const unsigned long capability_masks[4] = {1, 2, 4, ~0UL};

typedef unsigned long DWORD;
static DWORD features = 0;
static DWORD ext_features = 0;
static DWORD processor = 0;

/* Reconstruction of the earlier AMD3D implementation at 0x10027950.
 * The preserved 1999 AMD sample supplies the CPU model switch and names.
 * The target caches three words, uses 13 query values, and reads the vendor
 * inside CPU_MFG. The newer sample's strings, SSE and cache queries are absent.
 */
DWORD GetCPUCaps(CPUCAPS cap)
{
    static int init = 0;
    DWORD res = 0;
    if (!init) {
        __asm {
            pushfd
            pop eax
            mov edx, eax
            xor eax, 0200000h
            push eax
            popfd
            pushfd
            pop eax
            xor eax, edx
            jnz foundit
            mov eax, -1
            jmp no_features
        foundit:
            mov eax, 1
            cpuid
            mov features, edx
            mov eax, 080000000h
            cpuid
            cmp eax, 080000001h
            jb no_features
            mov eax, 080000001h
            cpuid
            mov processor, eax
            mov ext_features, edx
            mov eax, 1
        no_features:
            mov init, eax
        }
    }
    if (init == -1) return 0;
    switch (cap) {
    case HAS_CPUID: res = 1; break;
    case CPU_MFG:
        __asm {
            mov eax, 0
            cpuid
            cmp ebx, 068747541h
            jne intel
            cmp edx, 069746e65h
            jne intel
            cmp ecx, 0444d4163h
            jne intel
            mov eax, 1
            jmp vendor_done
        intel:
            cmp ebx, 0756e6547h
            jne cyrix
            cmp edx, 049656e69h
            jne cyrix
            cmp ecx, 06c65746eh
            jne cyrix
            mov eax, 2
            jmp vendor_done
        cyrix:
            cmp ebx, 069727943h
            jne centaur
            cmp edx, 0736e4978h
            jne centaur
            cmp ecx, 064656174h
            jne centaur
            mov eax, 3
            jmp vendor_done
        centaur:
            cmp ebx, 0746e6543h
            jne unknown_vendor
            cmp edx, 048727561h
            jne unknown_vendor
            cmp ecx, 0736c7561h
            jne unknown_vendor
            mov eax, 4
            jmp vendor_done
        unknown_vendor:
            mov eax, 0
        vendor_done:
            mov res, eax
        }
        break;
    case CPU_TYPE:
        // Return a member of the CPUTYPES enumeration
        // Note: do NOT use this for determining presence of chip features, such
        // as MMX and 3DNow!  Instead, use GetCPUCaps (HAS_MMX) and GetCPUCaps (HAS_3DNOW),
        // which will accurately detect the presence of these features on all chips which
        // support them.
        switch (GetCPUCaps (CPU_MFG))
        {
        case VENDOR_AMD:
            switch ((processor >> 8) & 0xf) // extract family code
            {
            case 4: // Am486/AM5x86
                res = AMD_Am486;
                break;

            case 5: // K6
                switch ((processor >> 4) & 0xf) // extract model code
                {
                case 0: res = AMD_K5;       break;
                case 1: res = AMD_K5;       break;
                case 2: res = AMD_K5;       break;
                case 3: res = AMD_K5;       break;
                case 4: res = AMD_K6;       break;
                case 5: res = AMD_K6;       break;
                case 6: res = AMD_K6;       break;
                case 7: res = AMD_K6;       break;
                case 8: res = AMD_K6_2;     break;
                case 9: res = AMD_K6_3;     break;
                }
                break;

            case 6: // Athlon
                // No model numbers are currently defined
                res = AMD_ATHLON;
                break;
            }
            break;

        case VENDOR_INTEL:
            switch ((processor >> 8) & 0xf) // extract family code
            {
            case 4:
                switch ((processor >> 4) & 0xf) // extract model code
                {
                case 0: res = INTEL_486DX;  break;
                case 1: res = INTEL_486DX;  break;
                case 2: res = INTEL_486SX;  break;
                case 3: res = INTEL_486DX2; break;
                case 4: res = INTEL_486SL;  break;
                case 5: res = INTEL_486SX2; break;
                case 7: res = INTEL_486DX2E;break;
                case 8: res = INTEL_486DX4; break;
                }
                break;

            case 5:
                switch ((processor >> 4) & 0xf) // extract model code
                {
                case 1: res = INTEL_Pentium;    break;
                case 2: res = INTEL_Pentium;    break;
                case 3: res = INTEL_Pentium;    break;
                case 4: res = INTEL_Pentium_MMX;break;
                }
                break;

            case 6:
                switch ((processor >> 4) & 0xf) // extract model code
                {
                case 1: res = INTEL_Pentium_Pro;break;
                case 3: res = INTEL_Pentium_II; break;
                case 5: res = INTEL_Pentium_II; break;  // actual differentiation depends on cache settings
                case 6: res = INTEL_Celeron;    break;
                case 7: res = INTEL_Pentium_III;break;  // actual differentiation depends on cache settings
                }
                break;
            }
            break;

        case VENDOR_CYRIX:
            res = UNKNOWN;
            break;

        case VENDOR_CENTAUR:
            res = UNKNOWN;
            break;
        }
        break;


    case HAS_FPU: res = (features >> 0) & 1; break;
    case HAS_VME: res = (features >> 1) & 1; break;
    case HAS_DEBUG: res = (features >> 2) & 1; break;
    case HAS_PSE: res = (features >> 3) & 1; break;
    case HAS_TSC: res = (features >> 4) & 1; break;
    case HAS_MSR: res = (features >> 5) & 1; break;
    case HAS_PAE: res = (features >> 6) & 1; break;
    case HAS_MCE: res = (features >> 7) & 1; break;
    case HAS_MMX: res = (features >> 23) & 1; break;
    case HAS_3DNOW: res = (ext_features >> 31) & 1; break;
    }
    return res;
}
