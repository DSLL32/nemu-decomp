// RSP storage recovered from native references and initialized data.
// Object names and original file grouping are inferred; no gap objects added.
#include "RSP.h"

// Original 0x1012BE00.
CpuU32 CPU_Instruction = 0;
// Original 0x1012BE04.
CpuU8 RSPCompareNotEqual[8] = {0};
// Original 0x1012BE0C.
CpuU16 RSPFlags[3] = {0};
// Original 0x1012BE18.
CpuU16 RSPVectors[32][8] = {0};
// Original 0x1012C018.
CpuU32 RSPSavedPC = 0;
// Original 0x1012C01C.
CpuU32 RSPDelayPC = 0;
// Original 0x1012C024.
CpuU32 RSPMemAddressReg = 0;
// Original 0x1012C028.
CpuU32 RSPDramAddressReg = 0;
// Original 0x1012C02C.
CpuU32 RSPReadLengthReg = 0;
// Original 0x1012C030.
CpuU32 RSPWriteLengthReg = 0;
// Original 0x1012C03C.
CpuU32 RSPPC = 0;
// Original 0x1012C048.
CpuU32 RSPRegisters[32] = {0};
// Original 0x1012C0C8.
CpuU32 RSPMemoryAddress = 0;
// Original 0x1012C0D0.
CpuS64 RSPAccumulator[8] = {0};
// Original 0x1012C110.
CpuU8 SPMemory[16384] = {0};
// Original 0x10130110.
CpuU8 RSPCompareCarry[8] = {0};
// Original 0x10130118.
int RSPFastMode = 0;

#include "RSPMicrocode.inc"
// Original 0x1003F3F0: retained pointer to the guest program.
unsigned *RSP_AudioMicrocodeImage = RSP_AudioMicrocode;

// Original 0x1003f3f8.
RSPOpcodeHandler RSP_MainDispatch[64] = {
    RSP_SPECIAL, RSP_REGIMM, RSP_J, RSP_JAL,
    RSP_BEQ, RSP_BNE, RSP_BLEZ, RSP_BGTZ,
    RSP_ADDI, RSP_ADDIU, RSP_SLTI, RSP_SLTIU,
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

// Original 0x1003f4f8.
RSPOpcodeHandler RSP_SpecialDispatch[64] = {
    RSP_SLL, RSP_InvalidOpcode, RSP_SRL, RSP_SRA,
    RSP_SLLV, RSP_InvalidOpcode, RSP_SRLV, RSP_SRAV,
    RSP_JR, RSP_JALR, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_BREAK, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_ADD, RSP_ADDU, RSP_SUB, RSP_SUBU,
    RSP_AND, RSP_OR, RSP_XOR, RSP_NOR,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_SLT, RSP_SLTU,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
    RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode, RSP_InvalidOpcode,
};

// Original 0x1003f5f8.
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

// Original 0x1003f678.
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

// Original 0x1003f6f8.
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

// Original 0x1003f778.
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

// Original 0x1003f7f8.
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

// Original 0x1003f878.
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

// Original 0x1003F978 and 0x1003F998: vector-element selection masks.
CpuU32 RSPElementLaneMask[8] = {7, 14, 12, 12, 0, 0, 0, 0};
CpuU32 RSPElementIndexMask[8] = {0, 1, 3, 3, 7, 7, 7, 7};
// Original 0x1003F9B8: word-access alias for the SP memory bank.
unsigned *RSPDMEM = (unsigned *)SPMemory;
