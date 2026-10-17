#include "DynarecOpcodes.h"
#include "Dynarec.h"
#include "DynarecFPU.h"
#include "DynarecMemoryOps.h"
#include "CPU.h"
#include "CpuControl.h"
#include "Memory.h"
#include "Plugins.h"
#include "RomBrowser.h"
#include "Rom.h"

// Private source names are inferred. This unit translates guest instructions
// into the typed intermediate operations consumed by the original x86 emitter.
#define RS ((CPU_Instruction >> 21) & 31)
#define RT ((CPU_Instruction >> 16) & 31)
#define RD ((CPU_Instruction >> 11) & 31)
#define SA ((CPU_Instruction >> 6) & 31)
#define FUNCT ((unsigned char)CPU_Instruction & 63)
#define SIMM ((short)CPU_Instruction)
#define UIMM ((unsigned short)CPU_Instruction)
#define ADDRESS(object) ((unsigned)&(object))

unsigned CPU_FpuModeChanges;

// 0040D590
void Recompile_Unknown()
{
    EmitMoveImmediate(1, X86_EAX, CPU_Instruction);
    EmitPush(X86_EAX);
    EmitMoveImmediate(1, X86_EAX, CPU_LastPC);
    EmitPush(X86_EAX);
    EmitCall((unsigned)ReportInvalidOpcode);
}

// 0040D5D0
void ReportDynarecError3()
{
    ReportCPUError("Compiler Error 3 in DRCPUM_DYNREC");
}

// 0040D5E0
void ReportDynarecError2()
{
    ReportCPUError("Compiler Error 2 in DRCPUM_DYNREC");
}

// 0040D5F0
void Recompile_Breakpoint()
{
    unsigned physical;
    if ((CPU_LastPC & 0xc0000000) == 0x80000000)
        physical = CPU_LastPC & 0x1fffffff;
    else
        physical = TranslateTLB(CPU_LastPC, 0);
    for (int i = CPUBreakpointCount - 1; i != -1; --i) {
        if (CPUBreakpoints[i].Address == physical) {
            CPU_Instruction = CPUBreakpoints[i].Instruction;
            if ((CPU_Instruction & 0xfc000000) == 0xec000000)
                EmitCall((unsigned)ReportDynarecError2);
            if (OpcodeStatsEnabled)
                EmitAddMemory(1, 0, 5, ADDRESS(OpcodeStats[CPU_Instruction >> 26]), 1);
            Dynarec_MainDispatch[CPU_Instruction >> 26]();
            return;
        }
    }
    EmitCall((unsigned)ReportDynarecError3);
}

void Recompile_SPECIAL()
{
    if (OpcodeStatsEnabled)
        EmitAddMemory(1, 0, 5, ADDRESS(OpcodeStats[0x40 + FUNCT]), 1);
    Dynarec_SpecialDispatch[FUNCT]();
}

void Recompile_REGIMM()
{
    if (OpcodeStatsEnabled)
        EmitAddMemory(1, 0, 5, ADDRESS(OpcodeStats[0x80 + RT]), 1);
    Dynarec_RegImmDispatch[RT]();
}

void Recompile_J()
{
    unsigned target = ((CPU_Instruction & 0x03ffffff) << 2) | (CPU_LastPC & 0xf0000000);
    if (target == CPU_LastPC) {
        if (CPU_LastPC == 0x8000c2c0 &&
            ((CurrentRom.CRC1 == 0x7c3829d9 && CurrentRom.CRC2 == 0x6e8247ce) ||
             (CurrentRom.CRC1 == 0x0b0ab4cd && CurrentRom.CRC2 == 0x7b158937)))
            return;
        Dynarec_BlockInstructionCount += 100;
    }
    RecompileDelaySlot();
    EmitBranchTarget(target);
    Dynarec_BlockFinished = 1;
}

void Recompile_JAL()
{
    unsigned target = ((CPU_Instruction & 0x03ffffff) << 2) | (CPU_LastPC & 0xf0000000);
    EmitLinkRegister(31);
    if (RuntimeLibraryEnabled) {
        void (*replacement)() = FindRuntimeReplacement(target);
        if (replacement) {
            RecompileDelaySlot();
            EmitCall((unsigned)replacement);
            EmitBranchTarget(CPU_LastPC + 8);
            return;
        }
    }
    if ((CurrentRom.CRC1 == 0x82dc04fd && CurrentRom.CRC2 == 0xcf2d82f4 &&
         CPU_LastPC == 0x8007a128 && target == 0x8007ad60) ||
        (CurrentRom.CRC1 == 0xa150743e && CurrentRom.CRC2 == 0xcf2522cd &&
         CPU_LastPC == 0x8008d448 && target == 0x8008e080))
        return;
    RecompileDelaySlot();
    if (CheckInlineFunction(target) < 10) {
        Dynarec_NextPC = target;
        AddInlineAddress(CPU_LastPC + 8);
    } else {
        EmitBranchTarget(target);
        Dynarec_BlockFinished = 1;
    }
}

void Recompile_BEQ()
{
    if (CPU_Instruction == 0x1000ffff) {
        RecompileDelaySlot();
        Dynarec_BlockInstructionCount += 100;
        EmitBranchTarget(CPU_LastPC);
        Dynarec_BlockFinished = 1;
    } else if (RT == 0 && RS == 0) {
        RecompileDelaySlot();
        EmitBranchTarget(CPU_LastPC + SIMM * 4 + 4);
    } else {
        RecompileEqualityBranch(0, 1);
    }
}

void Recompile_BNE() { RecompileEqualityBranch(0, 0); }
void Recompile_BLEZ() { RecompileSignedBranch(14, 0, 0); }
void Recompile_BGTZ() { RecompileSignedBranch(15, 0, 0); }
void Recompile_ADDI() { Recompile_ADDIU(); }

void Recompile_ADDIU()
{
    if (RS == 29 && RT == 29) {
        EmitAddImmediate(X86_EDI, SIMM);
        return;
    }
    LoadGPRLow(RS);
    EmitAddImmediate(X86_EAX, SIMM);
    StoreGPRSigned32(RT);
}

void Recompile_SLTI()
{
    LoadGPRWord(RS, 1, X86_EAX);
    if (CPU_Instruction & 0x8000)
        EmitCompareImmediate(1, X86_EAX, -1);
    else
        EmitCompareImmediate(1, X86_EAX, 0);
    EmitSet(12, X86_EAX);
    unsigned different = EmitConditionalJump(5);
    LoadGPRWord(RS, 0, X86_EAX);
    EmitCompareImmediate(1, X86_EAX, SIMM);
    EmitSet(2, X86_EAX);
    EmitLabel(different);
    EmitMoveUnsigned(0, X86_EAX, X86_EAX);
    StoreGPRSigned32(RT);
}

void Recompile_SLTIU()
{
    LoadGPRWord(RS, 1, X86_EAX);
    if (CPU_Instruction & 0x8000)
        EmitCompareImmediate(1, X86_EAX, -1);
    else
        EmitCompareImmediate(1, X86_EAX, 0);
    EmitSet(2, X86_EAX);
    unsigned different = EmitConditionalJump(5);
    LoadGPRWord(RS, 0, X86_EAX);
    EmitCompareImmediate(1, X86_EAX, SIMM);
    EmitSet(2, X86_EAX);
    EmitLabel(different);
    EmitMoveUnsigned(0, X86_EAX, X86_EAX);
    StoreGPRSigned32(RT);
}

void Recompile_ANDI()
{
    LoadGPRLow(RS);
    EmitAndImmediate(1, X86_EAX, UIMM);
    StoreGPRSigned32(RT);
}

void Recompile_ORI()
{
    LoadGPR64(RS);
    EmitOrAccumulator(1, UIMM);
    StoreGPR64(RT);
}

void Recompile_XORI()
{
    LoadGPR64(RS);
    EmitXorAccumulator(1, UIMM);
    StoreGPR64(RT);
}

void Recompile_LUI()
{
    if (RT == 29)
        EmitMoveImmediate(1, X86_EDI, CPU_Instruction << 16);
    else
        EmitStoreImmediate(1, 5, ADDRESS(CPU_GPR[RT]), CPU_Instruction << 16);
    if (CPU_Instruction & 0x8000)
        EmitStoreImmediate(1, 5, ADDRESS(CPU_GPR[RT]) + 4, -1);
    else
        EmitStoreImmediate(1, 5, ADDRESS(CPU_GPR[RT]) + 4, 0);
}

void Recompile_COP0()
{
    if (OpcodeStatsEnabled)
        EmitAddMemory(1, 0, 5, ADDRESS(OpcodeStats[0xa0 + RS]), 1);
    Dynarec_Cop0Dispatch[RS]();
}

void Recompile_COP1()
{
    if (OpcodeStatsEnabled)
        EmitAddMemory(1, 0, 5, ADDRESS(OpcodeStats[0xc0 + RS]), 1);
    Dynarec_Cop1Dispatch[RS]();
}

void Recompile_BEQL() { RecompileEqualityBranch(1, 1); }
void Recompile_BNEL() { RecompileEqualityBranch(1, 0); }
void Recompile_BLEZL() { RecompileSignedBranch(14, 1, 0); }
void Recompile_BGTZL() { RecompileSignedBranch(15, 1, 0); }

void Recompile_DADDI()
{
    LoadGPR64(RS);
    EmitAddImmediate(X86_EAX, SIMM);
    if (CPU_Instruction & 0x8000)
        EmitAdcImmediate(1, X86_EDX, -1);
    else
        EmitAdcImmediate(1, X86_EDX, 0);
    StoreGPR64(RT);
}

void Recompile_SLL()
{
    if (CPU_Instruction == 0)
        return;
    LoadGPRLow(RT);
    EmitShiftLeft(1, X86_EAX, SA);
    StoreGPRSigned32(RD);
}

void Recompile_SRL()
{
    LoadGPRLow(RT);
    EmitShiftRight(1, X86_EAX, SA);
    StoreGPRSigned32(RD);
}

void Recompile_SRA()
{
    LoadGPRLow(RT);
    EmitArithmeticShiftRight(1, X86_EAX, SA);
    StoreGPRSigned32(RD);
}

void Recompile_SLLV()
{
    LoadGPRPairLow(RT, RS);
    EmitShiftLeftCL(1, X86_EAX);
    StoreGPRSigned32(RD);
}

void Recompile_SRLV()
{
    LoadGPRPairLow(RT, RS);
    EmitShiftRightCL(1, X86_EAX);
    StoreGPRSigned32(RD);
}

void Recompile_SRAV()
{
    LoadGPRPairLow(RT, RS);
    EmitArithmeticShiftRightCL(1, X86_EAX);
    StoreGPRSigned32(RD);
}

void Recompile_JR()
{
    unsigned source = RS;
    if (source == 31 && Dynarec_InlineAddressCount) {
        RecompileDelaySlot();
        Dynarec_NextPC = Dynarec_InlineAddresses[--Dynarec_InlineAddressCount];
        return;
    }
    LoadGPRLow(source);
    EmitProgramCounter();
    RecompileDelaySlot();
    FinishInstruction();
    EmitReturn();
    Dynarec_BlockFinished = 1;
}

void Recompile_JALR()
{
    EmitLinkRegister(RD);
    LoadGPRLow(RS);
    EmitProgramCounter();
    RecompileDelaySlot();
    FinishInstruction();
    EmitReturn();
    Dynarec_BlockFinished = 1;
}

void Recompile_SYSCALL()
{
    EmitOrMemory(1, 5, ADDRESS(CPU_ActionFlags), 0x01000000);
    EmitStoreImmediate(1, 5, ADDRESS(CPU_PC), CPU_LastPC);
    FinishInstruction();
    EmitReturn();
    Dynarec_BlockFinished = 1;
}

void Recompile_BREAK()
{
    Recompile_Unknown();
    Dynarec_BlockFinished = 1;
}

void Recompile_MFHI()
{
    EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_HI));
    EmitLoad(1, X86_EDX, 5, ADDRESS(CPU_HI) + 4);
    StoreGPR64(RD);
}

void Recompile_MTHI()
{
    LoadGPR64(RS);
    EmitStore(1, X86_EAX, 5, ADDRESS(CPU_HI));
    EmitStore(1, X86_EDX, 5, ADDRESS(CPU_HI) + 4);
}

void Recompile_MFLO()
{
    EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_LO));
    EmitLoad(1, X86_EDX, 5, ADDRESS(CPU_LO) + 4);
    StoreGPR64(RD);
}

void Recompile_MTLO()
{
    LoadGPR64(RS);
    EmitStore(1, X86_EAX, 5, ADDRESS(CPU_LO));
    EmitStore(1, X86_EDX, 5, ADDRESS(CPU_LO) + 4);
}

void Recompile_DSLLV()
{
    LoadGPR64(RT);
    LoadGPRWord(RS, 0, X86_ECX);
    EmitAndImmediate(1, X86_ECX, 63);
    EmitCompareImmediate(1, X86_ECX, 32);
    unsigned wide = EmitConditionalJump(3);
    EmitDoubleShiftLeftCL(X86_EAX, X86_EDX);
    EmitShiftLeftCL(1, X86_EAX);
    unsigned done = EmitJump();
    EmitLabel(wide);
    EmitMove(1, X86_EAX, X86_EDX);
    EmitMoveImmediate(1, X86_EAX, 0);
    EmitAndImmediate(1, X86_ECX, 31);
    EmitShiftLeftCL(1, X86_EDX);
    EmitLabel(done);
    StoreGPR64(RD);
}

void Recompile_DSRLV()
{
    LoadGPR64(RT);
    LoadGPRWord(RS, 0, X86_ECX);
    EmitAndImmediate(1, X86_ECX, 63);
    EmitCompareImmediate(1, X86_ECX, 32);
    unsigned wide = EmitConditionalJump(3);
    EmitDoubleShiftRightCL(X86_EDX, X86_EAX);
    EmitShiftRightCL(1, X86_EDX);
    unsigned done = EmitJump();
    EmitLabel(wide);
    EmitMove(1, X86_EDX, X86_EAX);
    EmitMoveImmediate(1, X86_EDX, 0);
    EmitAndImmediate(1, X86_ECX, 31);
    EmitShiftRightCL(1, X86_EAX);
    EmitLabel(done);
    StoreGPR64(RD);
}

void Recompile_DSRAV()
{
    LoadGPR64(RT);
    LoadGPRWord(RS, 0, X86_ECX);
    EmitAndImmediate(1, X86_ECX, 63);
    EmitCompareImmediate(1, X86_ECX, 32);
    unsigned wide = EmitConditionalJump(3);
    EmitDoubleShiftRightCL(X86_EDX, X86_EAX);
    EmitArithmeticShiftRightCL(1, X86_EDX);
    unsigned done = EmitJump();
    EmitLabel(wide);
    EmitMove(1, X86_EDX, X86_EAX);
    EmitArithmeticShiftRight(1, X86_EDX, 31);
    EmitAndImmediate(1, X86_ECX, 31);
    EmitArithmeticShiftRightCL(1, X86_EAX);
    EmitLabel(done);
    StoreGPR64(RD);
}

void Recompile_MULT()
{
    LoadGPRPairLow(RS, RT);
    EmitSignedMultiply(1, X86_ECX);
    StoreMultiplyResult();
}

void Recompile_MULTU()
{
    LoadGPRPairLow(RS, RT);
    EmitMultiply(1, X86_ECX);
    StoreMultiplyResult();
}

void Recompile_DIV()
{
    LoadGPRPairLow(RS, RT);
    EmitMove(1, X86_EAX, X86_EDX);
    EmitArithmeticShiftRight(1, X86_EDX, 31);
    EmitCompareImmediate(1, X86_ECX, 0);
    unsigned zero = EmitConditionalJump(4);
    EmitSignedDivide(1, X86_ECX);
    StoreMultiplyResult();
    unsigned done = EmitJump();
    EmitLabel(zero);
    EmitStoreImmediate(1, 5, ADDRESS(CPU_LO), 1);
    EmitStoreImmediate(1, 5, ADDRESS(CPU_LO) + 4, 0);
    EmitStore(1, X86_EAX, 5, ADDRESS(CPU_HI));
    EmitStore(1, X86_EDX, 5, ADDRESS(CPU_HI) + 4);
    EmitLabel(done);
}

void Recompile_DIVU()
{
    LoadGPRPairLow(RS, RT);
    EmitMoveImmediate(1, X86_EDX, 0);
    EmitCompareImmediate(1, X86_ECX, 0);
    unsigned zero = EmitConditionalJump(4);
    EmitDivide(1, X86_ECX);
    StoreMultiplyResult();
    unsigned done = EmitJump();
    EmitLabel(zero);
    EmitStoreImmediate(1, 5, ADDRESS(CPU_LO), -1);
    EmitStoreImmediate(1, 5, ADDRESS(CPU_LO) + 4, -1);
    EmitStore(1, X86_EAX, 5, ADDRESS(CPU_HI));
    EmitArithmeticShiftRight(1, X86_EAX, 31);
    EmitStore(1, X86_EAX, 5, ADDRESS(CPU_HI) + 4);
    EmitLabel(done);
}

void Recompile_DMULT()
{
    LoadTwoGPR64(RS, RT);
    EmitPush(X86_EBX);
    EmitPush(X86_ECX);
    EmitPush(X86_EDX);
    EmitPush(X86_EAX);
    EmitCall((unsigned)MultiplySigned64);
}

void Recompile_DMULTU()
{
    LoadTwoGPR64(RS, RT);
    EmitPush(X86_EBX);
    EmitPush(X86_ECX);
    EmitPush(X86_EDX);
    EmitPush(X86_EAX);
    EmitCall((unsigned)MultiplyUnsigned64);
}

void Recompile_DDIV()
{
    LoadTwoGPR64(RS, RT);
    EmitPush(X86_EBX);
    EmitPush(X86_ECX);
    EmitPush(X86_EDX);
    EmitPush(X86_EAX);
    EmitCall((unsigned)DivideSigned64);
}

void Recompile_DDIVU()
{
    LoadTwoGPR64(RS, RT);
    EmitPush(X86_EBX);
    EmitPush(X86_ECX);
    EmitPush(X86_EDX);
    EmitPush(X86_EAX);
    EmitCall((unsigned)DivideUnsigned64);
}

void Recompile_ADD()
{
    LoadGPRPairLow(RS, RT);
    EmitAdd(1, X86_ECX, X86_EAX);
    StoreGPRSigned32(RD);
}

void Recompile_SUB()
{
    LoadGPRPairLow(RS, RT);
    EmitSubtract(1, X86_ECX, X86_EAX);
    StoreGPRSigned32(RD);
}

void Recompile_AND()
{
    LoadTwoGPR64(RS, RT);
    EmitAnd(1, X86_ECX, X86_EAX);
    EmitAnd(1, X86_EBX, X86_EDX);
    StoreGPR64(RD);
}

void Recompile_OR()
{
    LoadTwoGPR64(RS, RT);
    EmitOr(1, X86_ECX, X86_EAX);
    EmitOr(1, X86_EBX, X86_EDX);
    StoreGPR64(RD);
}

void Recompile_XOR()
{
    LoadTwoGPR64(RS, RT);
    EmitXor(1, X86_ECX, X86_EAX);
    EmitXor(1, X86_EBX, X86_EDX);
    StoreGPR64(RD);
}

void Recompile_NOR()
{
    LoadTwoGPR64(RS, RT);
    EmitOr(1, X86_ECX, X86_EAX);
    EmitNot(1, X86_EAX);
    EmitOr(1, X86_EBX, X86_EDX);
    EmitNot(1, X86_EDX);
    StoreGPR64(RD);
}

void Recompile_SLT()
{
    LoadGPRWord(RS, 1, X86_EDX);
    LoadGPRWord(RT, 1, X86_EBX);
    EmitCompare(1, X86_EBX, X86_EDX);
    EmitSet(12, X86_EAX);
    unsigned different = EmitConditionalJump(5);
    LoadGPRPairLow(RS, RT);
    EmitCompare(1, X86_ECX, X86_EAX);
    EmitSet(2, X86_EAX);
    EmitLabel(different);
    EmitMoveUnsigned(0, X86_EAX, X86_EAX);
    StoreGPRSigned32(RD);
}

void Recompile_SLTU()
{
    LoadGPRWord(RS, 1, X86_EDX);
    LoadGPRWord(RT, 1, X86_EBX);
    EmitCompare(1, X86_EBX, X86_EDX);
    EmitSet(2, X86_EAX);
    unsigned different = EmitConditionalJump(5);
    LoadGPRPairLow(RS, RT);
    EmitCompare(1, X86_ECX, X86_EAX);
    EmitSet(2, X86_EAX);
    EmitLabel(different);
    EmitMoveUnsigned(0, X86_EAX, X86_EAX);
    StoreGPRSigned32(RD);
}

void Recompile_DADD()
{
    LoadTwoGPR64(RS, RT);
    EmitAdd(1, X86_ECX, X86_EAX);
    EmitAdc(1, X86_EBX, X86_EDX);
    StoreGPR64(RD);
}

void Recompile_DSUB()
{
    LoadTwoGPR64(RS, RT);
    EmitSubtract(1, X86_ECX, X86_EAX);
    EmitSbb(1, X86_EBX, X86_EDX);
    StoreGPR64(RD);
}

// 004106A0: preserve the original diagnostic before compiling TEQ.
void WarnCrackedRom()
{
    if (CPU_GPR[24] == 0x0bad0000)
        ReportCPUError("This is a cracked rom and designed not to work in emus.\nError messages will follow, unless you stop emulation.");
}

void Recompile_TEQ()
{
    EmitCall((unsigned)WarnCrackedRom);
    Recompile_Unknown();
}

void Recompile_DSLL()
{
    LoadGPR64(RT);
    EmitDoubleShiftLeft(X86_EAX, X86_EDX, SA);
    EmitShiftLeft(1, X86_EAX, SA);
    StoreGPR64(RD);
}

void Recompile_DSRL()
{
    LoadGPR64(RT);
    EmitDoubleShiftRight(X86_EDX, X86_EAX, SA);
    EmitShiftRight(1, X86_EDX, SA);
    StoreGPR64(RD);
}

void Recompile_DSRA()
{
    LoadGPR64(RT);
    EmitDoubleShiftRight(X86_EDX, X86_EAX, SA);
    EmitArithmeticShiftRight(1, X86_EDX, SA);
    StoreGPR64(RD);
}

void Recompile_DSLL32()
{
    LoadGPRWord(RT, 0, X86_EDX);
    EmitShiftLeft(1, X86_EDX, SA);
    EmitMoveImmediate(1, X86_EAX, 0);
    StoreGPR64(RD);
}

void Recompile_DSRL32()
{
    LoadGPRWord(RT, 1, X86_EAX);
    EmitShiftRight(1, X86_EAX, SA);
    EmitMoveImmediate(1, X86_EDX, 0);
    StoreGPR64(RD);
}

void Recompile_DSRA32()
{
    LoadGPRWord(RT, 1, X86_EAX);
    EmitArithmeticShiftRight(1, X86_EAX, SA);
    StoreGPRSigned32(RD);
}

void Recompile_BLTZ() { RecompileSignedBranch(12, 0, 0); }
void Recompile_BGEZ() { RecompileSignedBranch(13, 0, 0); }
void Recompile_BLTZL() { RecompileSignedBranch(12, 1, 0); }
void Recompile_BGEZL() { RecompileSignedBranch(13, 1, 0); }
void Recompile_BLTZAL() { RecompileSignedBranch(12, 0, 1); }

void Recompile_BGEZAL()
{
    if (BootCicSeed == 0x3f) {
        if (CPU_LastPC == 0x800001c0 || CPU_LastPC == 0x800001c8) return;
    } else if (BootCicSeed == 0x78) {
        if (CPU_LastPC == 0x80000190 || CPU_LastPC == 0x80000198) return;
    } else if (BootCicSeed == 0x85) {
        if (CPU_LastPC == 0x800001a0 || CPU_LastPC == 0x800001a8) return;
    } else if (BootCicSeed == 0x91) {
        if (CPU_LastPC == 0x80000240 || CPU_LastPC == 0x80000248) return;
    }
    RecompileSignedBranch(13, 0, 1);
}

void Recompile_BLTZALL() { RecompileSignedBranch(12, 1, 1); }
void Recompile_BGEZALL() { RecompileSignedBranch(13, 1, 1); }

void Recompile_MFC0()
{
    switch (RD) {
    case CP0_Index:    EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Index])); break;
    case CP0_Random:   EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_Random)); break;
    case CP0_EntryLo0: EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_EntryLo0])); break;
    case CP0_EntryLo1: EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_EntryLo1])); break;
    case CP0_Context:  EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Context])); break;
    case CP0_PageMask: EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_PageMask])); break;
    case CP0_Wired:    EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Wired])); break;
    case CP0_BadVAddr: EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_BadVAddr])); break;
    case CP0_Count:
        EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Count]));
        EmitAddImmediate(X86_EAX, 10000);
        EmitSubtract(1, X86_ESI, X86_EAX);
        StoreGPRSigned32(RT);
        return;
    case CP0_EntryHi:  EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_EntryHi])); break;
    case CP0_Compare:  EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Compare])); break;
    case CP0_Status:   EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Status])); break;
    case CP0_Cause:    EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Cause])); break;
    case CP0_EPC:      EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_EPC])); break;
    case CP0_PRId:     EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_PRId])); break;
    case CP0_Config:   EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Config])); break;
    case CP0_LLAddr:   EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_LLAddr])); break;
    case CP0_WatchLo:  EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_WatchLo])); break;
    case CP0_WatchHi:  EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_WatchHi])); break;
    case CP0_XContext: EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_XContext])); break;
    case CP0_PErr:     EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_PErr])); break;
    case CP0_CacheErr: EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_CacheErr])); break;
    case CP0_TagLo:    EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_TagLo])); break;
    case CP0_TagHi:    EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_TagHi])); break;
    case CP0_ErrorEPC: EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_ErrorEPC])); break;
    }
    StoreGPRSigned32(RT);
}

// 00410B50
unsigned UpdateFpuRegisterMode()
{
    unsigned mode = (CPU_COP0[CP0_Status] >> 26) & 1;
    if (CPU_FRMode != mode) {
        ++CPU_FpuModeChanges;
        CPU_FRMode = mode;
        if (!Dynarec_DynamicFpuMode) {
            if (CPU_FpuModeChanges > 4) {
                ReportCPUError("Turning on fpu hybrid mode");
                Dynarec_DynamicFpuMode = 1;
            }
            CPU_ActionFlags |= 0x400;
            return 1;
        }
    }
    return 0;
}

// 00410BC0
void ClampRandomRegister()
{
    if (CPU_Random < CPU_COP0[CP0_Wired])
        CPU_Random = CPU_COP0[CP0_Wired];
}

// 00410BE0
void WarnFpuModeInDelaySlot()
{
    ReportCPUError("Changes FPU Mode 32/64 in delay slot");
}

void Recompile_MTC0()
{
    LoadGPRLow(RT);
    switch (RD) {
    case CP0_Index:    EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Index])); return;
    case CP0_EntryLo0: EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_EntryLo0])); return;
    case CP0_EntryLo1: EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_EntryLo1])); return;
    case CP0_Context:  EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Context])); return;
    case CP0_PageMask: EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_PageMask])); return;
    case CP0_Wired:
        EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Wired]));
        EmitCall((unsigned)ClampRandomRegister);
        return;
    case CP0_BadVAddr: EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_BadVAddr])); return;
    case CP0_Count:
        EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Count]));
        EmitMoveImmediate(1, X86_ESI, 10000);
        EmitLoad(1, X86_EDX, 5, ADDRESS(CPU_COP0[CP0_Compare]));
        EmitCompare(1, X86_EDX, X86_EAX);
        EmitSetMemory(2, 5, ADDRESS(CPU_ComparePending));
        return;
    case CP0_EntryHi: EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_EntryHi])); return;
    case CP0_Compare:
        EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Compare]));
        EmitLoad(1, X86_EDX, 5, ADDRESS(CPU_COP0[CP0_Count]));
        EmitCompare(1, X86_EAX, X86_EDX);
        EmitSetMemory(2, 5, ADDRESS(CPU_ComparePending));
        EmitAndMemory(1, 5, ADDRESS(CPU_COP0[CP0_Cause]), 0xffff7fff);
        return;
    case CP0_Status: {
        EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Status]));
        EmitCall((unsigned)UpdateFpuRegisterMode);
        EmitCompareImmediate(1, X86_EAX, 1);
        unsigned unchanged = EmitConditionalJump(5);
        if (Dynarec_DelaySlot) {
            EmitCall((unsigned)WarnFpuModeInDelaySlot);
            EmitLabel(unchanged);
            return;
        }
        EmitStoreImmediate(1, 5, ADDRESS(CPU_PC), CPU_LastPC + 4);
        FinishInstruction();
        EmitReturn();
        EmitLabel(unchanged);
        return;
    }
    case CP0_Cause:    EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Cause])); return;
    case CP0_EPC:      EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_EPC])); return;
    case CP0_PRId:     EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_PRId])); return;
    case CP0_Config:   EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Config])); return;
    case CP0_LLAddr:   EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_LLAddr])); return;
    case CP0_WatchLo:  EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_WatchLo])); return;
    case CP0_WatchHi:  EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_WatchHi])); return;
    case CP0_XContext: EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_XContext])); return;
    case CP0_PErr:     EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_PErr])); return;
    case CP0_CacheErr: EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_CacheErr])); return;
    case CP0_TagLo:    EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_TagLo])); return;
    case CP0_TagHi:    EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_TagHi])); return;
    case CP0_ErrorEPC: EmitStore(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_ErrorEPC])); return;
    }
}

void Recompile_TLB()
{
    if (OpcodeStatsEnabled)
        EmitAddMemory(1, 0, 5, ADDRESS(OpcodeStats[0xe0 + RS]), 1);
    Dynarec_Cop0FunctionDispatch[FUNCT]();
}

void Recompile_TLBR() { EmitCall((unsigned)TLBRead); }

// 00410FD0
void WarnTlbWriteInDelaySlot()
{
    ReportCPUError("TLBWI/TLBWR in delay slot. This might crash");
}

void Recompile_TLBWI()
{
    EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Index]));
    EmitAndImmediate(1, X86_EAX, 63);
    EmitPush(X86_EAX);
    EmitCall((unsigned)TLBWrite);
    if (Dynarec_DelaySlot) {
        EmitCall((unsigned)WarnTlbWriteInDelaySlot);
        return;
    }
    EmitStoreImmediate(1, 5, ADDRESS(CPU_PC), CPU_LastPC + 4);
    FinishInstruction();
    EmitReturn();
}

void Recompile_TLBWR()
{
    EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_Random));
    EmitAddMemory(1, 1, 5, ADDRESS(CPU_Random), 1);
    EmitCompareImmediate(1, X86_EAX, 30);
    unsigned withinRange = EmitConditionalJump(2);
    EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_Wired]));
    EmitAddMemory(1, 1, 5, ADDRESS(CPU_Random), 1);
    EmitStore(1, X86_EAX, 5, ADDRESS(CPU_Random));
    EmitLabel(withinRange);
    EmitPush(X86_EAX);
    EmitCall((unsigned)TLBWrite);
    if (Dynarec_DelaySlot) {
        EmitCall((unsigned)WarnTlbWriteInDelaySlot);
        return;
    }
    EmitStoreImmediate(1, 5, ADDRESS(CPU_PC), CPU_LastPC + 4);
    FinishInstruction();
    EmitReturn();
}

void Recompile_TLBP() { EmitCall((unsigned)TLBProbe); }

void Recompile_ERET()
{
    EmitTestMemory(1, 5, ADDRESS(CPU_COP0[CP0_Status]), 4);
    unsigned errorLevel = EmitConditionalJump(5);
    EmitAndMemory(1, 5, ADDRESS(CPU_COP0[CP0_Status]), 0xfffffffd);
    EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_EPC]));
    EmitProgramCounter();
    FinishInstruction();
    EmitReturn();
    EmitLabel(errorLevel);
    EmitAndMemory(1, 5, ADDRESS(CPU_COP0[CP0_Status]), 0xfffffffb);
    EmitLoad(1, X86_EAX, 5, ADDRESS(CPU_COP0[CP0_ErrorEPC]));
    EmitProgramCounter();
    FinishInstruction();
    EmitReturn();
    Dynarec_BlockFinished = 1;
}

// The seven original dispatch tables retain aliases and reserved entries.
// Original VA 0x00477820
DynarecOpcodeHandler Dynarec_MainDispatch[64] = {
    Recompile_SPECIAL, // 00 SPECIAL  0040D6B0
    Recompile_REGIMM, // 01 REGIMM   0040D6F0
    Recompile_J, // 02 J        0040D730
    Recompile_JAL, // 03 JAL      0040D7B0
    Recompile_BEQ, // 04 BEQ      0040D8A0
    Recompile_BNE, // 05 BNE      0040D920
    Recompile_BLEZ, // 06 BLEZ     0040D930
    Recompile_BGTZ, // 07 BGTZ     0040D940
    Recompile_ADDI, // 08 ADDI     0040D950
    Recompile_ADDIU, // 09 ADDIU    0040D960
    Recompile_SLTI, // 0A SLTI     0040D9B0
    Recompile_SLTIU, // 0B SLTIU    0040DA50
    Recompile_ANDI, // 0C ANDI     0040DAF0
    Recompile_ORI, // 0D ORI      0040DB30
    Recompile_XORI, // 0E XORI     0040DB60
    Recompile_LUI, // 0F LUI      0040DB90
    Recompile_COP0, // 10 COP0     0040DC00
    Recompile_COP1, // 11 COP1     0040DC40
    Recompile_Unknown, // 12 unused   0040D590
    Recompile_Unknown, // 13 unused   0040D590
    Recompile_BEQL, // 14 BEQL     0040DC80
    Recompile_BNEL, // 15 BNEL     0040DC90
    Recompile_BLEZL, // 16 BLEZL    0040DCA0
    Recompile_BGTZL, // 17 BGTZL    0040DCB0
    Recompile_DADDI, // 18 DADDI    0040DCC0
    Recompile_DADDI, // 19 DADDIU   0040DCC0
    Recompile_LDL, // 1A LDL      0040DDA0
    Recompile_LDR, // 1B LDR      0040DEC0
    Recompile_Unknown, // 1C unused   0040D590
    Recompile_Unknown, // 1D unused   0040D590
    Recompile_Unknown, // 1E unused   0040D590
    Recompile_Unknown, // 1F unused   0040D590
    Recompile_LB, // 20 LB       0040DF50
    Recompile_LH, // 21 LH       0040E010
    Recompile_LWL, // 22 LWL      0040E0D0
    Recompile_LW, // 23 LW       0040E210
    Recompile_LBU, // 24 LBU      0040E2C0
    Recompile_LHU, // 25 LHU      0040E380
    Recompile_LWR, // 26 LWR      0040E440
    Recompile_LWU, // 27 LWU      0040E580
    Recompile_SB, // 28 SB       0040E630
    Recompile_SH, // 29 SH       0040E730
    Recompile_SWL, // 2A SWL      0040E830
    Recompile_SW, // 2B SW       0040E9F0
    Recompile_SDL, // 2C SDL      0040EB70
    Recompile_SDR, // 2D SDR      0040ECA0
    Recompile_SWR, // 2E SWR      0040ED40
    Recompile_CACHE, // 2F CACHE    0040EFB0
    Recompile_Unimplemented, // 30 LL       0040F570
    Recompile_LWC1, // 31 LWC1     0040F130
    Recompile_Unknown, // 32 LWC2     0040D590
    Recompile_Unknown, // 33 unused   0040D590
    Recompile_Unimplemented, // 34 LLD      0040F570
    Recompile_LDC1, // 35 LDC1     0040F290
    Recompile_Unknown, // 36 LDC2     0040D590
    Recompile_LD, // 37 LD       0040F480
    Recompile_Unimplemented, // 38 SC       0040F570
    Recompile_SWC1, // 39 SWC1     0040F580
    Recompile_Unknown, // 3A SWC2     0040D590
    Recompile_Breakpoint, // 3B unused   0040D5F0
    Recompile_Unimplemented, // 3C SCD      0040F570
    Recompile_SDC1, // 3D SDC1     0040F720
    Recompile_Unknown, // 3E SDC2     0040D590
    Recompile_SD, // 3F SD       0040F8E0
};

// Original VA 0x00477920
DynarecOpcodeHandler Dynarec_SpecialDispatch[64] = {
    Recompile_SLL, // 00 SLL      0040FA00
    Recompile_Unknown, // 01 unused   0040D590
    Recompile_SRL, // 02 SRL      0040FA50
    Recompile_SRA, // 03 SRA      0040FA90
    Recompile_SLLV, // 04 SLLV     0040FAD0
    Recompile_Unknown, // 05 unused   0040D590
    Recompile_SRLV, // 06 SRLV     0040FB10
    Recompile_SRAV, // 07 SRAV     0040FB50
    Recompile_JR, // 08 JR       0040FB90
    Recompile_JALR, // 09 JALR     0040FBF0
    Recompile_Unknown, // 0A unused   0040D590
    Recompile_Unknown, // 0B unused   0040D590
    Recompile_SYSCALL, // 0C SYSCALL  0040FC40
    Recompile_BREAK, // 0D BREAK    0040FC80
    Recompile_Unknown, // 0E unused   0040D590
    DynarecNoOperation, // 0F SYNC     0041FEF0
    Recompile_MFHI, // 10 MFHI     0040FC90
    Recompile_MTHI, // 11 MTHI     0040FCD0
    Recompile_MFLO, // 12 MFLO     0040FD10
    Recompile_MTLO, // 13 MTLO     0040FD50
    Recompile_DSLLV, // 14 DSLLV    0040FD90
    Recompile_Unknown, // 15 unused   0040D590
    Recompile_DSRLV, // 16 DSRLV    0040FE50
    Recompile_DSRAV, // 17 DSRAV    0040FF10
    Recompile_MULT, // 18 MULT     0040FFD0
    Recompile_MULTU, // 19 MULTU    00410000
    Recompile_DIV, // 1A DIV      00410030
    Recompile_DIVU, // 1B DIVU     004100F0
    Recompile_DMULT, // 1C DMULT    004101B0
    Recompile_DMULTU, // 1D DMULTU   00410200
    Recompile_DDIV, // 1E DDIV     00410250
    Recompile_DDIVU, // 1F DDIVU    004102A0
    Recompile_ADD, // 20 ADD      004102F0
    Recompile_ADD, // 21 ADDU     004102F0
    Recompile_SUB, // 22 SUB      00410330
    Recompile_SUB, // 23 SUBU     00410330
    Recompile_AND, // 24 AND      00410370
    Recompile_OR, // 25 OR       004103C0
    Recompile_XOR, // 26 XOR      00410410
    Recompile_NOR, // 27 NOR      00410460
    Recompile_Unknown, // 28 unused   0040D590
    Recompile_Unknown, // 29 unused   0040D590
    Recompile_SLT, // 2A SLT      004104C0
    Recompile_SLTU, // 2B SLTU     00410560
    Recompile_DADD, // 2C DADD     00410600
    Recompile_DADD, // 2D DADDU    00410600
    Recompile_DSUB, // 2E DSUB     00410650
    Recompile_DSUB, // 2F DSUBU    00410650
    Recompile_Unimplemented, // 30 TGE      0040F570
    Recompile_Unimplemented, // 31 TGEU     0040F570
    Recompile_Unimplemented, // 32 TLT      0040F570
    Recompile_Unimplemented, // 33 TLTU     0040F570
    Recompile_TEQ, // 34 TEQ      004106D0
    Recompile_Unknown, // 35 unused   0040D590
    Recompile_Unimplemented, // 36 TNE      0040F570
    Recompile_Unknown, // 37 unused   0040D590
    Recompile_DSLL, // 38 DSLL     004106E0
    Recompile_Unknown, // 39 unused   0040D590
    Recompile_DSRL, // 3A DSRL     00410730
    Recompile_DSRA, // 3B DSRA     00410780
    Recompile_DSLL32, // 3C DSLL32   004107D0
    Recompile_Unknown, // 3D unused   0040D590
    Recompile_DSRL32, // 3E DSRL32   00410820
    Recompile_DSRA32, // 3F DSRA32   00410870
};

// Original VA 0x00477A20
DynarecOpcodeHandler Dynarec_RegImmDispatch[32] = {
    Recompile_BLTZ, // 00 BLTZ     004108B0
    Recompile_BGEZ, // 01 BGEZ     004108C0
    Recompile_BLTZL, // 02 BLTZL    004108D0
    Recompile_BGEZL, // 03 BGEZL    004108E0
    Recompile_Unknown, // 04 unused   0040D590
    Recompile_Unknown, // 05 unused   0040D590
    Recompile_Unknown, // 06 unused   0040D590
    Recompile_Unknown, // 07 unused   0040D590
    Recompile_Unimplemented, // 08 TGEI     0040F570
    Recompile_Unimplemented, // 09 TGEIU    0040F570
    Recompile_Unimplemented, // 0A TLTI     0040F570
    Recompile_Unimplemented, // 0B TLTIU    0040F570
    Recompile_Unimplemented, // 0C TEQI     0040F570
    Recompile_Unknown, // 0D unused   0040D590
    Recompile_Unimplemented, // 0E TNEI     0040F570
    Recompile_Unknown, // 0F unused   0040D590
    Recompile_BLTZAL, // 10 BLTZAL   004108F0
    Recompile_BGEZAL, // 11 BGEZAL   00410900
    Recompile_BLTZALL, // 12 BLTZALL  00410980
    Recompile_BGEZALL, // 13 BGEZALL  00410990
    Recompile_Unknown, // 14 reserved 0040D590
    Recompile_Unknown, // 15 reserved 0040D590
    Recompile_Unknown, // 16 reserved 0040D590
    Recompile_Unknown, // 17 reserved 0040D590
    Recompile_Unknown, // 18 reserved 0040D590
    Recompile_Unknown, // 19 reserved 0040D590
    Recompile_Unknown, // 1A reserved 0040D590
    Recompile_Unknown, // 1B reserved 0040D590
    Recompile_Unknown, // 1C reserved 0040D590
    Recompile_Unknown, // 1D reserved 0040D590
    Recompile_Unknown, // 1E reserved 0040D590
    Recompile_Unknown, // 1F reserved 0040D590
};

// Original VA 0x00477AA0
DynarecOpcodeHandler Dynarec_Cop0Dispatch[32] = {
    Recompile_MFC0, // 00 MFC0     004109A0
    Recompile_Unknown, // 01 unused   0040D590
    Recompile_Unknown, // 02 unused   0040D590
    Recompile_Unknown, // 03 unused   0040D590
    Recompile_MTC0, // 04 MTC0     00410BF0
    Recompile_Unknown, // 05 unused   0040D590
    Recompile_Unknown, // 06 unused   0040D590
    Recompile_Unknown, // 07 unused   0040D590
    Recompile_Unknown, // 08 unused   0040D590
    Recompile_Unknown, // 09 unused   0040D590
    Recompile_Unknown, // 0A unused   0040D590
    Recompile_Unknown, // 0B unused   0040D590
    Recompile_Unknown, // 0C unused   0040D590
    Recompile_Unknown, // 0D unused   0040D590
    Recompile_Unknown, // 0E unused   0040D590
    Recompile_Unknown, // 0F unused   0040D590
    Recompile_TLB, // 10 TLB      00410F80
    Recompile_Unknown, // 11 unused   0040D590
    Recompile_Unknown, // 12 unused   0040D590
    Recompile_Unknown, // 13 unused   0040D590
    Recompile_Unknown, // 14 unused   0040D590
    Recompile_Unknown, // 15 unused   0040D590
    Recompile_Unknown, // 16 unused   0040D590
    Recompile_Unknown, // 17 unused   0040D590
    Recompile_Unknown, // 18 unused   0040D590
    Recompile_Unknown, // 19 unused   0040D590
    Recompile_Unknown, // 1A unused   0040D590
    Recompile_Unknown, // 1B unused   0040D590
    Recompile_Unknown, // 1C unused   0040D590
    Recompile_Unknown, // 1D unused   0040D590
    Recompile_Unknown, // 1E unused   0040D590
    Recompile_Unknown, // 1F unused   0040D590
};

// Original VA 0x00477B20
DynarecOpcodeHandler Dynarec_Cop0FunctionDispatch[64] = {
    Recompile_Unknown, // 00 unused   0040D590
    Recompile_TLBR, // 01 TLBR     00410FC0
    Recompile_TLBWI, // 02 TLBWI    00410FE0
    Recompile_Unknown, // 03 unused   0040D590
    Recompile_Unknown, // 04 unused   0040D590
    Recompile_Unknown, // 05 unused   0040D590
    Recompile_TLBWR, // 06 TLBWR    00411050
    Recompile_Unknown, // 07 unused   0040D590
    Recompile_TLBP, // 08 TLBP     00411110
    Recompile_Unknown, // 09 unused   0040D590
    Recompile_Unknown, // 0A unused   0040D590
    Recompile_Unknown, // 0B unused   0040D590
    Recompile_Unknown, // 0C unused   0040D590
    Recompile_Unknown, // 0D unused   0040D590
    Recompile_Unknown, // 0E unused   0040D590
    Recompile_Unknown, // 0F unused   0040D590
    Recompile_Unknown, // 10 unused   0040D590
    Recompile_Unknown, // 11 unused   0040D590
    Recompile_Unknown, // 12 unused   0040D590
    Recompile_Unknown, // 13 unused   0040D590
    Recompile_Unknown, // 14 unused   0040D590
    Recompile_Unknown, // 15 unused   0040D590
    Recompile_Unknown, // 16 unused   0040D590
    Recompile_Unknown, // 17 unused   0040D590
    Recompile_ERET, // 18 ERET     00411120
    Recompile_Unknown, // 19 unused   0040D590
    Recompile_Unknown, // 1A unused   0040D590
    Recompile_Unknown, // 1B unused   0040D590
    Recompile_Unknown, // 1C unused   0040D590
    Recompile_Unknown, // 1D unused   0040D590
    Recompile_Unknown, // 1E unused   0040D590
    Recompile_Unknown, // 1F unused   0040D590
    Recompile_Unknown, // 20 unused   0040D590
    Recompile_Unknown, // 21 unused   0040D590
    Recompile_Unknown, // 22 unused   0040D590
    Recompile_Unknown, // 23 unused   0040D590
    Recompile_Unknown, // 24 unused   0040D590
    Recompile_Unknown, // 25 unused   0040D590
    Recompile_Unknown, // 26 unused   0040D590
    Recompile_Unknown, // 27 unused   0040D590
    Recompile_Unknown, // 28 unused   0040D590
    Recompile_Unknown, // 29 unused   0040D590
    Recompile_Unknown, // 2A unused   0040D590
    Recompile_Unknown, // 2B unused   0040D590
    Recompile_Unknown, // 2C unused   0040D590
    Recompile_Unknown, // 2D unused   0040D590
    Recompile_Unknown, // 2E unused   0040D590
    Recompile_Unknown, // 2F unused   0040D590
    Recompile_Unknown, // 30 unused   0040D590
    Recompile_Unknown, // 31 unused   0040D590
    Recompile_Unknown, // 32 unused   0040D590
    Recompile_Unknown, // 33 unused   0040D590
    Recompile_Unknown, // 34 unused   0040D590
    Recompile_Unknown, // 35 unused   0040D590
    Recompile_Unknown, // 36 unused   0040D590
    Recompile_Unknown, // 37 unused   0040D590
    Recompile_Unknown, // 38 unused   0040D590
    Recompile_Unknown, // 39 unused   0040D590
    Recompile_Unknown, // 3A unused   0040D590
    Recompile_Unknown, // 3B unused   0040D590
    Recompile_Unknown, // 3C unused   0040D590
    Recompile_Unknown, // 3D unused   0040D590
    Recompile_Unknown, // 3E unused   0040D590
    Recompile_Unknown, // 3F unused   0040D590
};

// Original VA 0x00477C20
DynarecOpcodeHandler Dynarec_Cop1Dispatch[32] = {
    Recompile_MFC1, // 00 MFC1     004111C0
    Recompile_DMFC1, // 01 DMFC1    004112A0
    Recompile_CFC1, // 02 CFC1     004113D0
    Recompile_Unknown, // 03 unused   0040D590
    Recompile_MTC1, // 04 MTC1     00411430
    Recompile_DMTC1, // 05 DMTC1    00411520
    Recompile_CTC1, // 06 CTC1     00411640
    Recompile_Unknown, // 07 unused   0040D590
    Recompile_COP1_BC, // 08 BC       00411690
    Recompile_Unknown, // 09 unused   0040D590
    Recompile_Unknown, // 0A unused   0040D590
    Recompile_Unknown, // 0B unused   0040D590
    Recompile_Unknown, // 0C unused   0040D590
    Recompile_Unknown, // 0D unused   0040D590
    Recompile_Unknown, // 0E unused   0040D590
    Recompile_Unknown, // 0F unused   0040D590
    Recompile_COP1_S, // 10 S        00411740
    Recompile_COP1_D, // 11 D        00411790
    Recompile_Unknown, // 12 unused   0040D590
    Recompile_Unknown, // 13 unused   0040D590
    Recompile_COP1_W, // 14 W        004117E0
    Recompile_COP1_L, // 15 L        00411830
    Recompile_Unknown, // 16 unused   0040D590
    Recompile_Unknown, // 17 unused   0040D590
    Recompile_Unknown, // 18 unused   0040D590
    Recompile_Unknown, // 19 unused   0040D590
    Recompile_Unknown, // 1A unused   0040D590
    Recompile_Unknown, // 1B unused   0040D590
    Recompile_Unknown, // 1C unused   0040D590
    Recompile_Unknown, // 1D unused   0040D590
    Recompile_Unknown, // 1E unused   0040D590
    Recompile_Unknown, // 1F unused   0040D590
};

// Original VA 0x00477CA0
DynarecOpcodeHandler Dynarec_Cop1FunctionDispatch[64] = {
    Recompile_COP1_ADD, // 00 ADD      00411880
    Recompile_COP1_SUB, // 01 SUB      004119C0
    Recompile_COP1_MUL, // 02 MUL      00411B00
    Recompile_COP1_DIV, // 03 DIV      00411C40
    Recompile_COP1_SQRT, // 04 SQRT     00411D80
    Recompile_COP1_ABS, // 05 ABS      00411E40
    Recompile_COP1_MOV, // 06 MOV      00411F00
    Recompile_COP1_NEG, // 07 NEG      00411FB0
    Recompile_COP1_ROUND_L, // 08 ROUND.L  00412070
    Recompile_COP1_TRUNC_L, // 09 TRUNC.L  00412140
    Recompile_COP1_CEIL_L, // 0A CEIL.L   00412210
    Recompile_COP1_FLOOR_L, // 0B FLOOR.L  004122E0
    Recompile_COP1_ROUND_W, // 0C ROUND.W  004123B0
    Recompile_COP1_TRUNC_W, // 0D TRUNC.W  00412480
    Recompile_COP1_CEIL_W, // 0E CEIL.W   00412550
    Recompile_COP1_FLOOR_W, // 0F FLOOR.W  00412620
    Recompile_Unknown, // 10 unused   0040D590
    Recompile_Unknown, // 11 unused   0040D590
    Recompile_Unknown, // 12 unused   0040D590
    Recompile_Unknown, // 13 unused   0040D590
    Recompile_Unknown, // 14 unused   0040D590
    Recompile_Unknown, // 15 unused   0040D590
    Recompile_Unknown, // 16 unused   0040D590
    Recompile_Unknown, // 17 unused   0040D590
    Recompile_Unknown, // 18 unused   0040D590
    Recompile_Unknown, // 19 unused   0040D590
    Recompile_Unknown, // 1A unused   0040D590
    Recompile_Unknown, // 1B unused   0040D590
    Recompile_Unknown, // 1C unused   0040D590
    Recompile_Unknown, // 1D unused   0040D590
    Recompile_Unknown, // 1E unused   0040D590
    Recompile_Unknown, // 1F unused   0040D590
    Recompile_COP1_CVT_S, // 20 CVT.S    004126F0
    Recompile_COP1_CVT_D, // 21 CVT.D    004127C0
    Recompile_Unknown, // 22 unused   0040D590
    Recompile_Unknown, // 23 unused   0040D590
    Recompile_COP1_CVT_W, // 24 CVT.W    00412890
    Recompile_COP1_CVT_L, // 25 CVT.L    00412960
    Recompile_Unknown, // 26 unused   0040D590
    Recompile_Unknown, // 27 unused   0040D590
    Recompile_Unknown, // 28 unused   0040D590
    Recompile_Unknown, // 29 unused   0040D590
    Recompile_Unknown, // 2A unused   0040D590
    Recompile_Unknown, // 2B unused   0040D590
    Recompile_Unknown, // 2C unused   0040D590
    Recompile_Unknown, // 2D unused   0040D590
    Recompile_Unknown, // 2E unused   0040D590
    Recompile_Unknown, // 2F unused   0040D590
    Recompile_COP1_C_F, // 30 C.F      00412A20
    Recompile_COP1_C_UN, // 31 C.UN     00412A40
    Recompile_COP1_C_EQ, // 32 C.EQ     00412EA0
    Recompile_COP1_C_EQ, // 33 C.UEQ    00412EA0
    Recompile_COP1_C_OLT, // 34 C.OLT    00412EB0
    Recompile_COP1_C_OLT, // 35 C.ULT    00412EB0
    Recompile_COP1_C_OLE, // 36 C.OLE    00412EC0
    Recompile_COP1_C_OLE, // 37 C.ULE    00412EC0
    Recompile_COP1_C_SF, // 38 C.SF     00412BD0
    Recompile_COP1_C_NGLE, // 39 C.NGLE   00412D30
    Recompile_COP1_C_EQ, // 3A C.SEQ    00412EA0
    Recompile_COP1_C_EQ, // 3B C.NGL    00412EA0
    Recompile_COP1_C_OLT, // 3C C.LT     00412EB0
    Recompile_COP1_C_OLT, // 3D C.NGE    00412EB0
    Recompile_COP1_C_OLE, // 3E C.LE     00412EC0
    Recompile_COP1_C_OLE, // 3F C.NGT    00412EC0
};
