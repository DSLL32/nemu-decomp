#ifndef NEMU64_DYNAREC_H
#define NEMU64_DYNAREC_H

#include <stdio.h>

// Private names are inferred. Original addresses and register-passed arguments
// are recorded separately in analysis/nemu-dynarec.json.
struct RecompiledInstruction {
    unsigned char *code;
    unsigned opcode;
    int destination, destination2;
    int source, source2, source3;
    unsigned immediate, address;
    int condition, mode, target, label;
    unsigned guestPC;
    unsigned char size, longJump, readsFlags, writesFlags;
    unsigned reserved;
};
struct DynarecJumpFixup { unsigned offset, address; };
struct DynarecAddressMap { unsigned physical, offset; };
struct RuntimeReplacement { unsigned address; void (*function)(); };
struct FunctionSignature {
    const char *name;
    unsigned length, hash, firstWord;
    void (*replacement)();
};
struct FunctionRelocations { int count; unsigned short offsets[256]; };

enum X86Register { X86_EAX, X86_ECX, X86_EDX, X86_EBX,
                   X86_ESP, X86_EBP, X86_ESI, X86_EDI };

extern unsigned Dynarec_Mode;
extern unsigned Dynarec_Inlining;
extern unsigned Dynarec_BlockCode[0x200000];
extern unsigned char Dynarec_MainCode[0x1400000];
extern unsigned char Dynarec_FastCode[0x100000];
extern unsigned Dynarec_MainCodeUsed, Dynarec_FastCodeUsed;
extern unsigned char *Dynarec_CodePointer, *Dynarec_EntryPoint;
extern unsigned Dynarec_Use32BitMemoryAccess;
extern unsigned Dynarec_DelaySlot, Dynarec_BlockFinished;
extern unsigned Dynarec_NextPC, Dynarec_BranchPending;
extern unsigned Dynarec_InstructionCount, Dynarec_LabelCount;
extern RecompiledInstruction Dynarec_Instructions[50000];
extern unsigned Dynarec_LabelInstructions[10000];
extern unsigned Dynarec_JumpCount, Dynarec_PendingCount;
extern DynarecJumpFixup Dynarec_JumpFixups[100000];
extern unsigned Dynarec_PendingAddresses[10000];
extern unsigned Dynarec_ConversionCount;
extern DynarecAddressMap Dynarec_AddressMap[500000];
extern unsigned char Dynarec_RecordAddresses;
extern unsigned Dynarec_LastWasGuestJump;
extern unsigned Dynarec_InlineAddressCount;
extern unsigned Dynarec_InlineAddresses[1024];
extern unsigned Dynarec_PageCode[1024];
extern unsigned Dynarec_PageAddress, Dynarec_PhysicalPC;
extern unsigned Dynarec_BlockStart, Dynarec_BlockInstructionCount;
extern unsigned char Dynarec_TrackingLoaded;

typedef void (*DynarecOpcodeHandler)();
extern DynarecOpcodeHandler Dynarec_MainDispatch[64];
extern DynarecOpcodeHandler Dynarec_SpecialDispatch[64];
extern DynarecOpcodeHandler Dynarec_RegImmDispatch[32];
extern DynarecOpcodeHandler Dynarec_Cop0Dispatch[32];
extern DynarecOpcodeHandler Dynarec_Cop0FunctionDispatch[64];
extern DynarecOpcodeHandler Dynarec_Cop1Dispatch[32];
extern DynarecOpcodeHandler Dynarec_Cop1FunctionDispatch[64];
extern unsigned char Dynarec_CompiledPages[2048];
extern unsigned Dynarec_ResetRequired;
extern unsigned Dynarec_ProfileStartLow, Dynarec_ProfileStartHigh;
void CompileBlock();
void CompileInstructions();
unsigned RequiresMemoryCheck();
unsigned CheckInlineFunction(unsigned address);
void BeginBlockTiming(unsigned *low, unsigned *high);
void EndBlockTiming(unsigned *low, unsigned *high, unsigned *total);
void EmitWatchHitCheck();
void EmitWatchCheck(unsigned flags, unsigned char doubleword);
void EmitTranslateAddress(unsigned char writing);
void RecompileDelaySlot();
void EmitBranchTarget(unsigned address);
void EmitProgramCounter();
void EmitLinkRegister(unsigned char reg);
void StoreMultiplyResult();
void RecompileEqualityBranch(unsigned char likely, unsigned char equal);
void RecompileSignedBranch(unsigned char condition, unsigned char likely, unsigned char link);
void TrackInstructionChanges();
void FinishInstruction();
void DynarecNoOperation();
void LoadGPRWord(unsigned reg, unsigned char high, unsigned destination);
void StoreGPRWord(unsigned reg, unsigned char high, unsigned source);
void LoadGPRLow(unsigned reg);
void LoadGPRPairLow(unsigned left, unsigned right);
void LoadGPR64(unsigned reg);
void LoadTwoGPR64(unsigned left, unsigned right);
void StoreGPRSigned32(unsigned reg);
void StoreGPRSigned32From(unsigned reg, unsigned source);
void StoreGPR64(unsigned reg);

void AddJumpTarget(unsigned label);
void AddInstruction(unsigned char size, unsigned opcode, int destination,
    int source, int source2, unsigned address, unsigned immediate, int mode,
    int condition, int target, unsigned char readsFlags, unsigned char writesFlags);
void AddInstructionThreeSources(unsigned char size, unsigned opcode,
    int destination, int source, int source2, int source3, unsigned address,
    unsigned immediate, int mode, int condition, int target,
    unsigned char readsFlags, unsigned char writesFlags);
void AddInstructionTwoDestinations(unsigned char size, unsigned opcode,
    int destination, int destination2, int source, int source2, int source3,
    unsigned address, unsigned immediate, int mode, int condition, int target,
    unsigned char readsFlags, unsigned char writesFlags);
void EmitLabel(unsigned label);
void EmitAddImmediate(unsigned char destination, unsigned value);
void EmitAddMemory(unsigned char size, unsigned char addressSize, unsigned char mode, unsigned address,
    unsigned char value);
void EmitCompareImmediate(unsigned char size, unsigned char source, unsigned value);
unsigned EmitConditionalJump(unsigned char condition);
unsigned EmitJump();
void AddPendingAddress(unsigned address);
void AddJumpFixup(unsigned offset, unsigned address);
void AddAddressMapping(unsigned physical, unsigned offset);
unsigned IsOpcodeBreakpoint(unsigned instruction);
void AddInlineAddress(unsigned address);
void SetLabelInstruction(unsigned label, unsigned index);
void ResetInstructionBuffer(unsigned char recordAddresses);
void AssembleInstructions(unsigned char fast);

void EmitAdc(unsigned char size, unsigned char source, unsigned char destination);
void EmitAdcImmediate(unsigned char size, unsigned char destination, unsigned value);
void EmitAdd(unsigned char size, unsigned char source, unsigned char destination);
void EmitAndMemory(unsigned char size, unsigned char mode, unsigned address, unsigned value);
void EmitAndImmediate(unsigned char size, unsigned char destination, unsigned value);
void EmitAnd(unsigned char size, unsigned char source, unsigned char destination);
void EmitSignExtend(unsigned char destination, unsigned char source);
void EmitCompare(unsigned char size, unsigned char right, unsigned char left);
void EmitCall(unsigned address);
void EmitDivide(unsigned char size, unsigned char source);
void EmitSignedDivide(unsigned char size, unsigned char source);
void EmitSignedMultiply(unsigned char size, unsigned char source);
void EmitConditionalJumpOffset(unsigned char condition, unsigned offset);
void EmitJumpOffset(unsigned offset);
void EmitJumpGuest(unsigned address);
void EmitMove(unsigned char size, unsigned char source, unsigned char destination);
void EmitLoad(unsigned char size, unsigned char destination, unsigned char mode, unsigned address);
void EmitStore(unsigned char size, unsigned char source, unsigned char mode, unsigned address);
void EmitStoreHalf(unsigned char source, unsigned char mode, unsigned address);
void EmitMoveImmediate(unsigned char size, unsigned char destination, unsigned value);
void EmitStoreImmediate(unsigned char size, unsigned char mode, unsigned address, unsigned value);
void EmitLoadSigned(unsigned char size, unsigned char destination, unsigned char mode, unsigned address);
void EmitMoveSigned(unsigned char size, unsigned char destination, unsigned char source);
void EmitLoadUnsigned(unsigned char size, unsigned char destination, unsigned char mode, unsigned address);
void EmitMoveUnsigned(unsigned char size, unsigned char destination, unsigned char source);
void EmitMultiply(unsigned char size, unsigned char source);
void EmitNegate(unsigned char size, unsigned char destination);
void EmitNot(unsigned char size, unsigned char destination);
void EmitOrAccumulator(unsigned char size, unsigned value);
void EmitOrMemory(unsigned char size, unsigned char mode, unsigned address, unsigned value);
void EmitOr(unsigned char size, unsigned char source, unsigned char destination);
void EmitPop(unsigned char destination);
void EmitPush(unsigned char source);
void EmitReadTimestamp();
void EmitReturn();
void EmitStoreFlags();
void EmitArithmeticShiftRight(unsigned char size, unsigned char destination, unsigned char count);
void EmitArithmeticShiftRightCL(unsigned char size, unsigned char destination);
void EmitSbb(unsigned char size, unsigned char source, unsigned char destination);
void EmitSetMemory(unsigned char condition, unsigned char mode, unsigned address);
void EmitSet(unsigned char condition, unsigned char destination);
void EmitShiftLeftCL(unsigned char size, unsigned char destination);
void EmitShiftLeft(unsigned char size, unsigned char destination, unsigned char count);
void EmitDoubleShiftLeftCL(unsigned char source, unsigned char destination);
void EmitDoubleShiftLeft(unsigned char source, unsigned char destination, unsigned char count);
void EmitShiftRightCL(unsigned char size, unsigned char destination);
void EmitShiftRight(unsigned char size, unsigned char destination, unsigned char count);
void EmitDoubleShiftRightCL(unsigned char source, unsigned char destination);
void EmitDoubleShiftRight(unsigned char source, unsigned char destination, unsigned char count);
void EmitSubtract(unsigned char size, unsigned char source, unsigned char destination);
void EmitExchange(unsigned char size, unsigned char left, unsigned char right);
void EmitTestMemory(unsigned char size, unsigned char mode, unsigned address, unsigned value);
void EmitXorAccumulator(unsigned char size, unsigned value);
void EmitXor(unsigned char size, unsigned char source, unsigned char destination);
void EmitFpuChangeSign();
void EmitFpuAddSingle(unsigned address);
void EmitFpuAddDouble(unsigned address);
void EmitFpuAbsolute();
void EmitFpuClearExceptions();
void EmitFpuCompareSingle(unsigned address);
void EmitFpuCompareDouble(unsigned address);
void EmitFpuDivideSingle(unsigned address);
void EmitFpuDivideDouble(unsigned address);
void EmitFpuLoadInt32(unsigned address);
void EmitFpuLoadInt64(unsigned address);
void EmitFpuStoreInt32(unsigned address);
void EmitFpuStoreInt64(unsigned address);
void EmitFpuLoadSingle(unsigned address);
void EmitFpuLoadDouble(unsigned address);
void EmitFpuLoadControl(unsigned address);
void EmitFpuMultiplySingle(unsigned address);
void EmitFpuMultiplyDouble(unsigned address);
void EmitFpuSquareRoot();
void EmitFpuPopSingle(unsigned address);
void EmitFpuPopDouble(unsigned address);
void EmitFpuStoreStatus();
void EmitFpuSubtractSingle(unsigned address);
void EmitFpuSubtractDouble(unsigned address);
extern int RuntimeLibraryEnabled;
extern int RuntimeReplacementCount;
extern RuntimeReplacement RuntimeReplacements[512];
extern FunctionSignature RuntimeSignatures[173];
extern FunctionRelocations RuntimeRelocations[173];

unsigned HashSignatureInstruction(unsigned hash, unsigned signature,
    unsigned offset, const unsigned *instruction);
void SaveFunctionSignatures(unsigned *start);
void ResetRuntimeReplacements();
void (*FindRuntimeReplacement(unsigned address))();
void EnablePIFSuccess();

#endif
