#ifndef NEMU64_ROM_H
#define NEMU64_ROM_H

#include <windows.h>

extern unsigned char* ROMCache;
extern int ROMCacheEnabled;
extern unsigned RomEntryPoint;
extern unsigned BootCicSeed;
extern unsigned VideoNTSC;
extern unsigned BootVersion;

bool ReadZipRom(const char* filename, bool showProgress, unsigned maximumBytes);
void FreeRomImage();
bool ReadRomImage(HWND parent, const char* filename, bool showProgress, unsigned maximumBytes);
bool LoadRom(HWND parent, const char* filename);

#endif
