#include "CPU.h"
#include "Memory.h"
#include "Dynarec.h"
#include "Plugins.h"
#include "RSP.h"
#include "WinMain.h"
#include "RomBrowser.h"
#include "State.h"
#include <stdio.h>
#include <mmsystem.h>
#include <math.h>
#include "CPU.h"
#include "Memory.h"
extern "C" __declspec(dllimport) int __stdcall kailleraModifyPlayValues(void *, int);

// 00409E00
inline void RaisePendingInterrupt(CpuU32 source, CpuU32 interrupt)
{
    if ((CPU_ActionFlags & source) && (CPU_COP0[CP0_Status] & 0x400) &&
        (CPU_COP0[CP0_Status] & 1) && !(CPU_COP0[CP0_Status] & 6)) {
        MIReg.Interrupt |= interrupt;
        CPU_ActionFlags &= ~source;
        CPU_COP0[CP0_Cause] = (CPU_COP0[CP0_Cause] & 0x7FFFFB83) | 0x400;
        CPU_COP0[CP0_Status] |= 2;
        CPU_COP0[CP0_EPC] = CPU_PC;
        CPU_PC = 0x80000180;
    }
}

// 0040C9A0. Instruction execution, peripheral timing and debugger requests.
void ExecuteInterpreter()
{
    DWORD lastTick = 0;
    CpuU32 measuredVIs = 0;
    CpuU32 lastRSPCount = 0;
    char status[100];
    for (;;) {
        CPU_Instruction = ReadPhysicalWord(((CPU_PC & 0xC0000000) == 0x80000000) ?
            CPU_PC & 0x1FFFFFFF : TranslateTLB(CPU_PC, 0));

        if (CPU_AudioEnabled && CPU_AudioCyclesRemaining < 0) {
            CPU_ActionFlags |= 0x10;
            AIReg.Status &= 0x3FFFFFFF;
            if (AIReg.NextLength != 0) {
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
            for (CpuU32 i = 0; i < ToolCount; ++i)
                if ((ToolCatalog[i].Flags & 0x10) && ToolCatalog[i].Invoke)
                    ToolCatalog[i].Invoke(0x10, 1);
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
            for (CpuU32 j = 0; j < ToolCount; ++j)
                if ((ToolCatalog[j].Flags & 0x10) && ToolCatalog[j].Invoke)
                    ToolCatalog[j].Invoke(0x10, 0);
            ++VICount;
            if (++measuredVIs >= 31) {
                DWORD now = timeGetTime();
                if (now != lastTick && lastTick != 0) {
                    sprintf(status, "VI/s: %3.2f", 31.0f / ((now - lastTick) * 0.001f));
                    if (!FullScreen)
                        SetStatusText(1, status);
                }
                lastTick = now;
                measuredVIs = 0;
            }
        }

        if (CPU_COP0[CP0_Count] < 20000) {
            if (!CPU_CompareReset) {
                CPU_ComparePending = 1;
                CPU_CompareReset = 1;
            }
        } else {
            CPU_CompareReset = 0;
        }
        if (CPU_ComparePending && CPU_COP0[CP0_Count] >= CPU_COP0[CP0_Compare]) {
            CPU_ActionFlags |= 0x40;
            CPU_ComparePending = 0;
        }
        if (CPU_ActionFlags) {
            CpuS32 oldPC = CPU_PC;
            RaisePendingInterrupt(1, 8);
            RaisePendingInterrupt(2, 2);
            RaisePendingInterrupt(4, 0x10);
            RaisePendingInterrupt(8, 1);
            RaisePendingInterrupt(0x10, 4);
            RaisePendingInterrupt(0x20, 0x20);
            if ((CPU_ActionFlags & 0x40) && (CPU_COP0[CP0_Status] & 0x8000) &&
                (CPU_COP0[CP0_Status] & 1) && !(CPU_COP0[CP0_Status] & 6)) {
                CPU_COP0[CP0_Cause] = (CPU_COP0[CP0_Cause] & 0x7FFF7F83) | 0x8000;
                CPU_COP0[CP0_Status] |= 2;
                CPU_COP0[CP0_EPC] = CPU_PC;
                CPU_PC = 0x80000180;
                CPU_ActionFlags &= ~0x40;
            }
            if (CPU_ActionFlags & 0x10000) {
                NotifyTools(2, 0);
                SuspendThread(CPU_Thread);
            }
            if ((CPU_ActionFlags & 0x08000000) && (CPU_COP0[CP0_Status] & 0x1000) &&
                (CPU_COP0[CP0_Status] & 1)) {
                CPU_COP0[CP0_Cause] = (CPU_COP0[CP0_Cause] & 0x7FFFEF83) | 0x1000;
                CPU_COP0[CP0_Status] |= 2;
                CPU_COP0[CP0_EPC] = CPU_PC;
                CPU_PC = 0x80000180;
                CPU_ActionFlags &= ~0x08000000;
            }
            if (CPU_ActionFlags & 0x02000000) {
                if (!(CPU_COP0[CP0_Status] & 2))
                    CPU_COP0[CP0_EPC] = CPU_PC;
                else
                    ReportCPUError("Doing cop1 unusable exception while exception is already being executed. Dunno if this will work");
                if (CPU_Cop1UnusableDelay)
                    CPU_COP0[CP0_Cause] |= 0x80000000;
                else
                    CPU_COP0[CP0_Cause] &= 0x7FFFFFFF;
                CPU_COP0[CP0_Cause] = (CPU_COP0[CP0_Cause] & 0xFFFFFF83) | 0x2C;
                CPU_COP0[CP0_Status] |= 2;
                CPU_COP0[CP0_Cause] = (CPU_COP0[CP0_Cause] & 0xCFFFFFFF) | 0x10000000;
                CPU_PC = 0x80000180;
                CPU_ActionFlags &= ~0x02000000;
            }
            if (CPU_ActionFlags & 0x80000) {
                ReportCPUError("COP1 Exception trap taken");
                if (!(CPU_COP0[CP0_Status] & 2))
                    CPU_COP0[CP0_EPC] = CPU_PC;
                else
                    ReportCPUError("Doing cop1 trap exception while exception is already being executed. Dunno if this will work");
                if (CPU_FpuExceptionDelay)
                    CPU_COP0[CP0_Cause] |= 0x80000000;
                else
                    CPU_COP0[CP0_Cause] &= 0x7FFFFFFF;
                CPU_COP0[CP0_Cause] = (CPU_COP0[CP0_Cause] & 0xFFFFFFBF) | 0x3C;
                CPU_COP0[CP0_Status] |= 2;
                CPU_PC = 0x80000180;
                CPU_ActionFlags &= ~0x80000;
            }
            if (CPU_ActionFlags & 0x40000) {
                if (!(CPU_COP0[CP0_Status] & 2))
                    CPU_COP0[CP0_EPC] = CPU_PC;
                else
                    ReportCPUError("Doing tlb exception while exception is already being executed. Dunno if this will work");
                if (CPU_TLBExceptionDelay) {
                    CPU_COP0[CP0_Cause] |= 0x80000000;
                    CPU_TLBExceptionDelay = 0;
                } else {
                    CPU_COP0[CP0_Cause] &= 0x7FFFFFFF;
                }
                CPU_COP0[CP0_Cause] &= 0xFFFFFF83;
                switch (TranslationError) {
                case 1:
                    if (TLBWriteAccess)
                        CPU_COP0[CP0_Cause] |= 0xC;
                    else
                        CPU_COP0[CP0_Cause] |= 8;
                    CPU_PC = ((CpuU8)CPU_COP0[CP0_Status] & 2) ? 0x80000180 : 0x80000000;
                    break;
                case 2: case 3:
                    if (TLBWriteAccess)
                        CPU_COP0[CP0_Cause] |= 0xC;
                    else
                        CPU_COP0[CP0_Cause] |= 8;
                    CPU_PC = 0x80000180;
                    break;
                case 4: case 5:
                    CPU_COP0[CP0_Cause] |= 4;
                    CPU_PC = 0x80000180;
                    break;
                }
                CPU_COP0[CP0_Context] = ((TLBAddress >> 9) & 0x7FFFF0) |
                    (CPU_COP0[CP0_Context] & 0xFF800000);
                CPU_COP0[CP0_BadVAddr] = TLBAddress;
                CPU_COP0[CP0_EntryHi] = (TLBAddress & 0xFFFFE000) |
                    (CPU_COP0[CP0_EntryHi] & 0xFF);
                CPU_COP0[CP0_Status] |= 2;
                CPU_ActionFlags &= ~0x40000;
            }
            if (CPU_ActionFlags & 0x01000000) {
                CPU_COP0[CP0_Cause] = (CPU_COP0[CP0_Cause] & 0x7FFFFF83) | 0x20;
                CPU_COP0[CP0_EPC] = CPU_ExceptionPC;
                CPU_COP0[CP0_Status] |= 2;
                CPU_PC = 0x80000180;
                CPU_ActionFlags &= ~0x01000000;
            }
            if (CPU_ActionFlags & 0x4000) {
                CpuU32 savedInstruction = CPU_Instruction;
                if (CPU_RSPCycles > 3000) {
                    CPU_RSPCycles = 0;
                    do {
                        for (CpuU32 i = 0; i < RSPBreakpointCount; ++i)
                            if (RSPBreakpoints[i] == (SPReg.PC & 0xFFF))
                                CPU_ActionFlags |= 0x20000;
                        if (CPU_ActionFlags & 0x20000) {
                            for (CpuU32 j = 0; j < ToolCount; ++j)
                                if ((ToolCatalog[j].Flags & 2) && ToolCatalog[j].Invoke)
                                    ToolCatalog[j].Invoke(2, 1);
                            SuspendThread(CPU_Thread);
                        }
                        CPU_Instruction = ReadPhysicalWord((SPReg.PC & 0xFFF) + 0x04001000);
                        RSPRegisters[0] = 0;
                        RSP_MainDispatch[CPU_Instruction >> 26]();
                        --CPU_RSPCycles;
                        if (RSPDelayPC) {
                            CpuU32 oldRSPPC = SPReg.PC;
                            bool breakpoint = false;
                            SPReg.PC = RSPDelayPC;
                            if (!(CPU_ActionFlags & 0x20000)) {
                                for (CpuU32 i = 0; i < RSPBreakpointCount; ++i) {
                                    if (RSPBreakpoints[i] == (SPReg.PC & 0xFFF)) {
                                        CPU_ActionFlags |= 0x20000;
                                        breakpoint = true;
                                        RSPDelayPC = 0;
                                        SPReg.PC -= 4;
                                    }
                                }
                            }
                            if (!breakpoint) {
                                CPU_Instruction = ReadPhysicalWord((SPReg.PC & 0xFFF) + 0x04001000);
                                RSP_MainDispatch[CPU_Instruction >> 26]();
                                SPReg.PC = oldRSPPC;
                                RSPDelayPC = 0;
                                --CPU_RSPCycles;
                            }
                        }
                    } while (CPU_RSPCycles > -3000 && (CPU_ActionFlags & 0x4000));
                } else {
                    if (lastRSPCount)
                        CPU_RSPCycles += CPU_COP0[CP0_Count] - lastRSPCount;
                    lastRSPCount = CPU_COP0[CP0_Count];
                }
                CPU_Instruction = savedInstruction;
            }
            if (CPU_ActionFlags & 0x200000)
                SuspendThread(CPU_Thread);
            if (CPU_ActionFlags & 0x1000) {
                CPU_ActionFlags &= ~0x1000;
                SaveState();
            }
            if (CPU_ActionFlags & 0x2000) {
                CPU_ActionFlags &= ~0x2000;
                LoadState(false);
            }
            if (CPU_ActionFlags & 0x04000000) {
                CPU_ActionFlags &= ~0x04000000;
                if (SelectedGraphicsPlugin->CloseDLL)
                    SelectedGraphicsPlugin->CloseDLL();
                if (SelectedGraphicsPlugin->RomClosed)
                    SelectedGraphicsPlugin->RomClosed();
                SelectedGraphicsPlugin = PendingGraphicsPlugin;
                PendingGraphicsPlugin = NULL;
                InitializeGraphicsPlugin();
                if (SelectedGraphicsPlugin->RomOpen)
                    SelectedGraphicsPlugin->RomOpen();
            }
            if (CPU_ActionFlags & 0x800) {
                ResetCPU();
                CPU_ActionFlags &= ~0x800;
            }
            if (oldPC != CPU_PC)
                goto instructionComplete;
        }
        {
            CPU_MainDispatch[CPU_Instruction >> 26]();
            ++CPU_COP0[CP0_Count];
            --CPU_VideoCyclesRemaining;
            --CPU_AudioCyclesRemaining;
            CPU_GPR[0] = 0;
            CPU_PC += 4;
        }
instructionComplete:
        if (CPU_ActionFlags & 0x400000) {
            CPU_ActionFlags &= ~0x400000;
            return;
        }
    }
}

// 0040D550. Called by generated code when an instruction is unsupported.
void __stdcall ReportInvalidOpcode(CpuU32 address, CpuU32 opcode)
{
    char message[256];
    sprintf(message, "Unhandled/Invalid opcode (0x%X), at address: 0x%X, continue?", opcode, address);
    ReportCPUError(message);
}
