#include "DynarecFPU.h"
#include "Dynarec.h"
#include "CPU.h"
#include "CpuControl.h"
#include "Plugins.h"

// Names and this source-file boundary are inferred. Register arguments and
// original function addresses are recorded in analysis/nemu-dynarec-fpu.json.
unsigned Dynarec_DynamicFpuMode;
unsigned Dynarec_FpuChecked;
unsigned Dynarec_FpuEndLabel, Dynarec_FpuModeLabel;

static unsigned short FpuTruncateInteger = 0x0f3f;
static unsigned short FpuTruncateSingle = 0x0c3f;
static unsigned short FpuTruncateDouble = 0x0e3f;
static unsigned short FpuNearestInteger = 0x033f;
static unsigned short FpuNearestSingle = 0x003f;
static unsigned short FpuNearestDouble = 0x023f;
static unsigned short FpuFloorInteger = 0x073f;
static unsigned short FpuFloorSingle = 0x043f;
static unsigned short FpuFloorDouble = 0x063f;
static unsigned short FpuCeilInteger = 0x0b3f;
static unsigned short FpuCeilSingle = 0x083f;
static unsigned short FpuCeilDouble = 0x0a3f;

#define FMT ((CPU_Instruction >> 21) & 31)
#define FT ((CPU_Instruction >> 16) & 31)
#define FS ((CPU_Instruction >> 11) & 31)
#define FD ((CPU_Instruction >> 6) & 31)

// These three expansions preserve the repeated FR-mode guard in each handler.
#define BEGIN_FPU_MODE() \
    if (Dynarec_DynamicFpuMode) { \
        EmitLoad(1, X86_EBX, 5, (unsigned)&CPU_FRMode); \
        EmitCompareImmediate(1, X86_EBX, 1); \
        Dynarec_FpuModeLabel = EmitConditionalJump(5); \
    }
#define NEXT_FPU_MODE() \
    if (Dynarec_DynamicFpuMode) { \
        Dynarec_FpuEndLabel = EmitJump(); \
        EmitLabel(Dynarec_FpuModeLabel); \
    }
#define END_FPU_MODE() \
    if (Dynarec_DynamicFpuMode) EmitLabel(Dynarec_FpuEndLabel)

// 004111C0.
void Recompile_MFC1()
{
    CheckFPUEnabled();
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        EmitLoad(1, X86_EAX, 5, (unsigned)&CPU_FPR64[FS]);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        EmitLoad(1, X86_EAX, 5, (unsigned)&CPU_FPR.words[FS]);
    }
    END_FPU_MODE();
    StoreGPRSigned32(FT);
}

// 004112A0.
void Recompile_DMFC1()
{
    CheckFPUEnabled();
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        EmitLoad(1, X86_EAX, 5, (unsigned)&CPU_FPR64[FS]);
        EmitLoad(1, X86_EDX, 5, (unsigned)&CPU_FPR64[FS] + 4);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        EmitLoad(1, X86_EAX, 5, (unsigned)&CPU_FPR.words[FS & 30]);
        EmitLoad(1, X86_EDX, 5, (unsigned)&CPU_FPR.words[((unsigned char)FS & 30) | 1]);
    }
    END_FPU_MODE();
    StoreGPR64(FT);
}

// 004113D0.
void Recompile_CFC1()
{
    CheckFPUEnabled();
    switch ((unsigned char)FS) {
    case 0:
        EmitMoveImmediate(1, X86_EAX, 0x0a00);
        break;
    case 31:
        EmitLoad(1, X86_EAX, 5, (unsigned)&CPU_FCR31);
        break;
    }
    StoreGPRSigned32(FT);
}

// 00411430.
void Recompile_MTC1()
{
    CheckFPUEnabled();
    LoadGPRLow(FT);
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        EmitStore(1, X86_EAX, 5, (unsigned)&CPU_FPR64[FS]);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        EmitStore(1, X86_EAX, 5, (unsigned)&CPU_FPR.words[FS]);
    }
    END_FPU_MODE();
}

// 00411520.
void Recompile_DMTC1()
{
    CheckFPUEnabled();
    LoadGPR64(FT);
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        EmitStore(1, X86_EAX, 5, (unsigned)&CPU_FPR64[FS]);
        EmitStore(1, X86_EDX, 5, (unsigned)&CPU_FPR64[FS] + 4);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        EmitStore(1, X86_EAX, 5, (unsigned)&CPU_FPR.words[FS & 30]);
        EmitStore(1, X86_EDX, 5, (unsigned)&CPU_FPR.words[((unsigned char)FS & 30) | 1]);
    }
    END_FPU_MODE();
}

// 00411640.
void Recompile_CTC1()
{
    CheckFPUEnabled();
    LoadGPRLow(FT);
    if ((unsigned char)FS == 31) {
        EmitStore(1, X86_EAX, 5, (unsigned)&CPU_FCR31);
        EmitPush(X86_EAX);
        EmitCall((unsigned)SetFpuRounding);
    }
}

// 00411690.
void Recompile_COP1_BC()
{
    CheckFPUEnabled();
    // The original reads the opcode field as an unaligned DWORD at byte 2.
    unsigned operation = *(unsigned *)((unsigned char *)&CPU_Instruction + 2) & 31;
    unsigned char likely = operation == 2 || operation == 3;
    unsigned char condition = operation == 0 || operation == 2 ? 5 : 4;
    EmitTestMemory(1, 5, (unsigned)&CPU_FCR31, 0x00800000);
    unsigned notTaken = EmitConditionalJump(condition);
    RecompileDelaySlot();
    EmitBranchTarget(CPU_LastPC + (short)CPU_Instruction * 4 + 4);
    EmitLabel(notTaken);
    if (likely)
        EmitBranchTarget(CPU_LastPC + 8);
    else
        EmitBranchTarget(CPU_LastPC + 4);
    Dynarec_BlockFinished = 1;
}

// 00411740.
void Recompile_COP1_S()
{
    CheckFPUEnabled();
    if (OpcodeStatsEnabled)
        EmitAddMemory(1, 0, 5, (unsigned)&OpcodeStats[0x120 + ((unsigned char)CPU_Instruction & 63)], 1);
    Dynarec_Cop1FunctionDispatch[(unsigned char)CPU_Instruction & 63]();
}

// 00411790.
void Recompile_COP1_D()
{
    CheckFPUEnabled();
    if (OpcodeStatsEnabled)
        EmitAddMemory(1, 0, 5, (unsigned)&OpcodeStats[0x160 + ((unsigned char)CPU_Instruction & 63)], 1);
    Dynarec_Cop1FunctionDispatch[(unsigned char)CPU_Instruction & 63]();
}

// 004117E0.
void Recompile_COP1_W()
{
    CheckFPUEnabled();
    if (OpcodeStatsEnabled)
        EmitAddMemory(1, 0, 5, (unsigned)&OpcodeStats[0x1a0 + ((unsigned char)CPU_Instruction & 63)], 1);
    Dynarec_Cop1FunctionDispatch[(unsigned char)CPU_Instruction & 63]();
}

// 00411830.
void Recompile_COP1_L()
{
    CheckFPUEnabled();
    if (OpcodeStatsEnabled)
        EmitAddMemory(1, 0, 5, (unsigned)&OpcodeStats[0x1e0 + ((unsigned char)CPU_Instruction & 63)], 1);
    Dynarec_Cop1FunctionDispatch[(unsigned char)CPU_Instruction & 63]();
}

// 00411880.
void Recompile_COP1_ADD()
{
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        switch (FMT) {
        case 16:
            EmitFpuAddSingle(GetFpuRegisterAddress(FT, 16, 1));
            break;
        case 17:
            EmitFpuAddDouble(GetFpuRegisterAddress(FT, 17, 1));
            break;
        }
        StoreFpuResult(1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        switch (FMT) {
        case 16:
            EmitFpuAddSingle(GetFpuRegisterAddress(FT, 16, 0));
            break;
        case 17:
            EmitFpuAddDouble(GetFpuRegisterAddress(FT, 17, 0));
            break;
        }
        StoreFpuResult(0);
    }
    END_FPU_MODE();
}

// 004119C0.
void Recompile_COP1_SUB()
{
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        switch (FMT) {
        case 16:
            EmitFpuSubtractSingle(GetFpuRegisterAddress(FT, 16, 1));
            break;
        case 17:
            EmitFpuSubtractDouble(GetFpuRegisterAddress(FT, 17, 1));
            break;
        }
        StoreFpuResult(1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        switch (FMT) {
        case 16:
            EmitFpuSubtractSingle(GetFpuRegisterAddress(FT, 16, 0));
            break;
        case 17:
            EmitFpuSubtractDouble(GetFpuRegisterAddress(FT, 17, 0));
            break;
        }
        StoreFpuResult(0);
    }
    END_FPU_MODE();
}

// 00411B00.
void Recompile_COP1_MUL()
{
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        switch (FMT) {
        case 16:
            EmitFpuMultiplySingle(GetFpuRegisterAddress(FT, 16, 1));
            break;
        case 17:
            EmitFpuMultiplyDouble(GetFpuRegisterAddress(FT, 17, 1));
            break;
        }
        StoreFpuResult(1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        switch (FMT) {
        case 16:
            EmitFpuMultiplySingle(GetFpuRegisterAddress(FT, 16, 0));
            break;
        case 17:
            EmitFpuMultiplyDouble(GetFpuRegisterAddress(FT, 17, 0));
            break;
        }
        StoreFpuResult(0);
    }
    END_FPU_MODE();
}

// 00411C40.
void Recompile_COP1_DIV()
{
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        switch (FMT) {
        case 16:
            EmitFpuDivideSingle(GetFpuRegisterAddress(FT, 16, 1));
            break;
        case 17:
            EmitFpuDivideDouble(GetFpuRegisterAddress(FT, 17, 1));
            break;
        }
        StoreFpuResult(1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        switch (FMT) {
        case 16:
            EmitFpuDivideSingle(GetFpuRegisterAddress(FT, 16, 0));
            break;
        case 17:
            EmitFpuDivideDouble(GetFpuRegisterAddress(FT, 17, 0));
            break;
        }
        StoreFpuResult(0);
    }
    END_FPU_MODE();
}

// 00411D80.
void Recompile_COP1_SQRT()
{
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        EmitFpuSquareRoot();
        StoreFpuResult(1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        EmitFpuSquareRoot();
        StoreFpuResult(0);
    }
    END_FPU_MODE();
}

// 00411E40.
void Recompile_COP1_ABS()
{
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        EmitFpuAbsolute();
        StoreFpuResult(1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        EmitFpuAbsolute();
        StoreFpuResult(0);
    }
    END_FPU_MODE();
}

// 00411F00.
void Recompile_COP1_MOV()
{
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        StoreFpuResult(1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        StoreFpuResult(0);
    }
    END_FPU_MODE();
}

// 00411FB0.
void Recompile_COP1_NEG()
{
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        EmitFpuChangeSign();
        StoreFpuResult(1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        EmitFpuChangeSign();
        StoreFpuResult(0);
    }
    END_FPU_MODE();
}

// 00412070.
void Recompile_COP1_ROUND_L()
{
    FpuRoundingControl(21, 3);
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        StoreFpuResultAs(21, 1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        StoreFpuResultAs(21, 0);
    }
    END_FPU_MODE();
    FpuRoundingControl(21, 1);
}

// 00412140.
void Recompile_COP1_TRUNC_L()
{
    FpuRoundingControl(21, 2);
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        StoreFpuResultAs(21, 1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        StoreFpuResultAs(21, 0);
    }
    END_FPU_MODE();
    FpuRoundingControl(21, 1);
}

// 00412210.
void Recompile_COP1_CEIL_L()
{
    FpuRoundingControl(21, 5);
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        StoreFpuResultAs(21, 1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        StoreFpuResultAs(21, 0);
    }
    END_FPU_MODE();
    FpuRoundingControl(21, 1);
}

// 004122E0.
void Recompile_COP1_FLOOR_L()
{
    FpuRoundingControl(21, 4);
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        StoreFpuResultAs(21, 1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        StoreFpuResultAs(21, 0);
    }
    END_FPU_MODE();
    FpuRoundingControl(21, 1);
}

// 004123B0.
void Recompile_COP1_ROUND_W()
{
    FpuRoundingControl(20, 3);
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        StoreFpuResultAs(20, 1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        StoreFpuResultAs(20, 0);
    }
    END_FPU_MODE();
    FpuRoundingControl(21, 1);
}

// 00412480.
void Recompile_COP1_TRUNC_W()
{
    FpuRoundingControl(20, 2);
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        StoreFpuResultAs(20, 1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        StoreFpuResultAs(20, 0);
    }
    END_FPU_MODE();
    FpuRoundingControl(21, 1);
}

// 00412550.
void Recompile_COP1_CEIL_W()
{
    FpuRoundingControl(20, 5);
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        StoreFpuResultAs(20, 1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        StoreFpuResultAs(20, 0);
    }
    END_FPU_MODE();
    FpuRoundingControl(21, 1);
}

// 00412620.
void Recompile_COP1_FLOOR_W()
{
    FpuRoundingControl(20, 4);
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        StoreFpuResultAs(20, 1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        StoreFpuResultAs(20, 0);
    }
    END_FPU_MODE();
    FpuRoundingControl(21, 1);
}

// 004126F0.
void Recompile_COP1_CVT_S()
{
    FpuRoundingControl(16, 1);
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        StoreFpuResultAs(16, 1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        StoreFpuResultAs(16, 0);
    }
    END_FPU_MODE();
    FpuRoundingControl(21, 1);
}

// 004127C0.
void Recompile_COP1_CVT_D()
{
    FpuRoundingControl(17, 1);
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        StoreFpuResultAs(17, 1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        StoreFpuResultAs(17, 0);
    }
    END_FPU_MODE();
    FpuRoundingControl(21, 1);
}

// 00412890.
void Recompile_COP1_CVT_W()
{
    FpuRoundingControl(20, 1);
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        StoreFpuResultAs(20, 1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        StoreFpuResultAs(20, 0);
    }
    END_FPU_MODE();
    FpuRoundingControl(21, 1);
}

// 00412960.
void Recompile_COP1_CVT_L()
{
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        StoreFpuResultAs(21, 1);
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        StoreFpuResultAs(21, 0);
    }
    END_FPU_MODE();
}

// 00412A20.
void Recompile_COP1_C_F()
{
    EmitAndMemory(1, 5, (unsigned)&CPU_FCR31, 0xff7fffff);
}

// 00412A40.
void Recompile_COP1_C_UN()
{
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        switch (FMT) {
        case 16:
            EmitFpuCompareSingle(GetFpuRegisterAddress(FT, 16, 1));
            break;
        case 17:
            EmitFpuCompareDouble(GetFpuRegisterAddress(FT, 17, 1));
            break;
        }
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        switch (FMT) {
        case 16:
            EmitFpuCompareSingle(GetFpuRegisterAddress(FT, 16, 0));
            break;
        case 17:
            EmitFpuCompareDouble(GetFpuRegisterAddress(FT, 17, 0));
            break;
        }
    }
    END_FPU_MODE();
    EmitFpuStoreStatus();
    EmitStoreFlags();
    unsigned unordered = EmitConditionalJump(10);
    EmitAndMemory(1, 5, (unsigned)&CPU_FCR31, 0xff7fffff);
    unsigned done = EmitJump();
    EmitLabel(unordered);
    EmitOrMemory(1, 5, (unsigned)&CPU_FCR31, 0x00800000);
    EmitLabel(done);
}

// 00412BD0.
void Recompile_COP1_C_SF()
{
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        switch (FMT) {
        case 16:
            EmitFpuCompareSingle(GetFpuRegisterAddress(FT, 16, 1));
            break;
        case 17:
            EmitFpuCompareDouble(GetFpuRegisterAddress(FT, 17, 1));
            break;
        }
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        switch (FMT) {
        case 16:
            EmitFpuCompareSingle(GetFpuRegisterAddress(FT, 16, 0));
            break;
        case 17:
            EmitFpuCompareDouble(GetFpuRegisterAddress(FT, 17, 0));
            break;
        }
    }
    END_FPU_MODE();
    EmitFpuStoreStatus();
    EmitStoreFlags();
    unsigned ordered = EmitConditionalJump(11);
    EmitLabel(ordered);
    EmitAndMemory(1, 5, (unsigned)&CPU_FCR31, 0xff7fffff);
}

// 00412D30.
void Recompile_COP1_C_NGLE()
{
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        switch (FMT) {
        case 16:
            EmitFpuCompareSingle(GetFpuRegisterAddress(FT, 16, 1));
            break;
        case 17:
            EmitFpuCompareDouble(GetFpuRegisterAddress(FT, 17, 1));
            break;
        }
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        switch (FMT) {
        case 16:
            EmitFpuCompareSingle(GetFpuRegisterAddress(FT, 16, 0));
            break;
        case 17:
            EmitFpuCompareDouble(GetFpuRegisterAddress(FT, 17, 0));
            break;
        }
    }
    END_FPU_MODE();
    EmitFpuStoreStatus();
    EmitStoreFlags();
    unsigned unordered = EmitConditionalJump(10);
    EmitAndMemory(1, 5, (unsigned)&CPU_FCR31, 0xff7fffff);
    unsigned done = EmitJump();
    EmitLabel(unordered);
    EmitLabel(done);
}

// 00412EA0.
void Recompile_COP1_C_EQ()
{
    RecompileFpuCompare(4);
}

// 00412EB0.
void Recompile_COP1_C_OLT()
{
    RecompileFpuCompare(2);
}

// 00412EC0.
void Recompile_COP1_C_OLE()
{
    RecompileFpuCompare(6);
}

// 004133F0.
void CheckFPUEnabled()
{
    if (!Dynarec_FpuChecked) {
        Dynarec_FpuChecked = 1;
        EmitTestMemory(1, 5, (unsigned)&CPU_COP0[CP0_Status], 0x20000000);
        unsigned enabled = EmitConditionalJump(5);
        EmitOrMemory(1, 5, (unsigned)&CPU_ActionFlags, 0x02000000);
        if (Dynarec_DelaySlot) {
            EmitStoreImmediate(1, 5, (unsigned)&CPU_Cop1UnusableDelay, 1);
            EmitStoreImmediate(1, 5, (unsigned)&CPU_PC, CPU_LastPC - 4);
        } else {
            EmitStoreImmediate(1, 5, (unsigned)&CPU_Cop1UnusableDelay, 0);
            EmitStoreImmediate(1, 5, (unsigned)&CPU_PC, CPU_LastPC);
        }
        FinishInstruction();
        EmitReturn();
        EmitLabel(enabled);
    }
}

// 004134A0.
void LoadFpuOperand(unsigned char fullMode)
{
    if (fullMode) {
        switch (FMT) {
        case 16: EmitFpuLoadSingle((unsigned)&CPU_FPR64[FS]); break;
        case 17: EmitFpuLoadDouble((unsigned)&CPU_FPR64[FS]); break;
        case 20: EmitFpuLoadInt32((unsigned)&CPU_FPR64[FS]); break;
        case 21: EmitFpuLoadInt64((unsigned)&CPU_FPR64[FS]); break;
        }
    } else {
        switch (FMT) {
        case 16: EmitFpuLoadSingle((unsigned)&CPU_FPR.words[FS]); break;
        case 17: EmitFpuLoadDouble((unsigned)&CPU_FPR.words[FS & 30]); break;
        case 20: EmitFpuLoadInt32((unsigned)&CPU_FPR.words[FS]); break;
        case 21: EmitFpuLoadInt64((unsigned)&CPU_FPR.words[FS & 30]); break;
        }
    }
}

// 004135A0.
void StoreFpuResult(unsigned char fullMode)
{
    if (fullMode) {
        switch (FMT) {
        case 16: EmitFpuPopSingle((unsigned)&CPU_FPR64[FD]); break;
        case 17: EmitFpuPopDouble((unsigned)&CPU_FPR64[FD]); break;
        }
    } else {
        switch (FMT) {
        case 16: EmitFpuPopSingle((unsigned)&CPU_FPR.words[FD]); break;
        case 17: EmitFpuPopDouble((unsigned)&CPU_FPR.words[FD & 30]); break;
        }
    }
}

// 00413620.
void StoreFpuResultAs(unsigned format, unsigned char fullMode)
{
    if (fullMode) {
        switch (format) {
        case 16: EmitFpuPopSingle((unsigned)&CPU_FPR64[FD]); break;
        case 17: EmitFpuPopDouble((unsigned)&CPU_FPR64[FD]); break;
        case 20: EmitFpuStoreInt32((unsigned)&CPU_FPR64[FD]); break;
        case 21: EmitFpuStoreInt64((unsigned)&CPU_FPR64[FD]); break;
        }
    } else {
        switch (format) {
        case 16: EmitFpuPopSingle((unsigned)&CPU_FPR.words[FD]); break;
        case 17: EmitFpuPopDouble((unsigned)&CPU_FPR.words[FD & 30]); break;
        case 20: EmitFpuStoreInt32((unsigned)&CPU_FPR.words[FD]); break;
        case 21: EmitFpuStoreInt64((unsigned)&CPU_FPR.words[FD & 30]); break;
        }
    }
}

// 00413730. Invalid formats report the original error and return zero.
unsigned GetFpuRegisterAddress(unsigned reg, unsigned format, unsigned char fullMode)
{
    if (fullMode) {
        switch (format) {
        case 16: return (unsigned)&CPU_FPR64[reg];
        case 17: return (unsigned)&CPU_FPR64[reg];
        case 20: return (unsigned)&CPU_FPR64[reg];
        case 21: return (unsigned)&CPU_FPR64[reg];
        }
    } else {
        switch (format) {
        case 16: return (unsigned)&CPU_FPR.words[reg];
        case 17: return (unsigned)&CPU_FPR.words[reg & ~1U];
        case 20: return (unsigned)&CPU_FPR.words[reg];
        case 21: return (unsigned)&CPU_FPR.words[reg & ~1U];
        }
    }
    ReportCompilerError("GetCOP1Reg: Error 0");
    return 0;
}

// 00414080. A rejected format intentionally leaves a null control address.
void FpuRoundingControl(unsigned format, unsigned mode)
{
    unsigned address = 0;
    switch (mode) {
    case 1:
        if (format == 16) address = (unsigned)&CPU_FpuControlSingle;
        else if (format == 17) address = (unsigned)&CPU_FpuControlExtended;
        else if (format == 20 || format == 21) address = (unsigned)&CPU_FpuControlDouble;
        break;
    case 3:
        if (format == 16) address = (unsigned)&FpuNearestSingle;
        else if (format == 17) address = (unsigned)&FpuNearestDouble;
        else if (format == 20 || format == 21) address = (unsigned)&FpuNearestInteger;
        break;
    case 2:
        if (format == 16) address = (unsigned)&FpuTruncateSingle;
        else if (format == 17) address = (unsigned)&FpuTruncateDouble;
        else if (format == 20 || format == 21) address = (unsigned)&FpuTruncateInteger;
        break;
    case 4:
        if (format == 16) address = (unsigned)&FpuFloorSingle;
        else if (format == 17) address = (unsigned)&FpuFloorDouble;
        else if (format == 20 || format == 21) address = (unsigned)&FpuFloorInteger;
        break;
    case 5:
        if (format == 16) address = (unsigned)&FpuCeilSingle;
        else if (format == 17) address = (unsigned)&FpuCeilDouble;
        else if (format == 20 || format == 21) address = (unsigned)&FpuCeilInteger;
        break;
    }
    EmitFpuClearExceptions();
    EmitFpuLoadControl(address);
}

// 00414230.
void RecompileFpuCompare(unsigned char condition)
{
    BEGIN_FPU_MODE();
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        LoadFpuOperand(1);
        switch (FMT) {
        case 16:
            EmitFpuCompareSingle(GetFpuRegisterAddress(FT, 16, 1));
            break;
        case 17:
            EmitFpuCompareDouble(GetFpuRegisterAddress(FT, 17, 1));
            break;
        }
    }
    NEXT_FPU_MODE();
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        LoadFpuOperand(0);
        switch (FMT) {
        case 16:
            EmitFpuCompareSingle(GetFpuRegisterAddress(FT, 16, 0));
            break;
        case 17:
            EmitFpuCompareDouble(GetFpuRegisterAddress(FT, 17, 0));
            break;
        }
    }
    END_FPU_MODE();
    EmitFpuStoreStatus();
    EmitStoreFlags();
    unsigned unordered = EmitConditionalJump(10);
    unsigned trueResult = EmitConditionalJump(condition);
    EmitAndMemory(1, 5, (unsigned)&CPU_FCR31, 0xff7fffff);
    unsigned done = EmitJump();
    EmitLabel(trueResult);
    EmitOrMemory(1, 5, (unsigned)&CPU_FCR31, 0x00800000);
    unsigned trueDone = EmitJump();
    EmitLabel(unordered);
    if (CPU_Instruction & 1)
        EmitOrMemory(1, 5, (unsigned)&CPU_FCR31, 0x00800000);
    else
        EmitAndMemory(1, 5, (unsigned)&CPU_FCR31, 0xff7fffff);
    EmitLabel(trueDone);
    EmitLabel(done);
}
