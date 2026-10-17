#ifndef NEMU64_DYNAREC_MEMORY_OPS_H
#define NEMU64_DYNAREC_MEMORY_OPS_H

#include "CPU.h"

// Names and source-unit boundary are inferred from the surviving executable.
// See analysis/nemu-dynarec-memory.json for original function addresses.
void __stdcall LoadDoubleLeft(unsigned address, unsigned reg);
void Recompile_LDL();
void __stdcall LoadDoubleRight(unsigned address, unsigned reg);
void Recompile_LDR();
void Recompile_LB();
void Recompile_LH();
void Recompile_LWL();
void Recompile_LW();
void Recompile_LBU();
void Recompile_LHU();
void Recompile_LWR();
void Recompile_LWU();
void Recompile_SB();
void Recompile_SH();
void Recompile_SWL();
void Recompile_SW();
void __stdcall StoreDoubleLeft(unsigned address, CpuU64 value);
void Recompile_SDL();
void __stdcall StoreDoubleRight(unsigned address, CpuU64 value);
void Recompile_SDR();
void Recompile_SWR();
void __stdcall InvalidateRecompiledPage(unsigned address);
void __stdcall RemovePageBreakpoints(unsigned page);
void Recompile_CACHE();
void Recompile_LWC1();
void Recompile_LDC1();
void Recompile_LD();
void Recompile_Unimplemented();
void Recompile_SWC1();
void Recompile_SDC1();
void Recompile_SD();

#endif
