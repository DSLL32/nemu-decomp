#include "CpuControl.h"
#include "Memory.h"
#include "RSP.h"
#include "Dynarec.h"
#include "DynarecOpcodes.h"
#include "DynarecFPU.h"
#include "Plugins.h"
#include "Rom.h"
#include "RomBrowser.h"
#include "WinMain.h"
#include "State.h"
#include "PIF.h"
#include "FrontendIni.h"
#include <stdio.h>
#include <stdarg.h>
#include <mmsystem.h>
#include <string.h>


extern "C" __declspec(dllimport) int __stdcall kailleraModifyPlayValues(void *, int);
extern "C" __declspec(dllimport) void __stdcall kailleraEndGame();
__declspec(dllimport) void GSInitialize(char *rom, char *rdram, char *iniPath);
__declspec(dllimport) void GSUpdate();

WORD CPU_HostFpuControl;
CpuU32 CPU_CycleShift;
CpuU32 CPU_DisableExceptions;
CpuU32 CPU_DisableVideoHLE;
CpuU32 CPU_DisableAudioHLE;
CpuU32 CPU_PeripheralRegisters[2];
WORD CPU_FpuControlDouble = 0xf3f;
WORD CPU_FpuControlSingle = 0xc3f;
WORD CPU_FpuControlExtended = 0xe3f;
int CPU_Executing;
DWORD CPU_ThreadId;
char SelectedRomFilename[256];
BYTE FaultingInstructions[2097152];
int MemoryBreakHit;
CpuU32 ResetUnknownWord;
CpuS64 PerformanceTicksPerMillisecond;
CpuS64 LastPerformanceCounter;
CpuU32 FrameCounter;
CpuU32 AudioTimingFlag;
CpuU32 LastRspCpuCount;
int RspEnabled = 0x1;
CpuU32 RspSavedPC;
CpuU32 ViMeasureCount;
CpuU32 ViMeasureTime;
CpuU32 RecompilerStepMode;
// Alternate cartridge mapping used by the original word readers and compiler.
static const CpuU32 ROMAlternateBase = 0x13FCB4D0;

static const BYTE BootRspImem[0x1000] = {
#include "res/BootRspImem.inc"
};
static const BYTE DefaultMemPack[0x8000] = {
#include "res/DefaultMemPack.inc"
};

// 00414490. Names are inferred; the exception-context accesses are recovered.
LONG CPUHostException(EXCEPTION_POINTERS *exception)
{
    char message[60];
    char opcodeMessage[100];
    if (exception->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION) {
        sprintf(message, "Unhandled Exception, you should stop emulation: %X",
                exception->ExceptionRecord->ExceptionCode);
        ReportCPUError(message);
        return EXCEPTION_EXECUTE_HANDLER;
    }

    CpuU32 index = Dynarec_ConversionCount - 1;
    while (index < Dynarec_ConversionCount) {
        if ((BYTE *)exception->ExceptionRecord->ExceptionAddress >=
            Dynarec_MainCode + Dynarec_AddressMap[index].offset)
            break;
        --index;
    }
    int disposition;
    // The original failure path reads an uninitialized local disposition.
    if (index > Dynarec_ConversionCount) {
        sprintf(message, "Invalid Access Violation: 0x%X. Can't recover execution",
                exception->ExceptionRecord->ExceptionAddress);
        MessageBox(MainWindow, message, "Error", MB_ICONERROR);
        CPU_ActionFlags |= 0x00400000;
        if (disposition == EXCEPTION_EXECUTE_HANDLER)
            CPU_GPR[29] = exception->ContextRecord->Edi;
        return disposition;
    }

    CpuU32 oldPC = CPU_PC;
    CpuU32 physicalPC = ((CPU_PC & 0xC0000000) == 0x80000000) ?
        CPU_PC & 0x1FFFFFFF : TranslateTLB(CPU_PC, FALSE);
    CPU_PC += Dynarec_AddressMap[index].physical - physicalPC;
    physicalPC = ((CPU_PC & 0xC0000000) == 0x80000000) ?
        CPU_PC & 0x1FFFFFFF : TranslateTLB(CPU_PC, FALSE);
    CpuU32 address;
    bool hostByteOrder;
    if (TLBEnabled && !TLBRamMapped) {
        address = exception->ContextRecord->Ebx;
        address = ((address & 0xC0000000) == 0x80000000) ?
            address & 0x1FFFFFFF : TranslateTLB(address, FALSE);
        hostByteOrder = false;
    } else {
        address = exception->ExceptionRecord->ExceptionInformation[1] - (CpuU32)RDRAM;
        hostByteOrder = true;
    }
    CpuU32 hostOffset = exception->ExceptionRecord->ExceptionInformation[1] - (CpuU32)RDRAM;
    if (address >= RDRAMSize ||
        (TLBEnabled && !TLBRamMapped && hostOffset >= RDRAMSize)) {
        switch (Dynarec_Mode) {
        case 0: case 3: case 4: case 5:
            if (physicalPC < RDRAMSize)
                memset(&Dynarec_BlockCode[(physicalPC >> 2) & 0x3FFFFC00], 0, 0x1000);
            break;
        case 1: case 2:
            for (CpuU32 i = 0; i < CPUBreakpointCount; ++i) {
                if ((CPUBreakpoints[i].Address & 0xFFFFF000) ==
                    (physicalPC & 0xFFFFF000)) {
                    if (ReadPhysicalWord(CPUBreakpoints[i].Address) ==
                        (CPUBreakpoints[i].CodeOffset | 0xEC000000))
                        WritePhysicalWord(CPUBreakpoints[i].Address,
                                  CPUBreakpoints[i].Instruction);
                    if (i + 1 <= CPUBreakpointCount - 1) {
                        if (i < CPUBreakpointCount)
                            memcpy(&CPUBreakpoints[i], &CPUBreakpoints[i + 1],
                                   (CPUBreakpointCount - i) * sizeof(DebugBreakpoint));
                        --CPUBreakpointCount;
                    }
                }
            }
            break;
        }
    }
    if (address == 0x00800001) {
        CPU_ActionFlags |= 0x00040000;
        FaultingInstructions[physicalPC >> 2] = 1;
        CPU_GPR[29] = exception->ContextRecord->Edi;
        return EXCEPTION_EXECUTE_HANDLER;
    }
    disposition = EXCEPTION_CONTINUE_EXECUTION;
    DWORD oldProtection;
    if (address < RDRAMSize &&
        (!TLBEnabled || TLBRamMapped || hostOffset < RDRAMSize)) {
        memset(&Dynarec_BlockCode[(address >> 2) & 0x3FFFFC00], 0, 0x1000);
        VirtualProtect(RDRAM + (address & 0xFFFFF000), 0x1000, PAGE_READWRITE, &oldProtection);
    } else {
        FaultingInstructions[physicalPC >> 2] = 1;
    }

    BYTE *code = (BYTE *)exception->ExceptionRecord->ExceptionAddress;
    CONTEXT *context = exception->ContextRecord;
    switch (code[0]) {
    case 0x88:
        WritePhysicalByte(address ^ (hostByteOrder ? 3 : 0),
                  (BYTE)*ContextRegister(context, (code[1] >> 3) & 7));
        context->Eip += 6;
        break;
    case 0x66:
        WritePhysicalHalf(address ^ (hostByteOrder ? 2 : 0),
                      (WORD)*ContextRegister(context, (code[2] >> 3) & 7));
        context->Eip += 7;
        break;
    case 0x89:
        WritePhysicalWord(address, *ContextRegister(context, (code[1] >> 3) & 7));
        context->Eip += 6;
        break;
    case 0xC7:
        WritePhysicalWord(address, *(CpuU32 *)(code + 6));
        context->Eip += 10;
        break;
    case 0x0F: {
        CpuU32 value; // Unsupported secondary opcodes retain the original uninitialized value.
        switch (code[1]) {
        case 0xB6: value = ReadPhysicalByte(address ^ (hostByteOrder ? 3 : 0)); break;
        case 0xB7: value = ReadPhysicalHalf(address ^ (hostByteOrder ? 2 : 0)); break;
        case 0xBE: value = (signed char)ReadPhysicalByte(address ^ (hostByteOrder ? 3 : 0)); break;
        case 0xBF: value = (short)ReadPhysicalHalf(address ^ (hostByteOrder ? 2 : 0)); break;
        }
        *ContextRegister(context, (code[2] >> 3) & 7) = value;
        context->Eip += 7;
        break;
    }
    case 0x8B:
        *ContextRegister(context, (code[1] >> 3) & 7) = ReadPhysicalWord(address);
        context->Eip += 6;
        break;
    default:
        sprintf(opcodeMessage, "Unhandled mov(sx/zx) caused exception: %X", code[0]);
        ReportCPUError(opcodeMessage);
        disposition = EXCEPTION_EXECUTE_HANDLER;
        break;
    }
    if ((address >= RDRAMSize ||
        (TLBEnabled && !TLBRamMapped && hostOffset >= RDRAMSize)) &&
        disposition == EXCEPTION_CONTINUE_EXECUTION) {
        CPU_COP0[9] -= 10000;
        CPU_VideoCyclesRemaining += 10000;
        CPU_AudioCyclesRemaining += 10000;
        context->Esi -= 10000;
    }
    if (MemoryBreakHit) {
        CPU_ActionFlags |= 0x10000;
        MemoryBreakHit = 0;
        disposition = EXCEPTION_EXECUTE_HANDLER;
    }
    if (disposition == EXCEPTION_CONTINUE_EXECUTION)
        CPU_PC = oldPC;
    else if (disposition == EXCEPTION_EXECUTE_HANDLER)
        CPU_GPR[29] = context->Edi;
    return disposition;
}

// 00414BA0. The original uses inline assembly for the four partial products.
// The first cross-product uses leftLow * leftHigh, as observed in the input.
void __stdcall MultiplyUnsigned64(CpuU32 leftLow, CpuU32 leftHigh,
                                 CpuU32 rightLow, CpuU32 rightHigh)
{
    __asm {
        mov eax, leftLow
        mov edx, rightLow
        mul edx
        mov dword ptr CPU_LO, eax
        mov eax, leftLow
        mov dword ptr CPU_LO[4], edx
        mov edx, leftHigh
        mul edx
        add dword ptr CPU_LO[4], eax
        mov eax, leftHigh
        mov dword ptr CPU_HI, edx
        mov edx, rightLow
        mov dword ptr CPU_HI[4], 0
        adc dword ptr CPU_HI, 0
        adc dword ptr CPU_HI[4], 0
        mul edx
        add dword ptr CPU_LO[4], eax
        mov eax, leftHigh
        adc dword ptr CPU_HI, edx
        mov edx, rightHigh
        adc dword ptr CPU_HI[4], 0
        mul edx
        add dword ptr CPU_HI, eax
        adc dword ptr CPU_HI[4], edx
    }
}

// 00414C20. Nested saves of EBP/EDI/ESI and explicit carry propagation identify
// the original inline assembly inside its compiler-generated function frame.
void __stdcall MultiplySigned64(CpuS64 left, CpuS64 right)
{
    __asm {
        push ebp
        push edi
        push esi
        mov ebx, dword ptr right
        mov ecx, dword ptr right[4]
        mov edx, ecx
        mov esi, dword ptr left
        mov edi, dword ptr left[4]
        mov ebp, edi
        sar edx, 31
        sar ebp, 31
        xor ecx, edx
        xor ebx, edx
        sub ebx, edx
        sbb ecx, edx
        xor esi, ebp
        xor edi, ebp
        sub esi, ebp
        sbb edi, ebp
        xor ebp, edx
        mov eax, esi
        mul ebx
        mov dword ptr CPU_LO, eax
        mov eax, esi
        mov dword ptr CPU_LO[4], edx
        mul ecx
        add dword ptr CPU_LO[4], eax
        mov eax, edi
        mov dword ptr CPU_HI, edx
        mov dword ptr CPU_HI[4], 0
        adc dword ptr CPU_HI, 0
        adc dword ptr CPU_HI[4], 0
        mul ebx
        add dword ptr CPU_LO[4], eax
        mov eax, edi
        adc dword ptr CPU_HI, edx
        adc dword ptr CPU_HI[4], 0
        mul ecx
        add dword ptr CPU_HI, eax
        adc dword ptr CPU_HI[4], edx
        xor dword ptr CPU_LO, ebp
        xor dword ptr CPU_LO[4], ebp
        xor dword ptr CPU_HI, ebp
        xor dword ptr CPU_HI[4], ebp
        sub dword ptr CPU_LO, ebp
        sbb dword ptr CPU_LO[4], ebp
        sbb dword ptr CPU_HI, ebp
        sbb dword ptr CPU_HI[4], ebp
        pop esi
        pop edi
        pop ebp
    }
}

// 00414CF0
void __stdcall DivideUnsigned64(CpuU64 left, CpuU64 right)
{
    if (right) {
        CPU_LO = left / right;
        CPU_HI = left % right;
    } else {
        CPU_LO = -1;
        CPU_HI = left;
    }
}

// 00414D50
void __stdcall DivideSigned64(CpuS64 left, CpuS64 right)
{
    if (right) {
        CPU_LO = left / right;
        CPU_HI = left % right;
    } else {
        CPU_LO = -1;
        CPU_HI = left;
    }
}

// 00414DB0
void ReportCPUErrorFormat(const char *format, ...)
{
    char message[300];
    va_list arguments;
    va_start(arguments, format);
    vsprintf(message, format, arguments);
    va_end(arguments);
    if (ShowErrorMessages && !FullScreen && CPU_Executing &&
        MessageBox(0, message, "CPU error, continue?", MB_YESNO | MB_ICONHAND) == IDNO) {
        CPU_Executing = 0;
        CPU_ActionFlags |= 0x00400000;
    }
}
// 00414E30. Original whole-program optimization passes message in ECX and address in EAX.
void ReportMemoryError(const char *message, CpuU32 address)
{
    char text[256];
    sprintf(text, "%s, at address: 0x%X, continue?", message, address);
    if (ShowErrorMessages && !FullScreen && ShowErrors && CPU_Executing &&
        MessageBox(0, text, "Memory error", MB_YESNO | MB_ICONHAND) == IDNO) {
        CPU_Executing = 0;
        CPU_ActionFlags |= 0x00400000;
    }
}
// 00414EB0
void ReportCPUError(const char *message)
{
    if (ShowErrorMessages && !FullScreen && CPU_Executing &&
        MessageBox(0, message, "CPU error, continue?", MB_YESNO | MB_ICONHAND) == IDNO) {
        CPU_Executing = 0;
        CPU_ActionFlags |= 0x00400000;
    }
}
// 00414F00
void ReportInterpreterError(const char *message)
{
    if (ShowErrorMessages && !FullScreen && CPU_Executing &&
        MessageBox(0, message, "CPU Interpreter error, continue?", MB_YESNO | MB_ICONHAND) == IDNO) {
        CPU_Executing = 0;
        CPU_ActionFlags |= 0x00400000;
    }
}
// 00414F50
void ReportCompilerError(const char *message)
{
    if (ShowErrorMessages && !FullScreen && CPU_Executing &&
        MessageBox(0, message, "Compiler error, continue?", MB_YESNO | MB_ICONHAND) == IDNO) {
        CPU_Executing = 0;
        CPU_ActionFlags |= 0x00400000;
    }
}
// 00414FA0
void __stdcall SetFpuRounding(CpuU32 value)
{
    CPU_FpuControlDouble = 0x033F;
    CPU_FpuControlSingle = 0x003F;
    CPU_FpuControlExtended = 0x023F;
    switch (value & 3) {
    case 1: CPU_FpuControlDouble = 0x0F3F; CPU_FpuControlSingle = 0x0C3F; CPU_FpuControlExtended = 0x0E3F; break;
    case 2: CPU_FpuControlDouble = 0x0B3F; CPU_FpuControlSingle = 0x083F; CPU_FpuControlExtended = 0x0A3F; break;
    case 3: CPU_FpuControlDouble = 0x073F; CPU_FpuControlSingle = 0x043F; CPU_FpuControlExtended = 0x063F; break;
    }
}
// 00415030
void ResetRecompiler(void)
{
    DWORD oldProtection;
    VirtualProtect(RDRAM, RDRAMSize, PAGE_READWRITE, &oldProtection);
    if ((CPUBreakpointCount && Dynarec_Mode == 1) || Dynarec_Mode == 2) {
        for (CpuU32 i = 0; i < CPUBreakpointCount; ++i) {
            if (ReadPhysicalWord(CPUBreakpoints[i].Address) ==
                (CPUBreakpoints[i].CodeOffset | 0xEC000000)) {
                AllowRomWrites = 1;
                WritePhysicalWord(CPUBreakpoints[i].Address, CPUBreakpoints[i].Instruction);
                AllowRomWrites = 0;
            }
        }
    }
    memset(Dynarec_BlockCode, 0, sizeof(Dynarec_BlockCode));
    CPUBreakpointCount = 0;
    if (ROMCacheEnabled)
        memset(ROMCache, 0, ROMSize);
    memset(Dynarec_CompiledPages, 0, sizeof(Dynarec_CompiledPages));
    Dynarec_MainCodeUsed = 0;
    Dynarec_FastCodeUsed = 0;
    Dynarec_ConversionCount = 1;
}
// 004157D0
void UpdateAudioTiming(void)
{
    CpuU32 rate = VideoNTSC == 0 ? 49656530 : 48681812;
    CPU_AudioEnabled = 1;
    CPU_AudioCyclesRemaining = (int)((int)(((CpuS64)(AIReg.Dacrate + 1) *
        (AIReg.Length >> 2) * 46875000) / rate) * 0.98f);
}
// 004169E0
void StartCPUThread(void)
{
    CPU_Thread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)RunCPU, 0, 0, &CPU_ThreadId);
}

// 00414420. Index 4 (ESP) is deliberately unsupported.
CpuU32 *ContextRegister(CONTEXT *context, CpuU32 index)
{
    switch ((BYTE)index) {
    case 0: return (CpuU32 *)&context->Eax;
    case 1: return (CpuU32 *)&context->Ecx;
    case 2: return (CpuU32 *)&context->Edx;
    case 3: return (CpuU32 *)&context->Ebx;
    case 5: return (CpuU32 *)&context->Ebp;
    case 6: return (CpuU32 *)&context->Esi;
    case 7: return (CpuU32 *)&context->Edi;
    default: return 0;
    }
}

// 00415120
void ResetCPU(void)
{
    CPU_ActionFlags &= ~0x4000;
    memset(CPUBreakpoints, 0, sizeof(CPUBreakpoints));
    ShowErrors = 1;
    CPU_PC = 0xA4000040;
    CPUBreakpointCount = 0;
    memset(FaultingInstructions, 0, sizeof(FaultingInstructions));
    ResetRecompiler();
    Dynarec_Use32BitMemoryAccess = CPU_Use32BitMemoryAccess;
    Dynarec_DelaySlot = 0;
    if (!ForcePIFSuccess)
        memset(RDRAM, 0, RDRAMSize);
    CopyAndSwapWords(SPMemory + 0x40, ROMImage + 0x40, 0xFC0);
    memcpy(SPMemory + 0x1000, BootRspImem, 0x1000);
    memset(PIFRam, 0, sizeof(PIFRam));
    AllowRomWrites = 0;
    FILE *file = 0;
    if (VideoNTSC == 0) {
        if (strlen(PIFNTSCFilename) == 0)
            goto initializeRegisters;
        file = fopen(PIFNTSCFilename, "r");
    }
    if (VideoNTSC == 1 && strlen(PIFPALFilename) != 0)
        file = fopen(PIFPALFilename, "r");
    if (file) {
        fread(PIFRom, 0x7C0, 1, file);
        fclose(file);
        CPU_PC = 0xBFC00000;
    }
initializeRegisters:
    CPU_GPR[2] = (CpuS64)(int)0xD1731BE9;
    CPU_GPR[3] = (CpuS64)(int)0xD1731BE9;
    CPU_GPR[12] = (CpuS64)(int)0xD1330BC3;
    CPU_GPR[13] = (CpuS64)(int)0xD1330BC3;
    CPU_GPR[20] = (CpuS32)VideoNTSC;
    TLBEnabled = CPU_TLBEnabled;
    CPU_LO = CPU_HI = 0;
    CPU_GPR[0] = 0;
    CPU_GPR[21] = (int)(ForcePIFSuccess != 0);
    CPU_GPR[22] = (CpuS32)BootCicSeed;
    CPU_GPR[23] = (CpuS32)BootVersion;
    CPU_GPR[1] = 0;
    CPU_GPR[4] = 0x1BE9;
    CPU_GPR[5] = (CpuS64)(int)0xF45231E5;
    CPU_GPR[6] = (CpuS64)(int)0xA4001F0C;
    CPU_GPR[7] = (CpuS64)(int)0xA4001F08;
    CPU_GPR[8] = 0xC0;
    CPU_GPR[9] = 0;
    CPU_GPR[10] = 0x40;
    CPU_GPR[11] = (CpuS64)(int)0xA4000040;
    CPU_GPR[14] = 0x25613A26;
    CPU_GPR[15] = 0x2EA04317;
    CPU_GPR[16] = CPU_GPR[17] = CPU_GPR[18] = CPU_GPR[19] = 0;
    CPU_GPR[24] = 0;
    CPU_GPR[25] = (CpuS64)(int)0xD73F2993;
    CPU_GPR[26] = CPU_GPR[27] = CPU_GPR[28] = 0;
    CPU_GPR[29] = (CpuS64)(int)0xA4001FF0;
    CPU_GPR[30] = 0;
    CPU_GPR[31] = (CpuS64)(int)0xA4001554;
    memset(CPU_COP0, 0, sizeof(CPU_COP0));
    memset(TLB, 0, sizeof(TLB));
    memset(CPU_FPR.words, 0, sizeof(CPU_FPR.words));
    memset(RSPRegisters, 0, sizeof(RSPRegisters));
    memset(RSPVectors, 0, sizeof(RSPVectors));
    memset(RSPAccumulator, 0, sizeof(RSPAccumulator));
    memset(&RIReg, 0, sizeof(RIReg));
    memset(&SPReg, 0, sizeof(SPReg));
    MIReg.Mode = MIReg.InterruptMask = 0;
    memset(&AIReg, 0, sizeof(AIReg));
    memset(&PIReg, 0, sizeof(PIReg));
    memset(&VIReg, 0, sizeof(VIReg));
    memset(&RDRAMReg, 0, sizeof(RDRAMReg));
    *(CpuU32 *)RSPFlags = 0;
    CPU_COP0[12] = 0x34000000;
    CPU_FRMode = 1;
    CPU_FpuModeChanges = 0;
    Dynarec_DynamicFpuMode = 0;
    CPU_COP0[15] = 0xB00;
    CPU_COP0[16] = 0x6E463;
    CPU_COP0[4] = 0;
    CPU_Random = 0;
    CPU_TLBExceptionDelay = 0;
    RSPFlags[2] = 0;
    RSPDelayPC = 0;
    MIReg.Interrupt = 0;
    SIAddress = 0;
    memset(&DPCReg, 0, sizeof(DPCReg));
    CPU_AudioEnabled = 0;
    CpuU32 cartridgeTiming = ReadPhysicalWord(0x10000000);
    PIReg.Domain1Latency = cartridgeTiming & 0xFF;
    PIReg.Domain1PulseWidth = (cartridgeTiming >> 8) & 0xFF;
    PIReg.Domain1PageSize = (cartridgeTiming >> 16) & 0xF;
    PIReg.Domain1Release = (cartridgeTiming >> 20) & 3;
    SPReg.Status = 0x41;
    DPCReg.Status = 0x88;
    CPU_CompareReset = 1;
    CPU_VideoPeriod = CPU_VideoCyclesRemaining = 6250000;
    VICurrentLine = 0;
    MemoryBreakHit = 0;
    AudioTimingFlag = 0;
    LastPerformanceCounter = 0;
    QueryPerformanceFrequency((LARGE_INTEGER *)&PerformanceTicksPerMillisecond);
    PerformanceTicksPerMillisecond /= 1000;
    FrameCounter = 0;
    ResetUnknownWord = 0;
    file = fopen(MemPackFilename, "rb");
    if (file) {
        fread(MemPackData, sizeof(MemPackData), 1, file);
        fclose(file);
        MemPackUsed = 1;
    } else {
        memcpy(MemPackData, DefaultMemPack, sizeof(MemPackData));
        MemPackUsed = 0;
    }
    file = fopen(SRAMFilename, "rb");
    if (file) {
        fread(SRAMData, sizeof(SRAMData), 1, file);
        fclose(file);
        SRAMUsed = 1;
    } else {
        memset(SRAMData, 0, sizeof(SRAMData));
        SRAMUsed = 0;
    }
    file = fopen(EEPROMFilename, "rb");
    if (file) {
        fread(EEPROMData, sizeof(EEPROMData), 1, file);
        fclose(file);
        EEPROMUsed = 1;
    } else {
        memset(EEPROMData, 0, sizeof(EEPROMData));
        EEPROMUsed = 0;
    }
    DisableSaveWrites = 0;
    if (ForcePIFSuccess)
        ForcePIFSuccess = 0;
    GSInitialize((char *)ROMImage, (char *)RDRAM, CheatsIniPath);
    UndoPosition = 0;
}

// 00415770. Save the host x87 control word before changing rounding modes.
void InitializeCPU(void)
{
    __asm {
        fwait
        fnstcw CPU_HostFpuControl
    }
    memcpy(RomHeader, ROMImage, sizeof(RomHeader));
    if (SelectedGraphicsPlugin->RomOpen)
        SelectedGraphicsPlugin->RomOpen();
    AIInit(MainWindow, RDRAM, CPU_COP0, (CpuU32 *)&AIReg, (CpuU32 *)&MIReg, CPU_PeripheralRegisters);
    ResetCPU();
}

// 00415840. Original generated-code register ABI is preserved in the two
// symbolic inline-assembly entry sequences, including timing and guest SP.
// The original retains unoptimized C++ loop and condition sequences around
// its generated-code boundary, including the explicit while (1) test.
#pragma optimize("", off)
void RunCPU(void)
{
    if (!LoadRom(MainWindow, SelectedRomFilename)) {
        char title[100];
        SetEmulationMenus(0);
        sprintf(title, "Nemu64 - Exploring %s", RomDirectory);
        SendMessage(MainWindow, WM_SETTEXT, 0, (LPARAM)title);
        SetStatusText(0, "I am Bender - please insert Gurder");
        SetStatusText(1, "");
        SetStatusText(2, "");
        CloseHandle(CPU_Thread);
        FreeRomImage();
        CPU_Thread = 0;
        return;
    }
    SetStatusText(0, "NEMUlating...");
    SetThreadPriority(CPU_Thread, THREAD_PRIORITY_NORMAL);
    InitializeCPU();
    CPU_Executing = 1;
    if (CPU_InterpreterMode)
        ExecuteInterpreter();
    else {
        LastRspCpuCount = 0;
        for (;;) {
            bool translated = (CPU_PC & 0xC0000000) != 0x80000000;
            CpuU32 physical = !translated ?
                CPU_PC & 0x1FFFFFFF : TranslateTLB(CPU_PC, FALSE);
            if (translated && physical == 0x00800001) {
                CPU_ActionFlags |= 0x40000;
            } else {
                CpuU32 offset;
                bool compile = false;
                if (Dynarec_Mode == 1 || Dynarec_Mode == 2) {
                    if (physical < RDRAMSize)
                        offset = *(CpuU32 *)(RDRAM + physical);
                    else if (physical >= 0x10000000 && physical < 0x1FBFFFFF && ROMCacheEnabled == 1) {
                        if (physical >= 0x14000000)
                            physical -= ROMAlternateBase;
                        // This unusual address-of-global lookup is present in the target.
                        offset = *(CpuU32 *)((BYTE *)&ROMCache + physical);
                        compile = offset == 0;
                        goto haveOffset;
                    } else
                        offset = ReadPhysicalWord(physical);
                    if ((offset >> 26) == 0x3B)
                        offset &= 0x03FFFFFF;
                    else
                        compile = true;
                } else {
                    if (physical < RDRAMSize)
                        offset = *(CpuU32 *)((BYTE *)Dynarec_BlockCode + physical);
                    else if (physical >= 0x10000000 && physical < 0x1FBFFFFF && ROMCacheEnabled == 1) {
                        if (physical >= 0x14000000)
                            physical -= ROMAlternateBase;
                        offset = *(CpuU32 *)((BYTE *)&ROMCache + physical);
                    } else
                        offset = 0;
                    compile = offset == 0;
                }
haveOffset:
                if (compile) {
                    NotifyTools(0x40, 1);
                    __try {
                        __asm fwait
                        CompileBlock();
                        __asm fwait
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) { ResetRecompiler(); CompileBlock(); }
                    NotifyTools(0x40, 0);
                } else
                    Dynarec_EntryPoint = Dynarec_MainCode + offset;
                __try {
                    __asm {
                        fwait
                        mov eax, Dynarec_EntryPoint
                        push ebx
                        push esi
                        push ebp
                        push edi
                        mov edi, dword ptr CPU_GPR[232]
                        mov esi, 10000
                        call eax
                        mov dword ptr CPU_GPR[232], edi
                        sub esi, 10000
                        mov ecx, CPU_CycleShift
                        shl esi, cl
                        add CPU_VideoCyclesRemaining, esi
                        add CPU_AudioCyclesRemaining, esi
                        sub dword ptr CPU_COP0[36], esi
                        pop edi
                        pop ebp
                        pop esi
                        pop ebx
                        fwait
                    }
                } __except (CPUHostException(GetExceptionInformation())) { }
            }
processEvents:
            if (CPU_COP0[9] < 20000 && !CPU_CompareReset) {
                CPU_ComparePending = 1;
                CPU_CompareReset = 1;
            }
            if (CPU_AudioEnabled && CPU_AudioCyclesRemaining < 0) {
                CPU_ActionFlags |= 0x10;
                AIReg.Status &= ~0x40000000;
                AIReg.Status &= ~0x80000000;
                if (AIReg.NextLength) {
                    AIReg.DramAddress = AIReg.NextDramAddress;
                    AIReg.NextDramAddress = 0;
                    AIReg.Length = AIReg.NextLength;
                    AIReg.NextLength = 0;
                    UpdateAudioTiming();
                    AISetReg(0, 0);
                } else {
                    AIReg.DramAddress = 0;
                    AIReg.Length = 0;
                    CPU_AudioEnabled = 0;
                }
            }
            if (CPU_VideoCyclesRemaining < 0) {
                CPU_VideoCyclesRemaining += CPU_VideoPeriod;
                if (MIReg.InterruptMask & 8)
                    CPU_ActionFlags |= 1;
                NotifyTools(0x10, 1);
                if (SelectedGraphicsPlugin->UpdateScreen)
                    SelectedGraphicsPlugin->UpdateScreen();
                if (NetPlayActive) {
                    CIGetJoyData();
                    CpuU32 values[4] = {Controllers[0].Buttons, 0, 0, 0};
                    kailleraModifyPlayValues(values, 4);
                    Controllers[0].Buttons = values[0];
                    Controllers[1].Buttons = values[1];
                    Controllers[2].Buttons = values[2];
                    Controllers[3].Buttons = values[3];
                }
                NotifyTools(0x10, 0);
                ++VICount;
                if (++ViMeasureCount >= 31) {
                    DWORD now = timeGetTime();
                    if (now != ViMeasureTime && ViMeasureTime) {
                        char text[124];
                        sprintf(text, "VI/s: %3.2f", 31.0 / ((CpuU64)(now - ViMeasureTime) / 1000.0));
                        if (!FullScreen)
                            SetStatusText(1, text);
                    }
                    ViMeasureTime = now;
                    ViMeasureCount = 0;
                }
            }
            if (CPU_ComparePending && CPU_COP0[9] >= CPU_COP0[11]) {
                CPU_ActionFlags |= 0x40;
                CPU_ComparePending = 0;
            }
            if (!CPU_ActionFlags)
                continue;
            if (CPU_ActionFlags & 1)
                GSUpdate();
            RaisePendingInterrupt(1, 8);
            RaisePendingInterrupt(2, 2);
            RaisePendingInterrupt(4, 0x10);
            RaisePendingInterrupt(8, 1);
            RaisePendingInterrupt(0x10, 4);
            RaisePendingInterrupt(0x20, 0x20);
            if ((CPU_ActionFlags & 0x40) && (CPU_COP0[12] & 0x8000) &&
                (CPU_COP0[12] & 1) && !(CPU_COP0[12] & 6)) {
                CPU_COP0[13] |= 0x8000;
                CPU_COP0[13] &= 0x7FFFFF83;
                CPU_COP0[12] |= 2;
                CPU_COP0[14] = CPU_PC;
                CPU_PC = 0x80000180;
                CPU_ActionFlags &= ~0x40;
            }
            if ((CPU_ActionFlags & 0x08000000) && (CPU_COP0[12] & 0x1000) && (CPU_COP0[12] & 1)) {
                CPU_COP0[13] |= 0x1000;
                CPU_COP0[13] &= 0x7FFFFF83;
                CPU_COP0[12] |= 2;
                CPU_COP0[14] = CPU_PC;
                CPU_PC = 0x80000180;
                CPU_ActionFlags &= ~0x08000000;
            }
            if (CPU_ActionFlags & 0x02000000) {
                if (!(CPU_COP0[12] & 2)) CPU_COP0[14] = CPU_PC;
                else ReportCPUError("Doing cop1 unusable exception while exception is already being executed. Dunno if this will work");
                if (CPU_Cop1UnusableDelay) CPU_COP0[13] |= 0x80000000;
                else CPU_COP0[13] &= 0x7FFFFFFF;
                CPU_COP0[13] &= 0xFFFFFF83;
                CPU_COP0[13] |= 0x2C;
                CPU_COP0[12] |= 2;
                CPU_COP0[13] &= 0xCFFFFFFF;
                CPU_COP0[13] |= 0x10000000;
                CPU_PC = 0x80000180;
                CPU_ActionFlags &= ~0x02000000;
            }
            if (CPU_ActionFlags & 0x80000) {
                ReportCPUError("COP1 Exception trap taken");
                if (!(CPU_COP0[12] & 2)) CPU_COP0[14] = CPU_PC;
                else ReportCPUError("Doing cop1 trap exception while exception is already being executed. Dunno if this will work");
                if (CPU_FpuExceptionDelay) CPU_COP0[13] |= 0x80000000;
                else CPU_COP0[13] &= 0x7FFFFFFF;
                CPU_COP0[13] &= 0xFFFFFF83;
                CPU_COP0[13] |= 0x3C;
                CPU_COP0[12] |= 2;
                CPU_PC = 0x80000180;
                CPU_ActionFlags &= ~0x80000;
            }
            if (CPU_ActionFlags & 0x40000) {
                if (!(CPU_COP0[12] & 2)) CPU_COP0[14] = CPU_PC;
                else ReportCPUError("Doing tlb exception while exception is already being executed. Dunno if this will work");
                if (CPU_TLBExceptionDelay) {
                    CPU_COP0[13] |= 0x80000000;
                    CPU_TLBExceptionDelay = 0;
                } else CPU_COP0[13] &= 0x7FFFFFFF;
                CPU_COP0[13] &= 0xFFFFFF83;
                switch (TranslationError) {
                case 1:
                    CPU_COP0[13] |= TLBWriteAccess ? 0xC : 8;
                    CPU_PC = (CPU_COP0[12] & 2) ? 0x80000180 : 0x80000000;
                    break;
                case 2: case 3:
                    CPU_COP0[13] |= TLBWriteAccess ? 0xC : 8;
                    CPU_PC = 0x80000180;
                    break;
                case 4: case 5:
                    CPU_COP0[13] |= 4;
                    CPU_PC = 0x80000180;
                    break;
                }
                CPU_COP0[4] = (CPU_COP0[4] & 0xFF800000) | ((TLBAddress >> 9) & 0x007FFFF0);
                CPU_COP0[8] = TLBAddress;
                CPU_COP0[10] = (CPU_COP0[10] & 0xFF) | (TLBAddress & 0xFFFFE000);
                CPU_COP0[12] |= 2;
                CPU_ActionFlags &= ~0x40000;
            }
            if (CPU_ActionFlags & 0x01000000) {
                CPU_COP0[13] &= 0x7FFFFF83;
                CPU_COP0[13] |= 0x20;
                CPU_COP0[14] = CPU_PC;
                CPU_COP0[12] |= 2;
                CPU_PC = 0x80000180;
                CPU_ActionFlags &= ~0x01000000;
            }
            if (CPU_ActionFlags & 0x400) { ResetRecompiler(); CPU_ActionFlags &= ~0x400; }
            if (CPU_ActionFlags & 0x800) { ResetCPU(); CPU_ActionFlags &= ~0x800; }
            if (CPU_ActionFlags & 0x4000) {
                NotifyTools(0x200, 1);
                if (RspEnabled) {
                    if (CPU_RSPCycles > 3000) {
                        CPU_RSPCycles = 0;
                        do {
                            for (CpuU32 i = 0; i < RSPBreakpointCount; ++i)
                                if (RSPBreakpoints[i] == (SPReg.PC & 0xFFF)) CPU_ActionFlags |= 0x20000;
                            if (CPU_ActionFlags & 0x20000) {
                                NotifyTools(2, 1);
                                SuspendThread(CPU_Thread);
                                RSPBreakpointInhibit = 1;
                            } else RSPBreakpointInhibit = 0;
                            CPU_Instruction = ReadPhysicalWord((SPReg.PC & 0xFFF) + 0x04001000);
                            RSPRegisters[0] = 0;
                            RSP_MainDispatch[(CPU_Instruction >> 26) & 63]();
                            --CPU_RSPCycles;
                            if (RSPDelayPC) {
                                RspSavedPC = SPReg.PC;
                                SPReg.PC = RSPDelayPC;
                                bool interrupted = false;
                                if (!(CPU_ActionFlags & 0x20000)) {
                                    for (CpuU32 i = 0; i < RSPBreakpointCount; ++i)
                                        if (RSPBreakpoints[i] == (SPReg.PC & 0xFFF)) {
                                            CPU_ActionFlags |= 0x20000;
                                            interrupted = true;
                                            RSPDelayPC = 0;
                                            SPReg.PC -= 4;
                                        }
                                }
                                if (!interrupted) {
                                    CPU_Instruction = ReadPhysicalWord((SPReg.PC & 0xFFF) + 0x04001000);
                                    RSP_MainDispatch[(CPU_Instruction >> 26) & 63]();
                                    SPReg.PC = RspSavedPC;
                                    RSPDelayPC = 0;
                                    --CPU_RSPCycles;
                                }
                            }
                        } while (CPU_RSPCycles > -3000 && (CPU_ActionFlags & 0x4000));
                    } else {
                        if (LastRspCpuCount)
                            CPU_RSPCycles += CPU_COP0[9] - LastRspCpuCount;
                        LastRspCpuCount = CPU_COP0[9];
                    }
                }
                NotifyTools(0x200, 0);
            }
            if (CPU_ActionFlags & 0x10000) {
                NotifyTools(2, 0);
                SuspendThread(CPU_Thread);
                CPU_LastPC = CPU_PC;
                CpuU32 physical = ((CPU_LastPC & 0xC0000000) == 0x80000000) ?
                    CPU_LastPC & 0x1FFFFFFF : TranslateTLB(CPU_LastPC, FALSE);
                CPU_Instruction = ReadPhysicalWord(physical);
                CpuU32 savedCodePosition = Dynarec_MainCodeUsed;
                Dynarec_BranchPending = 1;
                RecompilerStepMode = 1;
                Dynarec_BlockInstructionCount = 1;
                ResetInstructionBuffer(0);
                TrackInstructionChanges();
                Dynarec_MainDispatch[(CPU_Instruction >> 26) & 63]();
                FinishInstruction();
                EmitReturn();
                AssembleInstructions(0);
                Dynarec_RecordAddresses = 1;
                Dynarec_MainCodeUsed = savedCodePosition;
                Dynarec_BranchPending = 0;
                RecompilerStepMode = 0;
                CPU_PC += 4;
                BYTE *singleInstruction = Dynarec_MainCode + savedCodePosition;
                __asm {
                    push ebx
                    push esi
                    push ebp
                    mov esi, 10000
                    mov edi, dword ptr CPU_GPR[232]
                    call singleInstruction
                    mov dword ptr CPU_GPR[232], edi
                    sub esi, 10000
                    sub dword ptr CPU_COP0[36], esi
                    add CPU_VideoCyclesRemaining, esi
                    add CPU_AudioCyclesRemaining, esi
                    pop ebp
                    pop esi
                    pop ebx
                }
                goto processEvents;
            }
            if ((CPU_ActionFlags & 0x200000) && !ActivePause)
                SuspendThread(CPU_Thread);
            if (CPU_ActionFlags & 0x1000) { CPU_ActionFlags &= ~0x1000; LoadState(false); }
            if (CPU_ActionFlags & 0x2000) { CPU_ActionFlags &= ~0x2000; SaveState(); }
            if (CPU_ActionFlags & 0x04000000) {
                CPU_ActionFlags &= ~0x04000000;
                if (SelectedGraphicsPlugin->CloseDLL) SelectedGraphicsPlugin->CloseDLL();
                if (SelectedGraphicsPlugin->RomClosed) SelectedGraphicsPlugin->RomClosed();
                SelectedGraphicsPlugin = PendingGraphicsPlugin;
                PendingGraphicsPlugin = 0;
                InitializeGraphicsPlugin();
                if (SelectedGraphicsPlugin->RomOpen) SelectedGraphicsPlugin->RomOpen();
            }
            if (CPU_ActionFlags & 0x400000) {
                CPU_ActionFlags &= ~0x400000;
                break;
            }
        }
    }
    AIFree();
    if (SelectedGraphicsPlugin->RomClosed) SelectedGraphicsPlugin->RomClosed();
    if (RuntimeLibraryEnabled) DynarecNoOperation();
    if (DisableSaveWrites && (EEPROMUsed || MemPackUsed || SRAMUsed) &&
        MessageBox(MainWindow,
        "Do you want to replace your save files on disk (EEProm, SRAM, Flashram and/or Mempack) with the ones in memory?",
        "Store save data", MB_YESNO | MB_ICONQUESTION) == IDYES)
        FlushStateSaves();
    DisableSaveWrites = 0;
    EnableMenuItem(GetMenu(MainWindow), 0x8040, MF_GRAYED);
    if (!Closing) {
        char title[100];
        SetEmulationMenus(0);
        sprintf(title, "Nemu64 - Exploring %s", RomDirectory);
        SendMessage(MainWindow, WM_SETTEXT, 0, (LPARAM)title);
        SetStatusText(0, "I am Bender - please insert Gurder");
        SetStatusText(1, "");
        SetStatusText(2, "");
    }
    CloseHandle(CPU_Thread);
    FreeRomImage();
    CPU_Thread = 0;
    if (NetPlayActive) {
        kailleraEndGame();
        NetPlayActive = 0;
    }
    return;
}

#pragma optimize("", on)
