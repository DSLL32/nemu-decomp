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

unsigned Dynarec_Mode;
unsigned Dynarec_Inlining;
unsigned Dynarec_BlockCode[0x200000];
unsigned char Dynarec_MainCode[0x1400000];
unsigned char Dynarec_FastCode[0x100000];
unsigned Dynarec_MainCodeUsed, Dynarec_FastCodeUsed;
unsigned char *Dynarec_CodePointer, *Dynarec_EntryPoint;
unsigned Dynarec_Use32BitMemoryAccess;
unsigned Dynarec_DelaySlot, Dynarec_BlockFinished;
unsigned Dynarec_NextPC, Dynarec_BranchPending;
unsigned Dynarec_InstructionCount, Dynarec_LabelCount;
RecompiledInstruction Dynarec_Instructions[50000];
unsigned Dynarec_LabelInstructions[10000];
unsigned Dynarec_JumpCount, Dynarec_PendingCount;
DynarecJumpFixup Dynarec_JumpFixups[100000];
unsigned Dynarec_PendingAddresses[10000];
unsigned Dynarec_ConversionCount;
DynarecAddressMap Dynarec_AddressMap[500000];
unsigned char Dynarec_RecordAddresses;
unsigned Dynarec_LastWasGuestJump;
unsigned Dynarec_InlineAddressCount;
unsigned Dynarec_InlineAddresses[1024];
unsigned Dynarec_PageCode[1024];
unsigned Dynarec_PageAddress, Dynarec_PhysicalPC;
unsigned Dynarec_BlockStart, Dynarec_BlockInstructionCount;
unsigned char Dynarec_TrackingLoaded;

typedef char InstructionSizeIs64[sizeof(RecompiledInstruction) == 64 ? 1 : -1];

void AddJumpTarget(unsigned label)
{
    RecompiledInstruction &instruction = Dynarec_Instructions[Dynarec_InstructionCount];
    instruction.size = 1;
    instruction.opcode = 0;
    instruction.destination = -1;
    instruction.destination2 = -1;
    instruction.source = -1;
    instruction.source2 = -1;
    instruction.source3 = -1;
    instruction.address = 0;
    instruction.immediate = 0;
    instruction.condition = -1;
    instruction.mode = -1;
    instruction.target = -1;
    instruction.longJump = 1;
    instruction.label = label;
    instruction.guestPC = Dynarec_DelaySlot ? CPU_LastPC - 4 : CPU_LastPC;
    instruction.readsFlags = 0;
    instruction.writesFlags = 0;
    if (++Dynarec_InstructionCount == 50000)
        ReportCPUError("Error 0 in AddJumpTarget");
}

void AddInstruction(unsigned char size, unsigned opcode, int destination,
    int source, int source2, unsigned address, unsigned immediate, int mode,
    int condition, int target, unsigned char readsFlags, unsigned char writesFlags)
{
    RecompiledInstruction &instruction = Dynarec_Instructions[Dynarec_InstructionCount];
    instruction.size = size;
    instruction.opcode = opcode;
    instruction.destination = destination;
    instruction.destination2 = -1;
    instruction.source = source;
    instruction.source2 = source2;
    instruction.source3 = -1;
    instruction.address = address;
    instruction.immediate = immediate;
    instruction.condition = condition;
    instruction.mode = mode;
    instruction.target = target;
    instruction.longJump = 1;
    instruction.label = -1;
    instruction.guestPC = Dynarec_DelaySlot ? CPU_LastPC - 4 : CPU_LastPC;
    instruction.readsFlags = readsFlags;
    instruction.writesFlags = writesFlags;
    if (++Dynarec_InstructionCount == 50000)
        ReportCPUError("Error 0 in AddJumpTarget");
}

void AddInstructionThreeSources(unsigned char size, unsigned opcode,
    int destination, int source, int source2, int source3, unsigned address,
    unsigned immediate, int mode, int condition, int target,
    unsigned char readsFlags, unsigned char writesFlags)
{
    RecompiledInstruction &instruction = Dynarec_Instructions[Dynarec_InstructionCount];
    instruction.size = size;
    instruction.opcode = opcode;
    instruction.destination = destination;
    instruction.destination2 = -1;
    instruction.source = source;
    instruction.source2 = source2;
    instruction.source3 = source3;
    instruction.address = address;
    instruction.immediate = immediate;
    instruction.condition = condition;
    instruction.mode = mode;
    instruction.target = opcode;
    instruction.longJump = 1;
    instruction.label = -1;
    instruction.guestPC = Dynarec_DelaySlot ? CPU_LastPC - 4 : CPU_LastPC;
    instruction.readsFlags = readsFlags;
    instruction.writesFlags = writesFlags;
    if (++Dynarec_InstructionCount == 50000)
        ReportCPUError("Error 0 in AddJumpTarget");
}

void AddInstructionTwoDestinations(unsigned char size, unsigned opcode,
    int destination, int destination2, int source, int source2, int source3,
    unsigned address, unsigned immediate, int mode, int condition, int target,
    unsigned char readsFlags, unsigned char writesFlags)
{
    RecompiledInstruction &instruction = Dynarec_Instructions[Dynarec_InstructionCount];
    instruction.size = size;
    instruction.opcode = opcode;
    instruction.destination = destination;
    instruction.destination2 = destination2;
    instruction.source = source;
    instruction.source2 = source2;
    instruction.source3 = source3;
    instruction.address = address;
    instruction.immediate = immediate;
    instruction.condition = condition;
    instruction.mode = mode;
    instruction.target = opcode;
    instruction.longJump = 1;
    instruction.label = -1;
    instruction.guestPC = Dynarec_DelaySlot ? CPU_LastPC - 4 : CPU_LastPC;
    instruction.readsFlags = readsFlags;
    instruction.writesFlags = writesFlags;
    if (++Dynarec_InstructionCount == 50000)
        ReportCPUError("Error 0 in AddJumpTarget");
}

void EmitLabel(unsigned label)
{
    AddJumpTarget(label);
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

void EmitCompareImmediate(unsigned char size, unsigned char source, unsigned value)
{
    if (source == X86_EAX)
        AddInstruction(size, 15, -1, X86_EAX, -1, 0, value, -1, -1, -1, 0, 1);
    else
        AddInstruction(size, 16, -1, source, -1, 0, value, -1, -1, -1, 0, 1);
}

unsigned EmitConditionalJump(unsigned char condition)
{
    ++Dynarec_LabelCount;
    AddInstruction(1, 21, -1, -1, -1, 0, 0, -1, condition, Dynarec_LabelCount, 0, 0);
    return Dynarec_LabelCount;
}

unsigned EmitJump()
{
    ++Dynarec_LabelCount;
    AddInstruction(1, 23, -1, -1, -1, 0, 0, -1, -1, Dynarec_LabelCount, 0, 0);
    return Dynarec_LabelCount;
}

void EmitAdc(unsigned char size, unsigned char destination, unsigned char source)
{
    AddInstruction(size, 1, destination, destination, source, 0, 0, -1, -1, -1, 1, 1);
}

void EmitAdcImmediate(unsigned char size, unsigned char destination, unsigned value)
{
    AddInstruction(size, 2, destination, destination, -1, 0, value, -1, -1, -1, 1, 1);
}

void EmitAdd(unsigned char size, unsigned char destination, unsigned char source)
{
    AddInstruction(size, 3, destination, source, destination, 0, 0, -1, -1, -1, 0, 1);
}

void EmitAndMemory(unsigned char size, unsigned char mode, unsigned address, unsigned value)
{
    AddInstruction(size, 8, -1, -1, -1, address, value, mode, -1, -1, 0, 1);
}

void EmitAndImmediate(unsigned char size, unsigned char destination, unsigned value)
{
    AddInstruction(size, 9, destination, destination, -1, 0, value, -1, -1, -1, 0, 1);
}

void EmitAnd(unsigned char size, unsigned char destination, unsigned char source)
{
    AddInstruction(size, 10, destination, source, destination, 0, 0, -1, -1, -1, 0, 1);
}

void EmitSignExtend(unsigned char destination, unsigned char source)
{
    AddInstruction(1, 12, destination, source, -1, 0, 0, -1, -1, -1, 0, 1);
}

void EmitCompare(unsigned char size, unsigned char left, unsigned char right)
{
    AddInstruction(size, 13, -1, right, left, 0, 0, -1, -1, -1, 0, 1);
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

void EmitConditionalJumpOffset(unsigned char condition, unsigned offset)
{
    AddInstruction(1, 22, -1, -1, -1, offset, 0, -1, condition, -1, 0, 0);
}

void EmitJumpOffset(unsigned offset)
{
    AddInstruction(1, 24, -1, -1, -1, offset, 0, -1, -1, -1, 0, 0);
}

void EmitJumpGuest(unsigned address)
{
    AddInstruction(1, 65, -1, -1, -1, address, 0, -1, -1, -1, 0, 0);
}

void EmitMove(unsigned char size, unsigned char destination, unsigned char source)
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

void EmitOr(unsigned char size, unsigned char destination, unsigned char source)
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

void EmitSbb(unsigned char size, unsigned char destination, unsigned char source)
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

void EmitDoubleShiftLeftCL(unsigned char destination, unsigned char source)
{
    AddInstructionThreeSources(1, 52, destination, source, destination, 1, 0, 0, -1, -1, -1, 0, 1);
}

void EmitDoubleShiftLeft(unsigned char destination, unsigned char source, unsigned char count)
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

void EmitDoubleShiftRightCL(unsigned char destination, unsigned char source)
{
    AddInstructionThreeSources(1, 56, destination, source, destination, 1, 0, 0, -1, -1, -1, 0, 1);
}

void EmitDoubleShiftRight(unsigned char destination, unsigned char source, unsigned char count)
{
    AddInstruction(1, 57, destination, source, destination, 0, count, -1, -1, -1, 0, 1);
}

void EmitSubtract(unsigned char size, unsigned char destination, unsigned char source)
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

void EmitXor(unsigned char size, unsigned char destination, unsigned char source)
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

void AddPendingAddress(unsigned address)
{
    for (unsigned i = 0; i < Dynarec_PendingCount; ++i)
        if (Dynarec_PendingAddresses[i] == address)
            return;
    Dynarec_PendingAddresses[Dynarec_PendingCount++] = address;
    if (Dynarec_PendingCount >= 10000)
        ReportCompilerError("Compiler Error 0");
}

void AddJumpFixup(unsigned offset, unsigned address)
{
    Dynarec_JumpFixups[Dynarec_JumpCount].offset = offset;
    Dynarec_JumpFixups[Dynarec_JumpCount].address = address;
    if (++Dynarec_JumpCount >= 100000)
        ReportCompilerError("Compiler Error 1");
}

void AddAddressMapping(unsigned physical, unsigned offset)
{
    Dynarec_AddressMap[Dynarec_ConversionCount].physical = physical;
    Dynarec_AddressMap[Dynarec_ConversionCount].offset = offset;
    if (++Dynarec_ConversionCount > 499900) {
        ReportCPUError("Convert Table overflow. Resetting dynrec");
        CPU_ActionFlags |= 0x400;
    }
}

bool IsOpcodeBreakpoint(unsigned instruction)
{
    for (unsigned i = 0; i < OpcodeBreakpointCount; ++i)
        if ((instruction & OpcodeBreakpoints[i].Mask) ==
            OpcodeBreakpoints[i].Opcode)
            return true;
    return false;
}

void AddInlineAddress(unsigned address)
{
    Dynarec_InlineAddresses[Dynarec_InlineAddressCount++] = address;
}

void SetLabelInstruction(unsigned label, unsigned index)
{
    Dynarec_LabelInstructions[label] = index;
}

void ResetInstructionBuffer(unsigned char recordAddresses)
{
    Dynarec_InstructionCount = 0;
    Dynarec_RecordAddresses = recordAddresses;
    Dynarec_LabelCount = 0;
}

// These write x86 operands into the generated-code buffer, not the host image.
static inline void EmitByte(unsigned value)
{
    *Dynarec_CodePointer++ = (unsigned char)value;
}

static inline void EmitDword(unsigned value)
{
    *(unsigned *)Dynarec_CodePointer = value;
    Dynarec_CodePointer += 4;
}

void AssembleInstructions(unsigned char fast)
{
    unsigned savedConversions = Dynarec_ConversionCount;
    unsigned char *base = fast ? Dynarec_FastCode : Dynarec_MainCode;
    unsigned char *start = base + (fast ? Dynarec_FastCodeUsed : Dynarec_MainCodeUsed);
    bool retry;
    unsigned index;
    do {
        index = 0;
        retry = false;
        unsigned lastPC = unsigned(-1);
        Dynarec_CodePointer = start;
        Dynarec_ConversionCount = savedConversions;
        for (; index < Dynarec_InstructionCount; ++index) {
            if (retry)
                break;
            RecompiledInstruction &instruction = Dynarec_Instructions[index];
            instruction.code = Dynarec_CodePointer;
            switch (instruction.opcode) {
            case 0: {
                RecompiledInstruction &jump = Dynarec_Instructions[
                    Dynarec_LabelInstructions[instruction.label]];
                int displacement = Dynarec_CodePointer - jump.code - 2;
                if (jump.opcode == 21) {
                    if (!jump.longJump) {
                        if (displacement < 128 && displacement > -129) {
                            jump.code[0] = (unsigned char)(0x70 | jump.condition);
                            jump.code[1] = (unsigned char)displacement;
                        } else {
                            jump.longJump = 1;
                            retry = true;
                        }
                    } else {
                        jump.code[0] = 0x0f;
                        jump.code[1] = (unsigned char)(0x80 | jump.condition);
                        *(unsigned *)(jump.code + 2) = Dynarec_CodePointer - jump.code - 6;
                    }
                } else if (!jump.longJump) {
                    if (displacement > 127 || displacement < -128) {
                        jump.longJump = 1;
                        retry = true;
                    } else {
                        jump.code[0] = 0xeb;
                        jump.code[1] = (unsigned char)displacement;
                    }
                } else {
                    jump.code[0] = 0xe9;
                    *(unsigned *)(jump.code + 1) = Dynarec_CodePointer - jump.code - 5;
                }
                break;
            }
            case 1:
                EmitByte(0x10 | instruction.size);
                EmitByte(0xc0 | (instruction.source2 << 3) | instruction.destination);
                break;
            case 2:
                EmitByte(0x80 | instruction.size);
                EmitByte(0xd0 | instruction.destination);
                EmitDword(instruction.immediate);
                break;
            case 3:
                EmitByte(instruction.size);
                EmitByte(0xc0 | (instruction.source << 3) | instruction.destination);
                break;
            case 4:
                EmitByte(0x04 | instruction.size);
                EmitDword(instruction.immediate);
                break;
            case 5:
                EmitByte(0x80 | instruction.size);
                EmitByte(0xc0 | instruction.destination);
                if (!instruction.size) EmitByte(instruction.immediate);
                else EmitDword(instruction.immediate);
                break;
            case 6:
                EmitByte(0x80 | instruction.size);
                EmitByte(instruction.mode);
                EmitDword(instruction.address);
                if (!instruction.size) EmitByte(instruction.immediate);
                else EmitDword(instruction.immediate);
                break;
            case 7:
                EmitByte(0x24 | instruction.size);
                EmitDword(instruction.immediate);
                break;
            case 8:
                EmitByte(0x80 | instruction.size);
                EmitByte(0x20 | instruction.mode);
                EmitDword(instruction.address);
                EmitDword(instruction.immediate);
                break;
            case 9:
                EmitByte(0x80 | instruction.size);
                EmitByte(0xe0 | instruction.destination);
                EmitDword(instruction.immediate);
                break;
            case 10:
                EmitByte(0x20 | instruction.size);
                EmitByte(0xc0 | (instruction.source << 3) | instruction.destination);
                break;
            case 11:
                EmitByte(0x0f);
                EmitByte(0xc8 | instruction.destination);
                break;
            case 12:
                if (instruction.source != instruction.destination) {
                    EmitByte(0x89);
                    EmitByte(0xc0 | (instruction.source << 3) | instruction.destination);
                }
                EmitByte(0xc1);
                EmitByte(0xf8 | instruction.destination);
                EmitByte(31);
                break;
            case 13:
                EmitByte(0x38 | instruction.size);
                EmitByte(0xc0 | (instruction.source << 3) | instruction.source2);
                break;
            case 14:
                break;
            case 15:
                EmitByte(0x3c | instruction.size);
                if (!instruction.size) EmitByte(instruction.immediate);
                else EmitDword(instruction.immediate);
                break;
            case 16:
                EmitByte(0x80 | instruction.size);
                EmitByte(0xf8 | instruction.source);
                if (!instruction.size) EmitByte(instruction.immediate);
                else EmitDword(instruction.immediate);
                break;
            case 17:
                EmitByte(0xe8);
                EmitDword(instruction.address - (unsigned)Dynarec_CodePointer - 4);
                break;
            case 18:
                EmitByte(0xf6 | instruction.size);
                EmitByte(0xf0 | instruction.source);
                break;
            case 19:
                EmitByte(0xf6 | instruction.size);
                EmitByte(0xf8 | instruction.source);
                break;
            case 20:
                EmitByte(0xf6 | instruction.size);
                EmitByte(0xe8 | instruction.source);
                break;
            case 21:
                SetLabelInstruction(instruction.target, index);
                EmitByte(0);
                EmitByte(0);
                if (instruction.longJump) EmitDword(0);
                break;
            case 22:
                EmitByte(0x0f);
                EmitByte(0x80 | instruction.condition);
                EmitDword((unsigned)base + instruction.address - (unsigned)Dynarec_CodePointer - 4);
                break;
            case 23:
                SetLabelInstruction(instruction.target, index);
                EmitByte(0);
                if (instruction.longJump) EmitDword(0);
                else EmitByte(0);
                break;
            case 24:
                EmitByte(0xe9);
                EmitDword((unsigned)base + instruction.address - (unsigned)Dynarec_CodePointer - 4);
                break;
            case 25:
                EmitByte(0xff);
                EmitByte(0xe0 | instruction.source);
                break;
            case 26:
                EmitByte(0x88 | instruction.size);
                EmitByte(0xc0 | (instruction.source << 3) | instruction.destination);
                break;
            case 27:
                EmitByte(0x8a | instruction.size);
                EmitByte((instruction.destination << 3) | instruction.mode);
                EmitDword(instruction.address);
                break;
            case 28:
                EmitByte(0x88 | instruction.size);
                EmitByte((instruction.source << 3) | instruction.mode);
                EmitDword(instruction.address);
                break;
            case 29:
                EmitByte(0x66);
                EmitByte(0x89);
                EmitByte((instruction.source << 3) | instruction.mode);
                EmitDword(instruction.address);
                break;
            case 30:
                EmitByte(((instruction.size | 0xf6) << 3) | instruction.destination);
                EmitDword(instruction.immediate);
                break;
            case 31:
                EmitByte(0xc6 | instruction.size);
                EmitByte(instruction.mode);
                EmitDword(instruction.address);
                if (!instruction.size) EmitByte(instruction.immediate);
                else EmitDword(instruction.immediate);
                break;
            case 32:
                EmitByte(0x0f);
                EmitByte(0xbe | instruction.size);
                EmitByte(0xc0 | (instruction.destination << 3) | instruction.source);
                break;
            case 33:
                EmitByte(0x0f);
                EmitByte(0xb6 | instruction.size);
                EmitByte(0xc0 | (instruction.destination << 3) | instruction.source);
                break;
            case 34:
                EmitByte(0xf6 | instruction.size);
                EmitByte(0xe0 | instruction.source);
                break;
            case 35:
                EmitByte(0xf6 | instruction.size);
                EmitByte(0xd8 | instruction.destination);
                break;
            case 36:
                EmitByte(0x90);
                break;
            case 37:
                EmitByte(0xf6 | instruction.size);
                EmitByte(0xd0 | instruction.destination);
                break;
            case 38:
                EmitByte(0x0c | instruction.size);
                EmitDword(instruction.immediate);
                break;
            case 39:
                EmitByte(0x80 | instruction.size);
                EmitByte(0x08 | instruction.mode);
                EmitDword(instruction.address);
                EmitDword(instruction.immediate);
                break;
            case 40:
                EmitByte(0x08 | instruction.size);
                EmitByte(0xc0 | (instruction.source << 3) | instruction.destination);
                break;
            case 41:
                EmitByte(0x58 | instruction.destination);
                break;
            case 42:
                EmitByte(0x50 | instruction.source);
                break;
            case 43:
                EmitByte(0xc3);
                break;
            case 44:
                EmitByte(0x9e);
                break;
            case 45:
                EmitByte(0xc0 | instruction.size);
                EmitByte(0xf8 | instruction.destination);
                EmitByte(instruction.immediate);
                break;
            case 46:
                EmitByte(0xd2 | instruction.size);
                EmitByte(0xf8 | instruction.destination);
                break;
            case 47:
                EmitByte(0x18 | instruction.size);
                EmitByte(0xc0 | (instruction.source2 << 3) | instruction.destination);
                break;
            case 48:
                EmitByte(0x0f);
                EmitByte(0x90 | instruction.condition);
                EmitByte(instruction.mode);
                EmitDword(instruction.address);
                break;
            case 49:
                EmitByte(0x0f);
                EmitByte(0x90 | instruction.condition);
                EmitByte(0xc0 | instruction.destination);
                break;
            case 50:
                EmitByte(0xc0 | instruction.size);
                EmitByte(0xe0 | instruction.destination);
                EmitByte(instruction.immediate);
                break;
            case 51:
                EmitByte(0xd2 | instruction.size);
                EmitByte(0xe0 | instruction.destination);
                break;
            case 52:
                EmitByte(0x0f);
                EmitByte(0xa5);
                EmitByte(0xc0 | (instruction.source << 3) | instruction.destination);
                break;
            case 53:
                EmitByte(0x0f);
                EmitByte(0xa4);
                EmitByte(0xc0 | (instruction.source << 3) | instruction.destination);
                EmitByte(instruction.immediate);
                break;
            case 54:
                EmitByte(0xc0 | instruction.size);
                EmitByte(0xe8 | instruction.destination);
                EmitByte(instruction.immediate);
                break;
            case 55:
                EmitByte(0xd2 | instruction.size);
                EmitByte(0xe8 | instruction.destination);
                break;
            case 56:
                EmitByte(0x0f);
                EmitByte(0xad);
                EmitByte(0xc0 | (instruction.source << 3) | instruction.destination);
                break;
            case 57:
                EmitByte(0x0f);
                EmitByte(0xac);
                EmitByte(0xc0 | (instruction.source << 3) | instruction.destination);
                EmitByte(instruction.immediate);
                break;
            case 58:
                EmitByte(0x28 | instruction.size);
                EmitByte(0xc0 | (instruction.source2 << 3) | instruction.destination);
                break;
            case 59:
                EmitByte(0x86 | instruction.size);
                EmitByte(0xc0 | (instruction.source << 3) | instruction.source2);
                break;
            case 60:
                EmitByte(0xf6 | instruction.size);
                EmitByte(instruction.mode);
                EmitDword(instruction.address);
                EmitDword(instruction.immediate);
                break;
            case 61:
                EmitByte(0x34 | instruction.size);
                EmitDword(instruction.immediate);
                break;
            case 62:
                EmitByte(0x30 | instruction.size);
                EmitByte(0xc0 | (instruction.source << 3) | instruction.destination);
                break;
            case 63:
                EmitByte(0x14 | instruction.size);
                EmitDword(instruction.immediate);
                break;
            case 64:
                EmitByte(0x99);
                break;
            case 65:
                AddJumpFixup(Dynarec_CodePointer - base, instruction.address);
                EmitByte(0);
                EmitDword(0);
                break;
            case 66:
                EmitByte(0x0f);
                EmitByte(0xbe | instruction.size);
                EmitByte((instruction.destination << 3) | instruction.mode);
                EmitDword(instruction.address);
                break;
            case 67:
                EmitByte(0x0f);
                EmitByte(0xb6 | instruction.size);
                EmitByte((instruction.destination << 3) | instruction.mode);
                EmitDword(instruction.address);
                break;
            case 68:
                EmitByte(0x80 | instruction.size);
                EmitByte(0xc8 | instruction.destination);
                EmitDword(instruction.immediate);
                break;
            case 69:
                EmitByte(0x0f);
                EmitByte(0x31);
                break;
            case 1001:
                EmitByte(0xd8);
                EmitByte(0x5);
                EmitDword(instruction.address);
                break;
            case 1002:
                EmitByte(0xdc);
                EmitByte(0x5);
                EmitDword(instruction.address);
                break;
            case 1003:
                EmitByte(0xd9);
                EmitByte(0xe0);
                break;
            case 1004:
                EmitByte(0xdb);
                EmitByte(0xe2);
                break;
            case 1005:
                EmitByte(0xd8);
                EmitByte(0x1d);
                EmitDword(instruction.address);
                break;
            case 1006:
                EmitByte(0xdc);
                EmitByte(0x1d);
                EmitDword(instruction.address);
                break;
            case 1007:
                EmitByte(0xd8);
                EmitByte(0x35);
                EmitDword(instruction.address);
                break;
            case 1008:
                EmitByte(0xdc);
                EmitByte(0x35);
                EmitDword(instruction.address);
                break;
            case 1009:
                EmitByte(0xdb);
                EmitByte(0x5);
                EmitDword(instruction.address);
                break;
            case 1010:
                EmitByte(0xdf);
                EmitByte(0x2d);
                EmitDword(instruction.address);
                break;
            case 1011:
                EmitByte(0xdb);
                EmitByte(0x1d);
                EmitDword(instruction.address);
                break;
            case 1012:
                EmitByte(0xdf);
                EmitByte(0x3d);
                EmitDword(instruction.address);
                break;
            case 1013:
                EmitByte(0xd9);
                EmitByte(instruction.mode);
                EmitDword(instruction.address);
                break;
            case 1014:
                EmitByte(0xdd);
                EmitByte(instruction.mode);
                EmitDword(instruction.address);
                break;
            case 1015:
                EmitByte(0xd9);
                EmitByte(0x2d);
                EmitDword(instruction.address);
                break;
            case 1016:
                EmitByte(0xd8);
                EmitByte(0xd);
                EmitDword(instruction.address);
                break;
            case 1017:
                EmitByte(0xdc);
                EmitByte(0xd);
                EmitDword(instruction.address);
                break;
            case 1018:
                EmitByte(0xd9);
                EmitByte(0xfc);
                break;
            case 1019:
                EmitByte(0xd9);
                EmitByte(0xfa);
                break;
            case 1020:
                EmitByte(0xd9);
                EmitByte(0x3d);
                EmitDword(instruction.address);
                break;
            case 1021:
                EmitByte(0xd9);
                EmitByte(0x10 | instruction.mode);
                EmitDword(instruction.address);
                break;
            case 1022:
                EmitByte(0xdd);
                EmitByte(0x10 | instruction.mode);
                EmitDword(instruction.address);
                break;
            case 1023:
                EmitByte(0xd9);
                EmitByte(0x18 | instruction.mode);
                EmitDword(instruction.address);
                break;
            case 1024:
                EmitByte(0xdd);
                EmitByte(0x18 | instruction.mode);
                EmitDword(instruction.address);
                break;
            case 1025:
                EmitByte(0xdf);
                EmitByte(0xe0);
                break;
            case 1026:
                EmitByte(0xd8);
                EmitByte(0x25);
                EmitDword(instruction.address);
                break;
            case 1027:
                EmitByte(0xdc);
                EmitByte(0x25);
                EmitDword(instruction.address);
                break;
            case 1028:
                EmitByte(0x9b);
                break;
            case 1029:
                EmitByte(0xd9);
                EmitByte(0xc8 | instruction.immediate);
                break;
            case 1030:
                EmitByte(0xd8);
                EmitByte(0xc0 | instruction.immediate);
                break;
            case 1031:
                EmitByte(0xd8);
                EmitByte(0xf0 | instruction.immediate);
                break;
            case 1032:
                EmitByte(0xdd);
                EmitByte(0xc0 | instruction.immediate);
                break;
            case 1033:
                EmitByte(0xd8);
                EmitByte(0xc8 | instruction.immediate);
                break;
            case 1034:
                EmitByte(0xd8);
                EmitByte(0xe0 | instruction.immediate);
                break;
            case 1035:
                EmitByte(0xd9);
                EmitByte(0xf6);
                break;
            case 1036:
                EmitByte(0xdb);
                EmitByte(0x10 | instruction.mode);
                EmitDword(instruction.address);
                break;
            default:
                if (instruction.opcode == 1000) {
                    EmitByte(0xd9);
                    EmitByte(0xe1);
                }
                break;
            }
            if (Dynarec_RecordAddresses && lastPC != instruction.guestPC) {
                unsigned physical = (instruction.guestPC & 0xc0000000) == 0x80000000
                    ? instruction.guestPC & 0x1fffffff
                    : TranslateTLB(instruction.guestPC, 0);
                AddAddressMapping(physical, Dynarec_CodePointer - base);
            }
            lastPC = instruction.guestPC;
        }
        if (retry) {
            Dynarec_CodePointer = start;
            Dynarec_ConversionCount = savedConversions;
        }
    } while (retry);
    Dynarec_LastWasGuestJump = Dynarec_Instructions[index - 1].opcode == 65;
    if (fast)
        Dynarec_FastCodeUsed = Dynarec_CodePointer - Dynarec_FastCode;
    else
        Dynarec_MainCodeUsed = Dynarec_CodePointer - Dynarec_MainCode;
}

#include "DynarecSignatures.inc"

int RuntimeLibraryEnabled;
int RuntimeReplacementCount;
RuntimeReplacement RuntimeReplacements[512];

unsigned HashSignatureInstruction(unsigned hash, unsigned signature,
    unsigned offset, const unsigned *instruction)
{
    FunctionRelocations &relocations = RuntimeRelocations[signature];
    bool ordinary = true;
    for (int i = 0; i < relocations.count; ++i) {
        if (relocations.offsets[i] == offset) {
            unsigned word = *instruction;
            unsigned opcode = word >> 26;
            ordinary = false;
            if (opcode == 3 || opcode == 2)
                hash ^= opcode;
            else if (opcode == 15) {
                unsigned immediate = word & 0xffff;
                if (immediate >= 0xa400 && immediate <= 0xafff)
                    hash += word;
                else
                    hash ^= word >> 16;
            } else if (opcode == 13 || opcode == 35)
                hash ^= word >> 16;
            else
                hash += word >> 16;
        }
    }
    if (ordinary) {
        if (*instruction)
            hash += *instruction;
        else
            hash ^= 0xdeadbeef;
    }
    return hash;
}

void SaveFunctionSignatures(unsigned *start)
{
    char filename[260];
    char status[256];
    unsigned *lastMatch = 0;
    if (start < (unsigned *)RDRAM)
        start = (unsigned *)RDRAM;
    strcpy(filename, ApplicationDirectory);
    if (filename[strlen(filename) - 1] == '\\')
        strcat(filename, "cod\\");
    else
        strcat(filename, "\\cod\\");
    strcat(filename, CurrentRom.FileName);
    ReplaceExtension(filename, ".cod");
    FILE *file = fopen(filename, "wb");
    if (!file)
        return;
    unsigned *candidate = start;
    unsigned signature = 0;
    for (;;) {
        FunctionSignature &entry = RuntimeSignatures[signature];
        if (!entry.name)
            break;
        if (!*candidate) {
            do {
                ++candidate;
                if (candidate >= start + 0x40000)
                    break;
            } while (!*candidate);
        }
        int length = entry.length;
        if (length > 256)
            length = 256;
        if ((unsigned char *)candidate + length < (unsigned char *)(start + 0x40000)) {
            unsigned hash = 0;
            unsigned *instruction = candidate;
            if ((*candidate >> 16) == entry.firstWord) {
                for (int offset = 0; offset < length; offset += 4)
                    hash = HashSignatureInstruction(hash, signature, offset, instruction++);
            }
            if (hash == entry.hash) {
                fprintf(file, "0x%X,%s\r\n", (unsigned)candidate - (unsigned)RDRAM - 0x80000000,
                    entry.name);
                fflush(file);
                if (lastMatch) lastMatch = instruction;
                else lastMatch = start;
                unsigned address = ((unsigned)candidate - (unsigned)RDRAM) | 0x80000000;
                RuntimeReplacements[signature].function = entry.replacement;
                RuntimeReplacements[signature].address = address;
                sprintf(status, "Searching... 0x%X, %s", address, entry.name);
                SetStatusText(0, status);
                candidate = start;
                RuntimeReplacementCount = ++signature;
                if (signature == 512)
                    break;
            } else
                ++candidate;
        } else {
            RuntimeReplacements[signature].address = 0;
            RuntimeReplacements[signature].function = 0;
            RuntimeReplacementCount = ++signature;
            candidate = start;
        }
    }
    fclose(file);
    SetStatusText(0, "Nemulating...");
}

void ResetRuntimeReplacements()
{
    RuntimeLibraryEnabled = 1;
    for (int i = 0; i < 512; ++i) {
        RuntimeReplacements[i].address = 0;
        RuntimeReplacements[i].function = 0;
    }
}

void (*FindRuntimeReplacement(unsigned address))()
{
    for (int i = 0; i < RuntimeReplacementCount; ++i)
        if ((RuntimeReplacements[i].address & 0xffffff) == (address & 0xffffff))
            return RuntimeReplacements[i].function;
    return 0;
}

void EnablePIFSuccess()
{
    ForcePIFSuccess = 1;
}


void DynarecNoOperation()
{
}

unsigned char Dynarec_CompiledPages[2048];
unsigned Dynarec_ResetRequired;
unsigned Dynarec_ProfileStartLow, Dynarec_ProfileStartHigh;

void BeginBlockTiming(unsigned *low, unsigned *high)
{
    EmitPush(X86_EAX);
    EmitPush(X86_EDX);
    EmitReadTimestamp();
    EmitStore(1, X86_EAX, 5, (unsigned)low);
    EmitStore(1, X86_EDX, 5, (unsigned)high);
    EmitPop(X86_EDX);
    EmitPop(X86_EAX);
}

void EndBlockTiming(unsigned *low, unsigned *high, unsigned *total)
{
    EmitPush(X86_EAX);
    EmitPush(X86_EDX);
    EmitPush(X86_ECX);
    EmitPush(X86_EBX);
    EmitReadTimestamp();
    EmitLoad(1, X86_ECX, 5, (unsigned)low);
    EmitLoad(1, X86_EBX, 5, (unsigned)high);
    EmitSubtract(1, X86_EAX, X86_ECX);
    EmitSbb(1, X86_EDX, X86_EBX);
    EmitDoubleShiftRight(X86_EAX, X86_EDX, 10);
    EmitShiftRight(1, X86_EDX, 10);
    EmitLoad(1, X86_ECX, 5, (unsigned)total);
    EmitAdd(1, X86_EAX, X86_ECX);
    EmitStore(1, X86_EAX, 5, (unsigned)total);
    EmitPop(X86_EBX);
    EmitPop(X86_ECX);
    EmitPop(X86_EDX);
    EmitPop(X86_EAX);
}

void LoadGPRWord(unsigned reg, unsigned char high, unsigned char destination)
{
    unsigned offset = high ? 4 : 0;
    if (!reg)
        EmitMoveImmediate(1, destination, 0);
    else if (reg == 29 && !high)
        EmitMove(1, destination, X86_EDI);
    else
        EmitLoad(1, destination, 5, (unsigned)&CPU_GPR[reg] + offset);
    if (RegisterStatsEnabled)
        EmitAddMemory(1, 1, 5, (unsigned)&RegisterStats[reg], 1);
}

void StoreGPRWord(unsigned reg, unsigned char high, unsigned char source)
{
    unsigned offset = high ? 4 : 0;
    if (reg) {
        if (reg == 29 && !high)
            EmitMove(1, X86_EDI, source);
        else
            EmitStore(1, source, 5, (unsigned)&CPU_GPR[reg] + offset);
    }
    if (RegisterStatsEnabled)
        EmitAddMemory(1, 1, 5, (unsigned)&RegisterStats[reg], 1);
}

void LoadGPRLow(unsigned reg)
{
    LoadGPRWord(reg, 0, X86_EAX);
}

void LoadGPRPairLow(unsigned left, unsigned right)
{
    LoadGPRWord(left, 0, X86_EAX);
    LoadGPRWord(right, 0, X86_ECX);
}

void LoadGPR64(unsigned reg)
{
    LoadGPRWord(reg, 0, X86_EAX);
    LoadGPRWord(reg, 1, X86_EDX);
}

void LoadTwoGPR64(unsigned left, unsigned right)
{
    LoadGPRWord(left, 0, X86_EAX);
    LoadGPRWord(left, 1, X86_EDX);
    LoadGPRWord(right, 0, X86_ECX);
    LoadGPRWord(right, 1, X86_EBX);
}

void StoreGPRSigned32(unsigned reg)
{
    StoreGPRWord(reg, 0, X86_EAX);
    EmitSignExtend(X86_EAX, X86_EAX);
    StoreGPRWord(reg, 1, X86_EAX);
}

void StoreGPRSigned32From(unsigned reg, unsigned char source)
{
    StoreGPRWord(reg, 0, source);
    EmitSignExtend(source, source);
    StoreGPRWord(reg, 1, source);
}

void StoreGPR64(unsigned char reg)
{
    StoreGPRWord(reg, 0, X86_EAX);
    StoreGPRWord(reg, 1, X86_EDX);
}

void EmitWatchHitCheck()
{
    if (MemoryBreakpointCount && !Dynarec_BranchPending) {
        EmitLoad(1, X86_EAX, 5, (unsigned)&MemoryBreakHit);
        EmitCompareImmediate(1, X86_EAX, 1);
        unsigned skip = EmitConditionalJump(5);
        EmitStoreImmediate(1, 5, (unsigned)&MemoryBreakHit, 0);
        EmitLoad(1, X86_EAX, 5, (unsigned)&CPU_ActionFlags);
        EmitOrAccumulator(1, 0x10000);
        EmitStore(1, X86_EAX, 5, (unsigned)&CPU_ActionFlags);
        EmitStoreImmediate(1, 5, (unsigned)&CPU_PC,
            Dynarec_DelaySlot ? CPU_LastPC - 4 : CPU_LastPC);
        FinishInstruction();
        EmitReturn();
        EmitLabel(skip);
    }
}

void EmitWatchCheck(unsigned flags, unsigned char doubleword)
{
    if (!MemoryBreakpointCount || Dynarec_BranchPending)
        return;
    EmitPush(X86_EBX);
    EmitMove(1, X86_EBX, X86_EAX);
    EmitAndImmediate(1, X86_EBX, 0xfffffffc);
    for (unsigned i = 0; i < MemoryBreakpointCount; ++i) {
        if (MemoryBreakpoints[i].Access & flags) {
            EmitCompareImmediate(1, X86_EBX, MemoryBreakpoints[i].Address);
            unsigned skip = EmitConditionalJump(5);
            EmitLoad(1, X86_EAX, 5, (unsigned)&CPU_ActionFlags);
            EmitOrAccumulator(1, 0x10000);
            EmitStore(1, X86_EAX, 5, (unsigned)&CPU_ActionFlags);
            EmitStoreImmediate(1, 5, (unsigned)&CPU_PC,
                Dynarec_DelaySlot ? CPU_LastPC - 4 : CPU_LastPC);
            FinishInstruction();
            EmitPop(X86_EBX);
            EmitReturn();
            EmitLabel(skip);
            if (doubleword) {
                EmitCompareImmediate(1, X86_EBX, MemoryBreakpoints[i].Address - 4);
                skip = EmitConditionalJump(5);
                EmitLoad(1, X86_EAX, 5, (unsigned)&CPU_ActionFlags);
                EmitOrAccumulator(1, 0x10000);
                EmitStore(1, X86_EAX, 5, (unsigned)&CPU_ActionFlags);
                EmitStoreImmediate(1, 5, (unsigned)&CPU_PC,
                    Dynarec_DelaySlot ? CPU_LastPC - 4 : CPU_LastPC);
                FinishInstruction();
                EmitPop(X86_EBX);
                EmitReturn();
                EmitLabel(skip);
            }
        }
    }
    EmitPop(X86_EBX);
}

bool RequiresMemoryCheck()
{
    unsigned pc = CPU_LastPC;
    if (CPU_DisableExceptions)
        return true;
    if (Dynarec_BranchPending)
        return true;
    unsigned physical = (pc & 0xc0000000) == 0x80000000
        ? pc & 0x1fffffff : TranslateTLB(pc, 0);
    if (physical >= RDRAMSize)
        return true;
    if (Dynarec_DelaySlot)
        pc -= 4;
    physical = (pc & 0xc0000000) == 0x80000000
        ? pc & 0x1fffffff : TranslateTLB(pc, 0);
    return FaultingInstructions[physical >> 2] != 0;
}

unsigned CheckInlineFunction(unsigned address)
{
    unsigned savedInstruction = CPU_Instruction;
    if (!Dynarec_Inlining)
        return unsigned(-1);
    unsigned pc = address;
    unsigned physical = (address & 0xc0000000) == 0x80000000
        ? address & 0x1fffffff : TranslateTLB(address, 0);
    if ((physical & 0xfffff000) != Dynarec_PageAddress || Dynarec_BranchPending)
        return unsigned(-1);
    unsigned count = 0;
    unsigned result = unsigned(-1);
    do {
        physical = (pc & 0xc0000000) == 0x80000000
            ? pc & 0x1fffffff : TranslateTLB(pc, 0);
        if ((physical & 0xfffff000) != Dynarec_PageAddress) {
            CPU_Instruction = savedInstruction;
            return unsigned(-1);
        }
        ++count;
        physical = (pc & 0xc0000000) == 0x80000000
            ? pc & 0x1fffffff : TranslateTLB(pc, 0);
        CPU_Instruction = ReadPhysicalWord(physical);
        if ((CPU_Instruction & 0xfc000000) == 0xec000000 &&
            (Dynarec_Mode == 1 || Dynarec_Mode == 2)) {
            for (int i = CPUBreakpointCount - 1; i != -1; --i) {
                physical = (pc & 0xc0000000) == 0x80000000
                    ? pc & 0x1fffffff : TranslateTLB(pc, 0);
                if (CPUBreakpoints[i].Address == physical) {
                    CPU_Instruction = CPUBreakpoints[i].Instruction;
                    if ((CPU_Instruction & 0xfc000000) == 0xec000000)
                        ReportCPUError("Compiler error 2 in CheckInlineFunction");
                    break;
                }
            }
        }
        unsigned opcode = CPU_Instruction >> 26;
        unsigned function = CPU_Instruction & 63;
        if (!opcode && function == 8 && (CPU_Instruction & 0x03e00000) == 0x03e00000) {
            if (address < pc)
                result = (pc - address) >> 2;
            CPU_Instruction = savedInstruction;
            return result;
        }
        if (opcode == 4 || opcode == 5 || opcode == 20 || opcode == 21 ||
            opcode == 6 || opcode == 7 || opcode == 22 || opcode == 23 ||
            opcode == 1 || opcode == 2 || opcode == 3 ||
            (!opcode && (function == 8 || function == 9)) ||
            (opcode == 16 && (CPU_Instruction & 0x03e00000) == 0x02000000 && function == 24)) {
            CPU_Instruction = savedInstruction;
            return unsigned(-1);
        }
        pc += 4;
        if (count > 9) {
            CPU_Instruction = savedInstruction;
            return unsigned(-1);
        }
    } while (true);
}

void EmitTranslateAddress(unsigned char writing)
{
    unsigned done;
    if (TLBEnabled && (TLBRamMapped || RequiresMemoryCheck())) {
        EmitMove(1, X86_ECX, X86_EAX);
        EmitAndImmediate(1, X86_ECX, 0xc0000000);
        EmitCompareImmediate(1, X86_ECX, 0x80000000);
        unsigned direct = EmitConditionalJump(4);
        EmitMoveImmediate(1, X86_ECX, writing);
        EmitPush(X86_ECX);
        EmitPush(X86_EAX);
        EmitCall((unsigned)TranslateTLB);
        EmitCompareImmediate(1, X86_EAX, 0x800001);
        done = EmitConditionalJump(5);
        EmitOrMemory(1, 5, (unsigned)&CPU_ActionFlags, 0x40000);
        EmitStoreImmediate(1, 5, (unsigned)&CPU_PC,
            Dynarec_DelaySlot ? CPU_LastPC - 4 : CPU_LastPC);
        EmitStoreImmediate(1, 5, (unsigned)&CPU_TLBExceptionDelay,
            Dynarec_DelaySlot ? 1 : 0);
        EmitReturn();
        EmitLabel(direct);
    }
    EmitAndImmediate(1, X86_EAX, 0x1fffffff);
    if (TLBEnabled && (TLBRamMapped || RequiresMemoryCheck()))
        EmitLabel(done);
}

void EmitBranchTarget(unsigned address)
{
    unsigned physical = (address & 0xc0000000) == 0x80000000
        ? address & 0x1fffffff : TranslateTLB(address, 0);
    if ((physical & 0xfffff000) == Dynarec_PageAddress && !Dynarec_BranchPending &&
        !(Dynarec_PageAddress >= 0x04001000 && Dynarec_PageAddress < 0x04002000) &&
        !(Dynarec_PageAddress >= 0x1fc00000 && Dynarec_PageAddress < 0x1fc007bf)) {
        physical = (address & 0xc0000000) == 0x80000000
            ? address & 0x1fffffff : TranslateTLB(address, 0);
        unsigned offset = Dynarec_PageCode[(physical - Dynarec_PageAddress) >> 2];
        FinishInstruction();
        if (offset) {
            EmitConditionalJumpOffset(15, offset);
            EmitStoreImmediate(1, 5, (unsigned)&CPU_PC, address);
            EmitReturn();
        } else {
            EmitJumpGuest(address);
            AddPendingAddress(address);
        }
    } else {
        FinishInstruction();
        EmitStoreImmediate(1, 5, (unsigned)&CPU_PC, address);
        EmitReturn();
    }
}

void EmitProgramCounter()
{
    EmitStore(1, X86_EAX, 5, (unsigned)&CPU_PC);
}

void EmitLinkRegister(unsigned char reg)
{
    EmitStoreImmediate(1, 5, (unsigned)&CPU_GPR[reg], CPU_LastPC + 8);
    EmitStoreImmediate(1, 5, (unsigned)&CPU_GPR[reg] + 4,
        (int)(CPU_LastPC + 8) < 0 ? unsigned(-1) : 0);
}

void StoreMultiplyResult()
{
    EmitStore(1, X86_EAX, 5, (unsigned)&CPU_LO);
    EmitStore(1, X86_EDX, 5, (unsigned)&CPU_HI);
    EmitArithmeticShiftRight(1, X86_EAX, 31);
    EmitArithmeticShiftRight(1, X86_EDX, 31);
    EmitStore(1, X86_EAX, 5, (unsigned)&CPU_LO + 4);
    EmitStore(1, X86_EDX, 5, (unsigned)&CPU_HI + 4);
}

void FinishInstruction()
{
    if (BlockStatsEnabled && Dynarec_PageAddress < RDRAMSize)
        EndBlockTiming(&Dynarec_ProfileStartLow, &Dynarec_ProfileStartHigh,
            &BlockCycleCounts[Dynarec_BlockStart >> 2]);
    int cycles = (int)-(double)Dynarec_BlockInstructionCount;
    if (!cycles)
        cycles = -1;
    EmitAddImmediate(X86_ESI, cycles);
}

static void TrackWord()
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

static void TrackDoubleWord()
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

static void TrackGPR(unsigned char reg)
{
    LoadGPR64(reg);
    TrackDoubleWord();
}

static void TrackFPR(unsigned reg)
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

static void TrackRegister64(const CpuS64 *reg)
{
    EmitLoad(1, X86_EAX, 5, (unsigned)reg);
    EmitLoad(1, X86_EDX, 5, (unsigned)reg + 4);
    TrackDoubleWord();
}

static void TrackRegister32(const unsigned *reg)
{
    EmitLoad(1, X86_EAX, 5, (unsigned)reg);
    TrackWord();
}

static void TrackMemory()
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

static void TrackProgramCounter()
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
            TrackProgramCounter();
            break;
        case 8: case 12: case 13: case 15: case 48: case 49:
        case 50: case 51: case 52: case 54:
            TrackProgramCounter();
            break;
        case 19:
            TrackRegister64(&CPU_LO);
            break;
        case 24: case 25: case 26: case 27: case 28: case 29:
        case 30: case 31:
            TrackRegister64(&CPU_LO);
            TrackRegister64(&CPU_HI);
            break;
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
    case 3:
        TrackGPR(31);
        TrackProgramCounter();
        break;
    case 2: case 4: case 5: case 6: case 7:
    case 20: case 21: case 22: case 23:
        TrackProgramCounter();
        break;
    case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15:
    case 24: case 25: case 26: case 27: case 32: case 33: case 34: case 35:
    case 36: case 37: case 38: case 39: case 55:
        TrackGPR((unsigned char)((CPU_Instruction >> 16) & 31));
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
            if ((CPU_Instruction & 63) != 24)
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
    case 40: case 41: case 42: case 43: case 44: case 45: case 46:
    case 57: case 61: case 63:
        TrackMemory();
        break;
    case 49: case 53:
        TrackFPR((CPU_Instruction >> 16) & 31);
        break;
    }
    if (Dynarec_TrackingLoaded) {
        EmitStoreImmediate(1, 0x81, (unsigned)UndoData, CPU_Instruction);
        EmitAddImmediate(X86_ECX, 4);
        EmitAndImmediate(1, X86_ECX, 0x3ffffc);
        EmitStore(1, X86_ECX, 5, (unsigned)&UndoPosition);
    }
}

void RecompileEqualityBranch(unsigned char likely, unsigned char equal)
{
    LoadGPRPairLow((CPU_Instruction >> 21) & 31, (CPU_Instruction >> 16) & 31);
    EmitCompare(1, X86_ECX, X86_EAX);
    unsigned lowDifferent = EmitConditionalJump(5);
    LoadGPRWord((CPU_Instruction >> 21) & 31, 1, X86_EAX);
    LoadGPRWord((CPU_Instruction >> 16) & 31, 1, X86_ECX);
    EmitCompare(1, X86_ECX, X86_EAX);
    unsigned highDifferent = EmitConditionalJump(5);
    if (equal) {
        RecompileDelaySlot();
        EmitBranchTarget(CPU_LastPC + (short)CPU_Instruction * 4 + 4);
        EmitLabel(highDifferent);
        EmitLabel(lowDifferent);
        if (likely)
            EmitBranchTarget(CPU_LastPC + 8);
        else
            EmitBranchTarget(CPU_LastPC + 4);
    } else {
        if (Dynarec_ResetRequired) {
            EmitCall((unsigned)ResetRecompiler);
            EmitStoreImmediate(1, 5, (unsigned)&CPU_PC, CPU_LastPC + (likely ? 8 : 4));
            FinishInstruction();
            EmitReturn();
        }
        EmitBranchTarget(CPU_LastPC + (likely ? 8 : 4));
        EmitLabel(highDifferent);
        EmitLabel(lowDifferent);
        RecompileDelaySlot();
        EmitBranchTarget(CPU_LastPC + (short)CPU_Instruction * 4 + 4);
    }
    Dynarec_BlockFinished = 1;
}

void RecompileSignedBranch(unsigned char condition, unsigned char likely, unsigned char link)
{
    if ((unsigned short)CPU_Instruction == 0xffff)
        Dynarec_BlockInstructionCount += 100;
    unsigned char lowCondition;
    unsigned char testLow = link;
    switch (condition) {
    case 12: lowCondition = 2; testLow = 0; break;
    case 13: lowCondition = 3; testLow = 0; break;
    case 14: lowCondition = 6; testLow = 1; break;
    case 15: lowCondition = 7; testLow = 1; break;
    }
    if (link)
        EmitLinkRegister(31);
    LoadGPRWord((CPU_Instruction >> 21) & 31, 1, X86_EDX);
    EmitCompareImmediate(1, X86_EDX, 0);
    unsigned taken;
    if (testLow) {
        unsigned highNonzero = EmitConditionalJump(5);
        LoadGPRLow((CPU_Instruction >> 21) & 31);
        EmitCompareImmediate(1, X86_EAX, 0);
        unsigned lowTaken = EmitConditionalJump(lowCondition);
        unsigned skipHigh = EmitJump();
        EmitLabel(highNonzero);
        taken = EmitConditionalJump(condition);
        EmitLabel(skipHigh);
        if (likely)
            EmitBranchTarget(CPU_LastPC + 8);
        else
            EmitBranchTarget(CPU_LastPC + 4);
        EmitLabel(lowTaken);
    } else {
        taken = EmitConditionalJump(condition);
        EmitBranchTarget(CPU_LastPC + (likely ? 8 : 4));
    }
    EmitLabel(taken);
    RecompileDelaySlot();
    EmitBranchTarget(CPU_LastPC + (short)CPU_Instruction * 4 + 4);
    Dynarec_BlockFinished = 1;
}

void RecompileDelaySlot()
{
    if (Dynarec_DelaySlot)
        return;
    unsigned savedInstruction = CPU_Instruction;
    unsigned savedPC = CPU_LastPC;
    CPU_LastPC += 4;
    Dynarec_DelaySlot = 1;
    unsigned physical = (CPU_LastPC & 0xc0000000) == 0x80000000
        ? CPU_LastPC & 0x1fffffff : TranslateTLB(CPU_LastPC, 0);
    if (physical == 0x800001) {
        EmitOrMemory(1, 5, (unsigned)&CPU_ActionFlags, 0x40000);
        EmitStoreImmediate(1, 5, (unsigned)&CPU_TLBExceptionDelay, 1);
        EmitStoreImmediate(1, 5, (unsigned)&CPU_PC, CPU_LastPC - 4);
        EmitStoreImmediate(1, 5, (unsigned)&TLBAddress, TLBAddress);
        EmitStoreImmediate(1, 5, (unsigned)&TLBWriteAccess, TLBWriteAccess);
        EmitStoreImmediate(1, 5, (unsigned)&TranslationError, TranslationError);
        unsigned address = CPU_LastPC - 4;
        physical = (address & 0xc0000000) == 0x80000000
            ? address & 0x1fffffff : TranslateTLB(address, 0);
        EmitMoveImmediate(1, X86_EAX, physical & 0xfffff000);
        EmitPush(X86_EAX);
        if (Dynarec_Mode == 0 || Dynarec_Mode == 3 || Dynarec_Mode == 4 || Dynarec_Mode == 5)
            EmitCall((unsigned)InvalidateRecompiledPage);
        else if (Dynarec_Mode == 1 || Dynarec_Mode == 2)
            EmitCall((unsigned)RemovePageBreakpoints);
        FinishInstruction();
        EmitReturn();
    } else {
        CPU_Instruction = ReadPhysicalWord(physical);
        NotifyTools(0x100, CPU_Instruction);
        if (((CPU_ActionFlags & 0x100000) || IsOpcodeBreakpoint(CPU_Instruction)) &&
            !Dynarec_BranchPending) {
            EmitLoad(1, X86_EAX, 5, (unsigned)&CPU_ActionFlags);
            EmitOrAccumulator(1, 0x10000);
            EmitStore(1, X86_EAX, 5, (unsigned)&CPU_ActionFlags);
            EmitStoreImmediate(1, 5, (unsigned)&CPU_PC, CPU_LastPC - 4);
            FinishInstruction();
            EmitReturn();
        }
        if (OpcodeStatsEnabled)
            EmitAddMemory(1, 0, 5, (unsigned)&OpcodeStats[CPU_Instruction >> 26], 1);
        Dynarec_MainDispatch[CPU_Instruction >> 26]();
    }
    Dynarec_DelaySlot = 0;
    ++Dynarec_BlockInstructionCount;
    CPU_LastPC = savedPC;
    CPU_Instruction = savedInstruction;
}

void CompileInstructions()
{
    Dynarec_FpuChecked = 0;
    do {
        unsigned instruction = ReadPhysicalWord(Dynarec_PhysicalPC);
        CPU_Instruction = instruction;
        for (unsigned i = 0; i < ToolCount; ++i)
            if ((ToolCatalog[i].Flags & 0x100) && ToolCatalog[i].Invoke)
                ToolCatalog[i].Invoke(0x100, instruction);
        if ((CPU_ActionFlags & 0x100000) || IsOpcodeBreakpoint(CPU_Instruction)) {
            CPU_ActionFlags &= ~0x100000;
            EmitLoad(1, X86_EAX, 5, (unsigned)&CPU_ActionFlags);
            EmitOrAccumulator(1, 0x10000);
            EmitStore(1, X86_EAX, 5, (unsigned)&CPU_ActionFlags);
            EmitStoreImmediate(1, 5, (unsigned)&CPU_PC, CPU_LastPC);
            FinishInstruction();
            EmitReturn();
        }
        if (UndoEnabled)
            TrackInstructionChanges();
        if (OpcodeStatsEnabled)
            EmitAddMemory(1, 0, 5, (unsigned)&OpcodeStats[CPU_Instruction >> 26], 1);
        Dynarec_MainDispatch[CPU_Instruction >> 26]();
        ++Dynarec_BlockInstructionCount;
        CPU_LastPC += 4;
        if (Dynarec_NextPC) {
            CPU_LastPC = Dynarec_NextPC;
            Dynarec_NextPC = 0;
        }
        Dynarec_PhysicalPC = (CPU_LastPC & 0xc0000000) == 0x80000000
            ? CPU_LastPC & 0x1fffffff : TranslateTLB(CPU_LastPC, 0);
        if (Dynarec_BlockFinished)
            return;
        if (Dynarec_PhysicalPC == Dynarec_PageAddress + 0x1000 || Dynarec_PhysicalPC == 0x800001) {
            FinishInstruction();
            EmitBranchTarget(CPU_LastPC);
            return;
        }
    } while (true);
}

void CompileBlock()
{
    Dynarec_InlineAddressCount = 0;
    Dynarec_NextPC = 0;
    Dynarec_BranchPending = 0;
    if ((unsigned)CPU_PC == RomEntryPoint) {
        InvalidateRecompiledRange(0x318, 4);
        WritePhysicalWord(0x318, RDRAMSize);
    }
    unsigned physical = ((unsigned)CPU_PC & 0xc0000000) == 0x80000000
        ? (unsigned)CPU_PC & 0x1fffffff : TranslateTLB(CPU_PC, 0);
    Dynarec_PageAddress = physical & 0xfffff000;
    if (Dynarec_PageAddress < RDRAMSize)
        Dynarec_CompiledPages[physical >> 12] = 1;
    if (ROMCacheEnabled && Dynarec_PageAddress >= 0x10000000 && Dynarec_PageAddress <= 0x1fbfffff) {
        unsigned offset = Dynarec_PageAddress - 0x10000000;
        if (offset > 0x03ffffff)
            offset = Dynarec_PageAddress - 0x13fcb4d0;
        memcpy(Dynarec_PageCode, ROMCache + offset, 4096);
    } else {
        switch (Dynarec_Mode) {
        case 0: case 3: case 4: case 5:
            if (Dynarec_PageAddress < RDRAMSize)
                memcpy(Dynarec_PageCode, &Dynarec_BlockCode[Dynarec_PageAddress >> 2], 4096);
            else
                memset(Dynarec_PageCode, 0, 4096);
            break;
        case 1: case 2:
            if (Dynarec_PageAddress < RDRAMSize) {
                unsigned end = Dynarec_PageAddress + 0x1000;
                unsigned index = 0;
                for (unsigned address = Dynarec_PageAddress; address < end; address += 4) {
                    unsigned instruction = *(unsigned *)(RDRAM + address);
                    Dynarec_PageCode[index++] = (instruction & 0xfc000000) == 0xec000000
                        ? instruction & 0x03ffffff : 0;
                }
            } else
                memset(Dynarec_PageCode, 0, 4096);
            break;
        }
    }
    unsigned firstOffset = unsigned(-1);
    Dynarec_PendingCount = 0;
    Dynarec_JumpCount = 0;
    AddPendingAddress(CPU_PC);
    Dynarec_LastWasGuestJump = 0;
    do {
        Dynarec_ResetRequired = 0;
        ResetInstructionBuffer(1);
        if (16 - (Dynarec_MainCodeUsed & 15) < 8)
            Dynarec_MainCodeUsed += 16 - (Dynarec_MainCodeUsed & 15);
        CPU_LastPC = Dynarec_PendingAddresses[--Dynarec_PendingCount];
        physical = (CPU_LastPC & 0xc0000000) == 0x80000000
            ? CPU_LastPC & 0x1fffffff : TranslateTLB(CPU_LastPC, 0);
        Dynarec_PhysicalPC = physical;
        Dynarec_BlockStart = physical;
        Dynarec_BlockFinished = 0;
        Dynarec_BlockInstructionCount = 0;
        if (firstOffset == unsigned(-1)) {
            firstOffset = Dynarec_PageCode[(physical >> 2) & 0x3ff];
            if (firstOffset) {
                Dynarec_BlockFinished = 1;
                continue;
            }
            Dynarec_PageCode[(physical - Dynarec_PageAddress) >> 2] = Dynarec_MainCodeUsed;
            firstOffset = Dynarec_MainCodeUsed;
        } else
            Dynarec_PageCode[(physical - Dynarec_PageAddress) >> 2] = Dynarec_MainCodeUsed;
        if (BlockStatsEnabled && physical < RDRAMSize) {
            BeginBlockTiming(&Dynarec_ProfileStartLow, &Dynarec_ProfileStartHigh);
            EmitAddMemory(1, 1, 5, (unsigned)&BlockExecutionCounts[Dynarec_BlockStart >> 2], 1);
        }
        CPU_Instruction = ReadPhysicalWord(Dynarec_PhysicalPC);
        if ((Dynarec_Mode == 1 || Dynarec_Mode == 2) &&
            (CPU_Instruction & 0xfc000000) != 0xec000000) {
            CPUBreakpoints[CPUBreakpointCount].Instruction = CPU_Instruction;
            CPUBreakpoints[CPUBreakpointCount].Address = Dynarec_PhysicalPC;
            CPUBreakpoints[CPUBreakpointCount].CodeOffset = Dynarec_MainCodeUsed;
            ++CPUBreakpointCount;
            AllowRomWrites = true;
            WritePhysicalWord(Dynarec_PhysicalPC, Dynarec_MainCodeUsed | 0xec000000);
            AllowRomWrites = false;
        }
        CompileInstructions();
        AssembleInstructions(0);
    } while (Dynarec_PendingCount);
    unsigned endOffset = Dynarec_MainCodeUsed;
    while (Dynarec_JumpCount) {
        DynarecJumpFixup &jump = Dynarec_JumpFixups[--Dynarec_JumpCount];
        if (jump.address != unsigned(-1)) {
            Dynarec_MainCodeUsed = jump.offset;
            ResetInstructionBuffer(0);
            EmitJumpOffset(Dynarec_PageCode[(jump.address >> 2) & 0x3ff]);
            AssembleInstructions(0);
            Dynarec_MainCodeUsed = endOffset;
        }
    }
    if (Dynarec_Mode == 3 && Dynarec_PageAddress < RDRAMSize) {
        DWORD protection;
        VirtualProtect(RDRAM + Dynarec_PageAddress, 0xfff, PAGE_READONLY, &protection);
    }
    if (Dynarec_MainCodeUsed > 0x900000)
        CPU_ActionFlags |= 0x400;
    if (ROMCacheEnabled && Dynarec_PageAddress >= 0x10000000 && Dynarec_PageAddress <= 0x1fbfffff) {
        unsigned offset = Dynarec_PageAddress - 0x10000000;
        if (offset >= 0x04000000)
            offset = Dynarec_PageAddress - 0x13fcb4d0;
        memcpy(ROMCache + (offset & ~3), Dynarec_PageCode, 4096);
    } else if ((Dynarec_Mode == 0 || (Dynarec_Mode >= 3 && Dynarec_Mode <= 5)) &&
        Dynarec_PageAddress < RDRAMSize)
        memcpy(&Dynarec_BlockCode[Dynarec_PageAddress >> 2], Dynarec_PageCode, 4096);
    Dynarec_EntryPoint = Dynarec_MainCode + firstOffset;
}
