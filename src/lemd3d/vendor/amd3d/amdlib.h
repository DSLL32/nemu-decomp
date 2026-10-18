/* Earlier AMD3D query interface, reconstructed from the linked switch.
 * CPU type names follow AMD's preserved adetect.h. See README.md. */
#ifndef LEM_AMDLIB_H
#define LEM_AMDLIB_H
#define MFG_AMD 1
#define MFG_INTEL 2
#define MFG_CYRIX 3
#define MFG_CENTAUR 4
#define VENDOR_AMD MFG_AMD
#define VENDOR_INTEL MFG_INTEL
#define VENDOR_CYRIX MFG_CYRIX
#define VENDOR_CENTAUR MFG_CENTAUR
// Detected CPU models - returned by GetCPUCaps (CPU_TYPE);
typedef enum CPU_TYPES
{
    UNKNOWN,
    AMD_Am486,
    AMD_K5,
    AMD_K6,
    AMD_K6_2,
    AMD_K6_3,
    AMD_ATHLON,

    INTEL_486DX,
    INTEL_486SX,
    INTEL_486DX2,
    INTEL_486SL,
    INTEL_486SX2,
    INTEL_486DX2E,
    INTEL_486DX4,
    INTEL_Pentium,
    INTEL_Pentium_MMX,
    INTEL_Pentium_Pro,
    INTEL_Pentium_II,
    INTEL_Celeron,
    INTEL_Pentium_III,
} CPU_TYPES;



typedef enum CPUCAPS {
    CPU_MFG, CPU_TYPE, HAS_CPUID, HAS_FPU, HAS_VME, HAS_DEBUG,
    HAS_PSE, HAS_TSC, HAS_MSR, HAS_PAE, HAS_MCE, HAS_MMX, HAS_3DNOW
} CPUCAPS;
#ifdef __cplusplus
extern "C" {
#endif
unsigned long GetCPUCaps(CPUCAPS capability);
#ifdef __cplusplus
}
#endif
#endif
