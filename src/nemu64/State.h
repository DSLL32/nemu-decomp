#ifndef NEMU64_STATE_H
#define NEMU64_STATE_H

#include <stdio.h>

extern char StateFileName[256];
extern unsigned StateCompressedBytes;
extern unsigned StateUncompressedBytes;

void WriteStateChunk(FILE* file, unsigned type, const void* data, unsigned length, bool compress);
void SaveState();
void SaveMovieState();
bool LoadState(bool previewOnly);
void FlushStateSaves();

#endif
