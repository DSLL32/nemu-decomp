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

void AddJumpTarget(unsigned label)
{
    Dynarec_Instructions[Dynarec_InstructionCount].size = 1;
    Dynarec_Instructions[Dynarec_InstructionCount].opcode = 0;
    Dynarec_Instructions[Dynarec_InstructionCount].destination = -1;
    Dynarec_Instructions[Dynarec_InstructionCount].destination2 = -1;
    Dynarec_Instructions[Dynarec_InstructionCount].source = -1;
    Dynarec_Instructions[Dynarec_InstructionCount].source2 = -1;
    Dynarec_Instructions[Dynarec_InstructionCount].source3 = -1;
    Dynarec_Instructions[Dynarec_InstructionCount].address = 0;
    Dynarec_Instructions[Dynarec_InstructionCount].immediate = 0;
    Dynarec_Instructions[Dynarec_InstructionCount].condition = -1;
    Dynarec_Instructions[Dynarec_InstructionCount].mode = -1;
    Dynarec_Instructions[Dynarec_InstructionCount].target = -1;
    Dynarec_Instructions[Dynarec_InstructionCount].longJump = 1;
    Dynarec_Instructions[Dynarec_InstructionCount].label = label;
    if (Dynarec_DelaySlot)
        Dynarec_Instructions[Dynarec_InstructionCount].guestPC = CPU_LastPC - 4;
    else
        Dynarec_Instructions[Dynarec_InstructionCount].guestPC = CPU_LastPC;
    Dynarec_Instructions[Dynarec_InstructionCount].readsFlags = 0;
    Dynarec_Instructions[Dynarec_InstructionCount].writesFlags = 0;
    if (++Dynarec_InstructionCount == 50000)
        ReportCPUError("Error 0 in AddJumpTarget");
}

void AddInstruction(unsigned char size, unsigned opcode, int destination,
    int source, int source2, unsigned address, unsigned immediate, int mode,
    int condition, int target, unsigned char readsFlags, unsigned char writesFlags)
{
    Dynarec_Instructions[Dynarec_InstructionCount].size = size;
    Dynarec_Instructions[Dynarec_InstructionCount].opcode = opcode;
    Dynarec_Instructions[Dynarec_InstructionCount].destination = destination;
    Dynarec_Instructions[Dynarec_InstructionCount].destination2 = -1;
    Dynarec_Instructions[Dynarec_InstructionCount].source = source;
    Dynarec_Instructions[Dynarec_InstructionCount].source2 = source2;
    Dynarec_Instructions[Dynarec_InstructionCount].source3 = -1;
    Dynarec_Instructions[Dynarec_InstructionCount].address = address;
    Dynarec_Instructions[Dynarec_InstructionCount].immediate = immediate;
    Dynarec_Instructions[Dynarec_InstructionCount].condition = condition;
    Dynarec_Instructions[Dynarec_InstructionCount].mode = mode;
    Dynarec_Instructions[Dynarec_InstructionCount].target = target;
    Dynarec_Instructions[Dynarec_InstructionCount].longJump = 1;
    Dynarec_Instructions[Dynarec_InstructionCount].label = -1;
    if (Dynarec_DelaySlot)
        Dynarec_Instructions[Dynarec_InstructionCount].guestPC = CPU_LastPC - 4;
    else
        Dynarec_Instructions[Dynarec_InstructionCount].guestPC = CPU_LastPC;
    Dynarec_Instructions[Dynarec_InstructionCount].readsFlags = readsFlags;
    Dynarec_Instructions[Dynarec_InstructionCount].writesFlags = writesFlags;
    if (++Dynarec_InstructionCount == 50000)
        ReportCPUError("Error 0 in AddJumpTarget");
}

void AddInstructionThreeSources(unsigned char size, unsigned opcode,
    int destination, int source, int source2, int source3, unsigned address,
    unsigned immediate, int mode, int condition, int target,
    unsigned char readsFlags, unsigned char writesFlags)
{
    Dynarec_Instructions[Dynarec_InstructionCount].size = size;
    Dynarec_Instructions[Dynarec_InstructionCount].opcode = opcode;
    Dynarec_Instructions[Dynarec_InstructionCount].destination = destination;
    Dynarec_Instructions[Dynarec_InstructionCount].destination2 = -1;
    Dynarec_Instructions[Dynarec_InstructionCount].source = source;
    Dynarec_Instructions[Dynarec_InstructionCount].source2 = source2;
    Dynarec_Instructions[Dynarec_InstructionCount].source3 = source3;
    Dynarec_Instructions[Dynarec_InstructionCount].address = address;
    Dynarec_Instructions[Dynarec_InstructionCount].immediate = immediate;
    Dynarec_Instructions[Dynarec_InstructionCount].condition = condition;
    Dynarec_Instructions[Dynarec_InstructionCount].mode = mode;
    Dynarec_Instructions[Dynarec_InstructionCount].target = opcode;
    Dynarec_Instructions[Dynarec_InstructionCount].longJump = 1;
    Dynarec_Instructions[Dynarec_InstructionCount].label = -1;
    if (Dynarec_DelaySlot)
        Dynarec_Instructions[Dynarec_InstructionCount].guestPC = CPU_LastPC - 4;
    else
        Dynarec_Instructions[Dynarec_InstructionCount].guestPC = CPU_LastPC;
    Dynarec_Instructions[Dynarec_InstructionCount].readsFlags = readsFlags;
    Dynarec_Instructions[Dynarec_InstructionCount].writesFlags = writesFlags;
    if (++Dynarec_InstructionCount == 50000)
        ReportCPUError("Error 0 in AddJumpTarget");
}

void AddInstructionTwoDestinations(unsigned char size, unsigned opcode,
    int destination, int destination2, int source, int source2, int source3,
    unsigned address, unsigned immediate, int mode, int condition, int target,
    unsigned char readsFlags, unsigned char writesFlags)
{
    Dynarec_Instructions[Dynarec_InstructionCount].size = size;
    Dynarec_Instructions[Dynarec_InstructionCount].opcode = opcode;
    Dynarec_Instructions[Dynarec_InstructionCount].destination = destination;
    Dynarec_Instructions[Dynarec_InstructionCount].destination2 = destination2;
    Dynarec_Instructions[Dynarec_InstructionCount].source = source;
    Dynarec_Instructions[Dynarec_InstructionCount].source2 = source2;
    Dynarec_Instructions[Dynarec_InstructionCount].source3 = source3;
    Dynarec_Instructions[Dynarec_InstructionCount].address = address;
    Dynarec_Instructions[Dynarec_InstructionCount].immediate = immediate;
    Dynarec_Instructions[Dynarec_InstructionCount].condition = condition;
    Dynarec_Instructions[Dynarec_InstructionCount].mode = mode;
    Dynarec_Instructions[Dynarec_InstructionCount].target = opcode;
    Dynarec_Instructions[Dynarec_InstructionCount].longJump = 1;
    Dynarec_Instructions[Dynarec_InstructionCount].label = -1;
    if (Dynarec_DelaySlot)
        Dynarec_Instructions[Dynarec_InstructionCount].guestPC = CPU_LastPC - 4;
    else
        Dynarec_Instructions[Dynarec_InstructionCount].guestPC = CPU_LastPC;
    Dynarec_Instructions[Dynarec_InstructionCount].readsFlags = readsFlags;
    Dynarec_Instructions[Dynarec_InstructionCount].writesFlags = writesFlags;
    if (++Dynarec_InstructionCount == 50000)
        ReportCPUError("Error 0 in AddJumpTarget");
}

void EmitLabel(unsigned label)
{
    AddJumpTarget(label);
}

void EmitAdc(unsigned char size, unsigned char source, unsigned char destination)
{
    AddInstruction(size, 1, destination, destination, source, 0, 0, -1, -1, -1, 1, 1);
}

void EmitAdcImmediate(unsigned char size, unsigned char destination, unsigned value)
{
    AddInstruction(size, 2, destination, destination, -1, 0, value, -1, -1, -1, 1, 1);
}

void EmitAdd(unsigned char size, unsigned char source, unsigned char destination)
{
    AddInstruction(size, 3, destination, source, destination, 0, 0, -1, -1, -1, 0, 1);
}

void EmitAddImmediate(unsigned char destination, unsigned value)
{
    if (value) {
        unsigned char size = value <= 127 && value >= unsigned(-128) ? 0 : 1;
        AddInstruction(size, 5, destination, destination, -1, 0, value, -1, -1, -1, 0, 1);
    }
}

void EmitAddMemory(unsigned char size, unsigned char addressSize, unsigned char mode, unsigned address,
    unsigned char value)
{
    if (value)
        AddInstruction(size, 6, -1, -1, -1, address, value, mode, -1, -1, 0, 1);
}

void EmitAndMemory(unsigned char size, unsigned char mode, unsigned address, unsigned value)
{
    AddInstruction(size, 8, -1, -1, -1, address, value, mode, -1, -1, 0, 1);
}

void EmitAndImmediate(unsigned char size, unsigned char destination, unsigned value)
{
    AddInstruction(size, 9, destination, destination, -1, 0, value, -1, -1, -1, 0, 1);
}

void EmitAnd(unsigned char size, unsigned char source, unsigned char destination)
{
    AddInstruction(size, 10, destination, source, destination, 0, 0, -1, -1, -1, 0, 1);
}

void EmitSignExtend(unsigned char destination, unsigned char source)
{
    AddInstruction(1, 12, destination, source, -1, 0, 0, -1, -1, -1, 0, 1);
}

void EmitCompare(unsigned char size, unsigned char right, unsigned char left)
{
    AddInstruction(size, 13, -1, right, left, 0, 0, -1, -1, -1, 0, 1);
}

void EmitCompareImmediate(unsigned char size, unsigned char source, unsigned value)
{
    if (source == X86_EAX)
        AddInstruction(size, 15, -1, X86_EAX, -1, 0, value, -1, -1, -1, 0, 1);
    else
        AddInstruction(size, 16, -1, source, -1, 0, value, -1, -1, -1, 0, 1);
}

void EmitCall(unsigned address)
{
    AddInstruction(1, 17, -1, -1, -1, address, 0, -1, -1, -1, 0, 0);
}

void EmitDivide(unsigned char size, unsigned char source)
{
    AddInstructionTwoDestinations(size, 18, 0, 2, source, 0, 2, 0, 0, -1, -1, -1, 0, 1);
}

void EmitSignedDivide(unsigned char size, unsigned char source)
{
    AddInstructionTwoDestinations(size, 19, 0, 2, source, 0, 2, 0, 0, -1, -1, -1, 0, 1);
}

void EmitSignedMultiply(unsigned char size, unsigned char source)
{
    AddInstructionTwoDestinations(size, 20, 0, 2, source, 0, 2, 0, 0, -1, -1, -1, 0, 1);
}

unsigned EmitConditionalJump(unsigned char condition)
{
    ++Dynarec_LabelCount;
    AddInstruction(1, 21, -1, -1, -1, 0, 0, -1, condition, Dynarec_LabelCount, 0, 0);
    return Dynarec_LabelCount;
}

void EmitConditionalJumpOffset(unsigned char condition, unsigned offset)
{
    AddInstruction(1, 22, -1, -1, -1, offset, 0, -1, condition, -1, 0, 0);
}

unsigned EmitJump()
{
    ++Dynarec_LabelCount;
    AddInstruction(1, 23, -1, -1, -1, 0, 0, -1, -1, Dynarec_LabelCount, 0, 0);
    return Dynarec_LabelCount;
}

void EmitJumpOffset(unsigned offset)
{
    AddInstruction(1, 24, -1, -1, -1, offset, 0, -1, -1, -1, 0, 0);
}

void EmitJumpGuest(unsigned address)
{
    AddInstruction(1, 65, -1, -1, -1, address, 0, -1, -1, -1, 0, 0);
}

void EmitMove(unsigned char size, unsigned char source, unsigned char destination)
{
    AddInstruction(size, 26, destination, source, -1, 0, 0, -1, -1, -1, 0, 0);
}

void EmitLoad(unsigned char size, unsigned char destination, unsigned char mode, unsigned address)
{
    AddInstruction(size, 27, destination, -1, -1, address, 0, mode, -1, -1, 0, 0);
}

void EmitStore(unsigned char size, unsigned char source, unsigned char mode, unsigned address)
{
    AddInstruction(size, 28, -1, source, -1, address, 0, mode, -1, -1, 0, 0);
}

void EmitStoreHalf(unsigned char source, unsigned char mode, unsigned address)
{
    AddInstruction(1, 29, -1, source, -1, address, 0, mode, -1, -1, 0, 0);
}

void EmitMoveImmediate(unsigned char size, unsigned char destination, unsigned value)
{
    AddInstruction(size, 30, destination, -1, -1, 0, value, -1, -1, -1, 0, 0);
}

void EmitStoreImmediate(unsigned char size, unsigned char mode, unsigned address, unsigned value)
{
    AddInstruction(size, 31, -1, -1, -1, address, value, mode, -1, -1, 0, 0);
}

void EmitLoadSigned(unsigned char size, unsigned char destination, unsigned char mode, unsigned address)
{
    AddInstruction(size, 66, destination, destination, -1, address, 0, mode, -1, -1, 0, 0);
}

void EmitMoveSigned(unsigned char size, unsigned char destination, unsigned char source)
{
    AddInstruction(size, 32, destination, source, -1, 0, 0, -1, -1, -1, 0, 0);
}

void EmitLoadUnsigned(unsigned char size, unsigned char destination, unsigned char mode, unsigned address)
{
    AddInstruction(size, 67, destination, destination, -1, address, 0, mode, -1, -1, 0, 0);
}

void EmitMoveUnsigned(unsigned char size, unsigned char destination, unsigned char source)
{
    AddInstruction(size, 33, destination, source, -1, 0, 0, -1, -1, -1, 0, 0);
}

void EmitMultiply(unsigned char size, unsigned char source)
{
    AddInstructionTwoDestinations(size, 34, 0, 2, source, 0, 2, 0, 0, -1, -1, -1, 0, 0);
}

void EmitNegate(unsigned char size, unsigned char destination)
{
    AddInstruction(size, 35, destination, destination, -1, 0, 0, -1, -1, -1, 0, 1);
}

void EmitNot(unsigned char size, unsigned char destination)
{
    AddInstruction(size, 37, destination, destination, -1, 0, 0, -1, -1, -1, 0, 0);
}

void EmitOrAccumulator(unsigned char size, unsigned value)
{
    AddInstruction(size, 68, 0, 0, -1, 0, value, -1, -1, -1, 0, 1);
}

void EmitOrMemory(unsigned char size, unsigned char mode, unsigned address, unsigned value)
{
    AddInstruction(size, 39, -1, -1, -1, address, value, mode, -1, -1, 0, 1);
}

void EmitOr(unsigned char size, unsigned char source, unsigned char destination)
{
    AddInstruction(size, 40, destination, source, destination, 0, 0, -1, -1, -1, 0, 1);
}

void EmitPop(unsigned char destination)
{
    AddInstructionTwoDestinations(1, 41, destination, 4, 4, -1, -1, 0, 0, -1, -1, -1, 0, 0);
}

void EmitPush(unsigned char source)
{
    AddInstruction(1, 42, -1, source, 4, 0, 0, -1, -1, -1, 0, 0);
}

void EmitReadTimestamp()
{
    AddInstructionTwoDestinations(1, 69, 0, 2, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0);
}

void EmitReturn()
{
    AddInstruction(1, 43, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0);
}

void EmitStoreFlags()
{
    AddInstruction(1, 44, -1, 0, -1, 0, 0, -1, -1, -1, 0, 1);
}

void EmitArithmeticShiftRight(unsigned char size, unsigned char destination, unsigned char count)
{
    AddInstruction(size, 45, destination, destination, -1, 0, count, -1, -1, -1, 0, 1);
}

void EmitArithmeticShiftRightCL(unsigned char size, unsigned char destination)
{
    AddInstruction(size, 46, destination, destination, 1, 0, 0, -1, -1, -1, 0, 1);
}

void EmitSbb(unsigned char size, unsigned char source, unsigned char destination)
{
    AddInstruction(size, 47, destination, destination, source, 0, 0, -1, -1, -1, 1, 1);
}

void EmitSetMemory(unsigned char condition, unsigned char mode, unsigned address)
{
    AddInstruction(1, 48, -1, -1, -1, address, 0, mode, condition, -1, 1, 0);
}

void EmitSet(unsigned char condition, unsigned char destination)
{
    AddInstruction(1, 49, destination, -1, -1, 0, 0, -1, condition, -1, 1, 0);
}

void EmitShiftLeftCL(unsigned char size, unsigned char destination)
{
    AddInstruction(size, 51, destination, destination, 1, 0, 0, -1, -1, -1, 1, 1);
}

void EmitShiftLeft(unsigned char size, unsigned char destination, unsigned char count)
{
    AddInstruction(size, 50, destination, destination, -1, 0, count, -1, -1, -1, 1, 1);
}

void EmitDoubleShiftLeftCL(unsigned char source, unsigned char destination)
{
    AddInstructionThreeSources(1, 52, destination, source, destination, 1, 0, 0, -1, -1, -1, 0, 1);
}

void EmitDoubleShiftLeft(unsigned char source, unsigned char destination, unsigned char count)
{
    AddInstruction(1, 53, destination, source, destination, 0, count, -1, -1, -1, 0, 1);
}

void EmitShiftRightCL(unsigned char size, unsigned char destination)
{
    AddInstruction(size, 55, destination, destination, 1, 0, 0, -1, -1, -1, 1, 1);
}

void EmitShiftRight(unsigned char size, unsigned char destination, unsigned char count)
{
    AddInstruction(size, 54, destination, destination, -1, 0, count, -1, -1, -1, 1, 1);
}

void EmitDoubleShiftRightCL(unsigned char source, unsigned char destination)
{
    AddInstructionThreeSources(1, 56, destination, source, destination, 1, 0, 0, -1, -1, -1, 0, 1);
}

void EmitDoubleShiftRight(unsigned char source, unsigned char destination, unsigned char count)
{
    AddInstruction(1, 57, destination, source, destination, 0, count, -1, -1, -1, 0, 1);
}

void EmitSubtract(unsigned char size, unsigned char source, unsigned char destination)
{
    AddInstruction(size, 58, destination, destination, source, 0, 0, -1, -1, -1, 0, 1);
}

void EmitExchange(unsigned char size, unsigned char left, unsigned char right)
{
    AddInstructionTwoDestinations(size, 59, left, right, left, right, -1, 0, 0, -1, -1, -1, 0, 0);
}

void EmitTestMemory(unsigned char size, unsigned char mode, unsigned address, unsigned value)
{
    AddInstruction(size, 60, -1, -1, -1, address, value, mode, -1, -1, 0, 1);
}

void EmitXorAccumulator(unsigned char size, unsigned value)
{
    AddInstruction(size, 61, 0, 0, -1, 0, value, -1, -1, -1, 0, 1);
}

void EmitXor(unsigned char size, unsigned char source, unsigned char destination)
{
    AddInstruction(size, 62, destination, source, destination, 0, 0, -1, -1, -1, 0, 1);
}

void EmitFpuAbsolute()
{
    AddInstruction(1, 1000, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0);
}

void EmitFpuAddSingle(unsigned address)
{
    AddInstruction(1, 1001, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuAddDouble(unsigned address)
{
    AddInstruction(1, 1002, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuChangeSign()
{
    AddInstruction(1, 1003, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0);
}

void EmitFpuClearExceptions()
{
    AddInstruction(1, 1004, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0);
}

void EmitFpuCompareSingle(unsigned address)
{
    AddInstruction(1, 1005, -1, -1, -1, address, 0, 5, -1, -1, 0, 1);
}

void EmitFpuCompareDouble(unsigned address)
{
    AddInstruction(1, 1006, -1, -1, -1, address, 0, 5, -1, -1, 0, 1);
}

void EmitFpuDivideSingle(unsigned address)
{
    AddInstruction(1, 1007, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuDivideDouble(unsigned address)
{
    AddInstruction(1, 1008, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuLoadInt32(unsigned address)
{
    AddInstruction(1, 1009, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuLoadInt64(unsigned address)
{
    AddInstruction(1, 1010, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuStoreInt32(unsigned address)
{
    AddInstruction(1, 1011, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuStoreInt64(unsigned address)
{
    AddInstruction(1, 1012, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuLoadSingle(unsigned address)
{
    AddInstruction(1, 1013, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuLoadDouble(unsigned address)
{
    AddInstruction(1, 1014, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuLoadControl(unsigned address)
{
    AddInstruction(1, 1015, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuMultiplySingle(unsigned address)
{
    AddInstruction(1, 1016, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuMultiplyDouble(unsigned address)
{
    AddInstruction(1, 1017, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuSquareRoot()
{
    AddInstruction(1, 1019, -1, -1, -1, 0, 0, -1, -1, -1, 0, 0);
}

void EmitFpuPopSingle(unsigned address)
{
    AddInstruction(1, 1023, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuPopDouble(unsigned address)
{
    AddInstruction(1, 1024, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuStoreStatus()
{
    AddInstruction(1, 1025, -1, -1, -1, 0, 0, -1, -1, -1, 1, 0);
}

void EmitFpuSubtractSingle(unsigned address)
{
    AddInstruction(1, 1026, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}

void EmitFpuSubtractDouble(unsigned address)
{
    AddInstruction(1, 1027, -1, -1, -1, address, 0, 5, -1, -1, 0, 0);
}
