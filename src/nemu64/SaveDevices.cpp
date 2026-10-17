#include "PIF.h"
#include "Memory.h"
#include "CPU.h"
#include <string.h>


void SaveEEPROM(unsigned offset, unsigned length)
{
    if (DisableSaveWrites) return;
    FILE *file = fopen(EEPROMFilename, "r+b");
    if (!file) {
        file = fopen(EEPROMFilename, "wb");
        if (!file) return;
        fwrite(EEPROMData, 0x800, 1, file);
        fflush(file);
        fclose(file);
        return;
    }
    fseek(file, offset, SEEK_SET);
    fwrite(EEPROMData + offset, length, 1, file);
    fflush(file);
    fclose(file);
}

void SaveMemPack(unsigned offset, unsigned length)
{
    if (DisableSaveWrites) return;
    FILE *file = fopen(MemPackFilename, "r+b");
    if (!file) {
        file = fopen(MemPackFilename, "wb");
        if (!file) return;
        fwrite(MemPackData, 0x8000, 1, file);
        fflush(file);
        fclose(file);
        return;
    }
    fseek(file, offset, SEEK_SET);
    fwrite(MemPackData + offset, length, 1, file);
    fflush(file);
    fclose(file);
}

void RecordMovieInput()
{
    if (!MovieFile) {
        MovieFile = fopen("$$$temp.tmp", "wb");
        fwrite(&Controllers[0], 1, 8, MovieFile);
        fwrite(&Controllers[1], 1, 8, MovieFile);
        fwrite(&Controllers[2], 1, 8, MovieFile);
        fwrite(&Controllers[3], 1, 8, MovieFile);
        if (!MovieFile) return;
    }
    if (*(unsigned *)PIFResponse == MovieLastButtons && !MovieNewRun) {
        ++MovieRepeatCount;
        fseek(MovieFile, -6, SEEK_CUR);
    } else MovieRepeatCount = 0;
    fwrite(PIFResponse, 1, 4, MovieFile);
    fwrite(&MovieRepeatCount, 1, 2, MovieFile);
    MovieLastButtons = *(unsigned *)PIFResponse;
    MovieNewRun = 0;
}

void PlayMovieInput()
{
    unsigned buttons = 0;
    if (!MovieFile) {
        MovieFile = fopen("$$$temp.tmp", "rb");
        fread(&Controllers[0], 1, 8, MovieFile);
        fread(&Controllers[1], 1, 8, MovieFile);
        fread(&Controllers[2], 1, 8, MovieFile);
        fread(&Controllers[3], 1, 8, MovieFile);
        if (!MovieFile) return;
    }
    if (MovieRepeatCount > 0) {
        --MovieRepeatCount;
        *(unsigned *)PIFResponse = MovieLastButtons;
    } else {
        fread(&buttons, 1, 4, MovieFile);
        fread(&MovieRepeatCount, 1, 2, MovieFile);
        MovieLastButtons = buttons;
        *(unsigned *)PIFResponse = buttons;
    }
}

void StopMovie()
{
    fclose(MovieFile);
    MovieFile = 0;
    MovieNewRun = 1;
    MovieLastButtons = 0;
    MovieRepeatCount = 0;
    MoviePlay = 0;
    MovieRecord = 0;
}

void SaveSRAM(unsigned offset, unsigned length)
{
    if (DisableSaveWrites) return;
    FILE *file = fopen(SRAMFilename, "r+b");
    if (!file) {
        file = fopen(SRAMFilename, "wb");
        if (!file) return;
        fwrite(SRAMData, 0x40000, 1, file);
        fflush(file);
        fclose(file);
        return;
    }
    fseek(file, offset, SEEK_SET);
    fwrite(SRAMData + offset, length, 1, file);
    fflush(file);
    fclose(file);
}

void WriteSRAM(unsigned offset, const void *source, unsigned length)
{
    if (offset + length <= 0x40000) {
        CopyAndSwapWords(SRAMData + offset, source, length);
        SaveSRAM(offset, length);
        SRAMUsed = 1;
    }
}

void ReadSRAM(void *destination, unsigned offset, unsigned length)
{
    if (offset + length <= 0x40000) {
        CopyAndSwapWords(destination, SRAMData + offset, length);
        SRAMUsed = 1;
    }
}
