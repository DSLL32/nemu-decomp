#ifndef NEMU64_DYNAREC_FPU_H
#define NEMU64_DYNAREC_FPU_H

void Recompile_MFC1();
void Recompile_DMFC1();
void Recompile_CFC1();
void Recompile_MTC1();
void Recompile_DMTC1();
void Recompile_CTC1();
void Recompile_COP1_BC();
void Recompile_COP1_S();
void Recompile_COP1_D();
void Recompile_COP1_W();
void Recompile_COP1_L();
void Recompile_COP1_ADD();
void Recompile_COP1_SUB();
void Recompile_COP1_MUL();
void Recompile_COP1_DIV();
void Recompile_COP1_SQRT();
void Recompile_COP1_ABS();
void Recompile_COP1_MOV();
void Recompile_COP1_NEG();
void Recompile_COP1_ROUND_L();
void Recompile_COP1_TRUNC_L();
void Recompile_COP1_CEIL_L();
void Recompile_COP1_FLOOR_L();
void Recompile_COP1_ROUND_W();
void Recompile_COP1_TRUNC_W();
void Recompile_COP1_CEIL_W();
void Recompile_COP1_FLOOR_W();
void Recompile_COP1_CVT_S();
void Recompile_COP1_CVT_D();
void Recompile_COP1_CVT_W();
void Recompile_COP1_CVT_L();
void Recompile_COP1_C_F();
void Recompile_COP1_C_UN();
void Recompile_COP1_C_SF();
void Recompile_COP1_C_NGLE();
void Recompile_COP1_C_EQ();
void Recompile_COP1_C_OLT();
void Recompile_COP1_C_OLE();

extern unsigned Dynarec_DynamicFpuMode;
extern unsigned Dynarec_FpuChecked;
extern unsigned Dynarec_FpuEndLabel, Dynarec_FpuModeLabel;

void CheckFPUEnabled();
void LoadFpuOperand(unsigned char fullMode);
void StoreFpuResult(unsigned char fullMode);
void StoreFpuResultAs(unsigned format, unsigned char fullMode);
unsigned GetFpuRegisterAddress(unsigned reg, unsigned format, unsigned char fullMode);
void FpuRoundingControl(unsigned format, unsigned mode);
void RecompileFpuCompare(unsigned char condition);

#endif
