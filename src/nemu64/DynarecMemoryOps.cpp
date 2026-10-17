#include "DynarecMemoryOps.h"
#include "Dynarec.h"
#include "DynarecFPU.h"
#include "DynarecOpcodes.h"
#include "Memory.h"
#include "Plugins.h"
#include <string.h>

void __stdcall LoadDoubleLeft(unsigned address, unsigned reg)
{
    union { CpuU64 value; unsigned words[2]; } memory;
    memory.words[1] = ReadPhysicalWord(address & ~7u);
    memory.words[0] = ReadPhysicalWord((address & ~3u) | 4);
    CPU_GPR[reg] = (CPU_GPR[reg] & (0x00ffffffffffffffI64 >> (((7 - address) << 3) & 56)))
                | (memory.value << ((address & 7) * 8));
}

void Recompile_LDL()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(0);
    EmitWatchCheck(1, 1);
    EmitMoveImmediate(1, X86_EDX, (CPU_Instruction >> 16) & 31);
    EmitPush(X86_EDX);
    EmitPush(X86_EAX);
    EmitCall((unsigned)LoadDoubleLeft);
}

void __stdcall LoadDoubleRight(unsigned address, unsigned reg)
{
    union { CpuU64 value; unsigned words[2]; } memory;
    memory.words[1] = ReadPhysicalWord(address & ~7u);
    memory.words[0] = ReadPhysicalWord((address & ~3u) | 4);
    CPU_GPR[reg] = (memory.value >> (((7 - address) << 3) & 56))
                | (CPU_GPR[reg] & (0xffffffffffffff00I64 << ((address & 7) * 8)));
}

void Recompile_LDR()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(0);
    EmitWatchCheck(1, 1);
    EmitMoveImmediate(1, X86_EDX, (CPU_Instruction >> 16) & 31);
    EmitPush(X86_EDX);
    EmitPush(X86_EAX);
    EmitCall((unsigned)LoadDoubleRight);
}

void Recompile_LB()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(0);
    EmitWatchCheck(1, 0);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalByte);
        EmitMoveSigned(0, X86_ECX, X86_EAX);
    } else {
        EmitXorAccumulator(1, 3);
        EmitLoadSigned(0, X86_ECX, 0x80, (unsigned)RDRAM);
    }
    StoreGPRSigned32From((CPU_Instruction >> 16) & 31, X86_ECX);
}

void Recompile_LH()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(0);
    EmitWatchCheck(1, 0);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalHalf);
        EmitMoveSigned(1, X86_ECX, X86_EAX);
    } else {
        EmitXorAccumulator(1, 2);
        EmitLoadSigned(1, X86_ECX, 0x80, (unsigned)RDRAM);
    }
    StoreGPRSigned32From((CPU_Instruction >> 16) & 31, X86_ECX);
}

void Recompile_LWL()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(0);
    EmitWatchCheck(1, 0);
    EmitMove(1, X86_EAX, X86_ECX);
    EmitAndImmediate(1, X86_EAX, ~3u);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_ECX);
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalWord);
        EmitPop(X86_ECX);
    } else {
        EmitLoad(1, X86_EAX, 0x80, (unsigned)RDRAM);
    }
    EmitAndImmediate(1, X86_ECX, 3);
    EmitShiftLeft(1, X86_ECX, 3);
    EmitShiftLeftCL(1, X86_EAX);
    EmitAddImmediate(X86_ECX, -24);
    EmitNegate(1, X86_ECX);
    EmitMoveImmediate(1, X86_EBX, 0x00ffffff);
    EmitShiftRightCL(1, X86_EBX);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 0, X86_ECX);
    EmitAnd(1, X86_ECX, X86_EBX);
    EmitOr(1, X86_EBX, X86_EAX);
    StoreGPRSigned32((CPU_Instruction >> 16) & 31);
}

void Recompile_LW()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(0);
    EmitWatchCheck(1, 0);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalWord);
        StoreGPRSigned32From((CPU_Instruction >> 16) & 31, X86_EAX);
        return;
    }
    EmitLoad(1, X86_ECX, 0x80, (unsigned)RDRAM);
    StoreGPRSigned32From((CPU_Instruction >> 16) & 31, X86_ECX);
}

void Recompile_LBU()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(0);
    EmitWatchCheck(1, 0);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalByte);
        EmitMoveUnsigned(0, X86_ECX, X86_EAX);
    } else {
        EmitXorAccumulator(1, 3);
        EmitLoadUnsigned(0, X86_ECX, 0x80, (unsigned)RDRAM);
    }
    StoreGPRSigned32From((CPU_Instruction >> 16) & 31, X86_ECX);
}

void Recompile_LHU()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(0);
    EmitWatchCheck(1, 0);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalHalf);
        EmitMoveUnsigned(1, X86_ECX, X86_EAX);
    } else {
        EmitXorAccumulator(1, 2);
        EmitLoadUnsigned(1, X86_ECX, 0x80, (unsigned)RDRAM);
    }
    StoreGPRSigned32From((CPU_Instruction >> 16) & 31, X86_ECX);
}

void Recompile_LWR()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(0);
    EmitWatchCheck(1, 0);
    EmitMove(1, X86_EAX, X86_ECX);
    EmitAndImmediate(1, X86_EAX, ~3u);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_ECX);
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalWord);
        EmitPop(X86_ECX);
    } else {
        EmitLoad(1, X86_EAX, 0x80, (unsigned)RDRAM);
    }
    EmitAndImmediate(1, X86_ECX, 3);
    EmitShiftLeft(1, X86_ECX, 3);
    EmitMoveImmediate(1, X86_EBX, 0xffffff00);
    EmitShiftLeftCL(1, X86_EBX);
    EmitAddImmediate(X86_ECX, -24);
    EmitNegate(1, X86_ECX);
    EmitShiftRightCL(1, X86_EAX);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 0, X86_ECX);
    EmitAnd(1, X86_ECX, X86_EBX);
    EmitOr(1, X86_EBX, X86_EAX);
    StoreGPRSigned32((CPU_Instruction >> 16) & 31);
}

void Recompile_LWU()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(0);
    EmitWatchCheck(1, 0);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalWord);
    } else {
        EmitLoad(1, X86_EAX, 0x80, (unsigned)RDRAM);
    }
    EmitXor(1, X86_EDX, X86_EDX);
    StoreGPR64((CPU_Instruction >> 16) & 31);
}

void Recompile_SB()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(1);
    EmitWatchCheck(2, 0);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 0, X86_EDX);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EDX);
        EmitPush(X86_EAX);
        EmitCall((unsigned)WritePhysicalByte);
        return;
    }
    EmitXorAccumulator(1, 3);
    EmitStore(0, X86_EDX, 0x80, (unsigned)RDRAM);
    if (Dynarec_Mode == 5) {
        EmitCompareImmediate(1, X86_EAX, RDRAMSize);
        unsigned end = EmitConditionalJump(3);
        EmitAndImmediate(1, X86_EAX, ~3u);
        EmitStoreImmediate(1, 0x80, (unsigned)Dynarec_BlockCode, 0);
        EmitLabel(end);
    }
}

void Recompile_SH()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(1);
    EmitWatchCheck(2, 0);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 0, X86_EDX);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EDX);
        EmitPush(X86_EAX);
        EmitCall((unsigned)WritePhysicalHalf);
        return;
    }
    EmitXorAccumulator(1, 2);
    EmitStoreHalf(X86_EDX, 0x80, (unsigned)RDRAM);
    if (Dynarec_Mode == 5) {
        EmitCompareImmediate(1, X86_EAX, RDRAMSize);
        unsigned end = EmitConditionalJump(3);
        EmitAndImmediate(1, X86_EAX, ~3u);
        EmitStoreImmediate(1, 0x80, (unsigned)Dynarec_BlockCode, 0);
        EmitLabel(end);
    }
}

void Recompile_SWL()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(1);
    EmitWatchCheck(2, 0);
    EmitMove(1, X86_EAX, X86_ECX);
    EmitAndImmediate(1, X86_EAX, ~3u);
    EmitMove(1, X86_EAX, X86_EBX);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_ECX);
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalWord);
        EmitPop(X86_ECX);
    } else {
        EmitLoad(1, X86_EAX, 0x80, (unsigned)RDRAM);
    }
    EmitAndImmediate(1, X86_ECX, 3);
    EmitShiftLeft(1, X86_ECX, 3);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 0, X86_EDX);
    EmitShiftRightCL(1, X86_EDX);
    EmitPush(X86_EDX);
    EmitMoveImmediate(1, X86_EDX, 0xffffff00);
    EmitAddImmediate(X86_ECX, -24);
    EmitNegate(1, X86_ECX);
    EmitShiftLeftCL(1, X86_EDX);
    EmitAnd(1, X86_EDX, X86_EAX);
    EmitPop(X86_EDX);
    EmitOr(1, X86_EDX, X86_EAX);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EAX);
        EmitPush(X86_EBX);
        EmitCall((unsigned)WritePhysicalWord);
        EmitWatchHitCheck();
        return;
    }
    EmitStore(1, X86_EAX, 0x83, (unsigned)RDRAM);
    if (Dynarec_Mode == 5) {
        EmitCompareImmediate(1, X86_EAX, RDRAMSize);
        unsigned end = EmitConditionalJump(3);
        EmitStoreImmediate(1, 0x80, (unsigned)Dynarec_BlockCode, 0);
        EmitLabel(end);
    }
}

void Recompile_SW()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(1);
    EmitWatchCheck(2, 0);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 0, X86_EDX);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EDX);
        EmitPush(X86_EAX);
        EmitCall((unsigned)WritePhysicalWord);
        EmitWatchHitCheck();
        return;
    }
    EmitStore(1, X86_EDX, 0x80, (unsigned)RDRAM);
    if (Dynarec_Mode == 5) {
        EmitCompareImmediate(1, X86_EAX, RDRAMSize);
        unsigned end = EmitConditionalJump(3);
        EmitStoreImmediate(1, 0x80, (unsigned)Dynarec_BlockCode, 0);
        EmitLabel(end);
    }
}

void __stdcall StoreDoubleLeft(unsigned address, CpuU64 value)
{
    union { CpuU64 value; unsigned words[2]; } memory;
    memory.words[1] = ReadPhysicalWord(address & ~7u);
    memory.words[0] = ReadPhysicalWord((address & ~3u) | 4);
    CpuU64 result = (memory.value & (0xffffffffffffff00I64 << (((7 - address) << 3) & 56)))
                 | (value >> ((address & 7) * 8));
    WritePhysicalDoubleWord(address & ~7u, (unsigned)result, (unsigned)(result >> 32));
}

void Recompile_SDL()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(1);
    EmitWatchCheck(2, 1);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 1, X86_EDX);
    EmitPush(X86_EDX);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 0, X86_EDX);
    EmitPush(X86_EDX);
    EmitPush(X86_EAX);
    EmitCall((unsigned)StoreDoubleLeft);
}

void __stdcall StoreDoubleRight(unsigned address, CpuU64 value)
{
    union { CpuU64 value; unsigned words[2]; } memory;
    memory.words[1] = ReadPhysicalWord(address & ~7u);
    memory.words[0] = ReadPhysicalWord((address & ~3u) | 4);
    CpuU64 result = (value << (((7 - address) << 3) & 56))
                 | (memory.value & (0x00ffffffffffffffI64 >> ((address & 7) * 8)));
    WritePhysicalDoubleWord(address & ~7u, (unsigned)result, (unsigned)(result >> 32));
}

void Recompile_SDR()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(1);
    EmitWatchCheck(2, 1);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 1, X86_EDX);
    EmitPush(X86_EDX);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 0, X86_EDX);
    EmitPush(X86_EDX);
    EmitPush(X86_EAX);
    EmitCall((unsigned)StoreDoubleRight);
}

void Recompile_SWR()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(1);
    EmitWatchCheck(2, 0);
    EmitMove(1, X86_EAX, X86_ECX);
    EmitAndImmediate(1, X86_EAX, ~3u);
    EmitMove(1, X86_EAX, X86_EBX);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_ECX);
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalWord);
        EmitPop(X86_ECX);
    } else {
        EmitLoad(1, X86_EAX, 0x80, (unsigned)RDRAM);
    }
    EmitAndImmediate(1, X86_ECX, 3);
    EmitShiftLeft(1, X86_ECX, 3);
    EmitMoveImmediate(1, X86_EDX, 0x00ffffff);
    EmitShiftRightCL(1, X86_EDX);
    EmitAnd(1, X86_EDX, X86_EAX);
    EmitAddImmediate(X86_ECX, -24);
    EmitNegate(1, X86_ECX);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 0, X86_EDX);
    EmitShiftLeftCL(1, X86_EDX);
    EmitOr(1, X86_EDX, X86_EAX);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EAX);
        EmitPush(X86_EBX);
        EmitCall((unsigned)WritePhysicalWord);
        EmitWatchHitCheck();
        return;
    }
    EmitStore(1, X86_EAX, 0x83, (unsigned)RDRAM);
    if (Dynarec_Mode == 5) {
        EmitCompareImmediate(1, X86_EAX, RDRAMSize);
        unsigned end = EmitConditionalJump(3);
        EmitStoreImmediate(1, 0x80, (unsigned)Dynarec_BlockCode, 0);
        EmitLabel(end);
    }
}

void __stdcall InvalidateRecompiledPage(unsigned address)
{
    memset(&Dynarec_BlockCode[address >> 2], 0, 4096);
    if (Dynarec_Mode == 3)
        InvalidateRecompiledRange(address, 1);
}

void __stdcall RemovePageBreakpoints(unsigned page)
{
    for (unsigned i = 0; i < CPUBreakpointCount; i++) {
        DebugBreakpoint *entry = &CPUBreakpoints[i];
        unsigned address = entry->Address;
        if ((address & 0xfffff000) == page) {
            if (ReadPhysicalWord(address) == (entry->CodeOffset | 0xec000000))
                WritePhysicalWord(address, entry->Instruction);
            // The original leaves a matching last entry in place and advances
            // after compacting; preserve those observable boundary cases.
            unsigned last = CPUBreakpointCount - 1;
            if (i + 1 <= last) {
                if (i < last)
                    for (unsigned j = i; j < last; j++)
                        CPUBreakpoints[j] = CPUBreakpoints[j + 1];
                CPUBreakpointCount = last;
            }
        }
    }
}

void Recompile_CACHE()
{
    if (Dynarec_Mode == 1 || Dynarec_Mode == 3 || Dynarec_Mode == 5)
        return;
    if (((CPU_Instruction >> 16) & 3) != 0 && ((CPU_Instruction >> 16) & 3) != 2)
        return;
    unsigned operation = (CPU_Instruction >> 18) & 7;
    if (operation != 0 && (operation <= 3 || operation > 5))
        return;
    if (Dynarec_Mode == 4) {
        Dynarec_ResetRequired = 1;
        return;
    }
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    EmitTranslateAddress(0);
    EmitAndImmediate(1, X86_EAX, 0xfffff000);
    EmitCompareImmediate(1, X86_EAX, RDRAMSize);
    unsigned outside = EmitConditionalJump(3);
    EmitMove(1, X86_EAX, X86_ECX);
    EmitShiftRight(1, X86_ECX, 12);
    EmitLoad(0, X86_EBX, 0x81, (unsigned)Dynarec_CompiledPages);
    EmitCompareImmediate(0, X86_EBX, 1);
    unsigned empty = EmitConditionalJump(5);
    EmitStoreImmediate(0, 0x81, (unsigned)Dynarec_CompiledPages, 0);
    EmitPush(X86_EAX);
    switch (Dynarec_Mode) {
    case 0: EmitCall((unsigned)InvalidateRecompiledPage); break;
    case 2: EmitCall((unsigned)RemovePageBreakpoints); break;
    }
    EmitCompareImmediate(1, X86_EAX, Dynarec_PageAddress);
    unsigned otherPage = EmitConditionalJump(5);
    if (Dynarec_DelaySlot)
        ReportCPUError("ICACHE in delay slot - this might crash");
    else {
        EmitStoreImmediate(1, 5, (unsigned)&CPU_PC, CPU_LastPC + 4);
        FinishInstruction();
        EmitReturn();
    }
    EmitLabel(otherPage);
    EmitLabel(empty);
    EmitLabel(outside);
}

void Recompile_LWC1()
{
    CheckFPUEnabled();
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(0);
    EmitWatchCheck(1, 0);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalWord);
    } else {
        EmitLoad(1, X86_EAX, 0x80, (unsigned)RDRAM);
    }
    if (Dynarec_DynamicFpuMode) {
        EmitLoad(1, X86_EBX, 5, (unsigned)&CPU_FRMode);
        EmitCompareImmediate(1, X86_EBX, 1);
        Dynarec_FpuModeLabel = EmitConditionalJump(5);
    }
    if (Dynarec_DynamicFpuMode || CPU_FRMode)
        EmitStore(1, X86_EAX, 5, (unsigned)&CPU_FPR64[(CPU_Instruction >> 16) & 31]);
    if (Dynarec_DynamicFpuMode) {
        Dynarec_FpuEndLabel = EmitJump();
        EmitLabel(Dynarec_FpuModeLabel);
    }
    if (Dynarec_DynamicFpuMode || !CPU_FRMode)
        EmitStore(1, X86_EAX, 5, (unsigned)&CPU_FPR.words[(CPU_Instruction >> 16) & 31]);
    if (Dynarec_DynamicFpuMode)
        EmitLabel(Dynarec_FpuEndLabel);
}

void Recompile_LDC1()
{
    CheckFPUEnabled();
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(0);
    EmitWatchCheck(1, 1);
    if (RequiresMemoryCheck()) {
        EmitMove(1, X86_EAX, X86_EBX);
        EmitAddImmediate(X86_EBX, 4);
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalWord);
        EmitExchange(1, X86_EBX, X86_EAX);
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalWord);
        EmitMove(1, X86_EAX, X86_ECX);
    } else {
        EmitLoad(1, X86_ECX, 0x80, (unsigned)RDRAM + 4);
        EmitLoad(1, X86_EBX, 0x80, (unsigned)RDRAM);
    }
    if (Dynarec_DynamicFpuMode) {
        EmitLoad(1, X86_EAX, 5, (unsigned)&CPU_FRMode);
        EmitCompareImmediate(1, X86_EAX, 1);
        Dynarec_FpuModeLabel = EmitConditionalJump(5);
    }
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        EmitStore(1, X86_ECX, 5, (unsigned)&CPU_FPR64[(CPU_Instruction >> 16) & 31]);
        EmitStore(1, X86_EBX, 5, (unsigned)&CPU_FPR64[(CPU_Instruction >> 16) & 31] + 4);
    }
    if (Dynarec_DynamicFpuMode) {
        Dynarec_FpuEndLabel = EmitJump();
        EmitLabel(Dynarec_FpuModeLabel);
    }
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        EmitStore(1, X86_ECX, 5, (unsigned)&CPU_FPR.words[(CPU_Instruction >> 16) & 30]);
        EmitStore(1, X86_EBX, 5, (unsigned)&CPU_FPR.words[((CPU_Instruction >> 16) & 30) | 1]);
    }
    if (Dynarec_DynamicFpuMode)
        EmitLabel(Dynarec_FpuEndLabel);
}

void Recompile_LD()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(0);
    EmitWatchCheck(1, 1);
    if (RequiresMemoryCheck()) {
        EmitMove(1, X86_EAX, X86_EBX);
        EmitAddImmediate(X86_EBX, 4);
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalWord);
        EmitExchange(1, X86_EBX, X86_EAX);
        EmitPush(X86_EAX);
        EmitCall((unsigned)ReadPhysicalWord);
        EmitMove(1, X86_EBX, X86_EDX);
    } else {
        EmitLoad(1, X86_EDX, 0x80, (unsigned)RDRAM);
        EmitLoad(1, X86_EAX, 0x80, (unsigned)RDRAM + 4);
    }
    StoreGPR64((CPU_Instruction >> 16) & 31);
}

void Recompile_Unimplemented()
{
    Recompile_Unknown();
}

void Recompile_SWC1()
{
    CheckFPUEnabled();
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(1);
    EmitWatchCheck(2, 0);
    if (Dynarec_DynamicFpuMode) {
        EmitLoad(1, X86_EBX, 5, (unsigned)&CPU_FRMode);
        EmitCompareImmediate(1, X86_EBX, 1);
        Dynarec_FpuModeLabel = EmitConditionalJump(5);
    }
    if (Dynarec_DynamicFpuMode || CPU_FRMode)
        EmitLoad(1, X86_EDX, 5, (unsigned)&CPU_FPR64[(CPU_Instruction >> 16) & 31]);
    if (Dynarec_DynamicFpuMode) {
        Dynarec_FpuEndLabel = EmitJump();
        EmitLabel(Dynarec_FpuModeLabel);
    }
    if (Dynarec_DynamicFpuMode || !CPU_FRMode)
        EmitLoad(1, X86_EDX, 5, (unsigned)&CPU_FPR.words[(CPU_Instruction >> 16) & 31]);
    if (Dynarec_DynamicFpuMode)
        EmitLabel(Dynarec_FpuEndLabel);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EDX);
        EmitPush(X86_EAX);
        EmitCall((unsigned)WritePhysicalWord);
        EmitWatchHitCheck();
        return;
    }
    EmitStore(1, X86_EDX, 0x80, (unsigned)RDRAM);
    if (Dynarec_Mode == 5) {
        EmitCompareImmediate(1, X86_EAX, RDRAMSize);
        unsigned end = EmitConditionalJump(3);
        EmitStoreImmediate(1, 0x80, (unsigned)Dynarec_BlockCode, 0);
        EmitLabel(end);
    }
}

void Recompile_SDC1()
{
    CheckFPUEnabled();
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(1);
    EmitWatchCheck(2, 1);
    if (Dynarec_DynamicFpuMode) {
        EmitLoad(1, X86_ECX, 5, (unsigned)&CPU_FRMode);
        EmitCompareImmediate(1, X86_ECX, 1);
        Dynarec_FpuModeLabel = EmitConditionalJump(5);
    }
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        EmitLoad(1, X86_EDX, 5, (unsigned)&CPU_FPR64[(CPU_Instruction >> 16) & 31] + 4);
        EmitLoad(1, X86_ECX, 5, (unsigned)&CPU_FPR64[(CPU_Instruction >> 16) & 31]);
    }
    if (Dynarec_DynamicFpuMode) {
        Dynarec_FpuEndLabel = EmitJump();
        EmitLabel(Dynarec_FpuModeLabel);
    }
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        EmitLoad(1, X86_EDX, 5, (unsigned)&CPU_FPR.words[((CPU_Instruction >> 16) & 30) | 1]);
        EmitLoad(1, X86_ECX, 5, (unsigned)&CPU_FPR.words[(CPU_Instruction >> 16) & 30]);
    }
    if (Dynarec_DynamicFpuMode)
        EmitLabel(Dynarec_FpuEndLabel);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EDX);
        EmitPush(X86_ECX);
        EmitPush(X86_EAX);
        EmitCall((unsigned)WritePhysicalDoubleWord);
        return;
    }
    EmitStore(1, X86_EDX, 0x80, (unsigned)RDRAM);
    EmitStore(1, X86_ECX, 0x80, (unsigned)RDRAM + 4);
}

void Recompile_SD()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    if (TLBEnabled && !TLBRamMapped)
        EmitMove(1, X86_EAX, X86_EBX);
    EmitTranslateAddress(1);
    EmitWatchCheck(2, 1);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 1, X86_EDX);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 0, X86_ECX);
    if (RequiresMemoryCheck()) {
        EmitPush(X86_EDX);
        EmitPush(X86_ECX);
        EmitPush(X86_EAX);
        EmitCall((unsigned)WritePhysicalDoubleWord);
        return;
    }
    EmitStore(1, X86_EDX, 0x80, (unsigned)RDRAM);
    EmitStore(1, X86_ECX, 0x80, (unsigned)RDRAM + 4);
    if (Dynarec_Mode == 5) {
        EmitCompareImmediate(1, X86_EAX, RDRAMSize);
        unsigned end = EmitConditionalJump(3);
        EmitStoreImmediate(1, 0x80, (unsigned)Dynarec_BlockCode, 0);
        EmitStoreImmediate(1, 0x80, (unsigned)Dynarec_BlockCode + 4, 0);
        EmitLabel(end);
    }
}
