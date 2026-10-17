#ifndef NEMU64_PIF_H
#define NEMU64_PIF_H

#include <stdio.h>
#include "Plugins.h"

struct PIFCommandLength { int Tx, Rx; };
extern unsigned char PIFResponse[64];
extern char MemPackFilename[260];
extern PIFCommandLength PIFCommandLengths[256];
extern char SRAMFilename[260];
extern ControllerState Controllers[4];
extern unsigned char PackBlock[32];
extern int EEPROMSize;
extern unsigned char PIFCommand[64];
extern FILE *MovieFile;
extern int EEPROMEnabled;
extern char EEPROMFilename[260];
extern unsigned char SRAMData[0x40000];
extern unsigned char EEPROMData[0x800];
extern unsigned char MemPackData[0x8000];
extern int SRAMUsed, MemPackUsed, EEPROMUsed;
extern int DisableSaveWrites;
extern int MovieRecord, MoviePlay;
extern unsigned MovieLastButtons;
extern unsigned short MovieRepeatCount;
extern int MovieNewRun;
extern unsigned *CIC6105Table;
extern int CICState;
extern int ForcePIFSuccess;
extern char PIFNTSCFilename[256];
extern char PIFPALFilename[256];
extern unsigned char PifResponseTable1[256];
extern unsigned char PifResponseTable0[256];
extern unsigned long PifResponseHistory[32768];

void FindCICInput(FILE *file, const unsigned char *data);
void SaveCICInput(const unsigned char *data);
int CICUseTable1(const unsigned char *response);
void CIC6105Challenge(unsigned char *command);
unsigned char PIFDataCRC(const unsigned char *data);
void InitializePIFCommandLengths();
void ProcessPIF();
void SaveEEPROM(unsigned offset, unsigned length);
void SaveMemPack(unsigned offset, unsigned length);
void SaveSRAM(unsigned offset, unsigned length);
void RecordMovieInput();
void PlayMovieInput();
void StopMovie();
void WriteSRAM(unsigned offset, const void *source, unsigned length);
void ReadSRAM(void *destination, unsigned offset, unsigned length);

#endif
