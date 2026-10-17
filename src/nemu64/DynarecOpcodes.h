#ifndef NEMU64_DYNAREC_OPCODES_H
#define NEMU64_DYNAREC_OPCODES_H

#include "Dynarec.h"

extern unsigned CPU_FpuModeChanges;
void ReportDynarecError3();
void ReportDynarecError2();
void WarnCrackedRom();
unsigned UpdateFpuRegisterMode();
void ClampRandomRegister();
void WarnFpuModeInDelaySlot();
void WarnTlbWriteInDelaySlot();

extern DynarecOpcodeHandler Dynarec_MainDispatch[64];
extern DynarecOpcodeHandler Dynarec_SpecialDispatch[64];
extern DynarecOpcodeHandler Dynarec_RegImmDispatch[32];
extern DynarecOpcodeHandler Dynarec_Cop0Dispatch[32];
extern DynarecOpcodeHandler Dynarec_Cop0FunctionDispatch[64];
extern DynarecOpcodeHandler Dynarec_Cop1Dispatch[32];
extern DynarecOpcodeHandler Dynarec_Cop1FunctionDispatch[64];

void Recompile_Unknown(); // 0040D590
void Recompile_Breakpoint(); // 0040D5F0
void Recompile_SPECIAL(); // 0040D6B0
void Recompile_REGIMM(); // 0040D6F0
void Recompile_J(); // 0040D730
void Recompile_JAL(); // 0040D7B0
void Recompile_BEQ(); // 0040D8A0
void Recompile_BNE(); // 0040D920
void Recompile_BLEZ(); // 0040D930
void Recompile_BGTZ(); // 0040D940
void Recompile_ADDI(); // 0040D950
void Recompile_ADDIU(); // 0040D960
void Recompile_SLTI(); // 0040D9B0
void Recompile_SLTIU(); // 0040DA50
void Recompile_ANDI(); // 0040DAF0
void Recompile_ORI(); // 0040DB30
void Recompile_XORI(); // 0040DB60
void Recompile_LUI(); // 0040DB90
void Recompile_COP0(); // 0040DC00
void Recompile_COP1(); // 0040DC40
void Recompile_BEQL(); // 0040DC80
void Recompile_BNEL(); // 0040DC90
void Recompile_BLEZL(); // 0040DCA0
void Recompile_BGTZL(); // 0040DCB0
void Recompile_DADDI(); // 0040DCC0
void Recompile_LDL(); // 0040DDA0
void Recompile_LDR(); // 0040DEC0
void Recompile_LB(); // 0040DF50
void Recompile_LH(); // 0040E010
void Recompile_LWL(); // 0040E0D0
void Recompile_LW(); // 0040E210
void Recompile_LBU(); // 0040E2C0
void Recompile_LHU(); // 0040E380
void Recompile_LWR(); // 0040E440
void Recompile_LWU(); // 0040E580
void Recompile_SB(); // 0040E630
void Recompile_SH(); // 0040E730
void Recompile_SWL(); // 0040E830
void Recompile_SW(); // 0040E9F0
void Recompile_SDL(); // 0040EB70
void Recompile_SDR(); // 0040ECA0
void Recompile_SWR(); // 0040ED40
void Recompile_CACHE(); // 0040EFB0
void Recompile_LWC1(); // 0040F130
void Recompile_LDC1(); // 0040F290
void Recompile_LD(); // 0040F480
void Recompile_Unimplemented(); // 0040F570
void Recompile_SWC1(); // 0040F580
void Recompile_SDC1(); // 0040F720
void Recompile_SD(); // 0040F8E0
void Recompile_SLL(); // 0040FA00
void Recompile_SRL(); // 0040FA50
void Recompile_SRA(); // 0040FA90
void Recompile_SLLV(); // 0040FAD0
void Recompile_SRLV(); // 0040FB10
void Recompile_SRAV(); // 0040FB50
void Recompile_JR(); // 0040FB90
void Recompile_JALR(); // 0040FBF0
void Recompile_SYSCALL(); // 0040FC40
void Recompile_BREAK(); // 0040FC80
void Recompile_MFHI(); // 0040FC90
void Recompile_MTHI(); // 0040FCD0
void Recompile_MFLO(); // 0040FD10
void Recompile_MTLO(); // 0040FD50
void Recompile_DSLLV(); // 0040FD90
void Recompile_DSRLV(); // 0040FE50
void Recompile_DSRAV(); // 0040FF10
void Recompile_MULT(); // 0040FFD0
void Recompile_MULTU(); // 00410000
void Recompile_DIV(); // 00410030
void Recompile_DIVU(); // 004100F0
void Recompile_DMULT(); // 004101B0
void Recompile_DMULTU(); // 00410200
void Recompile_DDIV(); // 00410250
void Recompile_DDIVU(); // 004102A0
void Recompile_ADD(); // 004102F0
void Recompile_SUB(); // 00410330
void Recompile_AND(); // 00410370
void Recompile_OR(); // 004103C0
void Recompile_XOR(); // 00410410
void Recompile_NOR(); // 00410460
void Recompile_SLT(); // 004104C0
void Recompile_SLTU(); // 00410560
void Recompile_DADD(); // 00410600
void Recompile_DSUB(); // 00410650
void Recompile_TEQ(); // 004106D0
void Recompile_DSLL(); // 004106E0
void Recompile_DSRL(); // 00410730
void Recompile_DSRA(); // 00410780
void Recompile_DSLL32(); // 004107D0
void Recompile_DSRL32(); // 00410820
void Recompile_DSRA32(); // 00410870
void Recompile_BLTZ(); // 004108B0
void Recompile_BGEZ(); // 004108C0
void Recompile_BLTZL(); // 004108D0
void Recompile_BGEZL(); // 004108E0
void Recompile_BLTZAL(); // 004108F0
void Recompile_BGEZAL(); // 00410900
void Recompile_BLTZALL(); // 00410980
void Recompile_BGEZALL(); // 00410990
void Recompile_MFC0(); // 004109A0
void Recompile_MTC0(); // 00410BF0
void Recompile_TLB(); // 00410F80
void Recompile_TLBR(); // 00410FC0
void Recompile_TLBWI(); // 00410FE0
void Recompile_TLBWR(); // 00411050
void Recompile_TLBP(); // 00411110
void Recompile_ERET(); // 00411120

#endif
