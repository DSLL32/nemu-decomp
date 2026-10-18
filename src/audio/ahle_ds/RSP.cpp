// Audio RSP interpreter and MP3 guest-program translation.
// The original source filename is inferred from the native program grouping.
// See analysis/audio-rsp.json for exact and incomplete body evidence.
#include "RSP.h"
#include "transform.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#pragma intrinsic(_byteswap_ulong, memcpy)

// Original audio.dll: 0x10013B10.
void RSP_ReciprocalInstruction() {}

// Original audio.dll: 0x10013B20.
void RSP_VectorMultiplyExec(CpuU32 kind, int fast)
{
    CpuU8 accumulate = (Instruction.Value >> 3) & 1;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU8 roundAccumulate = 0;
    int unsignedMultiply;
    int productShift, accumulatorShift, rounding;
    CpuU16 resultMask;
    CpuS64 result;
    CpuU64 clampMin = 0xffffffffffff8000;
    CpuU64 clampMax = 0x0000000000007fff;
    CpuS64 clampMask = ~(((CpuU64)1 << 31) - 1);
    unsignedMultiply = 0;
    accumulatorShift = 0;
    productShift = 0;
    rounding = 0;
    resultMask = 0xffff;
    switch (kind) {
    case 0:
        rounding = accumulate ? 0 : 32768;
        accumulatorShift = 16; productShift = 1;
        break;
    case 1:
        unsignedMultiply = 1;
        rounding = accumulate ? 0 : 32768;
        accumulatorShift = 16; productShift = 1;
        clampMin = 0; clampMax = 0xffffffffffffffff;
        break;
    case 2:
        accumulatorShift = 16; productShift = ((CpuU8)Instruction.rd & 1) << 4;
        roundAccumulate = 1;
        break;
    case 3:
        rounding = accumulate ? 0 : 0x1f0000;
        accumulatorShift = 17; productShift = 16;
        clampMask = ~(((CpuU64)1 << 32) - 1);
        resultMask = 0xfff0;
        break;
    case 4:
        productShift = -16;
    case 6:
        clampMin = 0; clampMax = 0xffff;
        break;
    case 5:
        accumulatorShift = 16;
        break;
    case 7:
        accumulatorShift = 16; productShift = 16;
        break;
    }
    CpuU8 addAccumulator = accumulate | roundAccumulate;
    CpuU32 count = 8;
    if (fast) count = 1;
    for (CpuU32 i = 0; i < count; i++) {
        int lane = VectorElement(i);
        switch (kind) {
        case 0:
        case 1:
        case 7:
            result = (CpuS64)(CpuS16)RSPVectors[vt][lane] * (CpuS64)(CpuS16)RSPVectors[vs][i];
            break;
        case 2:
        {
            CpuS16 value = RSPVectors[vt][lane];
            if (accumulate) {
                if (AccumulatorValue(i) < 0) result = value;
                else result = 0;
            } else {
                if (AccumulatorValue(i) >= 0) result = value;
                else result = 0;
            }
            break;
        }
        case 3:
            if (accumulate) {
                CpuS32 quantized = AccumulatorMiddleValue(i) >> 6;
                CpuS64 roundBit = (AccumulatorMiddleValue(i) >> 5) & 1;
                if (quantized < 0 && roundBit == 0) result = 32;
                else if (quantized > 0 && roundBit == 0) result = -32;
                else result = 0;
            } else {
                result = (CpuS64)(CpuS16)RSPVectors[vt][lane] * (CpuS64)(CpuS16)RSPVectors[vs][i];
                if (result >= 0) rounding = 0;
                else rounding = 0x1f0000;
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
        }
        if (productShift > 0) result <<= productShift;
        else if (productShift < 0) result = (CpuU64)result >> -productShift;
        result += rounding;
        if (addAccumulator) result += AccumulatorValue(i);
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

// Original audio.dll: 0x10014050.
void RSP_VectorMultiply() { RSPPC += 4; RSP_VectorMultiplyExec(Instruction.funct & 7, RSPFastMode); }

// Original audio.dll: 0x10014080.
void RSP_VectorMultiplyOpcode(RSPInstructionWord opcode, int fast) { RSP_VectorMultiplyExec(opcode.funct & 7, fast); }

// Original audio.dll: 0x10014090.
unsigned RSP_SelectElement(unsigned element, unsigned lane) {
 if (element == 0) return lane;
 if ((element & 14) == 2) return (lane & 14) + (element & 1);
 if ((element & 12) == 4) return (lane & 12) + (element & 3);
 return element & 7;
}

// Original audio.dll: 0x100140D0.
void RSP_ReportError(const char *format, ...) {
 char buffer[300]; va_list arguments;
 va_start(arguments, format);
 vsprintf(buffer, format, arguments);
 va_end(arguments);
 MessageBoxA(NULL, buffer, "CPU error, continue?", MB_YESNO | MB_ICONHAND);
}

// Original audio.dll: 0x10014110.
static inline CpuU8 RSP_ReadByte(CpuU32 address) { return SPMemory[address]; }

// Original audio.dll: 0x10014120.
static inline void RSP_WriteByte(CpuU32 address, CpuU8 value) { SPMemory[address] = value; }

// Original audio.dll: 0x10014130.
void RSP_StoreVectorExec(RSPInstructionWord opcode)
{
    CpuU32 kind = opcode.rd, vt = opcode.rt;
    CpuS32 element = (opcode.Value >> 7) & 15;
    CpuU32 address;
    CpuU32 group;
    if (kind == 11) group = vt & 24;
    CpuS32 offset = (CpuS32)(opcode.Value << 25) >> 25;
    int i;
    switch (kind) {
    case 0:
        address = RSPRegisters[opcode.rs] + offset * 1;
        RSP_WriteByte(address + 0, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1]);
        break;
    case 1:
        address = RSPRegisters[opcode.rs] + offset * 2;
        RSP_WriteByte(address + 0, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1]);
        RSP_WriteByte(address + 1, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 1) ^ 1]);
        break;
    case 2:
        address = RSPRegisters[opcode.rs] + offset * 4;
        RSP_WriteByte(address + 0, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1]);
        RSP_WriteByte(address + 1, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 1) ^ 1]);
        RSP_WriteByte(address + 2, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 2) ^ 1]);
        RSP_WriteByte(address + 3, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 3) ^ 1]);
        break;
    case 3:
        address = RSPRegisters[opcode.rs] + offset * 8;
        RSP_WriteByte(address + 0, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1]);
        RSP_WriteByte(address + 1, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 1) ^ 1]);
        RSP_WriteByte(address + 2, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 2) ^ 1]);
        RSP_WriteByte(address + 3, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 3) ^ 1]);
        RSP_WriteByte(address + 4, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 4) ^ 1]);
        RSP_WriteByte(address + 5, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 5) ^ 1]);
        RSP_WriteByte(address + 6, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 6) ^ 1]);
        RSP_WriteByte(address + 7, ((CpuU8 (*)[16])RSPVectors)[vt][(element + 7) ^ 1]);
        break;
    case 4:
        address = RSPRegisters[opcode.rs] + offset * 16;
        for (i = 0; !i || ((address + i) & 15); i++)
            RSP_WriteByte(address + i, ((CpuU8 (*)[16])RSPVectors)[vt][(element + i) ^ 1]);
        break;
    case 5:
        address = RSPRegisters[opcode.rs] + offset * 16;
        for (i = 15; ((address + i) & 15) != 15; i--)
            RSP_WriteByte(address + i - 16, ((CpuU8 (*)[16])RSPVectors)[vt][(element + i) ^ 1]);
        break;
    case 7:
        address = RSPRegisters[opcode.rs] + offset * 8;
        for (i = 0; i < 8; i++)
            RSP_WriteByte(address + i, RSPVectors[vt][i] >> 7);
        break;
    case 6:
        address = RSPRegisters[opcode.rs] + offset * 8;
        for (i = 0; i < 8; i++)
            RSP_WriteByte(address + i, RSPVectors[vt][i] >> 8);
        break;
    case 8:
    {
        address = RSPRegisters[opcode.rs] + offset * 16;
        CpuU8 displacement = (CpuU8)address;
        displacement &= 7;
        CpuU32 base = address - displacement;
        for (i = 0; i < 8; i++)
            RSP_WriteByte(base + ((displacement + i * 2) & 15), RSPVectors[vt][i] >> 7);
        break;
    }
    case 9:
    {
        address = RSPRegisters[opcode.rs] + offset * 16;
        CpuU8 displacement = (CpuU8)address;
        displacement &= 7;
        CpuU32 base = address - displacement;
        for (i = 0; i < 4; i++)
            RSP_WriteByte(base + ((displacement + i * 4) & 15), RSPVectors[vt][(element >> 1) + i] >> 7);
        break;
    }
    case 10:
    case 11:
        {
            CpuU8 transpose[16];
            address = RSPRegisters[opcode.rs] + offset * 16;
            for (i = 0; i < 8; i++) {
                CpuU32 lane = ((element >> 1) + i) & 7;
                CpuU16 value = kind == 11 ? RSPVectors[group + lane][i] : RSPVectors[vt][lane];
                transpose[(address + i * 2) & 15] = value >> 8;
                transpose[(address + i * 2 + 1) & 15] = value;
            }
            if ((address & 15) < 8) {
                for (i = 0; i < 8; i++) {
                    RSP_WriteByte((address & ~7) + i * 2, transpose[i * 2]);
                    RSP_WriteByte((address & ~7) + i * 2 + 1, transpose[i * 2 + 1]);
                }
            } else {
                for (i = 0; i < 8; i++) {
                    RSP_WriteByte((address & ~7) + i * 2, transpose[(i * 2 + 8) & 15]);
                    RSP_WriteByte((address & ~7) + i * 2 + 1, transpose[(i * 2 + 9) & 15]);
                }
            }
        }
        break;
    }
    RSPMemoryAddress = address;
}

// Original audio.dll: 0x10014A00.
void RSP_StoreVector() { RSPPC += 4; RSP_StoreVectorExec(*(RSPInstructionWord *)&CPU_Instruction); }

// Original audio.dll: 0x10014A20.
void RSP_StoreVectorOpcode(RSPInstructionWord opcode) { RSP_StoreVectorExec(*(RSPInstructionWord *)&CPU_Instruction); }

// Original audio.dll: 0x10014A30.
void RSP_InvalidOpcode()
{
    RSP_ReportError("Invalid RSP Opcode:\nPC = 0x%X", PC);
    PC += 4;
}

// Original audio.dll: 0x10014A60.
void RSP_SPECIAL()
{
    RSP_SpecialDispatch[Instruction.Bytes[0] & 63]();
}

// Original audio.dll: 0x10014A80.
void RSP_REGIMM()
{
    RSP_RegImmDispatch[Instruction.rt]();
}

// Original audio.dll: 0x10014AA0.
void RSP_J()
{
    RSPDelayPC = PC + 4;
    PC = (Instruction.Value & 0x3ff) << 2;
}

// Original audio.dll: 0x10014AD0.
void RSP_JAL()
{
    RSPRegisters[31] = PC + 8;
    RSPDelayPC = PC + 4;
    PC = (Instruction.Value & 0x3ff) << 2;
}

// Original audio.dll: 0x10014B00.
void RSP_BEQ()
{
    PC += 4;
    if (RSPRegisters[Instruction.rs] == RSPRegisters[Instruction.rt])
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}

// Original audio.dll: 0x10014B50.
void RSP_BNE()
{
    PC += 4;
    if (RSPRegisters[Instruction.rs] != RSPRegisters[Instruction.rt])
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}

// Original audio.dll: 0x10014BA0.
void RSP_BLEZ()
{
    PC += 4;
    if ((CpuS32)RSPRegisters[Instruction.rs] <= 0)
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}

// Original audio.dll: 0x10014BE0.
void RSP_BGTZ()
{
    PC += 4;
    if ((CpuS32)RSPRegisters[Instruction.rs] > 0)
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}

// Original audio.dll: 0x10014C20.
void RSP_ADDI()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value;
}

// Original audio.dll: 0x10014C50.
void RSP_ADDIOpcode(RSPInstructionWord opcode)
{
    RSPRegisters[opcode.rt] = RSPRegisters[opcode.rs] + (CpuS16)opcode.Value;
}

// Original audio.dll: 0x10014C80.
void RSP_ADDIU()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value;
}

// Original audio.dll: 0x10014CB0.
void RSP_SLTI()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = (CpuS32)RSPRegisters[Instruction.rs] < (CpuS16)Instruction.Value;
}

// Original audio.dll: 0x10014CF0.
void RSP_SLTIU()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = RSPRegisters[Instruction.rs] < (CpuU32)(CpuS16)Instruction.Value;
}

// Original audio.dll: 0x10014D30.
void RSP_ANDI()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = RSPRegisters[Instruction.rs] & (Instruction.Value & 0xffff);
}

// Original audio.dll: 0x10014D70.
void RSP_ANDIOpcode(RSPInstructionWord opcode)
{
    RSPRegisters[opcode.rt] = RSPRegisters[opcode.rs] & (opcode.Value & 0xffff);
}

// Original audio.dll: 0x10014DA0.
void RSP_ORI()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = RSPRegisters[Instruction.rs] | (Instruction.Value & 0xffff);
}

// Original audio.dll: 0x10014DE0.
void RSP_XORI()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = RSPRegisters[Instruction.rs] ^ (Instruction.Value & 0xffff);
}

// Original audio.dll: 0x10014E20.
void RSP_LUI()
{
    PC += 4;
    RSPRegisters[Instruction.rt] = Instruction.Value << 16;
}

// Original audio.dll: 0x10014E50.
void RSP_COP0()
{
    RSP_Cop0Dispatch[Instruction.rs]();
}

// Original audio.dll: 0x10014E70.
void RSP_COP2()
{
    RSP_Cop2Dispatch[Instruction.rs]();
}

// Original audio.dll: 0x10014E90.
void RSP_LB()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    RSPRegisters[Instruction.rt] = (CpuS8)SPMemory[address];
}

// Original audio.dll: 0x10014ED0.
void RSP_LH()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    RSPRegisters[Instruction.rt] = (CpuS16)(SPMemory[address] << 8) | SPMemory[address + 1];
}

// Original audio.dll: 0x10014F20.
void RSP_LW()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    RSPRegisters[Instruction.rt] = (SPMemory[address] << 24) | (SPMemory[address + 1] << 16) |
        (SPMemory[address + 2] << 8) | SPMemory[address + 3];
}

// Original audio.dll: 0x10014F90.
void RSP_LWOpcode(RSPInstructionWord opcode)
{
    CpuU32 address = (RSPRegisters[opcode.rs] + (CpuS16)Instruction.Value) & 0xfff;
    RSPRegisters[opcode.rt] = (SPMemory[address] << 24) | (SPMemory[address + 1] << 16) |
        (SPMemory[address + 2] << 8) | SPMemory[address + 3];
}

// Original audio.dll: 0x10014FF0.
void RSP_LBU()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    RSPRegisters[Instruction.rt] = SPMemory[address];
}

// Original audio.dll: 0x10015030.
void RSP_LHU()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    RSPRegisters[Instruction.rt] = (SPMemory[address] << 8) | SPMemory[address + 1];
}

// Original audio.dll: 0x10015080.
void RSP_SB()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    SPMemory[address] = (CpuU8)RSPRegisters[Instruction.rt];
}

// Original audio.dll: 0x100150D0.
void RSP_SH()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    SPMemory[address] = (CpuU8)(RSPRegisters[Instruction.rt] >> 8);
    SPMemory[address + 1] = (CpuU8)(RSPRegisters[Instruction.rt]);
}

// Original audio.dll: 0x10015130.
void RSP_SW()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (CpuS16)Instruction.Value) & 0xfff;
    SPMemory[address] = (CpuU8)(RSPRegisters[Instruction.rt] >> 24);
    SPMemory[address + 1] = (CpuU8)(RSPRegisters[Instruction.rt] >> 16);
    SPMemory[address + 2] = (CpuU8)(RSPRegisters[Instruction.rt] >> 8);
    SPMemory[address + 3] = (CpuU8)(RSPRegisters[Instruction.rt]);
}

// Original audio.dll: 0x100151A0.
void RSP_SWOpcode(RSPInstructionWord opcode)
{
    CpuU32 address = (RSPRegisters[opcode.rs] + (CpuS16)Instruction.Value) & 0xfff;
    SPMemory[address] = (CpuU8)(RSPRegisters[opcode.rt] >> 24);
    SPMemory[address + 1] = (CpuU8)(RSPRegisters[opcode.rt] >> 16);
    SPMemory[address + 2] = (CpuU8)(RSPRegisters[opcode.rt] >> 8);
    SPMemory[address + 3] = (CpuU8)(RSPRegisters[opcode.rt]);
}

// Original audio.dll: 0x10015200.
void RSP_LWC2()
{
    RSP_VectorLoadDispatch[Instruction.rd]();
}

// Original audio.dll: 0x10015220.
void RSP_SWC2()
{
    RSP_VectorStoreDispatch[Instruction.rd]();
}

// Original audio.dll: 0x10015240.
void RSP_SLL()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rt] << Instruction.sa;
}

// Original audio.dll: 0x10015280.
void RSP_SLLOpcode(RSPInstructionWord opcode)
{
    RSPRegisters[opcode.rd] = RSPRegisters[opcode.rt] << opcode.sa;
}

// Original audio.dll: 0x100152B0.
void RSP_SRL()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rt] >> Instruction.sa;
}

// Original audio.dll: 0x100152F0.
void RSP_SRLOpcode(RSPInstructionWord opcode)
{
    RSPRegisters[opcode.rd] = RSPRegisters[opcode.rt] >> opcode.sa;
}

// Original audio.dll: 0x10015320.
void RSP_SRA()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = (CpuS32)RSPRegisters[Instruction.rt] >> Instruction.sa;
}

// Original audio.dll: 0x10015360.
void RSP_SLLV()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rt] << (RSPRegisters[Instruction.rs] & 31);
}

// Original audio.dll: 0x100153B0.
void RSP_SRLV()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rt] >> (RSPRegisters[Instruction.rs] & 31);
}

// Original audio.dll: 0x10015400.
void RSP_SRAV()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = (CpuS32)RSPRegisters[Instruction.rt] >> (RSPRegisters[Instruction.rs] & 31);
}

// Original audio.dll: 0x10015450.
int RSP_ClampSample(int value)
{
    if (value > 32767) return 32767;
    if (value < -32767) return -32767;
    return value;
}

// Original audio.dll: 0x10015470.
void RSP_MixAudio()
{
    CpuU32 command = RSPRegisters[26];
    int stepLeft = RSPRegisters[21];
    int stepRight = RSPRegisters[22];
    CpuU32 gainLeft = *(CpuU32 *)&RSPVectors[1][0];
    CpuU32 gainRight = *(CpuU32 *)&RSPVectors[1][2];
    CpuU32 gainWet = *(CpuU32 *)&RSPVectors[1][4];
    int stepWet = RSPRegisters[11];
    CpuU32 buffers = RSPRegisters[25];
    int disableWet = command & 0x10;
    int count = (command >> 8) & 0xff;
    short invert[4] = {
        -(int)((command >> 1) & 1), -(int)(command & 1),
        -(int)((command >> 1) & 4), -(int)((command >> 1) & 2)
    };
    int input = ((command >> 12) & 0xff0) >> 1;
    int left = ((buffers >> 20) & 0xff0) >> 1;
    int right = ((buffers >> 12) & 0xff0) >> 1;
    int wetLeft = ((buffers >> 4) & 0xff0) >> 1;
    int wetRight = (int)((buffers & 0xff) << 4) >> 1;
    CpuU16 *memory = (CpuU16 *)SPMemory;
    for (int i = 0; i < count; i += 8) {
        for (int j = 0; j < 8; j++) {
            int sample = (CpuS16)_rotl(memory[input], 8);
            input++;
            CpuU16 oldRight = memory[right];
            int valueLeft = (sample * (CpuU16)gainLeft >> 16) ^ invert[0];
            int valueRight = (sample * (CpuU16)gainRight >> 16) ^ invert[1];
            int oldWetLeft = (CpuS16)_rotl(memory[wetLeft], 8);
            int oldWetRight = (CpuS16)_rotl(memory[wetRight], 8);
            memory[left++] = (CpuU16)_rotl((CpuU16)RSP_ClampSample((CpuS16)_rotl(memory[left], 8) + valueLeft), 8);
            memory[right++] = (CpuU16)_rotl((CpuU16)RSP_ClampSample((CpuS16)_rotl(oldRight, 8) + valueRight), 8);
            if (!disableWet) {
                oldWetLeft += (valueLeft * (CpuU16)gainWet >> 16) ^ invert[2];
                oldWetRight += (valueRight * (CpuU16)gainWet >> 16) ^ invert[3];
            }
            memory[wetLeft++] = (CpuU16)_rotl((CpuU16)RSP_ClampSample(oldWetLeft), 8);
            memory[wetRight++] = (CpuU16)_rotl((CpuU16)RSP_ClampSample(oldWetRight), 8);
        }
        gainWet += stepWet;
        gainLeft += stepLeft;
        gainRight += stepRight;
    }
}

// Original audio.dll: 0x100156F0.
void RSP_JR()
{
    RSPDelayPC = PC + 4;
    PC = RSPRegisters[Instruction.rs] & 0xfff;
}

// Original audio.dll: 0x10015720.
void RSP_JALR()
{
    RSPRegisters[Instruction.rd] = PC + 8;
    RSPDelayPC = PC + 4;
    PC = RSPRegisters[Instruction.rs] & 0xfff;
}

// Original audio.dll: 0x10015760.
void RSP_BREAK()
{
}

// Original audio.dll: 0x10015770.
void RSP_ADD()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] + RSPRegisters[Instruction.rt];
}

// Original audio.dll: 0x100157C0.
void RSP_ADDOpcode(RSPInstructionWord opcode)
{
    RSPRegisters[opcode.rd] = RSPRegisters[opcode.rs] + RSPRegisters[opcode.rt];
}

// Original audio.dll: 0x10015800.
void RSP_ADDU()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] + RSPRegisters[Instruction.rt];
}

// Original audio.dll: 0x10015850.
void RSP_SUB()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] - RSPRegisters[Instruction.rt];
}

// Original audio.dll: 0x100158A0.
void RSP_SUBOpcode(RSPInstructionWord opcode)
{
    RSPRegisters[opcode.rd] = RSPRegisters[opcode.rs] - RSPRegisters[opcode.rt];
}

// Original audio.dll: 0x100158E0.
void RSP_SUBU()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] - RSPRegisters[Instruction.rt];
}

// Original audio.dll: 0x10015930.
void RSP_AND()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] & RSPRegisters[Instruction.rt];
}

// Original audio.dll: 0x10015980.
void RSP_OR()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] | RSPRegisters[Instruction.rt];
}

// Original audio.dll: 0x100159D0.
void RSP_OROpcode(RSPInstructionWord opcode)
{
    RSPRegisters[opcode.rd] = RSPRegisters[opcode.rs] | RSPRegisters[opcode.rt];
}

// Original audio.dll: 0x10015A10.
void RSP_XOR()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] ^ RSPRegisters[Instruction.rt];
}

// Original audio.dll: 0x10015A60.
void RSP_NOR()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = ~(RSPRegisters[Instruction.rs] | RSPRegisters[Instruction.rt]);
}

// Original audio.dll: 0x10015AB0.
void RSP_SLT()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = (CpuS32)RSPRegisters[Instruction.rs] < (CpuS32)RSPRegisters[Instruction.rt];
}

// Original audio.dll: 0x10015B00.
void RSP_SLTU()
{
    PC += 4;
    RSPRegisters[Instruction.rd] = RSPRegisters[Instruction.rs] < RSPRegisters[Instruction.rt];
}

// Original audio.dll: 0x10015B50.
void RSP_BLTZ()
{
    PC += 4;
    if ((CpuS32)RSPRegisters[Instruction.rs] < 0)
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}

// Original audio.dll: 0x10015B90.
void RSP_BGEZ()
{
    PC += 4;
    if ((CpuS32)RSPRegisters[Instruction.rs] >= 0)
    {
        RSPDelayPC = PC;
        PC += (CpuS16)Instruction.Value * 4;
    }
}

// Original audio.dll: 0x10015BD0.
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

// Original audio.dll: 0x10015C20.
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

// Original audio.dll: 0x10015C70.
static void RSP_WriteCOP0(CpuU32 address, CpuU32 value)
{
    if (address == 0x04040000) RSPMemAddressReg = value;
    else if (address == 0x04040004) RSPDramAddressReg = value;
    else if (address == 0x04040008) {
        RSPReadLengthReg = value;
        CpuU32 mem = RSPMemAddressReg & 0x1fff;
        CpuU32 ram = RSPDramAddressReg & 0xffffff;
        CpuU32 length = (value & 0xff8 | 7) + 1;
        CpuU32 count = ((value >> 12) & 0xff) + 1;
        CpuU32 skip = value >> 20;
        do {
            ReadAudioMemory(SPMemory + mem, RDRAM + ram, length);
            ram += length + skip;
            mem += length;
        } while (--count);
    }
    else if (address == 0x0404000c) {
        RSPWriteLengthReg = value;
        CpuU32 mem = RSPMemAddressReg & 0x1fff;
        CpuU32 ram = RSPDramAddressReg & 0xffffff;
        CpuU32 length = (value & 0xff8 | 7) + 1;
        CpuU32 count = ((value >> 12) & 0xff) + 1;
        CpuU32 skip = value >> 20;
        do {
            ReadAudioMemory(RDRAM + ram, SPMemory + mem, length);
            ram += length + skip;
            mem += length;
        } while (--count);
    }
}

// Original audio.dll: 0x10015D80.
CpuU32 RSP_ReadCOP0(CpuU32 address) { return 0; }

// Original audio.dll: 0x10015D90.
void RSP_MFC0()
{
    PC += 4;
    CpuU32 rt = Instruction.rt;
    switch (Instruction.rd)
    {
    case 0: RSPRegisters[rt] = RSP_ReadCOP0(0x04040000); break;
    case 1: RSPRegisters[rt] = RSP_ReadCOP0(0x04040004); break;
    case 2: RSPRegisters[rt] = RSP_ReadCOP0(0x04040008); break;
    case 3: RSPRegisters[rt] = RSP_ReadCOP0(0x0404000c); break;
    case 4: RSPRegisters[rt] = RSP_ReadCOP0(0x04040010); break;
    case 5: RSPRegisters[rt] = RSP_ReadCOP0(0x04040014); break;
    case 6: RSPRegisters[rt] = RSP_ReadCOP0(0x04040018); break;
    case 7: RSPRegisters[rt] = RSP_ReadCOP0(0x0404001c); break;
    case 8: RSPRegisters[rt] = RSP_ReadCOP0(0x04100000); break;
    case 9: RSPRegisters[rt] = RSP_ReadCOP0(0x04100004); break;
    case 10: RSPRegisters[rt] = RSP_ReadCOP0(0x04100008); break;
    case 11: RSPRegisters[rt] = RSP_ReadCOP0(0x0410000c); break;
    case 12: RSPRegisters[rt] = RSP_ReadCOP0(0x04100010); break;
    case 13: RSPRegisters[rt] = RSP_ReadCOP0(0x04100014); break;
    case 14: RSPRegisters[rt] = RSP_ReadCOP0(0x04100018); break;
    case 15: RSPRegisters[rt] = RSP_ReadCOP0(0x0410001c); break;
    }
}

// Original audio.dll: 0x10015F60.
void RSP_MTC0()
{
    CpuU32 rt = Instruction.rt;
    switch (Instruction.rd)
    {
    case 0:
        if (RSPRegisters[rt] & 0xffffe000)
            RSP_ReportError("Invalid value written to SP_MEM_ADDR_REG\n PC = 0x%X\n Value = 0x%X\n", PC, RSPRegisters[rt]);
        RSP_WriteCOP0(0x04040000, RSPRegisters[rt]);
        break;
    case 1: RSP_WriteCOP0(0x04040004, RSPRegisters[rt]); break;
    case 2:
        RSP_WriteCOP0(0x04040008, RSPRegisters[rt]);
        break;
    case 3:
        RSP_WriteCOP0(0x0404000c, RSPRegisters[rt]);
        break;
    case 4: RSP_WriteCOP0(0x04040010, RSPRegisters[rt]); break;
    case 7: RSP_WriteCOP0(0x0404001c, RSPRegisters[rt]); break;
    case 8: RSP_WriteCOP0(0x04100000, RSPRegisters[rt]); break;
    case 9: RSP_WriteCOP0(0x04100004, RSPRegisters[rt]); break;
    case 11: RSP_WriteCOP0(0x0410000c, RSPRegisters[rt]); break;
    case 12: RSP_WriteCOP0(0x04100010, RSPRegisters[rt]); break;
    }
    PC += 4;
}

// Original audio.dll: 0x10016040.
void RSP_MFC2()
{
    PC += 4;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    CpuU32 rd = Instruction.rd;
    RSPRegisters[Instruction.rt] = (CpuS16)(((CpuU8 (*)[16])RSPVectors)[rd][element ^ 1] << 8) | ((CpuU8 (*)[16])RSPVectors)[rd][(element + 1) ^ 1];
}

// Original audio.dll: 0x100160A0.
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
}

// Original audio.dll: 0x10016130.
void RSP_MTC2()
{
    PC += 4;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    CpuU32 rd = Instruction.rd;
    CpuU16 value = (CpuU16)RSPRegisters[Instruction.rt];
    ((CpuU8 (*)[16])RSPVectors)[rd][element ^ 1] = (CpuU8)(value >> 8);
    ((CpuU8 (*)[16])RSPVectors)[rd][(element + 1) ^ 1] = (CpuU8)value;
}

// Original audio.dll: 0x10016190.
void RSP_CTC2()
{
    PC += 4;
    CpuU32 rt = Instruction.rt;
    CpuU8 control = Instruction.rd & 3;
    if (control == 0) RSPFlags[0] = (CpuU16)RSPRegisters[rt];
    else if (control == 1) RSPFlags[1] = (CpuU16)RSPRegisters[rt];
    else if (control == 2) RSPFlags[2] = (CpuU8)RSPRegisters[rt];
}

// Original audio.dll: 0x100161F0.
void RSP_VECTOR()
{
    RSP_VectorDispatch[Instruction.Bytes[0] & 63]();
}

// Original audio.dll: 0x10016210.
void RSP_LBV()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (Instruction.Value & 127) * 1) & 0xFFF;
    CpuU32 vt = Instruction.rt;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1] = SPMemory[address + 0];
}

// Original audio.dll: 0x10016260.
void RSP_LSV()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (Instruction.Value & 127) * 2) & 0xFFF;
    CpuU32 vt = Instruction.rt;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1] = SPMemory[address + 0];
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 1) ^ 1] = SPMemory[address + 1];
}

// Original audio.dll: 0x100162D0.
void RSP_LLV()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (Instruction.Value & 127) * 4) & 0xFFF;
    CpuU32 vt = Instruction.rt;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1] = SPMemory[address + 0];
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 1) ^ 1] = SPMemory[address + 1];
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 2) ^ 1] = SPMemory[address + 2];
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 3) ^ 1] = SPMemory[address + 3];
}

// Original audio.dll: 0x10016370.
void RSP_LDV()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (Instruction.Value & 127) * 8) & 0xFFF;
    CpuU32 vt = Instruction.rt;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1] = SPMemory[address + 0];
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 1) ^ 1] = SPMemory[address + 1];
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 2) ^ 1] = SPMemory[address + 2];
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 3) ^ 1] = SPMemory[address + 3];
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 4) ^ 1] = SPMemory[address + 4];
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 5) ^ 1] = SPMemory[address + 5];
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 6) ^ 1] = SPMemory[address + 6];
    ((CpuU8 (*)[16])RSPVectors)[vt][(element + 7) ^ 1] = SPMemory[address + 7];
}

// Original audio.dll: 0x10016460.
void RSP_LQV()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (Instruction.Value & 127) * 16) & 0xFFF;
    CpuU32 end = address | 15;
    if (address <= end) {
        CpuU32 vectorOffset = Instruction.rt * 16;
        CpuU32 i = 0;
        do {
            ((CpuU8 *)RSPVectors)[vectorOffset + (i ^ 1)] = SPMemory[address];
            address++;
            i++;
        } while (address <= end);
    }
}

// Original audio.dll: 0x100164D0.
void RSP_LRV()
{
    PC += 4;
}

// Original audio.dll: 0x100164E0.
void RSP_LPV()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (Instruction.Value & 127) * 8) & 0xFFF;
    CpuU32 vt = Instruction.rt;
    RSPVectors[vt][0] = (CpuU16)SPMemory[address + 0] << 8;
    RSPVectors[vt][1] = (CpuU16)SPMemory[address + 1] << 8;
    RSPVectors[vt][2] = (CpuU16)SPMemory[address + 2] << 8;
    RSPVectors[vt][3] = (CpuU16)SPMemory[address + 3] << 8;
    RSPVectors[vt][4] = (CpuU16)SPMemory[address + 4] << 8;
    RSPVectors[vt][5] = (CpuU16)SPMemory[address + 5] << 8;
    RSPVectors[vt][6] = (CpuU16)SPMemory[address + 6] << 8;
    RSPVectors[vt][7] = (CpuU16)SPMemory[address + 7] << 8;
}

// Original audio.dll: 0x10016590.
void RSP_LUV()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (Instruction.Value & 127) * 8) & 0xFFF;
    CpuU32 vt = Instruction.rt;
    RSPVectors[vt][0] = (CpuU16)SPMemory[address + 0] << 7;
    RSPVectors[vt][1] = (CpuU16)SPMemory[address + 1] << 7;
    RSPVectors[vt][2] = (CpuU16)SPMemory[address + 2] << 7;
    RSPVectors[vt][3] = (CpuU16)SPMemory[address + 3] << 7;
    RSPVectors[vt][4] = (CpuU16)SPMemory[address + 4] << 7;
    RSPVectors[vt][5] = (CpuU16)SPMemory[address + 5] << 7;
    RSPVectors[vt][6] = (CpuU16)SPMemory[address + 6] << 7;
    RSPVectors[vt][7] = (CpuU16)SPMemory[address + 7] << 7;
}

// Original audio.dll: 0x10016660.
void RSP_LHV()
{
    PC += 4;
}

// Original audio.dll: 0x10016670.
void RSP_LFV()
{
    PC += 4;
}

// Original audio.dll: 0x10016680.
void RSP_LTV()
{
    PC += 4;
}

// Original audio.dll: 0x10016690.
void RSP_SBV()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (Instruction.Value & 127) * 1) & 0xFFF;
    CpuU32 vt = Instruction.rt;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    SPMemory[address + 0] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1];
}

// Original audio.dll: 0x100166F0.
void RSP_SSV()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (Instruction.Value & 127) * 2) & 0xFFF;
    CpuU32 vt = Instruction.rt;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    SPMemory[address + 0] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1];
    SPMemory[address + 1] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 1) ^ 1];
}

// Original audio.dll: 0x10016760.
void RSP_SLV()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (Instruction.Value & 127) * 4) & 0xFFF;
    CpuU32 vt = Instruction.rt;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    SPMemory[address + 0] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1];
    SPMemory[address + 1] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 1) ^ 1];
    SPMemory[address + 2] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 2) ^ 1];
    SPMemory[address + 3] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 3) ^ 1];
}

// Original audio.dll: 0x10016800.
void RSP_SDV()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (Instruction.Value & 127) * 8) & 0xFFF;
    CpuU32 vt = Instruction.rt;
    CpuU32 element = (Instruction.Value >> 7) & 15;
    SPMemory[address + 0] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1];
    SPMemory[address + 1] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 1) ^ 1];
    SPMemory[address + 2] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 2) ^ 1];
    SPMemory[address + 3] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 3) ^ 1];
    SPMemory[address + 4] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 4) ^ 1];
    SPMemory[address + 5] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 5) ^ 1];
    SPMemory[address + 6] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 6) ^ 1];
    SPMemory[address + 7] = ((CpuU8 (*)[16])RSPVectors)[vt][(element + 7) ^ 1];
}

// Original audio.dll: 0x100168E0.
void RSP_SQV()
{
    PC += 4;
    CpuU32 address = (RSPRegisters[Instruction.rs] + (Instruction.Value & 127) * 16) & 0xFFF;
    CpuU32 end = address | 15;
    if (address <= end) {
        CpuU32 vectorOffset = Instruction.rt * 16;
        CpuU32 i = 0;
        do {
            SPMemory[address] = ((CpuU8 *)RSPVectors)[vectorOffset + (i ^ 1)];
            address++;
            i++;
        } while (address <= end);
    }
}

// Original audio.dll: 0x10016950.
void RSP_SRV()
{
    PC += 4;
}

// Original audio.dll: 0x10016960.
void RSP_SPV()
{
    PC += 4;
}

// Original audio.dll: 0x10016970.
void RSP_SUV()
{
    PC += 4;
}

// Original audio.dll: 0x10016980.
void RSP_SHV()
{
    PC += 4;
}

// Original audio.dll: 0x10016990.
void RSP_SFV()
{
    PC += 4;
}

// Original audio.dll: 0x100169A0.
void RSP_SWV()
{
    PC += 4;
}

// Original audio.dll: 0x100169B0.
void RSP_STV()
{
    PC += 4;
}

// Original audio.dll: 0x100169C0.
void RSP_VMULF()
{
    PC += 4;
    CpuU32 element = Instruction.rs & 15;
    CpuU32 vd = Instruction.sa, vs = Instruction.rd, vt = Instruction.rt;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 lane;
        if (element == 0) lane = i;
        else if ((element & 14) == 2) lane = (i & 14) + (element & 1);
        else if ((element & 12) == 4) lane = (i & 12) + (element & 3);
        else lane = element & 7;
        CpuS64 result = (CpuS64)((CpuS16)RSPVectors[vt][lane] * (CpuS16)RSPVectors[vs][i]);
        result = (result + 0x4000) * 2;
        if ((result >> 47) & 1) result = 0xFFFF000000000000 | (result & 0xFFFFFFFFFFFF);
        else result &= 0xFFFFFFFFFFFF;
        AccumulatorValue(i) = result;
        result = AccumulatorMiddle(i);
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original audio.dll: 0x10016D60.
void RSP_VMULU()
{
    PC += 4;
    CpuU32 element = Instruction.rs & 15;
    CpuU32 vd = Instruction.sa, vs = Instruction.rd, vt = Instruction.rt;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 lane;
        if (element == 0) lane = i;
        else if ((element & 14) == 2) lane = (i & 14) + (element & 1);
        else if ((element & 12) == 4) lane = (i & 12) + (element & 3);
        else lane = element & 7;
        CpuS64 result = (CpuS64)(RSPVectors[vt][lane] * RSPVectors[vs][i]);
        result = (result + 0x4000) * 2;
        if ((result >> 47) & 1) result = 0xFFFF000000000000 | (result & 0xFFFFFFFFFFFF);
        else result &= 0xFFFFFFFFFFFF;
        AccumulatorValue(i) = result;
        if ((CpuU32)AccumulatorMiddle(i) > 0xFFFF) result = 0xFFFF;
        else result = AccumulatorMiddle(i);
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original audio.dll: 0x10017160.
void RSP_VRNDP()
{
    PC += 4;
}

// Original audio.dll: 0x10017170.
void RSP_VMULQ()
{
    PC += 4;
}

// Original audio.dll: 0x10017180.
void RSP_VMUDL()
{
    PC += 4;
    CpuU32 element = Instruction.rs & 15;
    CpuU32 vd = Instruction.sa, vs = Instruction.rd, vt = Instruction.rt;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 lane;
        if (element == 0) lane = i;
        else if ((element & 14) == 2) lane = (i & 14) + (element & 1);
        else if ((element & 12) == 4) lane = (i & 12) + (element & 3);
        else lane = element & 7;
        AccumulatorValue(i) = (CpuU16)((RSPVectors[vt][lane] * RSPVectors[vs][i]) >> 16);
        CpuS64 result = AccumulatorValue(i);
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original audio.dll: 0x10017460.
void RSP_VMUDM()
{
    PC += 4;
    CpuU32 element = Instruction.rs & 15;
    CpuU32 vd = Instruction.sa, vs = Instruction.rd, vt = Instruction.rt;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 lane;
        if (element == 0) lane = i;
        else if ((element & 14) == 2) lane = (i & 14) + (element & 1);
        else if ((element & 12) == 4) lane = (i & 12) + (element & 3);
        else lane = element & 7;
        AccumulatorValue(i) = RSPVectors[vt][lane] * (CpuS16)RSPVectors[vs][i];
        CpuS64 result;
        if (((CpuS32)AccumulatorValue(i) >> 16) > 32767) result = 32767;
        else if (((CpuS32)AccumulatorValue(i) >> 16) < -32768) result = -32768;
        else result = AccumulatorValue(i) >> 16;
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original audio.dll: 0x10017820.
void RSP_VMUDN()
{
    PC += 4;
    CpuU32 element = Instruction.rs & 15;
    CpuU32 vd = Instruction.sa, vs = Instruction.rd, vt = Instruction.rt;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 lane;
        if (element == 0) lane = i;
        else if ((element & 14) == 2) lane = (i & 14) + (element & 1);
        else if ((element & 12) == 4) lane = (i & 12) + (element & 3);
        else lane = element & 7;
        AccumulatorValue(i) = (CpuS16)(RSPVectors[vt][lane] * RSPVectors[vs][i]);
        CpuS64 result;
        if ((CpuS32)AccumulatorValue(i) > 32767) result = 32767;
        else if ((CpuS32)AccumulatorValue(i) < -32768) result = -32768;
        else result = AccumulatorValue(i);
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original audio.dll: 0x10017BA0.
void RSP_VMUDH()
{
    PC += 4;
    CpuU32 element = Instruction.rs & 15;
    CpuU32 vd = Instruction.sa, vs = Instruction.rd, vt = Instruction.rt;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 lane;
        if (element == 0) lane = i;
        else if ((element & 14) == 2) lane = (i & 14) + (element & 1);
        else if ((element & 12) == 4) lane = (i & 12) + (element & 3);
        else lane = element & 7;
        AccumulatorValue(i) = (CpuS64)((CpuS16)RSPVectors[vt][lane] * (CpuS16)RSPVectors[vs][i]) << 16;
        CpuS64 result;
        if (((CpuS32)AccumulatorValue(i) >> 16) > 32767) result = 32767;
        else if (((CpuS32)AccumulatorValue(i) >> 16) < -32768) result = -32768;
        else result = AccumulatorValue(i) >> 16;
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original audio.dll: 0x10017F60.
void RSP_VMACF()
{
    PC += 4;
    CpuU32 element = Instruction.rs & 15;
    CpuU32 vd = Instruction.sa, vs = Instruction.rd, vt = Instruction.rt;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 lane;
        if (element == 0) lane = i;
        else if ((element & 14) == 2) lane = (i & 14) + (element & 1);
        else if ((element & 12) == 4) lane = (i & 12) + (element & 3);
        else lane = element & 7;
        CpuS64 result = (CpuS64)(RSPVectors[vt][lane] * RSPVectors[vs][i]);
        result = result * 2 + AccumulatorValue(i);
        if ((result >> 47) & 1) result = 0xFFFF000000000000 | (result & 0xFFFFFFFFFFFF);
        else result &= 0xFFFFFFFFFFFF;
        AccumulatorValue(i) = result;
        if ((CpuS32)AccumulatorMiddle(i) > 32767) result = 32767;
        else if ((CpuS32)AccumulatorMiddle(i) < -32768) result = -32768;
        else result = AccumulatorMiddle(i);
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original audio.dll: 0x10018430.
void RSP_VMACU()
{
    PC += 4;
    CpuU32 element = Instruction.rs & 15;
    CpuU32 vd = Instruction.sa, vs = Instruction.rd, vt = Instruction.rt;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 lane;
        if (element == 0) lane = i;
        else if ((element & 14) == 2) lane = (i & 14) + (element & 1);
        else if ((element & 12) == 4) lane = (i & 12) + (element & 3);
        else lane = element & 7;
        CpuS64 result = (CpuS64)(RSPVectors[vt][lane] * RSPVectors[vs][i]);
        result = result * 2 + AccumulatorValue(i);
        if ((result >> 47) & 1) result = 0xFFFF000000000000 | (result & 0xFFFFFFFFFFFF);
        else result &= 0xFFFFFFFFFFFF;
        AccumulatorValue(i) = result;
        if ((CpuU32)AccumulatorMiddle(i) > 0xFFFF) result = 0xFFFF;
        else result = AccumulatorMiddle(i);
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original audio.dll: 0x100188B0.
void RSP_VRNDN()
{
    PC += 4;
}

// Original audio.dll: 0x100188C0.
void RSP_VMACQ()
{
    PC += 4;
}

// Original audio.dll: 0x100188D0.
void RSP_VMADL()
{
    PC += 4;
    CpuU32 element = Instruction.rs & 15;
    CpuU32 vd = Instruction.sa, vs = Instruction.rd, vt = Instruction.rt;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 lane;
        if (element == 0) lane = i;
        else if ((element & 14) == 2) lane = (i & 14) + (element & 1);
        else if ((element & 12) == 4) lane = (i & 12) + (element & 3);
        else lane = element & 7;
        AccumulatorValue(i) += (CpuU16)((RSPVectors[vt][lane] * RSPVectors[vs][i]) >> 16);
        CpuS64 result;
        if ((CpuS32)AccumulatorValue(i) > 32767) result = 32767;
        else if ((CpuS32)AccumulatorValue(i) < -32768) result = -32768;
        else result = AccumulatorValue(i);
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original audio.dll: 0x10018D30.
void RSP_VMADM()
{
    PC += 4;
    CpuU32 element = Instruction.rs & 15;
    CpuU32 vd = Instruction.sa, vs = Instruction.rd, vt = Instruction.rt;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 lane;
        if (element == 0) lane = i;
        else if ((element & 14) == 2) lane = (i & 14) + (element & 1);
        else if ((element & 12) == 4) lane = (i & 12) + (element & 3);
        else lane = element & 7;
        AccumulatorValue(i) += RSPVectors[vt][lane] * (CpuS16)RSPVectors[vs][i];
        CpuS64 result;
        if (((CpuS32)AccumulatorValue(i) >> 16) > 32767) result = 32767;
        else if (((CpuS32)AccumulatorValue(i) >> 16) < -32768) result = -32768;
        else result = AccumulatorValue(i) >> 16;
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original audio.dll: 0x100191B0.
void RSP_VMADN()
{
    PC += 4;
    CpuU32 element = Instruction.rs & 15;
    CpuU32 vd = Instruction.sa, vs = Instruction.rd, vt = Instruction.rt;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 lane;
        if (element == 0) lane = i;
        else if ((element & 14) == 2) lane = (i & 14) + (element & 1);
        else if ((element & 12) == 4) lane = (i & 12) + (element & 3);
        else lane = element & 7;
        AccumulatorValue(i) += (CpuS16)RSPVectors[vt][lane] * RSPVectors[vs][i];
        CpuS64 result;
        if ((CpuS32)AccumulatorValue(i) > 32767) result = 32767;
        else if ((CpuS32)AccumulatorValue(i) < -32768) result = -32768;
        else result = AccumulatorValue(i);
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original audio.dll: 0x10019600.
void RSP_VMADH()
{
    PC += 4;
    CpuU32 element = Instruction.rs & 15;
    CpuU32 vd = Instruction.sa, vs = Instruction.rd, vt = Instruction.rt;
    for (int i = 0; i < 8; i++)
    {
        CpuU32 lane;
        if (element == 0) lane = i;
        else if ((element & 14) == 2) lane = (i & 14) + (element & 1);
        else if ((element & 12) == 4) lane = (i & 12) + (element & 3);
        else lane = element & 7;
        AccumulatorValue(i) += (CpuS64)((CpuS16)RSPVectors[vt][lane] * (CpuS16)RSPVectors[vs][i]) * 0x10000;
        CpuS64 result;
        if (((CpuS32)AccumulatorValue(i) >> 16) > 32767) result = 32767;
        else if (((CpuS32)AccumulatorValue(i) >> 16) < -32768) result = -32768;
        else result = AccumulatorValue(i) >> 16;
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original audio.dll: 0x10019AC0.
void RSP_VADD()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    if (RSPFastMode && ((PC & 0xfff) < 0x7a4 || ((PC & 0xfff) > 0xab0 && (PC & 0xfff) < 0xbe0)))
    {
        RSPVectors[vd][0] = RSPVectors[vs][0] + RSPVectors[vt][0];
        return;
    }
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];

        CpuS32 result = (CpuS16)RSPVectors[vs][i] + (CpuS16)RSPVectors[vt][(lane & i) + offset] + ((RSPFlags[0] & (1 << i)) ? 1 : 0);
        AccumulatorLow(i) = (CpuU16)result;
        if (result > 32767) result = 32767;
        else if (result < -32768) result = -32768;
        RSPVectors[vd][i] = (CpuU16)result;
        RSPFlags[0] &= ~(0x101 << i);
    }
}

// Original audio.dll: 0x10019D90.
void RSP_VADDExec(RSPInstructionWord opcode, int fast)
{
    CpuU32 vd = opcode.sa;
    CpuU32 vs = opcode.rd;
    CpuU32 vt = opcode.rt;
    if (fast)
    {
        RSPVectors[vd][0] = RSPVectors[vs][0] + RSPVectors[vt][0];
        return;
    }
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(opcode.Value >> 22) & 7] & ((opcode.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(opcode.Value >> 22) & 7];

        CpuS32 result = (CpuS16)RSPVectors[vs][i] + (CpuS16)RSPVectors[vt][(lane & i) + offset] + ((RSPFlags[0] & (1 << i)) ? 1 : 0);
        AccumulatorLow(i) = (CpuU16)result;
        if (result > 32767) result = 32767;
        else if (result < -32768) result = -32768;
        RSPVectors[vd][i] = (CpuU16)result;
        RSPFlags[0] &= ~(0x101 << i);
    }
}

// Original audio.dll: 0x1001A030.
void RSP_VSUB()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    if (RSPFastMode)
    {
        RSPVectors[vd][0] = RSPVectors[vs][0] - RSPVectors[vt][0];
        return;
    }
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];

        CpuS32 result = (CpuS16)RSPVectors[vs][i] - (CpuS16)RSPVectors[vt][(lane & i) + offset] - ((RSPFlags[0] & (1 << i)) ? 1 : 0);
        AccumulatorLow(i) = (CpuU16)result;
        if (result > 32767) result = 32767;
        else if (result < -32768) result = -32768;
        RSPVectors[vd][i] = (CpuU16)result;
        RSPFlags[0] &= ~(0x101 << i);
    }
}

// Original audio.dll: 0x1001A2E0.
void RSP_VSUBExec(RSPInstructionWord opcode, int fast)
{
    CpuU32 vd = opcode.sa;
    CpuU32 vs = opcode.rd;
    CpuU32 vt = opcode.rt;
    if (fast)
    {
        RSPVectors[vd][0] = RSPVectors[vs][0] - RSPVectors[vt][0];
        return;
    }
    // The original helper uses the global instruction for element selection.
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];

        CpuS32 result = (CpuS16)RSPVectors[vs][i] - (CpuS16)RSPVectors[vt][(lane & i) + offset] - ((RSPFlags[0] & (1 << i)) ? 1 : 0);
        AccumulatorLow(i) = (CpuU16)result;
        if (result > 32767) result = 32767;
        else if (result < -32768) result = -32768;
        RSPVectors[vd][i] = (CpuU16)result;
        RSPFlags[0] &= ~(0x101 << i);
    }
}

// Original audio.dll: 0x1001A580.
void RSP_VSUT()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];

        CpuS32 result = (CpuS16)RSPVectors[vt][(lane & i) + offset] - (CpuS16)RSPVectors[vs][i] - ((RSPFlags[0] & (1 << i)) ? 1 : 0);
        AccumulatorLow(i) = (CpuU16)result;
        if (result > 32767) result = 32767;
        else if (result < -32768) result = -32768;
        RSPVectors[vd][i] = (CpuU16)result;
        RSPFlags[0] &= ~(0x1 << i);
    }
}

// Original audio.dll: 0x1001A800.
void RSP_VABS()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];

        CpuS16 source = (CpuS16)RSPVectors[vs][i];
        CpuS16 value = (CpuS16)RSPVectors[vt][(lane & i) + offset];
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

// Original audio.dll: 0x1001AA10.
void RSP_VADDC()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    RSPFlags[0] = 0;
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];

        CpuU32 result = (CpuU32)RSPVectors[vs][i] + RSPVectors[vt][(lane & i) + offset];
        AccumulatorLow(i) = (CpuU16)result;
        RSPFlags[0] |= (CpuU16)(result > 0xffff ? 1 : 0) << i;
        RSPVectors[vd][i] = (CpuU16)result;
    }
}

// Original audio.dll: 0x1001ABC0.
void RSP_VSUBC()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    RSPFlags[0] = 0;
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
        CpuS32 result = RSPVectors[vs][i] - RSPVectors[vt][(lane & i) + offset];
        AccumulatorLow(i) = (CpuU16)result;
        int flags;
        if (result < 0) flags = 0x101;
        else if (result > 0) flags = 0x100;
        else flags = 0;
        RSPVectors[vd][i] = (CpuU16)result;
        RSPFlags[0] |= flags << i;
    }
}

// Original audio.dll: 0x1001ADA0.
void RSP_VADDB() { RSPPC += 4; }

// Original audio.dll: 0x1001ADB0.
void RSP_VSUBB() { RSPPC += 4; }

// Original audio.dll: 0x1001ADC0.
void RSP_VACCB() { RSPPC += 4; }

// Original audio.dll: 0x1001ADD0.
void RSP_VSUCB() { RSPPC += 4; }

// Original audio.dll: 0x1001ADE0.
void RSP_VSAD() { RSPPC += 4; }

// Original audio.dll: 0x1001ADF0.
void RSP_VSAC() { RSPPC += 4; }

// Original audio.dll: 0x1001AE00.
void RSP_VSUM() { RSPPC += 4; }

// Original audio.dll: 0x1001AE10.
void RSP_VSAW()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 element = Instruction.rs & 7;
    if (element == 0) {
        for (int i = 0; i < 8; i++) RSPVectors[vd][i] = AccumulatorHigh(i);
    } else if (element == 1) {
        for (int j = 0; j < 8; j++) RSPVectors[vd][j] = AccumulatorMiddle(j);
    } else if (element == 2) {
        for (int k = 0; k < 8; k++) RSPVectors[vd][k] = AccumulatorLow(k);
    }
}

// Original audio.dll: 0x1001B070.
void RSP_VLT()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    for (CpuU32 j = 0; j < 8; j++)
    {
        RSPCompareCarry[j] = (RSPFlags[0] >> j) & 1;
        RSPCompareNotEqual[j] = (~RSPFlags[0] >> (j + 8)) & 1;
    }
    RSPFlags[1] = 0;
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
        CpuS16 left = (CpuS16)RSPVectors[vs][i];
        CpuS16 right = (CpuS16)RSPVectors[vt][(lane & i) + offset];
        if ((left < right) || (left == right && RSPCompareCarry[i] && !RSPCompareNotEqual[i]))
        {
            RSPFlags[1] |= 1 << i;
            AccumulatorLow(i) = left;
            RSPVectors[vd][i] = left;
        }
        else
        {
            RSPFlags[1] &= ~(1 << i);
            AccumulatorLow(i) = right;
            RSPVectors[vd][i] = right;
        }
        RSPFlags[0] &= ~(0x101 << i);
    }
}

// Original audio.dll: 0x1001B450.
void RSP_VEQ()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    for (CpuU32 j = 0; j < 8; j++)
    {
        RSPCompareCarry[j] = (RSPFlags[0] >> j) & 1;
        RSPCompareNotEqual[j] = (~RSPFlags[0] >> (j + 8)) & 1;
    }
    RSPFlags[1] = 0;
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
        CpuS16 left = (CpuS16)RSPVectors[vs][i];
        CpuS16 right = (CpuS16)RSPVectors[vt][(lane & i) + offset];
        if (left == right && RSPCompareNotEqual[i])
        {
            RSPFlags[1] |= 1 << i;
            AccumulatorLow(i) = left;
            RSPVectors[vd][i] = left;
        }
        else
        {
            RSPFlags[1] &= ~(1 << i);
            AccumulatorLow(i) = right;
            RSPVectors[vd][i] = right;
        }
        RSPFlags[0] &= ~(0x101 << i);
    }
}

// Original audio.dll: 0x1001B7F0.
void RSP_VNE()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    for (CpuU32 j = 0; j < 8; j++)
    {
        RSPCompareCarry[j] = (RSPFlags[0] >> j) & 1;
        RSPCompareNotEqual[j] = (~RSPFlags[0] >> (j + 8)) & 1;
    }
    RSPFlags[1] = 0;
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
        CpuS16 left = (CpuS16)RSPVectors[vs][i];
        CpuS16 right = (CpuS16)RSPVectors[vt][(lane & i) + offset];
        if (left != right || !RSPCompareNotEqual[i])
        {
            RSPFlags[1] |= 1 << i;
            AccumulatorLow(i) = left;
            RSPVectors[vd][i] = left;
        }
        else
        {
            RSPFlags[1] &= ~(1 << i);
            AccumulatorLow(i) = right;
            RSPVectors[vd][i] = right;
        }
        RSPFlags[0] &= ~(0x101 << i);
    }
}

// Original audio.dll: 0x1001BB90. Retained legacy handler.
void RSP_LegacyVGE()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 element = Instruction.rs & 15;
    for (int i = 0; i < 8; i++)
    {
        int lane;
        if (element == 0) lane = i;
        else if ((element & 14) == 2) lane = (i & 14) + (element & 1);
        else if ((element & 12) == 4) lane = (i & 12) + (element & 3);
        else lane = element & 7;
        CpuU16 right = RSPVectors[vt][lane];
        CpuU16 left = RSPVectors[vs][i];
        CpuU16 result;
        if (left > right)
        {
            RSPFlags[1] |= 1 << i;
            result = left;
        }
        else if (left == right && ((~RSPFlags[0] | RSPFlags[2]) & (1 << i)))
        {
            RSPFlags[1] |= 1 << i;
            result = left;
        }
        else
        {
            RSPFlags[1] &= ~(1 << i);
            result = right;
        }
        AccumulatorValue(i) = result;
        RSPVectors[vd][i] = result;
    }
    RSPFlags[0] = 0;
    RSPFlags[2] = 0;
}

// Original audio.dll: 0x1001BFE0.
void RSP_VGE()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    for (CpuU32 j = 0; j < 8; j++)
    {
        RSPCompareCarry[j] = (RSPFlags[0] >> j) & 1;
        RSPCompareNotEqual[j] = (~RSPFlags[0] >> (j + 8)) & 1;
    }
    RSPFlags[1] = 0;
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
        CpuS16 left = (CpuS16)RSPVectors[vs][i];
        CpuS16 right = (CpuS16)RSPVectors[vt][(lane & i) + offset];
        if ((left > right) || (left == right && (RSPCompareNotEqual[i] || !RSPCompareCarry[i])))
        {
            RSPFlags[1] |= 1 << i;
            AccumulatorLow(i) = left;
            RSPVectors[vd][i] = left;
        }
        else
        {
            RSPFlags[1] &= ~(1 << i);
            AccumulatorLow(i) = right;
            RSPVectors[vd][i] = right;
        }
        RSPFlags[0] &= ~(0x101 << i);
    }
}

// Original audio.dll: 0x1001C3C0.
void RSP_VCL()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 low = (RSPFlags[1] >> (CpuU16)i) & 1;
        CpuU32 high = (RSPFlags[1] >> (CpuU16)(i + 8)) & 1;
        CpuU32 equal = ~(RSPFlags[0] >> (CpuU16)(i + 8)) & 1;
        CpuU32 extension = (RSPFlags[2] >> (CpuU16)i) & 1;
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
        CpuU16 left = RSPVectors[vs][i];
        CpuU16 right = RSPVectors[vt][(lane & i) + offset];
        CpuS32 result;
        if ((RSPFlags[0] >> (CpuU16)i) & 1)
        {
            CpuS32 sum = left + right;
            CpuS32 carry = sum > 0xFFFF;
            if (equal)
            {
                CpuS32 zero = (sum & 0xFFFF) == 0;
                low = ((!extension & !carry & zero) |
                       (extension & (!carry | zero)));
            }
            result = low ? -right : left;
        }
        else
        {
            if (equal) high = (left - right) >= 0;
            result = high ? right : left;
        }
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
        RSPFlags[1] = (RSPFlags[1] & ~(0x101 << i)) | (low << i) | (high << (i + 8));
    }
    RSPFlags[2] = 0;
    RSPFlags[0] = 0;
}

// Original audio.dll: 0x1001C980.
void RSP_VCH()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    RSPFlags[2] = 0;
    RSPFlags[1] = 0;
    RSPFlags[0] = 0;
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
        CpuS16 left = (CpuS16)RSPVectors[vs][i];
        CpuS16 right = (CpuS16)RSPVectors[vt][(lane & i) + offset];
        CpuS32 low, high, extension;
        CpuS32 equal;
        CpuS32 carry = (CpuS16)(left ^ right) < 0;
        CpuS32 result;
        if (carry)
        {
            high = right < 0;
            CpuS32 sum = left + right;
            low = sum <= 0;
            extension = sum == -1;
            equal = (sum == 0) | (sum == -1);
            result = low ? -right : left;
        }
        else
        {
            low = right < 0;
            high = (left - right) >= 0;
            extension = 0;
            equal = left == right;
            result = high ? right : left;
        }
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
        RSPFlags[1] |= (low << i) | (high << (i + 8));
        RSPFlags[0] |= (carry << i) | ((~equal & 1) << (i + 8));
        RSPFlags[2] |= extension << i;
    }
}

// Original audio.dll: 0x1001CE50.
void RSP_VCR()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    RSPFlags[1] = 0;
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
        CpuS16 left = (CpuS16)RSPVectors[vs][i];
        CpuS16 right = (CpuS16)RSPVectors[vt][(lane & i) + offset];
        CpuS32 low, high;
        CpuS32 result;
        if ((CpuS16)(left ^ right) < 0)
        {
            high = right < 0;
            low = (left + right + 1) <= 0;
            result = low ? ~right : left;
        }
        else
        {
            low = right < 0;
            high = (left - right) >= 0;
            result = high ? right : left;
        }
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
        RSPFlags[1] |= (low << i) | (high << (i + 8));
    }
    RSPFlags[2] = 0;
    RSPFlags[0] = 0;
}

// Original audio.dll: 0x1001D1A0.
void RSP_VMRG()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    RSPFlags[0] = 0;
    for (CpuU32 i = 0; i < 8; i++)
    {
        CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
        CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
        CpuU16 left = RSPVectors[vs][i];
        CpuU16 right = RSPVectors[vt][(lane & i) + offset];
        if (RSPFlags[1] & (1 << i)) {
            RSPVectors[vd][i] = left;
            AccumulatorLow(i) = left;
        } else {
            RSPVectors[vd][i] = right;
            AccumulatorLow(i) = right;
        }
    }
}

// Original audio.dll: 0x1001D290.
void RSP_VAND()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
    CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = RSPVectors[vs][i] & RSPVectors[vt][(lane & i) + offset];
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
    }
}

// Original audio.dll: 0x1001D430.
void RSP_VNAND()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
    CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = ~(RSPVectors[vs][i] & RSPVectors[vt][(lane & i) + offset]);
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
    }
}

// Original audio.dll: 0x1001D5E0.
void RSP_VOR()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
    CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = RSPVectors[vs][i] | RSPVectors[vt][(lane & i) + offset];
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
    }
}

// Original audio.dll: 0x1001D780.
void RSP_VNOR()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
    CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = ~(RSPVectors[vs][i] | RSPVectors[vt][(lane & i) + offset]);
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
    }
}

// Original audio.dll: 0x1001D930.
void RSP_VXOR()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
    CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = RSPVectors[vs][i] ^ RSPVectors[vt][(lane & i) + offset];
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
    }
}

// Original audio.dll: 0x1001DAD0.
void RSP_VXOROpcode(RSPInstructionWord instruction)
{
    CpuU32 vd = instruction.sa;
    CpuU32 vs = instruction.rd;
    CpuU32 vt = instruction.rt;
    CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
    CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = RSPVectors[vs][i] ^ RSPVectors[vt][(lane & i) + offset];
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
    }
}

// Original audio.dll: 0x1001DC60.
void RSP_VNXOR()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 vs = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 offset = RSPElementIndexMask[(Instruction.Value >> 22) & 7] & ((Instruction.Value >> 21) & 15);
    CpuU32 lane = RSPElementLaneMask[(Instruction.Value >> 22) & 7];
    for (int i = 0; i < 8; i++)
    {
        CpuS32 result = ~(RSPVectors[vs][i] ^ RSPVectors[vt][(lane & i) + offset]);
        AccumulatorLow(i) = result;
        RSPVectors[vd][i] = result;
    }
}

// Original audio.dll: 0x1001DE10. Retained legacy handler.
void RSP_LegacyVRCP() { PC += 4; }

// Original audio.dll: 0x1001DE20. Retained legacy handler.
void RSP_LegacyVRCPL() { PC += 4; }

// Original audio.dll: 0x1001DE30. Retained legacy handler.
void RSP_LegacyVRCPH() { PC += 4; }

// Original audio.dll: 0x1001DE40. Retained legacy handler.
void RSP_LegacyVMOV()
{
    PC += 4;
    CpuU32 vd = Instruction.sa;
    CpuU32 destinationElement = Instruction.rd;
    CpuU32 vt = Instruction.rt;
    CpuU32 element = Instruction.rs & 15;
    CpuU16 result = RSPVectors[vt][element];
    AccumulatorLow(0) = result;
    RSPVectors[vd][destinationElement] = result;
}

// Original audio.dll: 0x1001DEA0. Retained legacy handler.
void RSP_LegacyVRSQ() { PC += 4; }

// Original audio.dll: 0x1001DEB0. Retained legacy handler.
void RSP_LegacyVRSQL() { PC += 4; }

// Original audio.dll: 0x1001DEC0. Retained legacy handler.
void RSP_LegacyVRSQH() { PC += 4; }

// Original audio.dll: 0x1001DED0.
void RSP_VNOOP()
{
    PC += 4;
}

// Original audio.dll: 0x1001DEE0.
void RSP_VEXTT()
{
    PC += 4;
}

// Original audio.dll: 0x1001DEF0.
void RSP_VEXTQ()
{
    PC += 4;
}

// Original audio.dll: 0x1001DF00.
void RSP_VEXTN()
{
    PC += 4;
}

// Original audio.dll: 0x1001DF10.
void RSP_VINST()
{
    PC += 4;
}

// Original audio.dll: 0x1001DF20.
void RSP_VINSQ()
{
    PC += 4;
}

// Original audio.dll: 0x1001DF30.
void RSP_VINSN()
{
    PC += 4;
}

// Original audio.dll: 0x1001DF40.
void RSP_PrepareAudioMicrocode(AudioCommand *command, AudioContext *context)
{
    RSPRegisters[25] = command->words[1];
    RSPRegisters[26] = command->words[0];
    RSPPC = 0x238;
    RSPDMEM[0xff8 / 4] = _byteswap_ulong(AudioTask.ucode_data);
    RSPDMEM[0xff4 / 4] = _byteswap_ulong(context->mp3State);
    RSPDMEM[0xff0 / 4] = _byteswap_ulong(context->mp3Address);
    memcpy(SPMemory + 0x1000, RSP_AudioMicrocode, 0x1000);
}

// Original audio.dll: 0x1001DFB0.
void RSP_InterpretAudioMicrocode()
{
    RSPFastMode = 1;
    CPU_Instruction = _byteswap_ulong(RSPDMEM[((RSPPC & 0xfff) + 0x1000) / 4]);
    while (CPU_Instruction != 0x09000472) {
        RSP_MainDispatch[CPU_Instruction >> 26]();
        if (RSPDelayPC) {
            RSPSavedPC = RSPPC;
            RSPPC = RSPDelayPC;
            CPU_Instruction = _byteswap_ulong(RSPDMEM[((RSPPC & 0xfff) + 0x1000) / 4]);
            RSP_MainDispatch[CPU_Instruction >> 26]();
            RSPPC = RSPSavedPC;
            RSPDelayPC = 0;
        }
        CPU_Instruction = _byteswap_ulong(RSPDMEM[((RSPPC & 0xfff) + 0x1000) / 4]);
    }
}

// Original audio.dll: 0x1001E080.
void RSP_AudioMicrocodeNoOp()
{
}

// Original audio.dll: 0x1001E090.
void RSP_AudioDMARead()
{
    CPU_Instruction = 0x40810000;
    RSP_MTC0();
    CPU_Instruction = 0x40820800;
    RSP_MTC0();
    CPU_Instruction = 0x40831000;
    RSP_MTC0();
}

// Original audio.dll: 0x1001E0C0.
void RSP_AudioDMAWrite()
{
    CPU_Instruction = 0x40810000;
    RSP_MTC0();
    CPU_Instruction = 0x40820800;
    RSP_MTC0();
    CPU_Instruction = 0x40831800;
    RSP_MTC0();
}

// Original audio.dll: 0x1001E0F0. Retained empty entry; original purpose unknown.
void RSP_UnusedEntry1001E0F0() {}

// Original audio.dll: 0x1001E100.
void RSP_LoadVectorExec(RSPInstructionWord opcode)
{
    CpuU32 vt = opcode.rt;
    CpuS32 element = (opcode.Value >> 7) & 15;
    CpuU32 kind = opcode.rd;
    CpuU32 address;
    CpuU32 group;
    if (kind == 11) group = vt & 24;
    CpuS32 offset = (CpuS32)(opcode.Value << 25) >> 25;
    int i;
    switch (kind) {
    case 0:
        address = RSPRegisters[opcode.rs] + offset * 1;
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1] = RSP_ReadByte(address + 0);
        break;
    case 1:
        address = RSPRegisters[opcode.rs] + offset * 2;
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1] = RSP_ReadByte(address + 0);
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 1) ^ 1] = RSP_ReadByte(address + 1);
        break;
    case 2:
        address = RSPRegisters[opcode.rs] + offset * 4;
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1] = RSP_ReadByte(address + 0);
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 1) ^ 1] = RSP_ReadByte(address + 1);
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 2) ^ 1] = RSP_ReadByte(address + 2);
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 3) ^ 1] = RSP_ReadByte(address + 3);
        break;
    case 3:
        address = RSPRegisters[opcode.rs] + offset * 8;
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 0) ^ 1] = RSP_ReadByte(address + 0);
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 1) ^ 1] = RSP_ReadByte(address + 1);
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 2) ^ 1] = RSP_ReadByte(address + 2);
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 3) ^ 1] = RSP_ReadByte(address + 3);
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 4) ^ 1] = RSP_ReadByte(address + 4);
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 5) ^ 1] = RSP_ReadByte(address + 5);
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 6) ^ 1] = RSP_ReadByte(address + 6);
        ((CpuU8 (*)[16])RSPVectors)[vt][(element + 7) ^ 1] = RSP_ReadByte(address + 7);
        break;
    case 4:
        address = RSPRegisters[opcode.rs] + offset * 16;
        for (i = 0; !i || ((address + i) & 15); i++)
            ((CpuU8 (*)[16])RSPVectors)[vt][(element + i) ^ 1] = RSP_ReadByte(address + i);
        break;
    case 5:
        address = RSPRegisters[opcode.rs] + offset * 16;
        for (i = 15; ((address + i) & 15) != 15; i--)
            if (element + i <= 15)
                ((CpuU8 (*)[16])RSPVectors)[vt][(element + i) ^ 1] = RSP_ReadByte(address + i - 16);
        break;
    case 7:
        address = RSPRegisters[opcode.rs] + offset * 8;
        for (i = 0; i < 8; i++)
            RSPVectors[vt][i] = RSP_ReadByte(address + i) << 7;
        break;
    case 6:
        address = RSPRegisters[opcode.rs] + offset * 8;
        for (i = 0; i < 8; i++)
            RSPVectors[vt][i] = RSP_ReadByte(address + i) << 8;
        break;
    case 8:
    {
        address = RSPRegisters[opcode.rs] + offset * 16;
        CpuU8 displacement = (CpuU8)address;
        displacement &= 7;
        CpuU32 base = address - displacement;
        for (i = 0; i < 8; i++)
            RSPVectors[vt][i] = RSP_ReadByte(base + ((displacement + i * 2) & 15)) << 7;
        break;
    }
    case 9:
    {
        address = RSPRegisters[opcode.rs] + offset * 16;
        CpuU8 displacement = (CpuU8)address;
        displacement &= 7;
        CpuU32 base = address - displacement;
        for (i = 0; i < 4; i++)
            RSPVectors[vt][(element >> 1) + i] = RSP_ReadByte(base + ((displacement + i * 4) & 15)) << 7;
        break;
    }
    case 11:
        {
            CpuU16 transpose[8];
            address = RSPRegisters[opcode.rs] + offset * 16;
            address &= ~7;
            int first = ((address >> 3) & 1) << 2;
            for (i = 0; i < 8; i++) {
                ((CpuU8 *)transpose)[((first + i) % 8) * 2 + 1] = RSP_ReadByte(address + i * 2);
                ((CpuU8 *)transpose)[((first + i) % 8) * 2] = RSP_ReadByte(address + i * 2 + 1);
            }
            for (i = 0; i < 8; i++) {
                RSPVectors[group + (((element >> 1) + i) & 7)][i] = transpose[((element >> 1) + i) & 7];
            }
        }
        break;
    }
    RSPMemoryAddress = address;
}

// Original audio.dll: 0x1001E8E0.
void RSP_LoadVector() { RSPPC += 4; RSP_LoadVectorExec(*(RSPInstructionWord *)&CPU_Instruction); }

// Original audio.dll: 0x1001E900.
void RSP_LoadVectorOpcode(RSPInstructionWord opcode) { RSP_LoadVectorExec(*(RSPInstructionWord *)&CPU_Instruction); }

#include "RSPKernel.inc"
extern "C" {
#include "RSPData.inc"
}
