#include "Dynarec.h"
#include "CPU.h"
#include "PIF.h"
#include "CpuControl.h"
#include "Rom.h"
#include "RSP.h"
#include "DynarecFPU.h"
#include "DynarecMemoryOps.h"
#include "Memory.h"
#include "Plugins.h"
#include "FrontendIni.h"
#include "WinMain.h"
#include "RomBrowser.h"
#include <string.h>

void TrackWord()
{
    if (!Dynarec_TrackingLoaded) {
        EmitLoad(1, X86_ECX, 5, (unsigned)&UndoPosition);
        Dynarec_TrackingLoaded = 1;
    }
    EmitStore(1, X86_EAX, 0x81, (unsigned)UndoData);
    EmitAddImmediate(X86_ECX, 4);
    EmitAndImmediate(1, X86_ECX, 0x3ffffc);
    EmitCompareImmediate(1, X86_ECX, 0x3ffff0);
    unsigned skip = EmitConditionalJump(2);
    EmitPush(X86_ECX);
    EmitCall((unsigned)DynarecNoOperation);
    EmitPop(X86_ECX);
    EmitLabel(skip);
}

void TrackDoubleWord()
{
    if (!Dynarec_TrackingLoaded) {
        EmitLoad(1, X86_ECX, 5, (unsigned)&UndoPosition);
        Dynarec_TrackingLoaded = 1;
    }
    EmitStore(1, X86_EAX, 0x81, (unsigned)UndoData);
    EmitAddImmediate(X86_ECX, 4);
    EmitAndImmediate(1, X86_ECX, 0x3ffffc);
    EmitStore(1, X86_EDX, 0x81, (unsigned)UndoData);
    EmitAddImmediate(X86_ECX, 4);
    EmitAndImmediate(1, X86_ECX, 0x3ffffc);
    EmitCompareImmediate(1, X86_ECX, 0x3ffff0);
    unsigned skip = EmitConditionalJump(2);
    EmitPush(X86_ECX);
    EmitCall((unsigned)DynarecNoOperation);
    EmitPop(X86_ECX);
    EmitLabel(skip);
}

void TrackGPR(unsigned reg)
{
    LoadGPR64((unsigned char)reg);
    TrackDoubleWord();
}

void TrackFPR(unsigned reg)
{
    if (Dynarec_DynamicFpuMode) {
        EmitLoad(1, X86_EBX, 5, (unsigned)&CPU_FRMode);
        EmitCompareImmediate(1, X86_EBX, 1);
        Dynarec_FpuModeLabel = EmitConditionalJump(5);
    }
    if (Dynarec_DynamicFpuMode || CPU_FRMode) {
        EmitLoad(1, X86_EDX, 5, (unsigned)&CPU_FPR64[reg] + 4);
        EmitLoad(1, X86_EAX, 5, (unsigned)&CPU_FPR64[reg]);
    }
    if (Dynarec_DynamicFpuMode) {
        Dynarec_FpuEndLabel = EmitJump();
        EmitLabel(Dynarec_FpuModeLabel);
    }
    if (Dynarec_DynamicFpuMode || !CPU_FRMode) {
        EmitLoad(1, X86_EDX, 5, (unsigned)&CPU_FPR.words[reg | 1]);
        EmitLoad(1, X86_EAX, 5, (unsigned)&CPU_FPR.words[reg & ~1]);
    }
    if (Dynarec_DynamicFpuMode)
        EmitLabel(Dynarec_FpuEndLabel);
    TrackDoubleWord();
}

void TrackRegister64(const CpuS64 *reg)
{
    EmitLoad(1, X86_EAX, 5, (unsigned)reg);
    EmitLoad(1, X86_EDX, 5, (unsigned)reg + 4);
    TrackDoubleWord();
}

void TrackRegister32(const unsigned *reg)
{
    EmitLoad(1, X86_EAX, 5, (unsigned)reg);
    TrackWord();
}

void TrackMemory()
{
    LoadGPRLow((CPU_Instruction >> 21) & 31);
    EmitAddImmediate(X86_EAX, (short)CPU_Instruction);
    unsigned savedBranch = Dynarec_BranchPending;
    Dynarec_BranchPending = 1;
    EmitTranslateAddress(1);
    Dynarec_BranchPending = savedBranch;
    EmitPush(X86_EAX);
    EmitCall((unsigned)ReadPhysicalWord);
    TrackDoubleWord();
}

void TrackProgramCounter()
{
    EmitMoveImmediate(1, X86_EAX, CPU_LastPC);
    TrackWord();
}

void TrackInstructionChanges()
{
    Dynarec_TrackingLoaded = 0;
    switch (CPU_Instruction >> 26) {
    case 0:
        switch (CPU_Instruction & 63) {
        case 0: case 2: case 3: case 4: case 6: case 7:
        case 16: case 18: case 20: case 22: case 23:
        case 32: case 33: case 34: case 35: case 36: case 37:
        case 38: case 39: case 42: case 43: case 44: case 45:
        case 46: case 47: case 56: case 58: case 59: case 60:
        case 62: case 63:
            TrackGPR((unsigned char)((CPU_Instruction >> 11) & 31));
            break;
        case 9:
            TrackGPR((unsigned char)((CPU_Instruction >> 11) & 31));
        case 8:
            TrackProgramCounter();
            break;
        case 12: case 13: case 15: case 48: case 49:
        case 50: case 51: case 52: case 54:
            TrackProgramCounter();
            break;
        case 19:
            TrackRegister64(&CPU_LO);
            break;
        case 24: case 25: case 26: case 27: case 28: case 29:
        case 30: case 31:
            TrackRegister64(&CPU_LO);
        case 17:
            TrackRegister64(&CPU_HI);
            break;
        default:
            return;
        }
        break;
    case 1:
        TrackProgramCounter();
        break;
    case 16:
        switch ((CPU_Instruction >> 21) & 31) {
        case 0:
            TrackGPR((unsigned char)((CPU_Instruction >> 16) & 31));
            break;
        case 4:
            TrackRegister32(&CPU_COP0[(CPU_Instruction >> 11) & 31]);
            break;
        case 16:
            if (((unsigned char)CPU_Instruction & 63) != 24)
                return;
            TrackProgramCounter();
            break;
        default:
            return;
        }
        break;
    case 17:
        switch ((CPU_Instruction >> 21) & 31) {
        case 0: case 1: case 2:
            TrackGPR((unsigned char)((CPU_Instruction >> 16) & 31));
            break;
        case 4: case 5:
            TrackFPR((CPU_Instruction >> 11) & 31);
            break;
        case 6:
            TrackRegister32(&CPU_FCR31);
            break;
        case 8:
            TrackProgramCounter();
            break;
        case 16: case 17: case 20: case 21:
            switch (CPU_Instruction & 63) {
            case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7:
            case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15:
            case 32: case 33: case 36: case 37:
                TrackFPR((CPU_Instruction >> 6) & 31);
                break;
            case 48: case 49: case 50: case 51: case 52: case 53: case 54: case 55:
            case 56: case 57: case 58: case 59: case 60: case 61: case 62: case 63:
                TrackRegister32(&CPU_FCR31);
                break;
            default:
                return;
            }
            break;
        default:
            return;
        }
        break;
    case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15:
    case 24: case 25: case 26: case 27: case 32: case 33: case 34: case 35:
    case 36: case 37: case 38: case 39: case 55:
        TrackGPR((unsigned char)((CPU_Instruction >> 16) & 31));
        break;
    case 3:
        TrackGPR(31);
    case 2: case 4: case 5: case 6: case 7:
    case 20: case 21: case 22: case 23:
        TrackProgramCounter();
        break;
    case 49: case 53:
        TrackFPR((CPU_Instruction >> 16) & 31);
        break;
    case 40: case 41: case 42: case 43: case 44: case 45: case 46:
    case 57: case 61: case 63:
        TrackMemory();
        break;
    }
    if (Dynarec_TrackingLoaded) {
        EmitStoreImmediate(1, 0x81, (unsigned)UndoData, CPU_Instruction);
        EmitAddImmediate(X86_ECX, 4);
        EmitAndImmediate(1, X86_ECX, 0x3ffffc);
        EmitStore(1, X86_ECX, 5, (unsigned)&UndoPosition);
    }
}
