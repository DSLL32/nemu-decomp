#include "RSP.h"
#include "Memory.h"
#include <stdio.h>
#include <math.h>

#define Instruction (*(RSPInstructionWord *)&CPU_Instruction)
#define PC SPReg.PC

CpuU32 RSPRegisters[32];
CpuU16 RSPVectors[32][8];
CpuU16 RSPFlags[3];
CpuS64 RSPAccumulator[8];
unsigned char SPMemory[0x4000];
CpuU32 RSPDelayPC;


extern void ReportCPUError(const char *message);
extern void ReportCPUErrorFormat(const char *format, ...);
CpuU32 RSPMemoryAddress;

// Original function 00401000.
bool RSP_CheckMemoryBreakpoint(CpuU32 address, CpuU32 access)
{
    if (MemoryBreakpointCount && !RSPBreakpointInhibit) {
        address &= 0xfffc;
        for (CpuU32 i = 0; i < MemoryBreakpointCount; i++)
            if (MemoryBreakpoints[i].Address >= 0x04000000 &&
                MemoryBreakpoints[i].Address < 0x04002000 &&
                address == (MemoryBreakpoints[i].Address & 0xffff) &&
                (MemoryBreakpoints[i].Access & access))
                return true;
    }
    return false;
}

// Original function 00401060.
bool RSP_CheckMemoryBreakpointRange(CpuU32 address, CpuU32 length, CpuU32 access)
{
    address &= 0xfffc;
    CpuU32 end = address + length;
    if (MemoryBreakpointCount && !RSPBreakpointInhibit)
        for (CpuU32 i = 0; i < MemoryBreakpointCount; i++)
            if (MemoryBreakpoints[i].Address >= 0x04000000 &&
                MemoryBreakpoints[i].Address < 0x04002000 &&
                (MemoryBreakpoints[i].Address & 0xffff) >= address &&
                (MemoryBreakpoints[i].Address & 0xffff) < end &&
                (MemoryBreakpoints[i].Access & access))
                return true;
    return false;
}

// Original function 004010D0.
inline CpuU8 RSP_ReadByte(CpuU32 address)
{
    if (RSP_CheckMemoryBreakpoint(RSPMemoryAddress, 1))
        CPU_ActionFlags |= 0x20000;
    return SPMemory[address & 0xfff];
}

// Original function 00401100.
inline void RSP_WriteByte(CpuU32 address, CpuU8 value)
{
    if (RSP_CheckMemoryBreakpoint(RSPMemoryAddress, 2))
        CPU_ActionFlags |= 0x20000;
    SPMemory[address & 0xfff] = value;
}

// Original selector tables 004798E8 and 00479908.
CpuU32 RSPElementLaneMask[8] = {7, 14, 12, 12, 0, 0, 0, 0};
CpuU32 RSPElementIndexMask[8] = {0, 1, 3, 3, 7, 7, 7, 7};
CpuU8 RSPCompareNotEqual[8]; // 00479E18
CpuU8 RSPCompareCarry[8]; // 00479E24

// The accumulator's upper sixteen storage bits are outside its signed 48-bit
// value. The bitfield assignment preserves those unused storage bits.
union RSPAccumulatorView {
    struct { CpuS64 Low : 16, Middle : 16, High : 16; } Words;
    struct { CpuS64 Value : 48; } Full;
    struct { CpuS64 Low : 16; CpuS64 Value : 32; } Middle;
};
#define AccumulatorValue(i) (((RSPAccumulatorView *)RSPAccumulator)[i].Full.Value)
#define AccumulatorMiddleValue(i) (((RSPAccumulatorView *)RSPAccumulator)[i].Middle.Value)

#define AccumulatorLow(i) (((RSPAccumulatorView *)RSPAccumulator)[i].Words.Low)
#define AccumulatorMiddle(i) (((RSPAccumulatorView *)RSPAccumulator)[i].Words.Middle)
#define AccumulatorHigh(i) (((RSPAccumulatorView *)RSPAccumulator)[i].Words.High)
#define VectorElement(i) ((RSPElementLaneMask[(Instruction.Value >> 22) & 7] & (i)) + ((Instruction.Value >> 21) & 15 & RSPElementIndexMask[(Instruction.Value >> 22) & 7]))

// Original table 00479368.
RSPOpcodeHandler RSP_MainDispatch[64] = {
    RSP_SPECIAL, RSP_REGIMM, RSP_J, RSP_JAL,
    RSP_BEQ, RSP_BNE, RSP_BLEZ, RSP_BGTZ,
    RSP_ADDI, RSP_ADDI, RSP_SLTI, RSP_SLTIU,
    RSP_ANDI, RSP_ORI, RSP_XORI, RSP_LUI,
    RSP_COP0, RSP_InvalidOpcode, RSP_COP2, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_LB, RSP_LH, RSP_InvalidOpcode, RSP_LW,
    RSP_LBU, RSP_LHU, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_SB, RSP_SH, RSP_InvalidOpcode, RSP_SW,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_LWC2, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_SWC2, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
};

// Original table 00479468.
RSPOpcodeHandler RSP_SpecialDispatch[64] = {
    RSP_SLL, RSP_InvalidOpcode, RSP_SRL, RSP_SRA,
    RSP_SLLV, RSP_InvalidOpcode, RSP_SRLV, RSP_SRAV,
    RSP_JR, RSP_JALR, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_BREAK, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_ADD, RSP_ADD, RSP_SUB, RSP_SUB,
    RSP_AND, RSP_OR, RSP_XOR, RSP_NOR,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_SLT, RSP_SLTU,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
};

// Original table 00479568.
RSPOpcodeHandler RSP_RegImmDispatch[32] = {
    RSP_BLTZ, RSP_BGEZ, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_BLTZAL, RSP_BGEZAL, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
};

// Original table 004795E8.
RSPOpcodeHandler RSP_Cop0Dispatch[32] = {
    RSP_MFC0, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_MTC0, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
};

// Original table 00479668.
RSPOpcodeHandler RSP_Cop2Dispatch[32] = {
    RSP_MFC2, RSP_InvalidOpcode, RSP_CFC2, RSP_InvalidOpcode,
    RSP_MTC2, RSP_InvalidOpcode, RSP_CTC2, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_VECTOR, RSP_VECTOR, RSP_VECTOR, RSP_VECTOR,
    RSP_VECTOR, RSP_VECTOR, RSP_VECTOR, RSP_VECTOR,
    RSP_VECTOR, RSP_VECTOR, RSP_VECTOR, RSP_VECTOR,
    RSP_VECTOR, RSP_VECTOR, RSP_VECTOR, RSP_VECTOR,
};

// Original table 004796E8.
RSPOpcodeHandler RSP_VectorLoadDispatch[32] = {
    RSP_LoadVector, RSP_LoadVector, RSP_LoadVector, RSP_LoadVector,
    RSP_LoadVector, RSP_LoadVector, RSP_LoadVector, RSP_LoadVector,
    RSP_LoadVector, RSP_LoadVector, RSP_InvalidOpcode, RSP_LoadVector,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
};

// Original table 00479768.
RSPOpcodeHandler RSP_VectorStoreDispatch[32] = {
    RSP_StoreVector, RSP_StoreVector, RSP_StoreVector, RSP_StoreVector,
    RSP_StoreVector, RSP_StoreVector, RSP_StoreVector, RSP_StoreVector,
    RSP_StoreVector, RSP_StoreVector, RSP_InvalidOpcode, RSP_StoreVector,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
};

// Original table 004797E8.
RSPOpcodeHandler RSP_VectorDispatch[64] = {
    RSP_VectorMultiply, RSP_VectorMultiply, RSP_VectorMultiply, RSP_VectorMultiply,
    RSP_VectorMultiply, RSP_VectorMultiply, RSP_VectorMultiply, RSP_VectorMultiply,
    RSP_VectorMultiply, RSP_VectorMultiply, RSP_VectorMultiply, RSP_VectorMultiply,
    RSP_VectorMultiply, RSP_VectorMultiply, RSP_VectorMultiply, RSP_VectorMultiply,
    RSP_VADD, RSP_VSUB, RSP_VSUT, RSP_VABS,
    RSP_VADDC, RSP_VSUBC, RSP_VADDB, RSP_VSUBB,
    RSP_VACCB, RSP_VSUCB, RSP_VSAD, RSP_VSAC,
    RSP_VSUM, RSP_VSAW, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_VLT, RSP_VEQ, RSP_VNE, RSP_VGE,
    RSP_VCL, RSP_VCH, RSP_VCR, RSP_VMRG,
    RSP_VAND, RSP_VNAND, RSP_VOR, RSP_VNOR,
    RSP_VXOR, RSP_VNXOR, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_ReciprocalInstruction, RSP_ReciprocalInstruction, RSP_ReciprocalInstruction, RSP_ReciprocalInstruction,
    RSP_ReciprocalInstruction, RSP_ReciprocalInstruction, RSP_ReciprocalInstruction, RSP_VNOOP,
    RSP_VEXTT, RSP_VEXTQ, RSP_VEXTN, RSP_InvalidOpcode,
    RSP_VINST, RSP_VINSQ, RSP_VINSN, RSP_InvalidOpcode,
};

// Original function 004026B0.
void RSP_InvalidOpcode()
{
    ReportCPUErrorFormat("Invalid RSP Opcode:\nPC = 0x%X", PC);
    PC += 4;
}

// Original function 004026E0.
void RSP_SPECIAL()
{
    RSP_SpecialDispatch[Instruction.Bytes[0] & 63]();
}


// Original function 00402700.
void RSP_REGIMM()
{
    RSP_RegImmDispatch[Instruction.rt]();
}


// Original function 00402720.
void RSP_J()
{
    RSPDelayPC = PC + 4;
    PC = (Instruction.Value & 0x3ff) << 2;
}


// Original function 00402750.
void RSP_JAL()
{
    RSPRegisters[31] = PC + 8;
    RSPDelayPC = PC + 4;
    PC = (Instruction.Value & 0x3ff) << 2;
}


// Original function 00402780.
void RSP_BEQ()
{
    PC += 4;
    if (RSPRegisters[Instruction.rs] == RSPRegisters[Instruction.rt])
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}


// Original function 004027D0.
void RSP_BNE()
{
    PC += 4;
    if (RSPRegisters[Instruction.rs] != RSPRegisters[Instruction.rt])
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}


// Original function 00402820.
void RSP_BLEZ()
{
    PC += 4;
    if ((CpuS32)RSPRegisters[Instruction.rs] <= 0)
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}


// Original function 00402860.
void RSP_BGTZ()
{
    PC += 4;
    if ((CpuS32)RSPRegisters[Instruction.rs] > 0)
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}


// Original function 004028A0.
void RSP_ADDI()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value;
}


// Original function 004028D0.
void RSP_SLTI()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = (CpuS32)RSPRegisters[Instruction.rs] < (CpuS16)Instruction.Value;
}


// Original function 00402910.
void RSP_SLTIU()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = RSPRegisters[Instruction.rs] < (CpuU32)(CpuS16)Instruction.Value;
}


// Original function 00402950.
void RSP_ANDI()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = RSPRegisters[Instruction.rs] & (Instruction.Value & 0xffff);
}


// Original function 00402990.
void RSP_ORI()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = RSPRegisters[Instruction.rs] | (Instruction.Value & 0xffff);
}


// Original function 004029D0.
void RSP_XORI()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = RSPRegisters[Instruction.rs] ^ (Instruction.Value & 0xffff);
}


// Original function 00402A10.
void RSP_LUI()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = Instruction.Value << 16;
}


// Original function 00402A40.
void RSP_COP0()
{
    RSP_Cop0Dispatch[Instruction.rs]();
}


// Original function 00402A60.
void RSP_COP2()
{
    RSP_Cop2Dispatch[Instruction.rs]();
}


// Original function 00402A80.
void RSP_LB()
{
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    if (RSP_CheckMemoryBreakpoint(address, 1))
    {
        CPU_ActionFlags |= 0x20000;
        return;
    }
    PC += 4;
    RSPRegisters[Instruction.rt] = (CpuS8)SPMemory[address];
}

// Original function 00402AF0.
void RSP_LH()
{
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    if (RSP_CheckMemoryBreakpoint(address, 1))
    {
        CPU_ActionFlags |= 0x20000;
        return;
    }
    PC += 4;
    RSPRegisters[Instruction.rt] = (CpuS16)(SPMemory[address] << 8) | SPMemory[address + 1];
}

// Original function 00402B70.
void RSP_LW()
{
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    if (RSP_CheckMemoryBreakpoint(address, 1))
    {
        CPU_ActionFlags |= 0x20000;
        return;
    }
    PC += 4;
    RSPRegisters[Instruction.rt] = (SPMemory[address] << 24) | (SPMemory[address + 1] << 16) |
        (SPMemory[address + 2] << 8) | SPMemory[address + 3];
}

// Original function 00402C00.
void RSP_LBU()
{
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    if (RSP_CheckMemoryBreakpoint(address, 1))
    {
        CPU_ActionFlags |= 0x20000;
        return;
    }
    PC += 4;
    RSPRegisters[Instruction.rt] = SPMemory[address];
}

// Original function 00402C70.
void RSP_LHU()
{
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    if (RSP_CheckMemoryBreakpoint(address, 1))
    {
        CPU_ActionFlags |= 0x20000;
        return;
    }
    PC += 4;
    RSPRegisters[Instruction.rt] = (SPMemory[address] << 8) | SPMemory[address + 1];
}

// Original function 00402CE0.
void RSP_SB()
{
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    if (RSP_CheckMemoryBreakpoint(address, 2))
    {
        CPU_ActionFlags |= 0x20000;
        return;
    }
    PC += 4;
    SPMemory[address] = (CpuU8)RSPRegisters[Instruction.rt];
}

// Original function 00402D50.
void RSP_SH()
{
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    if (RSP_CheckMemoryBreakpoint(address, 2))
    {
        CPU_ActionFlags |= 0x20000;
        return;
    }
    PC += 4;
    SPMemory[address] = (CpuU8)(RSPRegisters[Instruction.rt] >> 8);
    SPMemory[address + 1] = (CpuU8)(RSPRegisters[Instruction.rt]);
}

// Original function 00402DD0.
void RSP_SW()
{
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    if (RSP_CheckMemoryBreakpoint(address, 2))
    {
        CPU_ActionFlags |= 0x20000;
        return;
    }
    PC += 4;
    SPMemory[address] = (CpuU8)(RSPRegisters[Instruction.rt] >> 24);
    SPMemory[address + 1] = (CpuU8)(RSPRegisters[Instruction.rt] >> 16);
    SPMemory[address + 2] = (CpuU8)(RSPRegisters[Instruction.rt] >> 8);
    SPMemory[address + 3] = (CpuU8)(RSPRegisters[Instruction.rt]);
}

// Original function 00402E60.
void RSP_LWC2()
{
    RSP_VectorLoadDispatch[Instruction.rd]();
    if (RSP_CheckMemoryBreakpoint(RSPMemoryAddress, 1))
        CPU_ActionFlags |= 0x20000;
}

// Original function 00402EA0.
void RSP_SWC2()
{
    RSP_VectorStoreDispatch[Instruction.rd]();
    if (RSP_CheckMemoryBreakpoint(RSPMemoryAddress, 2))
        CPU_ActionFlags |= 0x20000;
}

// Original function 00402EE0.
void RSP_SLL()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rt] << Instruction.sa;
}


// Original function 00402F20.
void RSP_SRL()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rt] >> Instruction.sa;
}


// Original function 00402F60.
void RSP_SRA()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = (CpuS32)RSPRegisters[Instruction.rt] >> Instruction.sa;
}


// Original function 00402FA0.
void RSP_SLLV()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rt] << (RSPRegisters[Instruction.rs] & 31);
}


// Original function 00402FF0.
void RSP_SRLV()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rt] >> (RSPRegisters[Instruction.rs] & 31);
}


// Original function 00403040.
void RSP_SRAV()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = (CpuS32)RSPRegisters[Instruction.rt] >> (RSPRegisters[Instruction.rs] & 31);
}


// Original function 00403090.
void RSP_JR()
{
    RSPDelayPC = PC + 4;
    PC = RSPRegisters[Instruction.rs] & 0xfff;
}


// Original function 004030C0.
void RSP_JALR()
{
    RSPRegisters[Instruction.rd] = PC + 8;
    RSPDelayPC = PC + 4;
    PC = RSPRegisters[Instruction.rs] & 0xfff;
}


// Original function 00403100.
void RSP_BREAK()
{
    PC += 4;
    CPU_ActionFlags &= ~0x4000;
    SPReg.Status |= 3;
    if (SPReg.Status & 0x40)
        CPU_ActionFlags |= 8;
}


// Original function 00403130.
void RSP_ADD()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] + RSPRegisters[Instruction.rt];
}


// Original function 00403180.
void RSP_SUB()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] - RSPRegisters[Instruction.rt];
}


// Original function 004031D0.
void RSP_AND()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] & RSPRegisters[Instruction.rt];
}


// Original function 00403220.
void RSP_OR()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] | RSPRegisters[Instruction.rt];
}


// Original function 00403270.
void RSP_XOR()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] ^ RSPRegisters[Instruction.rt];
}


// Original function 004032C0.
void RSP_NOR()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = ~(RSPRegisters[Instruction.rs] | RSPRegisters[Instruction.rt]);
}


// Original function 00403310.
void RSP_SLT()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = (CpuS32)RSPRegisters[Instruction.rs] < (CpuS32)RSPRegisters[Instruction.rt];
}


// Original function 00403360.
void RSP_SLTU()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] < RSPRegisters[Instruction.rt];
}


// Original function 004033B0.
void RSP_BLTZ()
{
    PC += 4;
    if ((CpuS32)RSPRegisters[Instruction.rs] < 0)
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}


// Original function 004033F0.
void RSP_BGEZ()
{
    PC += 4;
    if ((CpuS32)RSPRegisters[Instruction.rs] >= 0)
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}


// Original function 00403430.
void RSP_BLTZAL()
{
    RSPRegisters[31] = PC + 8;
    PC += 4;
    if ((CpuS32)RSPRegisters[Instruction.rs] < 0)
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}


// Original function 00403480.
void RSP_BGEZAL()
{
    RSPRegisters[31] = PC + 8;
    PC += 4;
    if ((CpuS32)RSPRegisters[Instruction.rs] >= 0)
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}


// Original function 004034D0.
void RSP_MFC0()
{
    PC += 4;
    CpuU32 rt = Instruction.rt;
    switch (Instruction.rd)
    {
    case 0: RSPRegisters[rt] = ReadPhysicalWord(0x04040000); break;
    case 1: RSPRegisters[rt] = ReadPhysicalWord(0x04040004); break;
    case 2: RSPRegisters[rt] = ReadPhysicalWord(0x04040008); break;
    case 3: RSPRegisters[rt] = ReadPhysicalWord(0x0404000c); break;
    case 4: RSPRegisters[rt] = ReadPhysicalWord(0x04040010); break;
    case 5: RSPRegisters[rt] = ReadPhysicalWord(0x04040014); break;
    case 6: RSPRegisters[rt] = ReadPhysicalWord(0x04040018); break;
    case 7: RSPRegisters[rt] = ReadPhysicalWord(0x0404001c); break;
    case 8: RSPRegisters[rt] = ReadPhysicalWord(0x04100000); break;
    case 9: RSPRegisters[rt] = ReadPhysicalWord(0x04100004); break;
    case 10: RSPRegisters[rt] = ReadPhysicalWord(0x04100008); break;
    case 11: RSPRegisters[rt] = ReadPhysicalWord(0x0410000c); break;
    case 12: RSPRegisters[rt] = ReadPhysicalWord(0x04100010); break;
    case 13: RSPRegisters[rt] = ReadPhysicalWord(0x04100014); break;
    case 14: RSPRegisters[rt] = ReadPhysicalWord(0x04100018); break;
    case 15: RSPRegisters[rt] = ReadPhysicalWord(0x0410001c); break;
    default: ReportCPUError("Access (read) from invalid RSP COP0 register");
    }
}

// Original function 00403680.
void RSP_MTC0()
{
    CpuU32 rt = Instruction.rt;
    switch (Instruction.rd)
    {
    case 0:
        if (RSPRegisters[rt] & 0xffffe000)
            ReportCPUErrorFormat("Invalid value written to SP_MEM_ADDR_REG\n PC = 0x%X\n Value = 0x%X\n", PC, RSPRegisters[rt]);
        WritePhysicalWord(0x04040000, RSPRegisters[rt]);
        break;
    case 1: WritePhysicalWord(0x04040004, RSPRegisters[rt]); break;
    case 2:
        if (RSP_CheckMemoryBreakpointRange(ReadPhysicalWord(0x04040000) | 0x04000000,
                                          RSPRegisters[rt] + 1, 2))
        { CPU_ActionFlags |= 0x20000; return; }
        WritePhysicalWord(0x04040008, RSPRegisters[rt]);
        break;
    case 3:
        if (RSP_CheckMemoryBreakpointRange(ReadPhysicalWord(0x04040000) | 0x04000000,
                                          RSPRegisters[rt] + 1, 1))
        { CPU_ActionFlags |= 0x20000; return; }
        WritePhysicalWord(0x0404000c, RSPRegisters[rt]);
        break;
    case 4: WritePhysicalWord(0x04040010, RSPRegisters[rt]); break;
    case 7: WritePhysicalWord(0x0404001c, RSPRegisters[rt]); break;
    case 8: WritePhysicalWord(0x04100000, RSPRegisters[rt]); break;
    case 9: WritePhysicalWord(0x04100004, RSPRegisters[rt]); break;
    case 11: WritePhysicalWord(0x0410000c, RSPRegisters[rt]); break;
    case 12: WritePhysicalWord(0x04100010, RSPRegisters[rt]); break;
    default: ReportCPUError("Access (write) to invalid RSP COP0 register");
    }
    PC += 4;
}

// Original function 004038F0.
void RSP_MFC2()
{
    PC += 4;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    CpuU32 rd = Instruction.rd;
    RSPRegisters[Instruction.rt] = (CpuS16)(((CpuU8 (*)[16])RSPVectors)[rd][element ^ 1] << 8) | ((CpuU8 (*)[16])RSPVectors)[rd][(element + 1) ^ 1];
}

// Original function 00403950.
void RSP_CFC2()
{
    PC += 4;
    CpuU32 rt = Instruction.rt;
    CpuU8 control = Instruction.rd & 3;
    if (control == 0)
        RSPRegisters[rt] = (*(CpuU32 *)RSPFlags & 0x8000 ? 0xffff0000 : 0) | RSPFlags[0];
    else if (control == 1)
        RSPRegisters[rt] = (*(CpuU32 *)RSPFlags & 0x8000 ? 0xffff0000 : 0) | RSPFlags[1];
    else if (control == 2)
        RSPRegisters[rt] = RSPFlags[2];
    else ReportCPUError("RSP_CFC2 error: unknown control reg");
}

// Original function 004039F0.
void RSP_MTC2()
{
    PC += 4;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    CpuU32 rd = Instruction.rd;
    CpuU16 value = (CpuU16)RSPRegisters[Instruction.rt];
    ((CpuU8 (*)[16])RSPVectors)[rd][element ^ 1] = (CpuU8)(value >> 8);
    ((CpuU8 (*)[16])RSPVectors)[rd][(element + 1) ^ 1] = (CpuU8)value;
}

// Original function 00403A50.
void RSP_CTC2()
{
    PC += 4;
    CpuU32 rt = Instruction.rt;
    CpuU8 control = Instruction.rd & 3;
    if (control == 0) RSPFlags[0] = (CpuU16)RSPRegisters[rt];
    else if (control == 1) RSPFlags[1] = (CpuU16)RSPRegisters[rt];
    else if (control == 2) RSPFlags[2] = (CpuU8)RSPRegisters[rt];
    else ReportCPUError("RSP_CTC2 error: unknown control reg");
}

// Original function 00403AC0.
void RSP_VECTOR()
{
    RSP_VectorDispatch[Instruction.Bytes[0] & 63]();
}


// Original function 00404800.
void RSP_VADDB()
{
    PC += 4;
    ReportCPUError("RSP_VADDB");
}

// Original function 00404820.
void RSP_VSUBB()
{
    PC += 4;
    ReportCPUError("RSP_VSUBB");
}

// Original function 00404840.
void RSP_VACCB()
{
    PC += 4;
    ReportCPUError("RSP_VACCB");
}

// Original function 00404860.
void RSP_VSUCB()
{
    PC += 4;
    ReportCPUError("RSP_VSUCB");
}

// Original function 00404880.
void RSP_VSAD()
{
    PC += 4;
    ReportCPUError("RSP_VSAD");
}

// Original function 004048A0.
void RSP_VSAC()
{
    PC += 4;
    ReportCPUError("RSP_VSAC");
}

// Original function 004048C0.
void RSP_VSUM()
{
    PC += 4;
    ReportCPUError("RSP_VSUM");
}

// Original function 00404F20.
void RSP_VEQ()
{
    PC += 4;
    ReportCPUError("RSP_VEQ");
}

// Original function 00404F40.
void RSP_VNE()
{
    PC += 4;
    ReportCPUError("RSP_VNE");
}

// Original function 00405900.
void RSP_VCH()
{
    PC += 4;
    ReportCPUError("RSP_VCH");
}

// Original function 00405920.
void RSP_VCR()
{
    PC += 4;
    ReportCPUError("RSP_VCR");
}

// Original function 00406420.
void RSP_VNOOP()
{
    PC += 4;
    ReportCPUError("RSP_VNOOP");
}

// Original function 00406440.
void RSP_VEXTT()
{
    PC += 4;
    ReportCPUError("RSP_VEXTT");
}

// Original function 00406460.
void RSP_VEXTQ()
{
    PC += 4;
    ReportCPUError("RSP_VEXTQ");
}

// Original function 00406480.
void RSP_VEXTN()
{
    PC += 4;
    ReportCPUError("RSP_VEXTN");
}

// Original function 004064A0.
void RSP_VINST()
{
    PC += 4;
    ReportCPUError("RSP_VINST");
}

// Original function 004064C0.
void RSP_VINSQ()
{
    PC += 4;
    ReportCPUError("RSP_VINSQ");
}

// Original function 004064E0.
void RSP_VINSN()
{
    PC += 4;
    ReportCPUError("RSP_VINSN");
}


// Original function 00403AE0.
void RSP_VADD()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = (CpuS16)RSPVectors[vs][i] + (CpuS16)RSPVectors[vt][VectorElement(i)] + ((RSPFlags[0] & (1 << i)) != 0);
        AccumulatorLow(i) = (CpuU16)result;
        if (result > 32767) result = 32767;
        else if (result < -32768) result = -32768;
        RSPVectors[vd][i] = (CpuU16)result;
        RSPFlags[0] &= ~(0x101 << i);
    }
}

// Original function 00403D60.
void RSP_VSUB()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = (CpuS16)RSPVectors[vs][i] - (CpuS16)RSPVectors[vt][VectorElement(i)] - ((RSPFlags[0] & (1 << i)) != 0);
        AccumulatorLow(i) = (CpuU16)result;
        if (result > 32767) result = 32767;
        else if (result < -32768) result = -32768;
        RSPVectors[vd][i] = (CpuU16)result;
        RSPFlags[0] &= ~(0x101 << i);
    }
}

// Original function 00403FE0.
void RSP_VSUT()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = (CpuS16)RSPVectors[vt][VectorElement(i)] - (CpuS16)RSPVectors[vs][i] - ((RSPFlags[0] & (1 << i)) != 0);
        AccumulatorLow(i) = (CpuU16)result;
        if (result > 32767) result = 32767;
        else if (result < -32768) result = -32768;
        RSPVectors[vd][i] = (CpuU16)result;
        RSPFlags[0] &= ~(0x1 << i);
    }
}

// Original function 00404260.
void RSP_VABS()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    for (int i = 0; i < 8; i++)
    {
        CpuS16 source = (CpuS16)RSPVectors[vs][i];
        CpuS16 value = (CpuS16)RSPVectors[vt][VectorElement(i)];
        CpuS32 result;
        if (source < 0) result = -value;
        else if (source == 0) result = 0;
        else result = value;
        AccumulatorLow(i) = (CpuU16)result;
        if (result > 32767) result = 32767;
        else if (result < -32768) result = -32768;
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original function 00404470.
void RSP_VADDC()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    RSPFlags[0] = 0;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 result = (CpuU32)RSPVectors[vs][i] + RSPVectors[vt][VectorElement(i)];
        AccumulatorLow(i) = (CpuU16)result;
        RSPVectors[vd][i] = (CpuU16)result;
        RSPFlags[0] |= (result > 0xffff) << i;
    }
}

// Original function 00404620.
void RSP_VSUBC()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    RSPFlags[0] = 0;
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = (CpuU32)RSPVectors[vs][i] - RSPVectors[vt][VectorElement(i)];
        AccumulatorLow(i) = (CpuU16)result;
        int flags;
        if (result < 0) flags = 0x101;
        else if (result == 0) flags = 0;
        else flags = 0x100;
        RSPVectors[vd][i] = (CpuU16)result;
        RSPFlags[0] |= flags << i;
    }
}

// Original function 004048E0.
void RSP_VSAW()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 7;
    switch (element)
    {
    case 0:
        for (int i = 0; i < 8; i++) RSPVectors[vd][i] = AccumulatorHigh(i);
        break;
    case 1:
        for (int j = 0; j < 8; j++) RSPVectors[vd][j] = AccumulatorMiddle(j);
        break;
    case 2:
        for (int k = 0; k < 8; k++) RSPVectors[vd][k] = AccumulatorLow(k);
        break;
    default: ReportCPUError("RSP_VSAW error: invalid element");
    }
}

// Original function 00404B40.
void RSP_VLT()
{
    PC += 4;
    for (int j = 0; j < 8; j++)
    {
        RSPCompareCarry[j] = (RSPFlags[0] >> j) & 1;
        RSPCompareNotEqual[j] = ~(RSPFlags[0] >> (j + 8)) & 1;
    }
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    RSPFlags[1] = 0;
    for (int i = 0; i < 8; i++)
    {
        CpuS16 left = (CpuS16)RSPVectors[vs][i];
        CpuS16 right = (CpuS16)RSPVectors[vt][VectorElement(i)];
        CpuS16 result;
        if ((left < right) || (left == right && RSPCompareCarry[i] && !RSPCompareNotEqual[i]))
        {
            RSPFlags[1] |= 1 << i;
            result = left;
        }
        else
        {
            RSPFlags[1] &= ~(1 << i);
            result = right;
        }
        RSPVectors[vd][i] = (CpuU16)result;
        AccumulatorLow(i) = (CpuU16)result;
        RSPFlags[0] &= ~(0x101 << i);
    }
}

// Original function 00404F60.
void RSP_VGE()
{
    PC += 4;
    for (int j = 0; j < 8; j++)
    {
        RSPCompareCarry[j] = (RSPFlags[0] >> j) & 1;
        RSPCompareNotEqual[j] = ~(RSPFlags[0] >> (j + 8)) & 1;
    }
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    RSPFlags[1] = 0;
    for (int i = 0; i < 8; i++)
    {
        CpuS16 left = (CpuS16)RSPVectors[vs][i];
        CpuS16 right = (CpuS16)RSPVectors[vt][VectorElement(i)];
        CpuS16 result;
        if ((left > right) || (left == right && (RSPCompareNotEqual[i] || !RSPCompareCarry[i])))
        {
            RSPFlags[1] |= 1 << i;
            result = left;
        }
        else
        {
            RSPFlags[1] &= ~(1 << i);
            result = right;
        }
        RSPVectors[vd][i] = (CpuU16)result;
        AccumulatorLow(i) = (CpuU16)result;
        RSPFlags[0] &= ~(0x101 << i);
    }
}

// Original function 00405340.
void RSP_VCL()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 left = RSPVectors[vs][i];
        CpuU32 right = RSPVectors[vt][VectorElement(i)];
        CpuU32 low = (RSPFlags[1] >> i) & 1;
        CpuU32 high = (RSPFlags[1] >> (i + 8)) & 1;
        CpuU32 equal = ~(RSPFlags[0] >> (i + 8)) & 1;
        CpuU32 extension = (RSPFlags[2] >> i) & 1;
        CpuU16 result;
        if (!((RSPFlags[0] >> i) & 1))
        {
            if (equal) high = (CpuS32)(left - right) >= 0;
            result = (CpuU16)(high ? right : left);
        }
        else
        {
            CpuU32 sum = left + right;
            if (equal)
                low = ((!extension && sum < 0x10000 && !(sum & 0xffff)) |
                      (extension & (sum < 0x10000 || !(sum & 0xffff))));
            result = (CpuU16)(low ? -right : left);
        }
        RSPVectors[vd][i] = result;
        AccumulatorLow(i) = result;
        RSPFlags[1] = (RSPFlags[1] & ~(0x101 << i)) | (low << i) | (high << (i + 8));
    }
    RSPFlags[0] = 0;
    RSPFlags[2] = 0;
}

// Original function 00405940.
void RSP_VMRG()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    RSPFlags[0] = 0;
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU16 left = RSPVectors[vs][i];
        CpuU16 right = RSPVectors[vt][VectorElement(i)];
        if (RSPFlags[1] & (1 << i)) {
            RSPVectors[vd][i] = left;
            AccumulatorLow(i) = left;
        } else {
            RSPVectors[vd][i] = right;
            AccumulatorLow(i) = right;
        }
    }
}

// Original function 00405A30.
void RSP_VAND()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = RSPVectors[vs][i] & RSPVectors[vt][VectorElement(i)];
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
    }
}

// Original function 00405BD0.
void RSP_VNAND()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = ~(RSPVectors[vs][i] & RSPVectors[vt][VectorElement(i)]);
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
    }
}

// Original function 00405D80.
void RSP_VOR()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = RSPVectors[vs][i] | RSPVectors[vt][VectorElement(i)];
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
    }
}

// Original function 00405F20.
void RSP_VNOR()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = ~(RSPVectors[vs][i] | RSPVectors[vt][VectorElement(i)]);
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
    }
}

// Original function 004060D0.
void RSP_VXOR()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = RSPVectors[vs][i] ^ RSPVectors[vt][VectorElement(i)];
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
    }
}

// Original function 00406270.
void RSP_VNXOR()
{
    PC += 4;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 15;
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = ~(RSPVectors[vs][i] ^ RSPVectors[vt][VectorElement(i)]);
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
    }
}

// Original function 00401660: vector loads share address and element decoding.
void RSP_LoadVector()
{
    PC += 4;
    CpuU32 kind = Instruction.rd, vt = Instruction.rt;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    CpuU32 address;
    CpuU32 group;
    if (kind == 11) group = vt & 24;
    CpuS32 offset = (CpuS32)(Instruction.Value << 25) >> 25;
    CpuU8 *bytes = (CpuU8 *)RSPVectors[vt];
    int i;
    switch (kind) {
    case 0:
        address = RSPRegisters[Instruction.rs] + offset * 1;
        for (i = 0; i < 1; i++)
            bytes[(element + i) ^ 1] = RSP_ReadByte(address + i);
        break;
    case 1:
        address = RSPRegisters[Instruction.rs] + offset * 2;
        for (i = 0; i < 2; i++)
            bytes[(element + i) ^ 1] = RSP_ReadByte(address + i);
        break;
    case 2:
        address = RSPRegisters[Instruction.rs] + offset * 4;
        for (i = 0; i < 4; i++)
            bytes[(element + i) ^ 1] = RSP_ReadByte(address + i);
        break;
    case 3:
        address = RSPRegisters[Instruction.rs] + offset * 8;
        for (i = 0; i < 8; i++)
            bytes[(element + i) ^ 1] = RSP_ReadByte(address + i);
        break;
    case 4:
        address = RSPRegisters[Instruction.rs] + offset * 16;
        for (i = 0; !i || ((address + i) & 15); i++)
            bytes[(element + i) ^ 1] = RSP_ReadByte(address + i);
        break;
    case 5:
        address = RSPRegisters[Instruction.rs] + offset * 16;
        for (i = 15; ((address + i) & 15) != 15; i--)
            if ((int)(element + i) < 16)
                bytes[(element + i) ^ 1] = RSP_ReadByte(address + i - 16);
        break;
    case 6:
        address = RSPRegisters[Instruction.rs] + offset * 8;
        for (i = 0; i < 8; i++)
            RSPVectors[vt][i] = RSP_ReadByte(address + i) << 8;
        break;
    case 7:
        address = RSPRegisters[Instruction.rs] + offset * 8;
        for (i = 0; i < 8; i++)
            RSPVectors[vt][i] = RSP_ReadByte(address + i) << 7;
        break;
    case 8:
        address = RSPRegisters[Instruction.rs] + offset * 16;
        for (i = 0; i < 8; i++)
            RSPVectors[vt][i] = RSP_ReadByte((address & ~7) + (((address & 7) + i * 2) & 15)) << 7;
        break;
    case 9:
        address = RSPRegisters[Instruction.rs] + offset * 16;
        for (i = 0; i < 4; i++)
            RSPVectors[vt][(element >> 1) + i] = RSP_ReadByte((address & ~7) + (((address & 7) + i * 4) & 15)) << 7;
        break;
    case 11:
        {
            CpuU16 transpose[8];
            address = RSPRegisters[Instruction.rs] + offset * 16;
            int first = ((address >> 3) & 1) << 2;
            address &= ~7;
            for (i = 0; i < 8; i++) {
                ((CpuU8 *)transpose)[((first + i) % 8) * 2 + 1] = RSP_ReadByte(address + i * 2);
                ((CpuU8 *)transpose)[((first + i) % 8) * 2] = RSP_ReadByte(address + i * 2 + 1);
            }
            for (i = 0; i < 8; i++) {
                CpuU32 lane = ((element >> 1) + i) & 7;
                RSPVectors[group + lane][i] = transpose[lane];
            }
        }
        break;
    }
    RSPMemoryAddress = address;
}

// Original function 00401D60: vector stores, including transpose addressing.
void RSP_StoreVector()
{
    PC += 4;
    CpuU32 kind = Instruction.rd, vt = Instruction.rt;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    CpuU32 address;
    CpuU32 group;
    if (kind == 11) group = vt & 24;
    CpuS32 offset = (CpuS32)(Instruction.Value << 25) >> 25;
    CpuU8 *bytes = (CpuU8 *)RSPVectors[vt];
    int i;
    if (kind < 12) switch (kind) {
    case 0:
        address = RSPRegisters[Instruction.rs] + offset * 1;
        for (i = 0; i < 1; i++)
            RSP_WriteByte(address + i, bytes[(element + i) ^ 1]);
        break;
    case 1:
        address = RSPRegisters[Instruction.rs] + offset * 2;
        for (i = 0; i < 2; i++)
            RSP_WriteByte(address + i, bytes[(element + i) ^ 1]);
        break;
    case 2:
        address = RSPRegisters[Instruction.rs] + offset * 4;
        for (i = 0; i < 4; i++)
            RSP_WriteByte(address + i, bytes[(element + i) ^ 1]);
        break;
    case 3:
        address = RSPRegisters[Instruction.rs] + offset * 8;
        for (i = 0; i < 8; i++)
            RSP_WriteByte(address + i, bytes[(element + i) ^ 1]);
        break;
    case 4:
        address = RSPRegisters[Instruction.rs] + offset * 16;
        for (i = 0; !i || ((address + i) & 15); i++)
            RSP_WriteByte(address + i, bytes[(element + i) ^ 1]);
        break;
    case 5:
        address = RSPRegisters[Instruction.rs] + offset * 16;
        for (i = 15; ((address + i) & 15) != 15; i--)
            RSP_WriteByte(address + i - 16, bytes[(element + i) ^ 1]);
        break;
    case 6:
        address = RSPRegisters[Instruction.rs] + offset * 8;
        for (i = 0; i < 8; i++)
            RSP_WriteByte(address + i, RSPVectors[vt][i] >> 8);
        break;
    case 7:
        address = RSPRegisters[Instruction.rs] + offset * 8;
        for (i = 0; i < 8; i++)
            RSP_WriteByte(address + i, RSPVectors[vt][i] >> 7);
        break;
    case 8:
        address = RSPRegisters[Instruction.rs] + offset * 16;
        for (i = 0; i < 8; i++)
            RSP_WriteByte((address & ~7) + (((address & 7) + i * 2) & 15), RSPVectors[vt][i] >> 7);
        break;
    case 9:
        address = RSPRegisters[Instruction.rs] + offset * 16;
        for (i = 0; i < 4; i++)
            RSP_WriteByte((address & ~7) + (((address & 7) + i * 4) & 15), RSPVectors[vt][(element >> 1) + i] >> 7);
        break;
    default:
        {
            CpuU8 transpose[16];
            if (kind == 10) ReportCPUError("RSP_swv");
            address = RSPRegisters[Instruction.rs] + offset * 16;
            for (i = 0; i < 8; i++) {
                CpuU32 lane = ((element >> 1) + i) & 7;
                CpuU16 value = kind == 11 ? RSPVectors[group + lane][i] : RSPVectors[vt][lane];
                transpose[(address + i * 2) & 15] = value >> 8;
                transpose[(address + i * 2 + 1) & 15] = value;
            }
            if ((address & 15) > 7) {
                for (i = -8; i < 8; i += 2) {
                    RSP_WriteByte((address & ~7) + i + 8, transpose[i & 15]);
                    RSP_WriteByte((address & ~7) + i + 9, transpose[(i + 1) & 15]);
                }
            } else {
                for (i = 0; i < 8; i++) {
                    RSP_WriteByte((address & ~7) + i * 2, transpose[i * 2]);
                    RSP_WriteByte((address & ~7) + i * 2 + 1, transpose[i * 2 + 1]);
                }
            }
        }
        break;
    }
    RSPMemoryAddress = address;
}

// RSP division ROM and its optional arithmetic reconstruction. Original data
// 00478320, 00478328, 00479328, 00479E30 and 0047AE30.
bool RSPUseReciprocalROM = true;
CpuU32 RSPReciprocalROM[1024] = {
    0x0fff0, 0x0fef8, 0x0fe00, 0x0fd08, 0x0fc10, 0x0fb18, 0x0fa20, 0x0f928,
    0x0f830, 0x0f738, 0x0f640, 0x0f548, 0x0f450, 0x0f358, 0x0f260, 0x0f168,
    0x0f06e, 0x0ef85, 0x0ee9c, 0x0edb3, 0x0ecca, 0x0ebe1, 0x0eaf8, 0x0ea0f,
    0x0e926, 0x0e83d, 0x0e754, 0x0e66b, 0x0e582, 0x0e499, 0x0e3b0, 0x0e2c7,
    0x0e1d4, 0x0e0f8, 0x0e01c, 0x0df40, 0x0de64, 0x0dd88, 0x0dcac, 0x0dbd0,
    0x0daf4, 0x0da18, 0x0d93c, 0x0d860, 0x0d784, 0x0d6a8, 0x0d5cc, 0x0d4f0,
    0x0d410, 0x0d340, 0x0d270, 0x0d1a0, 0x0d0d0, 0x0d000, 0x0cf30, 0x0ce60,
    0x0cd90, 0x0ccc0, 0x0cbf0, 0x0cb20, 0x0ca50, 0x0c980, 0x0c8b0, 0x0c7e0,
    0x0c710, 0x0c64c, 0x0c588, 0x0c4c4, 0x0c400, 0x0c33c, 0x0c278, 0x0c1b4,
    0x0c0f0, 0x0c02c, 0x0bf68, 0x0bea4, 0x0bde0, 0x0bd1c, 0x0bc58, 0x0bb94,
    0x0bac4, 0x0ba0a, 0x0b950, 0x0b896, 0x0b7dc, 0x0b722, 0x0b668, 0x0b5ae,
    0x0b4f4, 0x0b43a, 0x0b380, 0x0b2c6, 0x0b20c, 0x0b152, 0x0b098, 0x0afde,
    0x0af1d, 0x0ae6d, 0x0adbd, 0x0ad0d, 0x0ac5d, 0x0abad, 0x0aafd, 0x0aa4d,
    0x0a99d, 0x0a8ed, 0x0a83d, 0x0a78d, 0x0a6dd, 0x0a62d, 0x0a57d, 0x0a4cd,
    0x0a410, 0x0a368, 0x0a2c0, 0x0a218, 0x0a170, 0x0a0c8, 0x0a020, 0x09f78,
    0x09ed0, 0x09e28, 0x09d80, 0x09cd8, 0x09c30, 0x09b88, 0x09ae0, 0x09a38,
    0x09990, 0x098f1, 0x09852, 0x097b3, 0x09714, 0x09675, 0x095d6, 0x09537,
    0x09498, 0x093f9, 0x0935a, 0x092bb, 0x0921c, 0x0917d, 0x090de, 0x0903f,
    0x08f93, 0x08efb, 0x08e63, 0x08dcb, 0x08d33, 0x08c9b, 0x08c03, 0x08b6b,
    0x08ad3, 0x08a3b, 0x089a3, 0x0890b, 0x08873, 0x087db, 0x08743, 0x086ab,
    0x0860f, 0x0857e, 0x084ed, 0x0845c, 0x083cb, 0x0833a, 0x082a9, 0x08218,
    0x08187, 0x080f6, 0x08065, 0x07fd4, 0x07f43, 0x07eb2, 0x07e21, 0x07d90,
    0x07cfd, 0x07c73, 0x07be9, 0x07b5f, 0x07ad5, 0x07a4b, 0x079c1, 0x07937,
    0x078ad, 0x07823, 0x07799, 0x0770f, 0x07685, 0x075fb, 0x07571, 0x074e7,
    0x07455, 0x073d1, 0x0734d, 0x072c9, 0x07245, 0x071c1, 0x0713d, 0x070b9,
    0x07035, 0x06fb1, 0x06f2d, 0x06ea9, 0x06e25, 0x06da1, 0x06d1d, 0x06c99,
    0x06c0f, 0x06b91, 0x06b13, 0x06a95, 0x06a17, 0x06999, 0x0691b, 0x0689d,
    0x0681f, 0x067a1, 0x06723, 0x066a5, 0x06627, 0x065a9, 0x0652b, 0x064ad,
    0x06425, 0x063ac, 0x06333, 0x062ba, 0x06241, 0x061c8, 0x0614f, 0x060d6,
    0x0605d, 0x05fe4, 0x05f6b, 0x05ef2, 0x05e79, 0x05e00, 0x05d87, 0x05d0e,
    0x05c91, 0x05c1d, 0x05ba9, 0x05b35, 0x05ac1, 0x05a4d, 0x059d9, 0x05965,
    0x058f1, 0x0587d, 0x05809, 0x05795, 0x05721, 0x056ad, 0x05639, 0x055c5,
    0x0554f, 0x054e0, 0x05471, 0x05402, 0x05393, 0x05324, 0x052b5, 0x05246,
    0x051d7, 0x05168, 0x050f9, 0x0508a, 0x0501b, 0x04fac, 0x04f3d, 0x04ece,
    0x04e58, 0x04ded, 0x04d82, 0x04d17, 0x04cac, 0x04c41, 0x04bd6, 0x04b6b,
    0x04b00, 0x04a95, 0x04a2a, 0x049bf, 0x04954, 0x048e9, 0x0487e, 0x04813,
    0x047a8, 0x04742, 0x046dc, 0x04676, 0x04610, 0x045aa, 0x04544, 0x044de,
    0x04478, 0x04412, 0x043ac, 0x04346, 0x042e0, 0x0427a, 0x04214, 0x041ae,
    0x0413b, 0x040d9, 0x04077, 0x04015, 0x03fb3, 0x03f51, 0x03eef, 0x03e8d,
    0x03e2b, 0x03dc9, 0x03d67, 0x03d05, 0x03ca3, 0x03c41, 0x03bdf, 0x03b7d,
    0x03b0e, 0x03aaf, 0x03a50, 0x039f1, 0x03992, 0x03933, 0x038d4, 0x03875,
    0x03816, 0x037b7, 0x03758, 0x036f9, 0x0369a, 0x0363b, 0x035dc, 0x0357d,
    0x0351c, 0x034c1, 0x03466, 0x0340b, 0x033b0, 0x03355, 0x032fa, 0x0329f,
    0x03244, 0x031e9, 0x0318e, 0x03133, 0x030d8, 0x0307d, 0x03022, 0x02fc7,
    0x02f63, 0x02f0b, 0x02eb3, 0x02e5b, 0x02e03, 0x02dab, 0x02d53, 0x02cfb,
    0x02ca3, 0x02c4b, 0x02bf3, 0x02b9b, 0x02b43, 0x02aeb, 0x02a93, 0x02a3b,
    0x029df, 0x0298a, 0x02935, 0x028e0, 0x0288b, 0x02836, 0x027e1, 0x0278c,
    0x02737, 0x026e2, 0x0268d, 0x02638, 0x025e3, 0x0258e, 0x02539, 0x024e4,
    0x0248d, 0x0243b, 0x023e9, 0x02397, 0x02345, 0x022f3, 0x022a1, 0x0224f,
    0x021fd, 0x021ab, 0x02159, 0x02107, 0x020b5, 0x02063, 0x02011, 0x01fbf,
    0x01f6c, 0x01f1d, 0x01ece, 0x01e7f, 0x01e30, 0x01de1, 0x01d92, 0x01d43,
    0x01cf4, 0x01ca5, 0x01c56, 0x01c07, 0x01bb8, 0x01b69, 0x01b1a, 0x01acb,
    0x01a77, 0x01a2b, 0x019df, 0x01993, 0x01947, 0x018fb, 0x018af, 0x01863,
    0x01817, 0x017cb, 0x0177f, 0x01733, 0x016e7, 0x0169b, 0x0164f, 0x01603,
    0x015ad, 0x01563, 0x01519, 0x014cf, 0x01485, 0x0143b, 0x013f1, 0x013a7,
    0x0135d, 0x01313, 0x012c9, 0x0127f, 0x01235, 0x011eb, 0x011a1, 0x01157,
    0x0110d, 0x010c6, 0x0107f, 0x01038, 0x00ff1, 0x00faa, 0x00f63, 0x00f1c,
    0x00ed5, 0x00e8e, 0x00e47, 0x00e00, 0x00db9, 0x00d72, 0x00d2b, 0x00ce4,
    0x00c93, 0x00c4e, 0x00c09, 0x00bc4, 0x00b7f, 0x00b3a, 0x00af5, 0x00ab0,
    0x00a6b, 0x00a26, 0x009e1, 0x0099c, 0x00957, 0x00912, 0x008cd, 0x00888,
    0x0083e, 0x007fb, 0x007b8, 0x00775, 0x00732, 0x006ef, 0x006ac, 0x00669,
    0x00626, 0x005e3, 0x005a0, 0x0055d, 0x0051a, 0x004d7, 0x00494, 0x00451,
    0x0040c, 0x003cc, 0x0038c, 0x0034c, 0x0030c, 0x002cc, 0x0028c, 0x0024c,
    0x0020c, 0x001cc, 0x0018c, 0x0014c, 0x0010c, 0x000cc, 0x0008c, 0x0004c,
    0x069ff, 0x0fff0, 0x06953, 0x0fefc, 0x068a7, 0x0fe08, 0x067fb, 0x0fd14,
    0x0674f, 0x0fc20, 0x066a3, 0x0fb2c, 0x065f7, 0x0fa38, 0x0654b, 0x0f944,
    0x0649f, 0x0f850, 0x063f3, 0x0f75c, 0x06347, 0x0f668, 0x0629b, 0x0f574,
    0x061ef, 0x0f480, 0x06143, 0x0f38c, 0x06097, 0x0f298, 0x05feb, 0x0f1a4,
    0x05f31, 0x0f0a9, 0x05e93, 0x0efca, 0x05df5, 0x0eeeb, 0x05d57, 0x0ee0c,
    0x05cb9, 0x0ed2d, 0x05c1b, 0x0ec4e, 0x05b7d, 0x0eb6f, 0x05adf, 0x0ea90,
    0x05a41, 0x0e9b1, 0x059a3, 0x0e8d2, 0x05905, 0x0e7f3, 0x05867, 0x0e714,
    0x057c9, 0x0e635, 0x0572b, 0x0e556, 0x0568d, 0x0e477, 0x055ef, 0x0e398,
    0x0554c, 0x0e2ab, 0x054bb, 0x0e1de, 0x0542a, 0x0e111, 0x05399, 0x0e044,
    0x05308, 0x0df77, 0x05277, 0x0deaa, 0x051e6, 0x0dddd, 0x05155, 0x0dd10,
    0x050c4, 0x0dc43, 0x05033, 0x0db76, 0x04fa2, 0x0daa9, 0x04f11, 0x0d9dc,
    0x04e80, 0x0d90f, 0x04def, 0x0d842, 0x04d5e, 0x0d775, 0x04ccd, 0x0d6a8,
    0x04c32, 0x0d5cc, 0x04bac, 0x0d50e, 0x04b26, 0x0d450, 0x04aa0, 0x0d392,
    0x04a1a, 0x0d2d4, 0x04994, 0x0d216, 0x0490e, 0x0d158, 0x04888, 0x0d09a,
    0x04802, 0x0cfdc, 0x0477c, 0x0cf1e, 0x046f6, 0x0ce60, 0x04670, 0x0cda2,
    0x045ea, 0x0cce4, 0x04564, 0x0cc26, 0x044de, 0x0cb68, 0x04458, 0x0caaa,
    0x043ca, 0x0c9e7, 0x0434e, 0x0c937, 0x042d2, 0x0c887, 0x04256, 0x0c7d7,
    0x041da, 0x0c727, 0x0415e, 0x0c677, 0x040e2, 0x0c5c7, 0x04066, 0x0c517,
    0x03fea, 0x0c467, 0x03f6e, 0x0c3b7, 0x03ef2, 0x0c307, 0x03e76, 0x0c257,
    0x03dfa, 0x0c1a7, 0x03d7e, 0x0c0f7, 0x03d02, 0x0c047, 0x03c86, 0x0bf97,
    0x03bfc, 0x0bedf, 0x03b88, 0x0be3b, 0x03b14, 0x0bd97, 0x03aa0, 0x0bcf3,
    0x03a2c, 0x0bc4f, 0x039b8, 0x0bbab, 0x03944, 0x0bb07, 0x038d0, 0x0ba63,
    0x0385c, 0x0b9bf, 0x037e8, 0x0b91b, 0x03774, 0x0b877, 0x03700, 0x0b7d3,
    0x0368c, 0x0b72f, 0x03618, 0x0b68b, 0x035a4, 0x0b5e7, 0x03530, 0x0b543,
    0x034b9, 0x0b499, 0x0344d, 0x0b400, 0x033e1, 0x0b367, 0x03375, 0x0b2ce,
    0x03309, 0x0b235, 0x0329d, 0x0b19c, 0x03231, 0x0b103, 0x031c5, 0x0b06a,
    0x03159, 0x0afd1, 0x030ed, 0x0af38, 0x03081, 0x0ae9f, 0x03015, 0x0ae06,
    0x02fa9, 0x0ad6d, 0x02f3d, 0x0acd4, 0x02ed1, 0x0ac3b, 0x02e65, 0x0aba2,
    0x02df0, 0x0ab01, 0x02d8b, 0x0aa72, 0x02d26, 0x0a9e3, 0x02cc1, 0x0a954,
    0x02c5c, 0x0a8c5, 0x02bf7, 0x0a836, 0x02b92, 0x0a7a7, 0x02b2d, 0x0a718,
    0x02ac8, 0x0a689, 0x02a63, 0x0a5fa, 0x029fe, 0x0a56b, 0x02999, 0x0a4dc,
    0x02934, 0x0a44d, 0x028cf, 0x0a3be, 0x0286a, 0x0a32f, 0x02805, 0x0a2a0,
    0x02795, 0x0a203, 0x02736, 0x0a17c, 0x026d7, 0x0a0f5, 0x02678, 0x0a06e,
    0x02619, 0x09fe7, 0x025ba, 0x09f60, 0x0255b, 0x09ed9, 0x024fc, 0x09e52,
    0x0249d, 0x09dcb, 0x0243e, 0x09d44, 0x023df, 0x09cbd, 0x02380, 0x09c36,
    0x02321, 0x09baf, 0x022c2, 0x09b28, 0x02263, 0x09aa1, 0x02204, 0x09a1a,
    0x0219c, 0x09992, 0x02142, 0x09913, 0x020e8, 0x09894, 0x0208e, 0x09815,
    0x02034, 0x09796, 0x01fda, 0x09717, 0x01f80, 0x09698, 0x01f26, 0x09619,
    0x01ecc, 0x0959a, 0x01e72, 0x0951b, 0x01e18, 0x0949c, 0x01dbe, 0x0941d,
    0x01d64, 0x0939e, 0x01d0a, 0x0931f, 0x01cb0, 0x092a0, 0x01c56, 0x09221,
    0x01bfc, 0x0919e, 0x01ba8, 0x09126, 0x01b54, 0x090ae, 0x01b00, 0x09036,
    0x01aac, 0x08fbe, 0x01a58, 0x08f46, 0x01a04, 0x08ece, 0x019b0, 0x08e56,
    0x0195c, 0x08dde, 0x01908, 0x08d66, 0x018b4, 0x08cee, 0x01860, 0x08c76,
    0x0180c, 0x08bfe, 0x017b8, 0x08b86, 0x01764, 0x08b0e, 0x01710, 0x08a96,
    0x016ad, 0x08a1c, 0x0165d, 0x089ab, 0x0160d, 0x0893a, 0x015bd, 0x088c9,
    0x0156d, 0x08858, 0x0151d, 0x087e7, 0x014cd, 0x08776, 0x0147d, 0x08705,
    0x0142d, 0x08694, 0x013dd, 0x08623, 0x0138d, 0x085b2, 0x0133d, 0x08541,
    0x012ed, 0x084d0, 0x0129d, 0x0845f, 0x0124d, 0x083ee, 0x011fd, 0x0837d,
    0x011a8, 0x08303, 0x0115c, 0x08298, 0x01110, 0x0822d, 0x010c4, 0x081c2,
    0x01078, 0x08157, 0x0102c, 0x080ec, 0x00fe0, 0x08081, 0x00f94, 0x08016,
    0x00f48, 0x07fab, 0x00efc, 0x07f40, 0x00eb0, 0x07ed5, 0x00e64, 0x07e6a,
    0x00e18, 0x07dff, 0x00dcc, 0x07d94, 0x00d80, 0x07d29, 0x00d34, 0x07cbe,
    0x00ce6, 0x07c47, 0x00c9e, 0x07be1, 0x00c56, 0x07b7b, 0x00c0e, 0x07b15,
    0x00bc6, 0x07aaf, 0x00b7e, 0x07a49, 0x00b36, 0x079e3, 0x00aee, 0x0797d,
    0x00aa6, 0x07917, 0x00a5e, 0x078b1, 0x00a16, 0x0784b, 0x009ce, 0x077e5,
    0x00986, 0x0777f, 0x0093e, 0x07719, 0x008f6, 0x076b3, 0x008ae, 0x0764d,
    0x00861, 0x075e3, 0x0081d, 0x07582, 0x007d9, 0x07521, 0x00795, 0x074c0,
    0x00751, 0x0745f, 0x0070d, 0x073fe, 0x006c9, 0x0739d, 0x00685, 0x0733c,
    0x00641, 0x072db, 0x005fd, 0x0727a, 0x005b9, 0x07219, 0x00575, 0x071b8,
    0x00531, 0x07157, 0x004ed, 0x070f6, 0x004a9, 0x07095, 0x00465, 0x07034,
    0x00414, 0x06fcf, 0x003d3, 0x06f73, 0x00392, 0x06f17, 0x00351, 0x06ebb,
    0x00310, 0x06e5f, 0x002cf, 0x06e03, 0x0028e, 0x06da7, 0x0024d, 0x06d4b,
    0x0020c, 0x06cef, 0x001cb, 0x06c93, 0x0018a, 0x06c37, 0x00149, 0x06bdb,
    0x00108, 0x06b7f, 0x000c7, 0x06b23, 0x00086, 0x06ac7, 0x00045, 0x06a6b,
};
CpuU32 RSPReciprocalElement[16] = {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7};
CpuU32 RSPComputedReciprocalROM[1024];
CpuU32 *RSPReciprocalTable;
CpuS32 RSPReciprocalInput; // 044CC544
CpuS32 RSPMoveResult; // 044CC548
CpuU32 RSPReciprocalResult; // 044CC54C
bool RSPReciprocalInitialized; // 044CC550
CpuU32 RSPReciprocalHighPending; // 044CC554

// Original function 00406820.
CpuU32 RSP_Reciprocal(CpuS32 value, CpuU32 squareRoot, CpuU32 singlePrecision)
{
    CpuU32 magnitude = value;
    if (singlePrecision) {
        if (value < 0) magnitude = -value;
    } else if (value < 0) {
        if (value < -32768) magnitude = ~value;
        else magnitude = -value;
    }
    bool found = false;
    int shift = 0, leading = 0;
    for (int i = 31; i >= 0; i--) {
        if ((magnitude & (1 << i)) && !found) {
            found = true;
            shift = leading;
        }
        leading++;
    }
    if (!magnitude) shift = singlePrecision ? 16 : 0;
    CpuU32 index = ((CpuS32)(magnitude << shift) >> 22) & 0x1ff;
    if (squareRoot) index = (index & 0x1fe) | (shift % 2) | 0x200;
    CpuU32 result = (CpuS32)((RSPReciprocalTable[index] | 0x10000) << 14) >>
        (squareRoot ? ((~shift >> 1) & 15) : (~shift & 31));
    if (value < 0) result = ~result;
    if (!value) return 0x7fffffff;
    if (value == -32768) result = 0xffff0000;
    return result;
}

// Original function 00406500; seven reciprocal/move opcodes use this body.
void RSP_ReciprocalInstruction()
{
    PC += 4;
    CpuU32 vt = Instruction.rt, element = Instruction.rs & 15;
    CpuU32 vd = Instruction.sa, destinationElement = Instruction.rd & 7;
    CpuU32 kind = Instruction.Value & 7;
    if (!RSPReciprocalInitialized) {
        RSPReciprocalInitialized = true;
        RSP_InitializeReciprocalTable();
    }
    CpuU16 input = RSPVectors[vt][RSPReciprocalElement[element]];
    CpuU32 result;
    switch (kind) {
    case 0:
    case 4:
        RSPReciprocalInput = (CpuS16)input;
        RSPReciprocalResult = RSP_Reciprocal(RSPReciprocalInput, kind == 4, 1);
        RSPReciprocalHighPending = 0;
        result = RSPReciprocalResult;
        break;
    case 1:
    case 5:
        if (!RSPReciprocalHighPending) RSPReciprocalInput = (CpuS16)input;
        else RSPReciprocalInput |= input;
        RSPReciprocalResult = RSP_Reciprocal(RSPReciprocalInput, kind == 5, !RSPReciprocalHighPending);
        RSPReciprocalHighPending = 0;
        result = RSPReciprocalResult;
        break;
    case 2:
    case 6:
        RSPReciprocalInput = (CpuS16)input << 16;
        result = RSPReciprocalResult >> 16;
        RSPReciprocalHighPending = 1;
        break;
    case 3:
        RSPMoveResult = (CpuS16)input;
        result = RSPMoveResult;
        break;
    default:
        goto copyAccumulator;
    }
    RSPVectors[vd][destinationElement] = result;
copyAccumulator:
    for (int i = 0; i < 8; i++) {
        AccumulatorLow(i) = RSPVectors[vt][VectorElement(i)];
        if (kind == 3 && destinationElement == i)
            RSPVectors[vd][destinationElement] = AccumulatorLow(i);
    }
}

// Original function 00406900. The diagnostic path intentionally leaves the
// stream open, as in the original routine; normal execution selects the ROM.
void RSP_InitializeReciprocalTable()
{
    if (RSPUseReciprocalROM) {
        RSPReciprocalTable = RSPReciprocalROM;
        return;
    }
    RSPReciprocalTable = RSPComputedReciprocalROM;
    RSPComputedReciprocalROM[0] = 0x1ffff;
    int i;
    for (i = 1; i < 512; i++)
        RSPComputedReciprocalROM[i] = (int)(512.0 / (512 + i) * 131072.0);
    RSPComputedReciprocalROM[513] = 0x1ffff;
    for (i = 2; i < 512; i += 2)
        RSPComputedReciprocalROM[513 + i] = (int)(sqrt(512.0 / (512 + i)) * 131072.0);
    RSPComputedReciprocalROM[512] = (int)(sqrt(0.5) * 131072.0);
    for (i = 2; i < 512; i += 2)
        RSPComputedReciprocalROM[512 + i] = (int)(sqrt(512.0 / ((512 + i) * 2.0)) * 131072.0);
    FILE *file = fopen("Test.txt", "wb");
    fprintf(file, "\n\n");
    for (i = 0; i < 1024; i++)
        fprintf(file, "divrom[%03x]=%04x \n", i, RSPReciprocalTable[i]);
    fprintf(file, "\n\n");
}

// Original function 00401140: VMUL*, VMUD*, VMAC*, VMAD* and rounding variants.
void RSP_VectorMultiply()
{
    PC += 4;
    CpuU32 vs = Instruction.rd, vt = Instruction.rt, vd = Instruction.sa;
    CpuU8 accumulate = (Instruction.Value >> 3) & 1;
    CpuU32 kind = Instruction.Value & 7;
    CpuU8 roundAccumulate = 0;
    int unsignedMultiply = 0;
    int productShift = 0, accumulatorShift = 0, rounding = 0;
    CpuU64 clampMin = 0xffffffffffff8000;
    CpuU64 clampMax = 0x0000000000007fff;
    CpuS64 clampMask = ~(((CpuU64)1 << 31) - 1);
    CpuU16 resultMask = 0xffff;
    CpuS64 result;
    switch (kind) {
    case 0:
        accumulatorShift = 16; productShift = 1;
        rounding = accumulate ? 0 : 32768;
        break;
    case 1:
        unsignedMultiply = 1; accumulatorShift = 16; productShift = 1;
        clampMin = 0; clampMax = 0xffffffffffffffff;
        rounding = accumulate ? 0 : 32768;
        break;
    case 2:
        accumulatorShift = 16; productShift = (Instruction.rd & 1) << 4;
        roundAccumulate = 1;
        break;
    case 3:
        accumulatorShift = 17; productShift = 16;
        clampMask = ~(((CpuU64)1 << 32) - 1);
        resultMask = 0xfff0; rounding = accumulate ? 0 : 0x1f0000;
        break;
    case 4:
        productShift = -16; clampMin = 0; clampMax = 0xffff;
        break;
    case 5:
        accumulatorShift = 16;
        break;
    case 6:
        break;
    case 7:
        accumulatorShift = 16; productShift = 16;
        break;
    default:
        ReportCPUError("RSP_VmultExec");
    }
    for (int i = 0; i < 8; i++) {
        CpuU32 element = Instruction.rs & 15;
        int lane = VectorElement(i);
        switch (kind) {
        case 0:
        case 1:
        case 7:
            result = (CpuS64)(CpuS16)RSPVectors[vt][lane] * (CpuS64)(CpuS16)RSPVectors[vs][i];
            break;
        case 2:
            if (accumulate) {
                if (AccumulatorValue(i) < 0) result = (CpuS16)RSPVectors[vt][lane];
                else result = 0;
            } else {
                if (AccumulatorValue(i) >= 0) result = (CpuS16)RSPVectors[vt][lane];
                else result = 0;
            }
            break;
        case 3:
            if (accumulate) {
                CpuS64 quantized = AccumulatorMiddleValue(i) >> 6;
                CpuS64 roundBit = (AccumulatorMiddleValue(i) >> 5) & 1;
                if (quantized < 0 && roundBit == 0) result = 32;
                else if (quantized > 0 && roundBit == 0) result = -32;
                else result = 0;
            } else {
                result = (CpuS64)(CpuS16)RSPVectors[vt][lane] * (CpuS64)(CpuS16)RSPVectors[vs][i];
                if (result < 0) rounding = 0x1f0000;
                else rounding = 0;
            }
            break;
        case 4:
            result = (CpuS64)RSPVectors[vt][lane] * (CpuS64)RSPVectors[vs][i];
            break;
        case 5:
            result = (CpuS64)(CpuS16)RSPVectors[vs][i] * (CpuS64)RSPVectors[vt][lane];
            break;
        case 6:
            result = (CpuS64)(CpuS16)RSPVectors[vt][lane] * (CpuS64)RSPVectors[vs][i];
            break;
        default:
            ReportCPUError("RSP_VmultExec");
        }
        if (productShift > 0) result <<= productShift;
        else if (productShift < 0) result = (CpuU64)result >> -productShift;
        result += rounding;
        if (accumulate | roundAccumulate) result += AccumulatorValue(i);
        if ((result >> 47) & 1) result = 0xffff000000000000 | (result & 0xffffffffffff);
        else result &= 0xffffffffffff;
        AccumulatorValue(i) = result;
        if (result < 0) {
            if (unsignedMultiply) result = 0;
            else if (~result & clampMask) result = (CpuS64)clampMin << accumulatorShift;
        } else {
            if (result & clampMask) result = (CpuS64)clampMax << accumulatorShift;
        }
        RSPVectors[vd][i] = (CpuU16)(result >> accumulatorShift) & resultMask;
    }
}
