#ifndef NEMU64_CPU_CONTROL_H
#define NEMU64_CPU_CONTROL_H

#include "CPU.h"

extern WORD CPU_HostFpuControl;
extern CpuU32 CPU_CycleShift;
extern CpuU32 CPU_DisableExceptions;
extern CpuU32 CPU_DisableVideoHLE;
extern CpuU32 CPU_DisableAudioHLE;
extern CpuU32 CPU_PeripheralRegisters[2];
extern WORD CPU_FpuControlDouble;
extern WORD CPU_FpuControlSingle;
extern WORD CPU_FpuControlExtended;
extern int CPU_Executing;
extern DWORD CPU_ThreadId;
extern char SelectedRomFilename[256];
extern BYTE FaultingInstructions[2097152];
extern int MemoryBreakHit;
extern CpuU32 ResetUnknownWord;
extern CpuS64 PerformanceTicksPerMillisecond;
extern CpuS64 LastPerformanceCounter;
extern CpuU32 FrameCounter;
extern CpuU32 AudioTimingFlag;
extern CpuU32 LastRspCpuCount;
extern int RspEnabled;
extern CpuU32 RspSavedPC;
extern CpuU32 ViMeasureCount;
extern CpuU32 ViMeasureTime;
extern CpuU32 RecompilerStepMode;

void __stdcall MultiplyUnsigned64(CpuU32 leftLow, CpuU32 leftHigh, CpuU32 rightLow, CpuU32 rightHigh);
void __stdcall MultiplySigned64(CpuS64 left, CpuS64 right);
void __stdcall DivideUnsigned64(CpuU64 left, CpuU64 right);
void __stdcall DivideSigned64(CpuS64 left, CpuS64 right);
void ResetRecompiler();

#endif
