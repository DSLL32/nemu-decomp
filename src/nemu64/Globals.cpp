// Shared application storage reconstructed from original consumers and image
// data. Names are inferred; addresses, extents, and evidence are recorded in
// analysis/nemu-globals.json. Zero-initialized objects have no file payload.
#include "PIF.h"
#include "RSP.h"

unsigned char PIFResponse[64];
char MemPackFilename[260];
PIFCommandLength PIFCommandLengths[256];
char SRAMFilename[260];
ControllerState Controllers[4];
unsigned char PackBlock[32];
int EEPROMSize;
unsigned char PIFCommand[64];
FILE *MovieFile;
int EEPROMEnabled;
char EEPROMFilename[260];
unsigned char SRAMData[0x40000];
unsigned char EEPROMData[0x800];
unsigned char MemPackData[0x8000];
int SRAMUsed;
int MemPackUsed;
int EEPROMUsed;
int DisableSaveWrites;
int MovieRecord;
int MoviePlay;
unsigned MovieLastButtons;
unsigned short MovieRepeatCount;
int MovieNewRun = 1;
unsigned *CIC6105Table;
int CICState;
int ForcePIFSuccess;
char PIFNTSCFilename[256];
char PIFPALFilename[256];

// Interpreter, compiler, and debugger tools share these same objects.
bool UndoEnabled;
unsigned int OpcodeStats[0x220];
unsigned int CPUBreakpointCount;
unsigned int OpcodeBreakpointCount;
unsigned int OpcodeStatsEnabled;
DebugOpcodeBreakpoint OpcodeBreakpoints[256];
unsigned int BlockStatsEnabled;
unsigned int BlockExecutionCounts[0x200000];
DebugBreakpoint CPUBreakpoints[100000];
unsigned int RegisterStatsEnabled;
unsigned int RegisterStats[32];
unsigned int BlockCycleCounts[0x200000];
unsigned int UndoPosition;
unsigned char UndoData[0x400000];
CpuU32 MemoryBreakpointCount;
CpuU32 RSPBreakpointInhibit;
// Inferred capacity: the next independently referenced object follows exactly
// 100 eight-byte records. No original producer-side capacity check survives.
RSPMemoryBreakpoint MemoryBreakpoints[100];

// The build generates these documented CIC state tables from their algorithm.
#include "PifTables.inc"
