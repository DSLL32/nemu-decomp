#ifndef NEMU64_CPU_H
#define NEMU64_CPU_H

#include <windows.h>

typedef unsigned char CpuU8;
typedef signed char CpuS8;
typedef unsigned short CpuU16;
typedef signed short CpuS16;
typedef unsigned int CpuU32;
typedef signed int CpuS32;
typedef unsigned __int64 CpuU64;
typedef signed __int64 CpuS64;

/* Register banks are separate objects in the executable. Their source names
   are inferred; the correspondence with the image is in nemu-cpu.json. */
union CPUFloatingRegisters {
    CpuU32 words[32];
    CpuS32 signedWords[32];
    float single[32];
    double dbl[16];
    CpuS64 longs[16];
    CpuU64 unsignedLongs[16];
};

enum CPUCop0Register {
    CP0_Index = 0, CP0_Random = 1, CP0_EntryLo0 = 2, CP0_EntryLo1 = 3,
    CP0_Context = 4, CP0_PageMask = 5, CP0_Wired = 6,
    CP0_BadVAddr = 8, CP0_Count = 9, CP0_EntryHi = 10,
    CP0_Compare = 11, CP0_Status = 12, CP0_Cause = 13, CP0_EPC = 14,
    CP0_PRId = 15, CP0_Config = 16, CP0_LLAddr = 17,
    CP0_WatchLo = 18, CP0_WatchHi = 19, CP0_XContext = 20,
    CP0_PErr = 26, CP0_CacheErr = 27, CP0_TagLo = 28,
    CP0_TagHi = 29, CP0_ErrorEPC = 30
};

extern CpuS64 CPU_GPR[32];
extern CpuU32 CPU_COP0[32];
extern CpuS32 CPU_PC;
extern CpuU32 CPU_Instruction;
extern CpuS64 CPU_HI;
extern CpuS64 CPU_LO;
extern CPUFloatingRegisters CPU_FPR;
extern CpuS64 CPU_FPR64[32];
extern CpuU32 CPU_FCR31;
extern CpuU32 CPU_ActionFlags;
extern CpuU32 CPU_Running;
extern HANDLE CPU_Thread;
extern CpuU32 CPU_LastPC;
extern CpuU32 CPU_Random;
extern CpuU32 CPU_ComparePending;
extern CpuU32 CPU_FRMode;
extern CpuU32 CPU_ExceptionPC;
extern int CPU_AudioEnabled;
extern int CPU_AudioCyclesRemaining;
extern int CPU_VideoCyclesRemaining;
extern int CPU_VideoPeriod;
extern int CPU_RSPCycles;
extern int CPU_Use32BitMemoryAccess;
extern CpuU32 CPU_InterpreterMode;
extern int CPU_TLBEnabled;
extern CpuU32 CPU_CompareReset;
extern CpuU32 CPU_Cop1UnusableDelay;
extern CpuU32 CPU_FpuExceptionDelay;
extern CpuU32 CPU_TLBExceptionDelay;
extern WORD CPU_HostFpuControl;
extern CpuU32 CPU_FpuModeChanges;
extern CpuU32 CPU_CycleShift;
extern CpuU32 CPU_DisableExceptions;
extern CpuU32 CPU_DisableVideoHLE;
extern CpuU32 CPU_DisableAudioHLE;
extern CpuU32 CPU_PeripheralRegisters[2];
#define CPU_ForceCFB (CPU_PeripheralRegisters[1])

typedef void (*CPUOpcodeHandler)();
extern CPUOpcodeHandler CPU_MainDispatch[64];
extern CPUOpcodeHandler CPU_SpecialDispatch[64];
extern CPUOpcodeHandler CPU_RegImmDispatch[32];
extern CPUOpcodeHandler CPU_Cop0Dispatch[32];
extern CPUOpcodeHandler CPU_Cop0FunctionDispatch[64];
extern CPUOpcodeHandler CPU_Cop1Dispatch[32];
extern CPUOpcodeHandler CPU_Cop1FunctionDispatch[64];

void ExecuteDelaySlot();
void Branch(bool link, CpuU32 condition, bool likely);
void ExecuteInterpreter();
void __stdcall ReportInvalidOpcode(CpuU32 address, CpuU32 opcode);
void RaisePendingInterrupt(CpuU32 source, CpuU32 interrupt);
void ReportCPUError(const char *message);
void ReportCPUErrorFormat(const char *format, ...);
void ReportMemoryError(const char *message, CpuU32 address);
void ReportInterpreterError(const char *message);
void ReportCompilerError(const char *message);
void UpdateAudioTiming();
void __stdcall SetFpuRounding(CpuU32 value);
void ResetCPU();
void InitializeCPU();
void RunCPU();
void StartCPUThread();
LONG CPUHostException(EXCEPTION_POINTERS *exception);
CpuU32 *ContextRegister(CONTEXT *context, CpuU8 index);

#endif
