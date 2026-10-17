// CPU interpreter reconstructed from the original VC7 executable.
// Inferred source names and image correspondence: analysis/nemu-cpu.json.
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

CpuS64 CPU_GPR[32];
CpuU32 CPU_COP0[32];
CpuS32 CPU_PC;
CpuU32 CPU_Instruction;
CpuS64 CPU_HI, CPU_LO;
CPUFloatingRegisters CPU_FPR;
CpuS64 CPU_FPR64[32];
CpuU32 CPU_FCR31, CPU_ActionFlags, CPU_Running;
HANDLE CPU_Thread;
CpuU32 CPU_LastPC, CPU_Random, CPU_ComparePending, CPU_FRMode, CPU_ExceptionPC;
int CPU_AudioEnabled, CPU_AudioCyclesRemaining, CPU_VideoCyclesRemaining;
int CPU_VideoPeriod, CPU_RSPCycles;
int CPU_Use32BitMemoryAccess, CPU_TLBEnabled;
CpuU32 CPU_InterpreterMode, CPU_CompareReset;
CpuU32 CPU_Cop1UnusableDelay, CPU_FpuExceptionDelay, CPU_TLBExceptionDelay;

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

// 00409E90
void ExecuteDelaySlot()
{
    CpuU32 savedInstruction = CPU_Instruction;
    CpuU32 address = CPU_PC + 4;
    if ((address & 0xC0000000) == 0x80000000)
        address &= 0x1FFFFFFF;
    else
        address = TranslateTLB(address, 0);
    CPU_Instruction = ReadPhysicalWord(address);
    CPU_MainDispatch[CPU_Instruction >> 26]();
    CPU_Instruction = savedInstruction;
    CPU_GPR[0] = 0;
}

// Original 00409EF0. Wrapper ABI verified with VC7 LTCG.
// The qualifier is inferred from separate stack reads at 00409F0B and
// 00409FCD; the original parameter spelling and width are not recovered.
void Branch(bool link, volatile CpuU32 condition, bool likely)
{
    if (link)
        CPU_GPR[31] = (CpuS64)CPU_PC + 8;
    CpuU8 taken;
    switch ((CpuU8)condition) {
    case 14: taken = CPU_GPR[(CPU_Instruction >> 21) & 31] <= 0; break;
    case 12: taken = CPU_GPR[(CPU_Instruction >> 21) & 31] < 0; break;
    case 13: taken = CPU_GPR[(CPU_Instruction >> 21) & 31] >= 0; break;
    case 15: taken = CPU_GPR[(CPU_Instruction >> 21) & 31] > 0; break;
    default: taken = (CpuU8)condition; break;
    }
    if (taken) {
        ExecuteDelaySlot();
        CPU_PC += (CpuS16)CPU_Instruction * 4;
    } else if (likely)
        CPU_PC += 4;
}


// Integer and COP0 interpreter handlers reconstructed from nemu64.exe.
// Names are inferred; function addresses in comments identify the evidence.
// No opcode behavior is corrected to conform to the MIPS specification.
#include "CPU.h"
#include "Memory.h"

#define RS ((CPU_Instruction >> 21) & 31)
#define RT ((CPU_Instruction >> 16) & 31)
#define RD ((CPU_Instruction >> 11) & 31)
#define SA ((CPU_Instruction >> 6) & 31)

// Recovered virtual-address translation macro. Its address argument is
// repeated before translation, matching the original expansion and compiler code.
#define CPU_PHYSICAL_ADDRESS(address) \
    ((((address) & 0xC0000000) == 0x80000000) ? \
        ((address) & 0x1FFFFFFF) : TranslateTLB((address), false))

// 00409E80
void Op_Unknown()
{
    ReportInterpreterError("Error: invalid opcode called in interpreter");
}

// 0040A010
void Op_SPECIAL()
{
    CPU_SpecialDispatch[(CpuU8)CPU_Instruction & 63]();
}

// 0040A030
void Op_REGIMM()
{
    CPU_RegImmDispatch[RT]();
}

// 0040A050
void Op_J()
{
    CpuU32 target = ((CPU_Instruction & 0x03FFFFFF) << 2) | ((CpuU32)CPU_PC & 0xF0000000);
    if (target == (CpuU32)CPU_PC)
        CPU_COP0[9] += 100;
    ExecuteDelaySlot();
    CPU_PC = target - 4;
}

// 0040A090
void Op_JAL()
{
    CpuU32 target = ((CPU_Instruction & 0x03FFFFFF) << 2) | ((CpuU32)CPU_PC & 0xF0000000);
    CPU_GPR[31] = (CpuS64)CPU_PC + 8;
    ExecuteDelaySlot();
    CPU_PC = target - 4;
}

// 0040A0E0
void Op_BEQ()
{
    if (CPU_Instruction == 0x1000FFFF)
    {
        ExecuteDelaySlot();
        CPU_COP0[9] += 100;
        CPU_PC -= 4;
        return;
    }
    if (RT == 0 && RS == 0)
    {
        ExecuteDelaySlot();
        CPU_PC += (CpuS16)CPU_Instruction * 4;
        return;
    }
    if (CPU_GPR[RS] == CPU_GPR[RT])
    {
        ExecuteDelaySlot();
        CPU_PC += (CpuS16)CPU_Instruction * 4;
    }
}

// 0040A180
void Op_BNE()
{
    if ((CPU_GPR[RS] == CPU_GPR[RT]) ^ 1)
    {
        ExecuteDelaySlot();
        CPU_PC += (CpuS16)CPU_Instruction * 4;
    }
}

// 0040A1E0
void Op_BLEZ()
{
    Branch(false, 14, false);
}

// 0040A1F0
void Op_BGTZ()
{
    Branch(false, 15, false);
}

// 0040A200; ADDI and ADDIU share this entry; no overflow trap.
void Op_ADDI()
{
    CPU_GPR[RT] = (CpuS32)((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction);
}

// 0040A240
void Op_SLTI()
{
    CPU_GPR[RT] = CPU_GPR[RS] < (CpuS16)CPU_Instruction;
}

// 0040A290
void Op_SLTIU()
{
    CPU_GPR[RT] = (CpuU64)CPU_GPR[RS] < (CpuU64)(CpuS64)(CpuS16)CPU_Instruction;
}

// 0040A2E0
void Op_ANDI()
{
    CPU_GPR[RT] = CPU_GPR[RS] & (CpuU16)CPU_Instruction;
}

// 0040A320
void Op_ORI()
{
    CPU_GPR[RT] = CPU_GPR[RS] | (CpuU16)CPU_Instruction;
}

// 0040A360
void Op_XORI()
{
    CPU_GPR[RT] = CPU_GPR[RS] ^ (CpuU16)CPU_Instruction;
}

// 0040A3A0
void Op_LUI()
{
    CPU_GPR[RT] = (CpuS32)((CpuU32)(CpuU16)CPU_Instruction << 16);
}

// 0040A3D0
void Op_COP0()
{
    CPU_Cop0Dispatch[RS]();
}

// 0040A3F0
void Op_COP1()
{
    CPU_Cop1Dispatch[RS]();
}

// 0040A410
void Op_BEQL()
{
    if (CPU_GPR[RS] == CPU_GPR[RT])
    {
        ExecuteDelaySlot();
        CPU_PC += (CpuS16)CPU_Instruction * 4;
    }
    else
        CPU_PC += 4;
}

// 0040A470
void Op_BNEL()
{
    if ((CPU_GPR[RS] == CPU_GPR[RT]) ^ 1)
    {
        ExecuteDelaySlot();
        CPU_PC += (CpuS16)CPU_Instruction * 4;
    }
    else
        CPU_PC += 4;
}

// 0040A4E0
void Op_BLEZL()
{
    Branch(false, 14, true);
}

// 0040A4F0
void Op_BGTZL()
{
    Branch(false, 15, true);
}

// 0040A500; DADDI and DADDIU share this entry; no overflow trap.
void Op_DADDI()
{
    CPU_GPR[RT] = (CpuU64)CPU_GPR[RS] + (CpuS16)CPU_Instruction;
}

// 0040A540
void Op_LDL()
{
    ReportInterpreterError("Unhandled opcode: LDL");
}

// 0040A550
void Op_LDR()
{
    ReportInterpreterError("Unhandled opcode: LDR");
}

// 0040A560
void Op_LB()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    CPU_GPR[RT] = (CpuS8)ReadPhysicalByte(address);
}

// 0040A5C0
void Op_LH()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    CPU_GPR[RT] = (CpuS16)ReadPhysicalHalf(address);
}

// 0040A620
void Op_LWL()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    CpuU32 word = ReadPhysicalWord(address & ~3U);
    CpuU32 shift = (address & 3) * 8;
    CPU_GPR[RT] = (CpuS32)(((CpuU32)CPU_GPR[RT] & (0x00FFFFFF >> (24 - shift))) | (word << shift));
}

// 0040A6B0
void Op_LW()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    CPU_GPR[RT] = (CpuS32)ReadPhysicalWord(address);
}

// 0040A710
void Op_LBU()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    CPU_GPR[RT] = (CpuU8)ReadPhysicalByte(address);
}

// 0040A770
void Op_LHU()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    CPU_GPR[RT] = (CpuU16)ReadPhysicalHalf(address);
}

// 0040A7D0
void Op_LWR()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    CpuU32 word = ReadPhysicalWord(address & ~3U);
    CpuU32 shift = (address & 3) * 8;
    CPU_GPR[RT] = (CpuS32)((word >> (24 - shift)) | ((CpuU32)CPU_GPR[RT] & (0xFFFFFF00U << shift)));
}

// 0040A860
void Op_LWU()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    CPU_GPR[RT] = (CpuU32)ReadPhysicalWord(address);
}

// 0040A8C0
void Op_SB()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    WritePhysicalByte(address, (CpuU8)CPU_GPR[RT]);
}

// 0040A920
void Op_SH()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    WritePhysicalHalf(address, (CpuU16)CPU_GPR[RT]);
}

// 0040A980
void Op_SWL()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    CpuU32 aligned = address & ~3U;
    CpuU32 oldWord = ReadPhysicalWord(aligned);
    CpuU32 shift = (address & 3) * 8;
    WritePhysicalWord(aligned, (oldWord & (0xFFFFFF00U << (24 - shift))) | ((CpuU32)CPU_GPR[RT] >> shift));
}

// 0040AA00
void Op_SW()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    WritePhysicalWord(address, (CpuU32)CPU_GPR[RT]);
}

// 0040AA60
void Op_SDL()
{
    ReportInterpreterError("Unhandled opcode: SDL");
}

// 0040AA70
void Op_SDR()
{
    ReportInterpreterError("Unhandled opcode: SDR");
}

// 0040AA80
void Op_SWR()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    CpuU32 aligned = address & ~3U;
    CpuU32 oldWord = ReadPhysicalWord(aligned);
    CpuU32 shift = (address & 3) * 8;
    WritePhysicalWord(aligned, ((CpuU32)CPU_GPR[RT] << (24 - shift)) | (oldWord & (0x00FFFFFF >> shift)));
}

// 0040AB10
void Op_LWC1()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    CPU_FPR.words[RT] = ReadPhysicalWord(address);
}

// 0040AB70; Original evaluates translation twice, then reads low word before high word.
void Op_LDC1()
{
    CpuU32 address = (CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction;
    CpuU32 lowAddress = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    CpuU32 highAddress = CPU_PHYSICAL_ADDRESS(address);
    CPU_FPR.longs[RT >> 1] = ((CpuU64)ReadPhysicalWord(highAddress) << 32) |
        ReadPhysicalWord(lowAddress + 4);
}

// 0040AC10; Original evaluates translation twice, then reads low word before high word.
void Op_LD()
{
    CpuU32 address = (CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction;
    CpuU32 lowAddress = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    CpuU32 highAddress = CPU_PHYSICAL_ADDRESS(address);
    CPU_GPR[RT] = ((CpuU64)ReadPhysicalWord(highAddress) << 32) |
        ReadPhysicalWord(lowAddress + 4);
}

// 0040ACA0
void Op_SWC1()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    WritePhysicalWord(address, CPU_FPR.words[RT]);
}

// 0040AD00
void Op_SDC1()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    WritePhysicalDoubleWord(address, CPU_FPR.words[RT & 0x1E], CPU_FPR.words[(RT & 0x1E) | 1]);
}

// 0040AD70
void Op_SD()
{
    CpuU32 address = CPU_PHYSICAL_ADDRESS(((CpuU32)CPU_GPR[RS] + (CpuS16)CPU_Instruction));
    WritePhysicalDoubleWord(address, (CpuU32)CPU_GPR[RT], (CpuU32)((CpuU64)CPU_GPR[RT] >> 32));
}

// 0040ADE0
void Op_SLL()
{
    CPU_GPR[RD] = (CpuS32)((CpuU32)CPU_GPR[RT] << SA);
}

// 0040AE20
void Op_SRL()
{
    CPU_GPR[RD] = (CpuS32)((CpuU32)CPU_GPR[RT] >> SA);
}

// 0040AE60
void Op_SRA()
{
    CPU_GPR[RD] = (CpuS32)((CpuS32)CPU_GPR[RT] >> SA);
}

// 0040AEA0
void Op_SLLV()
{
    CPU_GPR[RD] = (CpuS32)((CpuU32)CPU_GPR[RT] << (CpuU32)CPU_GPR[RS]);
}

// 0040AEE0
void Op_SRLV()
{
    CPU_GPR[RD] = (CpuS32)((CpuU32)CPU_GPR[RT] >> (CpuU32)CPU_GPR[RS]);
}

// 0040AF20
void Op_SRAV()
{
    CPU_GPR[RD] = (CpuS32)((CpuS32)CPU_GPR[RT] >> (CpuU32)CPU_GPR[RS]);
}

// 0040AF60; The register target is read after executing the delay slot.
void Op_JR()
{
    ExecuteDelaySlot();
    CPU_PC = (CpuU32)CPU_GPR[RS] - 4;
    *(CpuU32 *)(RDRAM + 0x318) = RDRAMSize;
}

// 0040AFA0; Link and target accesses both occur after executing the delay slot.
void Op_JALR()
{
    ExecuteDelaySlot();
    CPU_GPR[RD] = (CpuS64)CPU_PC + 8;
    CPU_PC = (CpuU32)CPU_GPR[RS] - 4;
}

// 0040AFF0
void Op_SYSCALL()
{
    CPU_ExceptionPC = CPU_PC;
    CPU_ActionFlags |= 0x01000000;
}

// 0040B010
void Op_BREAK()
{
    ReportInterpreterError("Break");
}

// 0040B020
void Op_MFHI()
{
    CPU_GPR[RD] = CPU_HI;
}

// 0040B050
void Op_MTHI()
{
    CPU_HI = CPU_GPR[RS];
}

// 0040B080
void Op_MFLO()
{
    CPU_GPR[RD] = CPU_LO;
}

// 0040B0B0
void Op_MTLO()
{
    CPU_LO = CPU_GPR[RS];
}

// 0040B0E0; Both DSLLV and DSRLV dispatch here. The original shifts left for both.
void Op_DSLLV()
{
    CPU_GPR[RD] = (CpuU64)CPU_GPR[RT] << (CpuU32)CPU_GPR[RS];
}

// 0040B130
void Op_DSRAV()
{
    CPU_GPR[RD] = CPU_GPR[RT] >> (CpuU32)CPU_GPR[RS];
}

// 0040B180
void Op_MULT()
{
    CpuS64 product = (CpuS64)(CpuS32)CPU_GPR[RT] * (CpuS32)CPU_GPR[RS];
    CPU_LO = (CpuS32)product;
    CPU_HI = (CpuS32)(product >> 32);
}

// 0040B1D0
void Op_MULTU()
{
    CpuU64 product = (CpuU64)(CpuU32)CPU_GPR[RT] * (CpuU32)CPU_GPR[RS];
    CPU_LO = (CpuS32)product;
    CPU_HI = (CpuS32)(product >> 32);
}

// 0040B220; No signed-overflow guard around the host division.
void Op_DIV()
{
    CpuS32 divisor = (CpuS32)CPU_GPR[RT];
    if (divisor != 0)
    {
        CpuS32 dividend = (CpuS32)CPU_GPR[RS];
        CPU_LO = dividend / divisor;
        CPU_HI = dividend % divisor;
    }
}

// 0040B270; Both results are zero-extended, unlike the signed DIV handler.
void Op_DIVU()
{
    CpuU32 divisor = (CpuU32)CPU_GPR[RT];
    if (divisor != 0)
    {
        CpuU32 dividend = (CpuU32)CPU_GPR[RS];
        CPU_LO = dividend / divisor;
        CPU_HI = dividend % divisor;
    }
}

// 0040B2C0; DMULT and DMULTU share this incomplete 64-bit product handler.
void Op_DMULT()
{
    CPU_LO = (CpuU64)CPU_GPR[RT] * (CpuU64)CPU_GPR[RS];
    CPU_HI = 0;
}

// 0040B320
void Op_DDIV()
{
    CpuS64 divisor = (CpuS64)CPU_GPR[RT];
    if (divisor != 0)
    {
        CpuS64 dividend = (CpuS64)CPU_GPR[RS];
        CPU_LO = dividend / divisor;
        CPU_HI = dividend % divisor;
    }
}

// 0040B380
void Op_DDIVU()
{
    CpuU64 divisor = (CpuU64)CPU_GPR[RT];
    if (divisor != 0)
    {
        CpuU64 dividend = (CpuU64)CPU_GPR[RS];
        CPU_LO = dividend / divisor;
        CPU_HI = dividend % divisor;
    }
}

// 0040B3E0; ADD and ADDU share this entry.
void Op_ADD()
{
    CPU_GPR[RD] = (CpuS32)((CpuU32)CPU_GPR[RT] + (CpuU32)CPU_GPR[RS]);
}

// 0040B420; SUB and SUBU share this entry.
void Op_SUB()
{
    CPU_GPR[RD] = (CpuS32)((CpuU32)CPU_GPR[RS] - (CpuU32)CPU_GPR[RT]);
}

// 0040B460
void Op_AND()
{
    CPU_GPR[RD] = CPU_GPR[RT] & CPU_GPR[RS];
}

// 0040B4C0
void Op_OR()
{
    CPU_GPR[RD] = CPU_GPR[RT] | CPU_GPR[RS];
}

// 0040B520
void Op_XOR()
{
    CPU_GPR[RD] = CPU_GPR[RT] ^ CPU_GPR[RS];
}

// 0040B580
void Op_NOR()
{
    CPU_GPR[RD] = ~(CPU_GPR[RT] | CPU_GPR[RS]);
}

// 0040B5E0
void Op_SLT()
{
    CPU_GPR[RD] = CPU_GPR[RS] < CPU_GPR[RT];
}

// 0040B640
void Op_SLTU()
{
    CPU_GPR[RD] = (CpuU64)CPU_GPR[RS] < (CpuU64)CPU_GPR[RT];
}

// 0040B6A0; DADD and DADDU share this entry.
void Op_DADD()
{
    CPU_GPR[RD] = (CpuU64)CPU_GPR[RT] + (CpuU64)CPU_GPR[RS];
}

// 0040B6F0; DSUB and DSUBU share this entry.
void Op_DSUB()
{
    CPU_GPR[RD] = (CpuU64)CPU_GPR[RS] - (CpuU64)CPU_GPR[RT];
}

// 0040B740
void Op_TGE()
{
    ReportInterpreterError("TGE");
}

// 0040B750
void Op_TGEU()
{
    ReportInterpreterError("TGEU");
}

// 0040B760
void Op_TLT()
{
    ReportInterpreterError("TLT");
}

// 0040B770
void Op_TLTU()
{
    ReportInterpreterError("TLTU");
}

// 0040B780
void Op_TEQ()
{
    ReportInterpreterError("TEQ");
}

// 0040B790
void Op_TNE()
{
    ReportInterpreterError("TNE");
}

// 0040B7A0
void Op_DSLL()
{
    CPU_GPR[RD] = (CpuU64)CPU_GPR[RT] << SA;
}

// 0040B7F0
void Op_DSRL()
{
    CPU_GPR[RD] = (CpuU64)CPU_GPR[RT] >> SA;
}

// 0040B840
void Op_DSRA()
{
    CPU_GPR[RD] = (CpuS64)CPU_GPR[RT] >> SA;
}

// 0040B890
void Op_DSLL32()
{
    CPU_GPR[RD] = (CpuU64)CPU_GPR[RT] << (SA + 32);
}

// 0040B8E0
void Op_DSRL32()
{
    CPU_GPR[RD] = (CpuU64)CPU_GPR[RT] >> (SA + 32);
}

// 0040B930
void Op_DSRA32()
{
    CPU_GPR[RD] = (CpuS64)CPU_GPR[RT] >> (SA + 32);
}

// 0040B980
void Op_BLTZ()
{
    Branch(false, 12, false);
}

// 0040B990
void Op_BGEZ()
{
    Branch(false, 13, false);
}

// 0040B9A0
void Op_BLTZL()
{
    Branch(false, 12, true);
}

// 0040B9B0
void Op_BGEZL()
{
    Branch(false, 13, true);
}

// 0040B9C0
void Op_TGEI()
{
    ReportInterpreterError("TGEI");
}

// 0040B9D0
void Op_TGEIU()
{
    ReportInterpreterError("TGEIU");
}

// 0040B9E0
void Op_TLTI()
{
    ReportInterpreterError("TLTI");
}

// 0040B9F0
void Op_TLTIU()
{
    ReportInterpreterError("TLTIU");
}

// 0040BA00
void Op_TEQI()
{
    ReportInterpreterError("TEQI");
}

// 0040BA10
void Op_TNEI()
{
    ReportInterpreterError("TNEI");
}

// 0040BA20
void Op_BLTZAL()
{
    Branch(true, 12, false);
}

// 0040BA30
void Op_BGEZAL()
{
    Branch(true, 13, false);
}

// 0040BA40
void Op_BLTZALL()
{
    Branch(true, 12, true);
}

// 0040BA50
void Op_BGEZALL()
{
    Branch(true, 13, true);
}

// 0040BA60
void Op_MFC0()
{
    switch (RD)
    {
    case 0: CPU_GPR[RT] = (CpuS32)CPU_COP0[0]; break;
    case 1: CPU_GPR[RT] = (CpuS32)CPU_Random; break;
    case 2: CPU_GPR[RT] = (CpuS32)CPU_COP0[2]; break;
    case 3: CPU_GPR[RT] = (CpuS32)CPU_COP0[3]; break;
    case 4: CPU_GPR[RT] = (CpuS32)CPU_COP0[4]; break;
    case 5: CPU_GPR[RT] = (CpuS32)CPU_COP0[5]; break;
    case 6: CPU_GPR[RT] = (CpuS32)CPU_COP0[6]; break;
    case 8: CPU_GPR[RT] = (CpuS32)CPU_COP0[8]; break;
    case 9: CPU_GPR[RT] = (CpuS32)CPU_COP0[9]; break;
    case 10: CPU_GPR[RT] = (CpuS32)CPU_COP0[10]; break;
    case 11: CPU_GPR[RT] = (CpuS32)CPU_COP0[11]; break;
    case 12: CPU_GPR[RT] = (CpuS32)CPU_COP0[12]; break;
    case 13: CPU_GPR[RT] = (CpuS32)CPU_COP0[13]; break;
    case 14: CPU_GPR[RT] = (CpuS32)CPU_COP0[14]; break;
    case 15: CPU_GPR[RT] = (CpuS32)CPU_COP0[15]; break;
    case 16: CPU_GPR[RT] = (CpuS32)CPU_COP0[16]; break;
    case 17: CPU_GPR[RT] = (CpuS32)CPU_COP0[17]; break;
    case 18: CPU_GPR[RT] = (CpuS32)CPU_COP0[18]; break;
    case 19: CPU_GPR[RT] = (CpuS32)CPU_COP0[19]; break;
    case 20: CPU_GPR[RT] = (CpuS32)CPU_COP0[20]; break;
    case 26: CPU_GPR[RT] = (CpuS32)CPU_COP0[26]; break;
    case 27: CPU_GPR[RT] = (CpuS32)CPU_COP0[27]; break;
    case 28: CPU_GPR[RT] = (CpuS32)CPU_COP0[28]; break;
    case 29: CPU_GPR[RT] = (CpuS32)CPU_COP0[29]; break;
    case 30: CPU_GPR[RT] = (CpuS32)CPU_COP0[30]; break;
    }
}

// 0040BBD0
void Op_MTC0()
{
    switch (RD)
    {
    case 0:
        CPU_COP0[0] = (CpuU32)CPU_GPR[RT];
        break;
    case 2:
        CPU_COP0[2] = (CpuU32)CPU_GPR[RT];
        break;
    case 3:
        CPU_COP0[3] = (CpuU32)CPU_GPR[RT];
        break;
    case 4:
        CPU_COP0[4] = (CpuU32)CPU_GPR[RT];
        break;
    case 5:
        CPU_COP0[5] = (CpuU32)CPU_GPR[RT];
        break;
    case 6:
        CPU_COP0[6] = (CpuU32)CPU_GPR[RT];
        if (CPU_Random < CPU_COP0[6])
            CPU_Random = CPU_COP0[6];
        break;
    case 8:
        CPU_COP0[8] = (CpuU32)CPU_GPR[RT];
        break;
    case 9:
        CPU_COP0[9] = (CpuU32)CPU_GPR[RT];
        CPU_ComparePending = CPU_COP0[9] < CPU_COP0[11];
        break;
    case 10:
        CPU_COP0[10] = (CpuU32)CPU_GPR[RT];
        break;
    case 11:
        CPU_COP0[11] = (CpuU32)CPU_GPR[RT];
        CPU_ComparePending = CPU_COP0[9] < CPU_COP0[11];
        break;
    case 12:
        CPU_COP0[12] = (CpuU32)CPU_GPR[RT];
        CPU_FRMode = (CPU_COP0[12] >> 26) & 1;
        break;
    case 13:
        CPU_COP0[13] = (CpuU32)CPU_GPR[RT];
        break;
    case 14:
        CPU_COP0[14] = (CpuU32)CPU_GPR[RT];
        break;
    case 15:
        CPU_COP0[15] = (CpuU32)CPU_GPR[RT];
        break;
    case 16:
        CPU_COP0[16] = (CpuU32)CPU_GPR[RT];
        break;
    case 17:
        CPU_COP0[17] = (CpuU32)CPU_GPR[RT];
        break;
    case 18:
        CPU_COP0[18] = (CpuU32)CPU_GPR[RT];
        break;
    case 19:
        CPU_COP0[19] = (CpuU32)CPU_GPR[RT];
        break;
    case 20:
        CPU_COP0[20] = (CpuU32)CPU_GPR[RT];
        break;
    case 26:
        CPU_COP0[26] = (CpuU32)CPU_GPR[RT];
        break;
    case 27:
        CPU_COP0[27] = (CpuU32)CPU_GPR[RT];
        break;
    case 28:
        CPU_COP0[28] = (CpuU32)CPU_GPR[RT];
        break;
    case 29:
        CPU_COP0[29] = (CpuU32)CPU_GPR[RT];
        break;
    case 30:
        CPU_COP0[30] = (CpuU32)CPU_GPR[RT];
        break;
    }
}

// 0040BEF0
void Op_COP0Function()
{
    CPU_Cop0FunctionDispatch[(CpuU8)CPU_Instruction & 63]();
}

// 0040BF10
void Op_TLBR()
{
    TLBRead();
}

// 0040BF20
void Op_TLBWI()
{
    TLBWrite(CPU_COP0[0]);
}

// 0040BF30
void Op_TLBWR()
{
    TLBWrite(CPU_Random);
    ++CPU_Random;
    if (CPU_Random >= 30)
        CPU_Random = CPU_COP0[6];
}

// 0040BF60
void Op_TLBP()
{
    TLBProbe();
}

// 0040BF70
void Op_ERET()
{
    if (CPU_COP0[12] & 4)
    {
        CPU_COP0[12] &= ~4U;
        CPU_PC = CPU_COP0[30] - 4;
    }
    else
    {
        CPU_COP0[12] &= ~2U;
        CPU_PC = CPU_COP0[14] - 4;
    }
}

#undef RS
#undef RT
#undef RD
#undef SA
#undef CPU_PHYSICAL_ADDRESS

// Reconstructed interpreter COP1 handlers. Names are inferred.
// Original code: 0040BFB0..0040C9A0. Relocation-masked comparison is tracked separately.
#include "CPU.h"
#include <math.h>

// The binary always pairs adjacent 32-bit FPRs for 64-bit values.
// Byte-sized decoder results preserve the original narrowing and paired shifts.
#define FPU_FMT ((CpuU8)((CPU_Instruction >> 21) & 31))
#define FPU_FT ((CpuU8)((CPU_Instruction >> 16) & 31))
#define FPU_FS ((CpuU8)((CPU_Instruction >> 11) & 31))
#define FPU_FD ((CpuU8)((CPU_Instruction >> 6) & 31))

// 0040BFB0
void Op_MFC1()
{
    CPU_GPR[FPU_FT] = (CpuS32)CPU_FPR.words[FPU_FS];
}

// 0040BFE0
void Op_DMFC1()
{
    CPU_GPR[FPU_FT] = CPU_FPR.longs[FPU_FS >> 1];
}

// 0040C020
void Op_CFC1()
{
    switch (FPU_FS) {
    case 0:
        CPU_GPR[FPU_FT] = 0xA00;
        break;
    case 31:
        CPU_GPR[FPU_FT] = (CpuS32)CPU_FCR31;
        break;
    }
}

// 0040C070
void Op_MTC1()
{
    CPU_FPR.words[FPU_FS] = (CpuU32)CPU_GPR[FPU_FT];
}

// 0040C0A0
void Op_DMTC1()
{
    CPU_FPR.longs[FPU_FS >> 1] = CPU_GPR[FPU_FT];
}

// 0040C0E0
void Op_CTC1()
{
    if (FPU_FS == 31)
        CPU_FCR31 = (CpuU32)CPU_GPR[FPU_FT];
}

// The branch decoder uses a packed I-format view with a 16-bit immediate
// followed by unsigned-int bitfields. This accounts for the original masked
// DWORD access at instruction + 2; other FPU fields use byte-sized decoders.
#pragma pack(push, 1)
struct CPUImmediateInstructionFields {
    CpuU16 immediate;
    CpuU32 rt : 5;
    CpuU32 rs : 5;
    CpuU32 op : 6;
};
#pragma pack(pop)

// 0040C110
void Op_BC1()
{
    CpuU32 condition = ((CPUImmediateInstructionFields *)&CPU_Instruction)->rt;
    bool likely = condition == 2 || condition == 3;
    bool branchFalse = condition == 0 || condition == 2;
    if (((CPU_FCR31 >> 23) & 1) ^ branchFalse) {
        ExecuteDelaySlot();
        CPU_PC += (short)CPU_Instruction * 4;
    } else if (likely) {
        CPU_PC += 4;
    }
}

// 0040C180
void Op_COP1Function()
{
    CPU_Cop1FunctionDispatch[(CpuU8)CPU_Instruction & 63]();
}

// 0040C1A0
void Op_FADD()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = CPU_FPR.single[FPU_FS] + CPU_FPR.single[FPU_FT]; break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = CPU_FPR.dbl[FPU_FS >> 1] + CPU_FPR.dbl[FPU_FT >> 1]; break;
    }
}

// 0040C220
void Op_FSUB()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = CPU_FPR.single[FPU_FS] - CPU_FPR.single[FPU_FT]; break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = CPU_FPR.dbl[FPU_FS >> 1] - CPU_FPR.dbl[FPU_FT >> 1]; break;
    }
}

// 0040C2A0
void Op_FMUL()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = CPU_FPR.single[FPU_FS] * CPU_FPR.single[FPU_FT]; break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = CPU_FPR.dbl[FPU_FS >> 1] * CPU_FPR.dbl[FPU_FT >> 1]; break;
    }
}

// 0040C320
void Op_FDIV()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = CPU_FPR.single[FPU_FS] / CPU_FPR.single[FPU_FT]; break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = CPU_FPR.dbl[FPU_FS >> 1] / CPU_FPR.dbl[FPU_FT >> 1]; break;
    }
}

// 0040C3A0
void Op_FSQRT()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = (float)sqrt(CPU_FPR.single[FPU_FS]); break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = sqrt(CPU_FPR.dbl[FPU_FS >> 1]); break;
    }
}

// 0040C400
void Op_FABS()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = (float)fabs(CPU_FPR.single[FPU_FS]); break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = fabs(CPU_FPR.dbl[FPU_FS >> 1]); break;
    }
}

// 0040C460. Integer aliases preserve the original bitwise MOV operations.
void Op_FMOV()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.words[FPU_FD] = CPU_FPR.words[FPU_FS]; break;
    case 17: CPU_FPR.longs[FPU_FD >> 1] = CPU_FPR.longs[FPU_FS >> 1]; break;
    }
}

// 0040C4D0
void Op_FNEG()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.single[FPU_FD] = -CPU_FPR.single[FPU_FS]; break;
    case 17: CPU_FPR.dbl[FPU_FD >> 1] = -CPU_FPR.dbl[FPU_FS >> 1]; break;
    }
}

// 0040C530. ROUND.L, TRUNC.L, CEIL.L and FLOOR.L all share this handler.
// The input uses the ordinary compiler float-to-integer conversion for all four.
void Op_ROUND_L()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.longs[FPU_FD >> 1] = (CpuS64)CPU_FPR.single[FPU_FS]; break;
    case 17: CPU_FPR.longs[FPU_FD >> 1] = (CpuS64)CPU_FPR.dbl[FPU_FS >> 1]; break;
    }
}

// 0040C590. ROUND.W, TRUNC.W, CEIL.W and FLOOR.W likewise share one handler.
void Op_ROUND_W()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.words[FPU_FD] = (CpuS32)CPU_FPR.single[FPU_FS]; break;
    case 17: CPU_FPR.words[FPU_FD] = (CpuS32)CPU_FPR.dbl[FPU_FS >> 1]; break;
    }
}

// 0040C5F0
void Op_CVT_S()
{
    switch (FPU_FMT) {
    case 17: CPU_FPR.single[FPU_FD] = (float)CPU_FPR.dbl[FPU_FS >> 1]; break;
    case 20: CPU_FPR.single[FPU_FD] = (float)(CpuS32)CPU_FPR.words[FPU_FS]; break;
    case 21: CPU_FPR.single[FPU_FD] = (float)CPU_FPR.longs[FPU_FS >> 1]; break;
    }
}

// 0040C670
void Op_CVT_D()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.dbl[FPU_FD >> 1] = CPU_FPR.single[FPU_FS]; break;
    case 20: CPU_FPR.dbl[FPU_FD >> 1] = (CpuS32)CPU_FPR.words[FPU_FS]; break;
    // Original CVT.D.L narrows the source to signed 32 bits before conversion.
    case 21: CPU_FPR.dbl[FPU_FD >> 1] = (CpuS32)CPU_FPR.longs[FPU_FS >> 1]; break;
    }
}

// 0040C6F0
void Op_CVT_W()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.words[FPU_FD] = (CpuS32)CPU_FPR.single[FPU_FS]; break;
    case 17: CPU_FPR.words[FPU_FD] = (CpuS32)CPU_FPR.dbl[FPU_FS >> 1]; break;
    case 21: CPU_FPR.words[FPU_FD] = (CpuU32)CPU_FPR.longs[FPU_FS >> 1]; break;
    }
}

// 0040C780
void Op_CVT_L()
{
    switch (FPU_FMT) {
    case 16: CPU_FPR.longs[FPU_FD >> 1] = (CpuS64)CPU_FPR.single[FPU_FS]; break;
    case 17: CPU_FPR.longs[FPU_FD >> 1] = (CpuS64)CPU_FPR.dbl[FPU_FS >> 1]; break;
    // Original CVT.L.W zero-extends its 32-bit input.
    case 20: CPU_FPR.longs[FPU_FD >> 1] = CPU_FPR.words[FPU_FS]; break;
    }
}

// 0040C810. Also shared by C.UEQ, C.SEQ and C.NGL.
void Op_C_EQ()
{
    switch (FPU_FMT) {
    case 16:
        if (CPU_FPR.single[FPU_FS] == CPU_FPR.single[FPU_FT]) CPU_FCR31 |= 0x800000;
        else CPU_FCR31 &= ~0x800000;
        break;
    case 17:
        if (CPU_FPR.dbl[FPU_FS >> 1] == CPU_FPR.dbl[FPU_FT >> 1]) CPU_FCR31 |= 0x800000;
        else CPU_FCR31 &= ~0x800000;
        break;
    }
}

// 0040C890. Also shared by C.ULT, C.LT and C.NGE.
void Op_C_LT()
{
    switch (FPU_FMT) {
    case 16:
        if (CPU_FPR.single[FPU_FS] < CPU_FPR.single[FPU_FT]) CPU_FCR31 |= 0x800000;
        else CPU_FCR31 &= ~0x800000;
        break;
    case 17:
        if (CPU_FPR.dbl[FPU_FS >> 1] < CPU_FPR.dbl[FPU_FT >> 1]) CPU_FCR31 |= 0x800000;
        else CPU_FCR31 &= ~0x800000;
        break;
    }
}

// 0040C910. Also shared by C.UN, C.SF and C.NGLE, for every format.
void Op_C_F()
{
    CPU_FCR31 &= ~0x800000;
}

// 0040C920. Also shared by C.ULE, C.LE and C.NGT.
void Op_C_LE()
{
    switch (FPU_FMT) {
    case 16:
        if (CPU_FPR.single[FPU_FS] <= CPU_FPR.single[FPU_FT]) CPU_FCR31 |= 0x800000;
        else CPU_FCR31 &= ~0x800000;
        break;
    case 17:
        if (CPU_FPR.dbl[FPU_FS >> 1] <= CPU_FPR.dbl[FPU_FT >> 1]) CPU_FCR31 |= 0x800000;
        else CPU_FCR31 &= ~0x800000;
        break;
    }
}

#undef FPU_FMT
#undef FPU_FT
#undef FPU_FS
#undef FPU_FD


// Dispatch table reconstruction from original pointer data.
// Duplicate opcodes deliberately retain their original shared handler.

void Op_ADD();
void Op_ADDI();
void Op_AND();
void Op_ANDI();
void Op_BC1();
void Op_BEQ();
void Op_BEQL();
void Op_BGEZ();
void Op_BGEZAL();
void Op_BGEZALL();
void Op_BGEZL();
void Op_BGTZ();
void Op_BGTZL();
void Op_BLEZ();
void Op_BLEZL();
void Op_BLTZ();
void Op_BLTZAL();
void Op_BLTZALL();
void Op_BLTZL();
void Op_BNE();
void Op_BNEL();
void Op_BREAK();
void Op_CFC1();
void Op_COP0();
void Op_COP0Function();
void Op_COP1();
void Op_COP1Function();
void Op_CTC1();
void Op_CVT_D();
void Op_CVT_L();
void Op_CVT_S();
void Op_CVT_W();
void Op_C_EQ();
void Op_C_F();
void Op_C_LE();
void Op_C_LT();
void Op_DADD();
void Op_DADDI();
void Op_DDIV();
void Op_DDIVU();
void Op_DIV();
void Op_DIVU();
void Op_DMFC1();
void Op_DMTC1();
void Op_DMULT();
void Op_DSLL();
void Op_DSLL32();
void Op_DSLLV();
void Op_DSRA();
void Op_DSRA32();
void Op_DSRAV();
void Op_DSRL();
void Op_DSRL32();
void Op_DSUB();
void Op_ERET();
void Op_FABS();
void Op_FADD();
void Op_FDIV();
void Op_FMOV();
void Op_FMUL();
void Op_FNEG();
void Op_FSQRT();
void Op_FSUB();
void Op_J();
void Op_JAL();
void Op_JALR();
void Op_JR();
void Op_LB();
void Op_LBU();
void Op_LD();
void Op_LDC1();
void Op_LDL();
void Op_LDR();
void Op_LH();
void Op_LHU();
void Op_LUI();
void Op_LW();
void Op_LWC1();
void Op_LWL();
void Op_LWR();
void Op_LWU();
void Op_MFC0();
void Op_MFC1();
void Op_MFHI();
void Op_MFLO();
void Op_MTC0();
void Op_MTC1();
void Op_MTHI();
void Op_MTLO();
void Op_MULT();
void Op_MULTU();
void Op_NOR();
void DynarecNoOperation();
void Op_OR();
void Op_ORI();
void Op_REGIMM();
void Op_ROUND_L();
void Op_ROUND_W();
void Op_SB();
void Op_SD();
void Op_SDC1();
void Op_SDL();
void Op_SDR();
void Op_SH();
void Op_SLL();
void Op_SLLV();
void Op_SLT();
void Op_SLTI();
void Op_SLTIU();
void Op_SLTU();
void Op_SPECIAL();
void Op_SRA();
void Op_SRAV();
void Op_SRL();
void Op_SRLV();
void Op_SUB();
void Op_SW();
void Op_SWC1();
void Op_SWL();
void Op_SWR();
void Op_SYSCALL();
void Op_TEQ();
void Op_TEQI();
void Op_TGE();
void Op_TGEI();
void Op_TGEIU();
void Op_TGEU();
void Op_TLBP();
void Op_TLBR();
void Op_TLBWI();
void Op_TLBWR();
void Op_TLT();
void Op_TLTI();
void Op_TLTIU();
void Op_TLTU();
void Op_TNE();
void Op_TNEI();
void Op_Unknown();
void Op_XOR();
void Op_XORI();

// Original VA 00477DA0.
CPUOpcodeHandler CPU_MainDispatch[64] = {
    Op_SPECIAL,            // 00 SPECIAL   0040A010
    Op_REGIMM,             // 01 REGIMM    0040A030
    Op_J,                  // 02 J         0040A050
    Op_JAL,                // 03 JAL       0040A090
    Op_BEQ,                // 04 BEQ       0040A0E0
    Op_BNE,                // 05 BNE       0040A180
    Op_BLEZ,               // 06 BLEZ      0040A1E0
    Op_BGTZ,               // 07 BGTZ      0040A1F0
    Op_ADDI,               // 08 ADDI      0040A200
    Op_ADDI,               // 09 ADDIU     0040A200
    Op_SLTI,               // 0A SLTI      0040A240
    Op_SLTIU,              // 0B SLTIU     0040A290
    Op_ANDI,               // 0C ANDI      0040A2E0
    Op_ORI,                // 0D ORI       0040A320
    Op_XORI,               // 0E XORI      0040A360
    Op_LUI,                // 0F LUI       0040A3A0
    Op_COP0,               // 10 COP0      0040A3D0
    Op_COP1,               // 11 COP1      0040A3F0
    Op_Unknown,            // 12 unused    00409E80
    Op_Unknown,            // 13 unused    00409E80
    Op_BEQL,               // 14 BEQL      0040A410
    Op_BNEL,               // 15 BNEL      0040A470
    Op_BLEZL,              // 16 BLEZL     0040A4E0
    Op_BGTZL,              // 17 BGTZL     0040A4F0
    Op_DADDI,              // 18 DADDI     0040A500
    Op_DADDI,              // 19 DADDIU    0040A500
    Op_LDL,                // 1A LDL       0040A540
    Op_LDR,                // 1B LDR       0040A550
    Op_Unknown,            // 1C unused    00409E80
    Op_Unknown,            // 1D unused    00409E80
    Op_Unknown,            // 1E unused    00409E80
    Op_Unknown,            // 1F unused    00409E80
    Op_LB,                 // 20 LB        0040A560
    Op_LH,                 // 21 LH        0040A5C0
    Op_LWL,                // 22 LWL       0040A620
    Op_LW,                 // 23 LW        0040A6B0
    Op_LBU,                // 24 LBU       0040A710
    Op_LHU,                // 25 LHU       0040A770
    Op_LWR,                // 26 LWR       0040A7D0
    Op_LWU,                // 27 LWU       0040A860
    Op_SB,                 // 28 SB        0040A8C0
    Op_SH,                 // 29 SH        0040A920
    Op_SWL,                // 2A SWL       0040A980
    Op_SW,                 // 2B SW        0040AA00
    Op_SDL,                // 2C SDL       0040AA60
    Op_SDR,                // 2D SDR       0040AA70
    Op_SWR,                // 2E SWR       0040AA80
    DynarecNoOperation,        // 2F CACHE     0041FEF0
    DynarecNoOperation,        // 30 LL        0041FEF0
    Op_LWC1,               // 31 LWC1      0040AB10
    Op_Unknown,            // 32 LWC2      00409E80
    Op_Unknown,            // 33 unused    00409E80
    DynarecNoOperation,        // 34 LLD       0041FEF0
    Op_LDC1,               // 35 LDC1      0040AB70
    Op_Unknown,            // 36 LDC2      00409E80
    Op_LD,                 // 37 LD        0040AC10
    DynarecNoOperation,        // 38 SC        0041FEF0
    Op_SWC1,               // 39 SWC1      0040ACA0
    Op_Unknown,            // 3A SWC2      00409E80
    Op_Unknown,            // 3B unused    00409E80
    DynarecNoOperation,        // 3C SCD       0041FEF0
    Op_SDC1,               // 3D SDC1      0040AD00
    Op_Unknown,            // 3E SDC2      00409E80
    Op_SD,                 // 3F SD        0040AD70
};

// Original VA 00477EA0.
CPUOpcodeHandler CPU_SpecialDispatch[64] = {
    Op_SLL,                // 00 SLL       0040ADE0
    Op_Unknown,            // 01 unused    00409E80
    Op_SRL,                // 02 SRL       0040AE20
    Op_SRA,                // 03 SRA       0040AE60
    Op_SLLV,               // 04 SLLV      0040AEA0
    Op_Unknown,            // 05 unused    00409E80
    Op_SRLV,               // 06 SRLV      0040AEE0
    Op_SRAV,               // 07 SRAV      0040AF20
    Op_JR,                 // 08 JR        0040AF60
    Op_JALR,               // 09 JALR      0040AFA0
    Op_Unknown,            // 0A unused    00409E80
    Op_Unknown,            // 0B unused    00409E80
    Op_SYSCALL,            // 0C SYSCALL   0040AFF0
    Op_BREAK,              // 0D BREAK     0040B010
    Op_Unknown,            // 0E unused    00409E80
    DynarecNoOperation,        // 0F SYNC      0041FEF0
    Op_MFHI,               // 10 MFHI      0040B020
    Op_MTHI,               // 11 MTHI      0040B050
    Op_MFLO,               // 12 MFLO      0040B080
    Op_MTLO,               // 13 MTLO      0040B0B0
    Op_DSLLV,              // 14 DSLLV     0040B0E0
    Op_Unknown,            // 15 unused    00409E80
    Op_DSLLV,              // 16 DSRLV     0040B0E0
    Op_DSRAV,              // 17 DSRAV     0040B130
    Op_MULT,               // 18 MULT      0040B180
    Op_MULTU,              // 19 MULTU     0040B1D0
    Op_DIV,                // 1A DIV       0040B220
    Op_DIVU,               // 1B DIVU      0040B270
    Op_DMULT,              // 1C DMULT     0040B2C0
    Op_DMULT,              // 1D DMULTU    0040B2C0
    Op_DDIV,               // 1E DDIV      0040B320
    Op_DDIVU,              // 1F DDIVU     0040B380
    Op_ADD,                // 20 ADD       0040B3E0
    Op_ADD,                // 21 ADDU      0040B3E0
    Op_SUB,                // 22 SUB       0040B420
    Op_SUB,                // 23 SUBU      0040B420
    Op_AND,                // 24 AND       0040B460
    Op_OR,                 // 25 OR        0040B4C0
    Op_XOR,                // 26 XOR       0040B520
    Op_NOR,                // 27 NOR       0040B580
    Op_Unknown,            // 28 unused    00409E80
    Op_Unknown,            // 29 unused    00409E80
    Op_SLT,                // 2A SLT       0040B5E0
    Op_SLTU,               // 2B SLTU      0040B640
    Op_DADD,               // 2C DADD      0040B6A0
    Op_DADD,               // 2D DADDU     0040B6A0
    Op_DSUB,               // 2E DSUB      0040B6F0
    Op_DSUB,               // 2F DSUBU     0040B6F0
    Op_TGE,                // 30 TGE       0040B740
    Op_TGEU,               // 31 TGEU      0040B750
    Op_TLT,                // 32 TLT       0040B760
    Op_TLTU,               // 33 TLTU      0040B770
    Op_TEQ,                // 34 TEQ       0040B780
    Op_Unknown,            // 35 unused    00409E80
    Op_TNE,                // 36 TNE       0040B790
    Op_Unknown,            // 37 unused    00409E80
    Op_DSLL,               // 38 DSLL      0040B7A0
    Op_Unknown,            // 39 unused    00409E80
    Op_DSRL,               // 3A DSRL      0040B7F0
    Op_DSRA,               // 3B DSRA      0040B840
    Op_DSLL32,             // 3C DSLL32    0040B890
    Op_Unknown,            // 3D unused    00409E80
    Op_DSRL32,             // 3E DSRL32    0040B8E0
    Op_DSRA32,             // 3F DSRA32    0040B930
};

// Original VA 00477FA0.
CPUOpcodeHandler CPU_RegImmDispatch[32] = {
    Op_BLTZ,               // 00 BLTZ      0040B980
    Op_BGEZ,               // 01 BGEZ      0040B990
    Op_BLTZL,              // 02 BLTZL     0040B9A0
    Op_BGEZL,              // 03 BGEZL     0040B9B0
    Op_Unknown,            // 04 unused    00409E80
    Op_Unknown,            // 05 unused    00409E80
    Op_Unknown,            // 06 unused    00409E80
    Op_Unknown,            // 07 unused    00409E80
    Op_TGEI,               // 08 TGEI      0040B9C0
    Op_TGEIU,              // 09 TGEIU     0040B9D0
    Op_TLTI,               // 0A TLTI      0040B9E0
    Op_TLTIU,              // 0B TLTIU     0040B9F0
    Op_TEQI,               // 0C TEQI      0040BA00
    Op_Unknown,            // 0D unused    00409E80
    Op_TNEI,               // 0E TNEI      0040BA10
    Op_Unknown,            // 0F unused    00409E80
    Op_BLTZAL,             // 10 BLTZAL    0040BA20
    Op_BGEZAL,             // 11 BGEZAL    0040BA30
    Op_BLTZALL,            // 12 BLTZALL   0040BA40
    Op_BGEZALL,            // 13 BGEZALL   0040BA50
    Op_Unknown,            // 14 reserved  00409E80
    Op_Unknown,            // 15 reserved  00409E80
    Op_Unknown,            // 16 reserved  00409E80
    Op_Unknown,            // 17 reserved  00409E80
    Op_Unknown,            // 18 reserved  00409E80
    Op_Unknown,            // 19 reserved  00409E80
    Op_Unknown,            // 1A reserved  00409E80
    Op_Unknown,            // 1B reserved  00409E80
    Op_Unknown,            // 1C reserved  00409E80
    Op_Unknown,            // 1D reserved  00409E80
    Op_Unknown,            // 1E reserved  00409E80
    Op_Unknown,            // 1F reserved  00409E80
};

// Original VA 00478020.
CPUOpcodeHandler CPU_Cop0Dispatch[32] = {
    Op_MFC0,               // 00 MFC0      0040BA60
    Op_Unknown,            // 01 unused    00409E80
    Op_Unknown,            // 02 unused    00409E80
    Op_Unknown,            // 03 unused    00409E80
    Op_MTC0,               // 04 MTC0      0040BBD0
    Op_Unknown,            // 05 unused    00409E80
    Op_Unknown,            // 06 unused    00409E80
    Op_Unknown,            // 07 unused    00409E80
    Op_Unknown,            // 08 unused    00409E80
    Op_Unknown,            // 09 unused    00409E80
    Op_Unknown,            // 0A unused    00409E80
    Op_Unknown,            // 0B unused    00409E80
    Op_Unknown,            // 0C unused    00409E80
    Op_Unknown,            // 0D unused    00409E80
    Op_Unknown,            // 0E unused    00409E80
    Op_Unknown,            // 0F unused    00409E80
    Op_COP0Function,       // 10 TLB       0040BEF0
    Op_Unknown,            // 11 unused    00409E80
    Op_Unknown,            // 12 unused    00409E80
    Op_Unknown,            // 13 unused    00409E80
    Op_Unknown,            // 14 unused    00409E80
    Op_Unknown,            // 15 unused    00409E80
    Op_Unknown,            // 16 unused    00409E80
    Op_Unknown,            // 17 unused    00409E80
    Op_Unknown,            // 18 unused    00409E80
    Op_Unknown,            // 19 unused    00409E80
    Op_Unknown,            // 1A unused    00409E80
    Op_Unknown,            // 1B unused    00409E80
    Op_Unknown,            // 1C unused    00409E80
    Op_Unknown,            // 1D unused    00409E80
    Op_Unknown,            // 1E unused    00409E80
    Op_Unknown,            // 1F unused    00409E80
};

// Original VA 004780A0.
CPUOpcodeHandler CPU_Cop0FunctionDispatch[64] = {
    Op_Unknown,            // 00 unused    00409E80
    Op_TLBR,               // 01 TLBR      0040BF10
    Op_TLBWI,              // 02 TLBWI     0040BF20
    Op_Unknown,            // 03 unused    00409E80
    Op_Unknown,            // 04 unused    00409E80
    Op_Unknown,            // 05 unused    00409E80
    Op_TLBWR,              // 06 TLBWR     0040BF30
    Op_Unknown,            // 07 unused    00409E80
    Op_TLBP,               // 08 TLBP      0040BF60
    Op_Unknown,            // 09 unused    00409E80
    Op_Unknown,            // 0A unused    00409E80
    Op_Unknown,            // 0B unused    00409E80
    Op_Unknown,            // 0C unused    00409E80
    Op_Unknown,            // 0D unused    00409E80
    Op_Unknown,            // 0E unused    00409E80
    Op_Unknown,            // 0F unused    00409E80
    Op_Unknown,            // 10 unused    00409E80
    Op_Unknown,            // 11 unused    00409E80
    Op_Unknown,            // 12 unused    00409E80
    Op_Unknown,            // 13 unused    00409E80
    Op_Unknown,            // 14 unused    00409E80
    Op_Unknown,            // 15 unused    00409E80
    Op_Unknown,            // 16 unused    00409E80
    Op_Unknown,            // 17 unused    00409E80
    Op_ERET,               // 18 ERET      0040BF70
    Op_Unknown,            // 19 unused    00409E80
    Op_Unknown,            // 1A unused    00409E80
    Op_Unknown,            // 1B unused    00409E80
    Op_Unknown,            // 1C unused    00409E80
    Op_Unknown,            // 1D unused    00409E80
    Op_Unknown,            // 1E unused    00409E80
    Op_Unknown,            // 1F unused    00409E80
    Op_Unknown,            // 20 unused    00409E80
    Op_Unknown,            // 21 unused    00409E80
    Op_Unknown,            // 22 unused    00409E80
    Op_Unknown,            // 23 unused    00409E80
    Op_Unknown,            // 24 unused    00409E80
    Op_Unknown,            // 25 unused    00409E80
    Op_Unknown,            // 26 unused    00409E80
    Op_Unknown,            // 27 unused    00409E80
    Op_Unknown,            // 28 unused    00409E80
    Op_Unknown,            // 29 unused    00409E80
    Op_Unknown,            // 2A unused    00409E80
    Op_Unknown,            // 2B unused    00409E80
    Op_Unknown,            // 2C unused    00409E80
    Op_Unknown,            // 2D unused    00409E80
    Op_Unknown,            // 2E unused    00409E80
    Op_Unknown,            // 2F unused    00409E80
    Op_Unknown,            // 30 unused    00409E80
    Op_Unknown,            // 31 unused    00409E80
    Op_Unknown,            // 32 unused    00409E80
    Op_Unknown,            // 33 unused    00409E80
    Op_Unknown,            // 34 unused    00409E80
    Op_Unknown,            // 35 unused    00409E80
    Op_Unknown,            // 36 unused    00409E80
    Op_Unknown,            // 37 unused    00409E80
    Op_Unknown,            // 38 unused    00409E80
    Op_Unknown,            // 39 unused    00409E80
    Op_Unknown,            // 3A unused    00409E80
    Op_Unknown,            // 3B unused    00409E80
    Op_Unknown,            // 3C unused    00409E80
    Op_Unknown,            // 3D unused    00409E80
    Op_Unknown,            // 3E unused    00409E80
    Op_Unknown,            // 3F unused    00409E80
};

// Original VA 004781A0.
CPUOpcodeHandler CPU_Cop1Dispatch[32] = {
    Op_MFC1,               // 00 MFC1      0040BFB0
    Op_DMFC1,              // 01 DMFC1     0040BFE0
    Op_CFC1,               // 02 CFC1      0040C020
    Op_Unknown,            // 03 unused    00409E80
    Op_MTC1,               // 04 MTC1      0040C070
    Op_DMTC1,              // 05 DMTC1     0040C0A0
    Op_CTC1,               // 06 CTC1      0040C0E0
    Op_Unknown,            // 07 unused    00409E80
    Op_BC1,                // 08 BC        0040C110
    Op_Unknown,            // 09 unused    00409E80
    Op_Unknown,            // 0A unused    00409E80
    Op_Unknown,            // 0B unused    00409E80
    Op_Unknown,            // 0C unused    00409E80
    Op_Unknown,            // 0D unused    00409E80
    Op_Unknown,            // 0E unused    00409E80
    Op_Unknown,            // 0F unused    00409E80
    Op_COP1Function,       // 10 S         0040C180
    Op_COP1Function,       // 11 D         0040C180
    Op_Unknown,            // 12 unused    00409E80
    Op_Unknown,            // 13 unused    00409E80
    Op_COP1Function,       // 14 W         0040C180
    Op_COP1Function,       // 15 L         0040C180
    Op_Unknown,            // 16 unused    00409E80
    Op_Unknown,            // 17 unused    00409E80
    Op_Unknown,            // 18 unused    00409E80
    Op_Unknown,            // 19 unused    00409E80
    Op_Unknown,            // 1A unused    00409E80
    Op_Unknown,            // 1B unused    00409E80
    Op_Unknown,            // 1C unused    00409E80
    Op_Unknown,            // 1D unused    00409E80
    Op_Unknown,            // 1E unused    00409E80
    Op_Unknown,            // 1F unused    00409E80
};

// Original VA 00478220.
CPUOpcodeHandler CPU_Cop1FunctionDispatch[64] = {
    Op_FADD,               // 00 ADD       0040C1A0
    Op_FSUB,               // 01 SUB       0040C220
    Op_FMUL,               // 02 MUL       0040C2A0
    Op_FDIV,               // 03 DIV       0040C320
    Op_FSQRT,              // 04 SQRT      0040C3A0
    Op_FABS,               // 05 ABS       0040C400
    Op_FMOV,               // 06 MOV       0040C460
    Op_FNEG,               // 07 NEG       0040C4D0
    Op_ROUND_L,            // 08 ROUND.L   0040C530
    Op_ROUND_L,            // 09 TRUNC.L   0040C530
    Op_ROUND_L,            // 0A CEIL.L    0040C530
    Op_ROUND_L,            // 0B FLOOR.L   0040C530
    Op_ROUND_W,            // 0C ROUND.W   0040C590
    Op_ROUND_W,            // 0D TRUNC.W   0040C590
    Op_ROUND_W,            // 0E CEIL.W    0040C590
    Op_ROUND_W,            // 0F FLOOR.W   0040C590
    Op_Unknown,            // 10 unused    00409E80
    Op_Unknown,            // 11 unused    00409E80
    Op_Unknown,            // 12 unused    00409E80
    Op_Unknown,            // 13 unused    00409E80
    Op_Unknown,            // 14 unused    00409E80
    Op_Unknown,            // 15 unused    00409E80
    Op_Unknown,            // 16 unused    00409E80
    Op_Unknown,            // 17 unused    00409E80
    Op_Unknown,            // 18 unused    00409E80
    Op_Unknown,            // 19 unused    00409E80
    Op_Unknown,            // 1A unused    00409E80
    Op_Unknown,            // 1B unused    00409E80
    Op_Unknown,            // 1C unused    00409E80
    Op_Unknown,            // 1D unused    00409E80
    Op_Unknown,            // 1E unused    00409E80
    Op_Unknown,            // 1F unused    00409E80
    Op_CVT_S,              // 20 CVT.S     0040C5F0
    Op_CVT_D,              // 21 CVT.D     0040C670
    Op_Unknown,            // 22 unused    00409E80
    Op_Unknown,            // 23 unused    00409E80
    Op_CVT_W,              // 24 CVT.W     0040C6F0
    Op_CVT_L,              // 25 CVT.L     0040C780
    Op_Unknown,            // 26 unused    00409E80
    Op_Unknown,            // 27 unused    00409E80
    Op_Unknown,            // 28 unused    00409E80
    Op_Unknown,            // 29 unused    00409E80
    Op_Unknown,            // 2A unused    00409E80
    Op_Unknown,            // 2B unused    00409E80
    Op_Unknown,            // 2C unused    00409E80
    Op_Unknown,            // 2D unused    00409E80
    Op_Unknown,            // 2E unused    00409E80
    Op_Unknown,            // 2F unused    00409E80
    Op_C_F,                // 30 C.F       0040C910
    Op_C_F,                // 31 C.UN      0040C910
    Op_C_EQ,               // 32 C.EQ      0040C810
    Op_C_EQ,               // 33 C.UEQ     0040C810
    Op_C_LT,               // 34 C.OLT     0040C890
    Op_C_LT,               // 35 C.ULT     0040C890
    Op_C_LE,               // 36 C.OLE     0040C920
    Op_C_LE,               // 37 C.ULE     0040C920
    Op_C_F,                // 38 C.SF      0040C910
    Op_C_F,                // 39 C.NGLE    0040C910
    Op_C_EQ,               // 3A C.SEQ     0040C810
    Op_C_EQ,               // 3B C.NGL     0040C810
    Op_C_LT,               // 3C C.LT      0040C890
    Op_C_LT,               // 3D C.NGE     0040C890
    Op_C_LE,               // 3E C.LE      0040C920
    Op_C_LE,               // 3F C.NGT     0040C920
};
CpuU32 VICount, RSPBreakpointCount, RSPBreakpoints[100];
extern "C" __declspec(dllimport) int __stdcall kailleraModifyPlayValues(void *, int);

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
