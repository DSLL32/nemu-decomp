// Reconstructed interpreter COP1 handlers. Names are inferred.
// Original code: 0040BFB0..0040C9A0. Relocation-masked comparison is tracked separately.
#include "CPU.h"
#include <math.h>

// The binary always pairs adjacent 32-bit FPRs for 64-bit values.
// Byte-sized decoder results preserve the original narrowing and paired shifts.
#define FPU_FMT ((CpuU8)((CPU_Instruction >> 21) & 31))
#define FPU_FT ((CpuU8)((CPU_Instruction >> 16) & 31))
#define FPU_FS ((CpuU8)((CPU_Instruction >> 11) & 31))
#define FPU_FD ((CpuU8)((CPU_Instruction >> 6) & 31))

// 0040BFB0
void Op_MFC1()
{
    CPU_GPR[FPU_FT] = (CpuS32)CPU_FPR.words[FPU_FS];
}

// 0040BFE0
void Op_DMFC1()
{
    CPU_GPR[FPU_FT] = CPU_FPR.longs[FPU_FS >> 1];
}

// 0040C020
void Op_CFC1()
{
    switch (FPU_FS) {
    case 0:
        CPU_GPR[FPU_FT] = 0xA00;
        break;
    case 31:
        CPU_GPR[FPU_FT] = (CpuS32)CPU_FCR31;
        break;
    }
}

// 0040C070
void Op_MTC1()
{
    CPU_FPR.words[FPU_FS] = (CpuU32)CPU_GPR[FPU_FT];
}

// 0040C0A0
void Op_DMTC1()
{
    CPU_FPR.longs[FPU_FS >> 1] = CPU_GPR[FPU_FT];
}

// 0040C0E0
void Op_CTC1()
{
    if (FPU_FS == 31)
        CPU_FCR31 = (CpuU32)CPU_GPR[FPU_FT];
}

// The branch decoder uses a packed I-format view with a 16-bit immediate
// followed by unsigned-int bitfields. This accounts for the original masked
// DWORD access at instruction + 2; other FPU fields use byte-sized decoders.
#pragma pack(push, 1)
struct CPUImmediateInstructionFields {
    CpuU16 immediate;
    CpuU32 rt : 5;
    CpuU32 rs : 5;
    CpuU32 op : 6;
};
#pragma pack(pop)

// 0040C110
void Op_BC1()
{
    CpuU32 condition = ((CPUImmediateInstructionFields *)&CPU_Instruction)->rt;
    bool likely = condition == 2 || condition == 3;
    bool branchFalse = condition == 0 || condition == 2;
    if (((CPU_FCR31 >> 23) & 1) ^ branchFalse) {
        ExecuteDelaySlot();
        CPU_PC += (short)CPU_Instruction * 4;
    } else if (likely) {
        CPU_PC += 4;
    }
}

// 0040C180
void Op_COP1Function()
{
    CPU_Cop1FunctionDispatch[(CpuU8)CPU_Instruction & 63]();
}

// 0040C1A0
void Op_FADD()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = CPU_FPR.single[FPU_FS] + CPU_FPR.single[FPU_FT]; break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = CPU_FPR.dbl[FPU_FS >> 1] + CPU_FPR.dbl[FPU_FT >> 1]; break;
    }
}

// 0040C220
void Op_FSUB()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = CPU_FPR.single[FPU_FS] - CPU_FPR.single[FPU_FT]; break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = CPU_FPR.dbl[FPU_FS >> 1] - CPU_FPR.dbl[FPU_FT >> 1]; break;
    }
}

// 0040C2A0
void Op_FMUL()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = CPU_FPR.single[FPU_FS] * CPU_FPR.single[FPU_FT]; break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = CPU_FPR.dbl[FPU_FS >> 1] * CPU_FPR.dbl[FPU_FT >> 1]; break;
    }
}

// 0040C320
void Op_FDIV()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = CPU_FPR.single[FPU_FS] / CPU_FPR.single[FPU_FT]; break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = CPU_FPR.dbl[FPU_FS >> 1] / CPU_FPR.dbl[FPU_FT >> 1]; break;
    }
}

// 0040C3A0
void Op_FSQRT()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = (float)sqrt(CPU_FPR.single[FPU_FS]); break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = sqrt(CPU_FPR.dbl[FPU_FS >> 1]); break;
    }
}

// 0040C400
void Op_FABS()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = (float)fabs(CPU_FPR.single[FPU_FS]); break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = fabs(CPU_FPR.dbl[FPU_FS >> 1]); break;
    }
}

// 0040C460. Integer aliases preserve the original bitwise MOV operations.
void Op_FMOV()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.words[FPU_FD] = CPU_FPR.words[FPU_FS]; break;
    case 17: CPU_FPR.longs[FPU_FD >> 1] = CPU_FPR.longs[FPU_FS >> 1]; break;
    }
}

// 0040C4D0
void Op_FNEG()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = -CPU_FPR.single[FPU_FS]; break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = -CPU_FPR.dbl[FPU_FS >> 1]; break;
    }
}

// 0040C530. ROUND.L, TRUNC.L, CEIL.L and FLOOR.L all share this handler.
// The input uses the ordinary compiler float-to-integer conversion for all four.
void Op_ROUND_L()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.longs[FPU_FD >> 1] = (CpuS64)CPU_FPR.single[FPU_FS]; break;
    case 17: CPU_FPR.longs[FPU_FD >> 1] = (CpuS64)CPU_FPR.dbl[FPU_FS >> 1]; break;
    }
}

// 0040C590. ROUND.W, TRUNC.W, CEIL.W and FLOOR.W likewise share one handler.
void Op_ROUND_W()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.words[FPU_FD] = (CpuS32)CPU_FPR.single[FPU_FS]; break;
    case 17: CPU_FPR.words[FPU_FD] = (CpuS32)CPU_FPR.dbl[FPU_FS >> 1]; break;
    }
}

// 0040C5F0
void Op_CVT_S()
{
    switch (FPU_FMT) {
    case 17: CPU_FPR.single[FPU_FD] = (float)CPU_FPR.dbl[FPU_FS >> 1]; break;
    case 20: CPU_FPR.single[FPU_FD] = (float)(CpuS32)CPU_FPR.words[FPU_FS]; break;
    case 21: CPU_FPR.single[FPU_FD] = (float)CPU_FPR.longs[FPU_FS >> 1]; break;
    }
}

// 0040C670
void Op_CVT_D()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.dbl[FPU_FD >> 1] = CPU_FPR.single[FPU_FS]; break;
    case 20: CPU_FPR.dbl[FPU_FD >> 1] = (CpuS32)CPU_FPR.words[FPU_FS]; break;
    // Original CVT.D.L narrows the source to signed 32 bits before conversion.
    case 21: CPU_FPR.dbl[FPU_FD >> 1] = (CpuS32)CPU_FPR.longs[FPU_FS >> 1]; break;
    }
}

// 0040C6F0
void Op_CVT_W()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.words[FPU_FD] = (CpuS32)CPU_FPR.single[FPU_FS]; break;
    case 17: CPU_FPR.words[FPU_FD] = (CpuS32)CPU_FPR.dbl[FPU_FS >> 1]; break;
    case 21: CPU_FPR.words[FPU_FD] = (CpuU32)CPU_FPR.longs[FPU_FS >> 1]; break;
    }
}

// 0040C780
void Op_CVT_L()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.longs[FPU_FD >> 1] = (CpuS64)CPU_FPR.single[FPU_FS]; break;
    case 17: CPU_FPR.longs[FPU_FD >> 1] = (CpuS64)CPU_FPR.dbl[FPU_FS >> 1]; break;
    // Original CVT.L.W zero-extends its 32-bit input.
    case 20: CPU_FPR.longs[FPU_FD >> 1] = CPU_FPR.words[FPU_FS]; break;
    }
}

// 0040C810. Also shared by C.UEQ, C.SEQ and C.NGL.
void Op_C_EQ()
{
    switch (FPU_FMT) {
    case 16:
        if (CPU_FPR.single[FPU_FS] == CPU_FPR.single[FPU_FT]) CPU_FCR31 |= 0x800000;
        else CPU_FCR31 &= ~0x800000;
        break;
    case 17:
        if (CPU_FPR.dbl[FPU_FS >> 1] == CPU_FPR.dbl[FPU_FT >> 1]) CPU_FCR31 |= 0x800000;
        else CPU_FCR31 &= ~0x800000;
        break;
    }
}

// 0040C890. Also shared by C.ULT, C.LT and C.NGE.
void Op_C_LT()
{
    switch (FPU_FMT) {
    case 16:
        if (CPU_FPR.single[FPU_FS] < CPU_FPR.single[FPU_FT]) CPU_FCR31 |= 0x800000;
        else CPU_FCR31 &= ~0x800000;
        break;
    case 17:
        if (CPU_FPR.dbl[FPU_FS >> 1] < CPU_FPR.dbl[FPU_FT >> 1]) CPU_FCR31 |= 0x800000;
        else CPU_FCR31 &= ~0x800000;
        break;
    }
}

// 0040C910. Also shared by C.UN, C.SF and C.NGLE, for every format.
void Op_C_F()
{
    CPU_FCR31 &= ~0x800000;
}

// 0040C920. Also shared by C.ULE, C.LE and C.NGT.
void Op_C_LE()
{
    switch (FPU_FMT) {
    case 16:
        if (CPU_FPR.single[FPU_FS] <= CPU_FPR.single[FPU_FT]) CPU_FCR31 |= 0x800000;
        else CPU_FCR31 &= ~0x800000;
        break;
    case 17:
        if (CPU_FPR.dbl[FPU_FS >> 1] <= CPU_FPR.dbl[FPU_FT >> 1]) CPU_FCR31 |= 0x800000;
        else CPU_FCR31 &= ~0x800000;
        break;
    }
}

#undef FPU_FMT
#undef FPU_FT
#undef FPU_FS
#undef FPU_FD


