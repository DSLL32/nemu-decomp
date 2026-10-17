#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "State.h"
#include "CPU.h"
#include "CpuControl.h"
#include "DynarecFPU.h"
#include "RSP.h"
#include "Memory.h"
#include "PIF.h"
#include "Plugins.h"
#include "WinMain.h"
#include "RomBrowser.h"
#include "resource.h"
#include "vendor/zlib-1.1.3/zlib.h"

unsigned StateCompressedBytes;
unsigned StateUncompressedBytes;

void WriteStateChunk(FILE* file, unsigned type, const void* data, unsigned length, bool compressed)
{
    unsigned long packedLength = 0;
    fwrite(&type, 4, 1, file);
    fwrite(&length, 4, 1, file);
    if (compressed) {
        packedLength = (unsigned)(length * 0.01 + 0.5) + 12 + length;
        unsigned char* packed = (unsigned char*)malloc(packedLength);
        compress2(packed, &packedLength, (const unsigned char*)data, length, 1);
        fwrite(&packedLength, 4, 1, file);
        fwrite(packed, packedLength, 1, file);
        free(packed);
        StateCompressedBytes += 12 + packedLength;
        StateUncompressedBytes += 12 + length;
    } else {
        fwrite(&packedLength, 4, 1, file);
        fwrite(data, length, 1, file);
        StateCompressedBytes += 12 + length;
        StateUncompressedBytes += 12 + length;
    }
}

void SaveState()
{
    char status[256];
    ResetRecompiler();
    StateUncompressedBytes = 0;
    StateCompressedBytes = 0;
    FILE* file = fopen(StateFileName, "wb");
    WriteStateChunk(file, 100, StatePreview, 0x9600, true);
    WriteStateChunk(file, 1, RDRAM, RDRAMSize, true);
    WriteStateChunk(file, 2, SPMemory, 0x4000, true);
    WriteStateChunk(file, 3, PIFRam, 0x40, false);
    WriteStateChunk(file, 10, CPU_GPR, 0x100, false);
    WriteStateChunk(file, 11, CPU_COP0, 0x80, false);
    WriteStateChunk(file, 12, &CPU_FPR, 0x80, false);
    WriteStateChunk(file, 13, &CPU_FCR31, 4, false);
    WriteStateChunk(file, 14, TLB, 0x880, false);
    WriteStateChunk(file, 15, &CPU_PC, 4, false);
    WriteStateChunk(file, 16, &CPU_LO, 4, false);
    WriteStateChunk(file, 17, &CPU_HI, 4, false);
    WriteStateChunk(file, 19, RSPRegisters, 0x80, false);
    WriteStateChunk(file, 20, RSPVectors, 0x200, false);
    WriteStateChunk(file, 21, RSPFlags, 6, false);
    WriteStateChunk(file, 22, RSPAccumulator, 0x40, false);
    WriteStateChunk(file, 30, &RIReg, 0x20, false);
    WriteStateChunk(file, 31, &SPReg, 0x20, false);
    WriteStateChunk(file, 32, &MIReg, 0x0c, false);
    WriteStateChunk(file, 33, &AIReg, 0x24, false);
    WriteStateChunk(file, 34, &SIAddress, 4, false);
    WriteStateChunk(file, 35, &RDRAMReg, 0x28, false);
    WriteStateChunk(file, 36, &VIReg, 0x38, false);
    WriteStateChunk(file, 37, &PIReg, 0x34, false);
    WriteStateChunk(file, 38, &DPCReg, 0x20, false);
    WriteStateChunk(file, 39, Controllers, 0x20, false);
    WriteStateChunk(file, 50, &CPU_HostFpuControl, 4, false);
    WriteStateChunk(file, 51, &CPU_ActionFlags, 4, false);
    WriteStateChunk(file, 52, &CPU_Random, 4, false);
    WriteStateChunk(file, 53, &CPU_VideoCyclesRemaining, 4, false);
    WriteStateChunk(file, 60, Controllers, 0x20, false);
    WriteStateChunk(file, 70, &FlashStatus, 4, false);
    WriteStateChunk(file, 71, &FlashCommand, 4, false);
    WriteStateChunk(file, 72, &FlashRamAddress, 4, false);
    WriteStateChunk(file, 73, &FlashEnabled, 4, false);
    if (EEPROMUsed)
        WriteStateChunk(file, 80, EEPROMData, EEPROMSize, false);
    if (MemPackUsed)
        WriteStateChunk(file, 81, MemPackData, 0x8000, true);
    if (SRAMUsed)
        WriteStateChunk(file, 82, SRAMData, 0x40000, true);
    fclose(file);
    sprintf(status, "State saved. Compression effiency: %i%%",
            (StateUncompressedBytes - StateCompressedBytes) * 100 / StateUncompressedBytes);
    SetStatusText(0, status);
}

void SaveMovieState()
{
    char status[256];
    FILE* file = fopen("$$$temp.tmp", "rb");
    fseek(file, 0, SEEK_END);
    unsigned length = ftell(file);
    fseek(file, 0, SEEK_SET);
    void* data = malloc(length);
    fread(data, 1, length, file);
    fclose(file);
    file = fopen(StateFileName, "r+b");
    fseek(file, 0, SEEK_END);
    WriteStateChunk(file, 90, data, length, true);
    fclose(file);
    free(data);
    sprintf(status, "Movie state %s saved. \n", StateFileName);
    SetStatusText(0, status);
}

bool LoadState(bool previewOnly)
{
    bool foundPreview = false;
    if (!previewOnly) {
        ResetRecompiler();
        EEPROMUsed = 0;
        MemPackUsed = 0;
        SRAMUsed = 0;
    }
    FILE* file = fopen(StateFileName, "rb");
    if (!file)
        return false;
    fpos_t fileLength, position = 0;
    fseek(file, 0, SEEK_END);
    fgetpos(file, &fileLength);
    fseek(file, 0, SEEK_SET);
    void* movieData = NULL;
    FILE* movieFile;
    while (position < fileLength) {
        unsigned type;
        unsigned long length, packedLength;
        fread(&type, 4, 1, file);
        fread(&length, 4, 1, file);
        fread(&packedLength, 4, 1, file);
        void* data = NULL;
        if (previewOnly) {
            if (type == 100) {
                data = StatePreview;
                foundPreview = true;
            }
        } else {
            switch (type) {
            case 1:
                if (RDRAMSize != length) {
                    DecommitRDRAM();
                    RDRAMSize = length;
                    CommitRDRAM();
                }
                data = RDRAM;
                break;
            case 2: data = SPMemory; break;
            case 3: data = PIFRam; break;
            case 10: data = CPU_GPR; break;
            case 11: data = CPU_COP0; break;
            case 12: data = &CPU_FPR; break;
            case 13: data = &CPU_FCR31; break;
            case 14: data = TLB; break;
            case 15: data = &CPU_PC; break;
            case 16: data = &CPU_LO; break;
            case 17: data = &CPU_HI; break;
            case 19: data = RSPRegisters; break;
            case 20: data = RSPVectors; break;
            case 21: data = RSPFlags; break;
            case 22: data = RSPAccumulator; break;
            case 30: data = &RIReg; break;
            case 31: data = &SPReg; break;
            case 32: data = &MIReg; break;
            case 33: data = &AIReg; break;
            case 34: data = &SIAddress; break;
            case 35: data = &RDRAMReg; break;
            case 36: data = &VIReg; break;
            case 37: data = &PIReg; break;
            case 38: data = &DPCReg; break;
            case 39: case 60: data = Controllers; break;
            case 50: data = &CPU_HostFpuControl; break;
            case 51: data = &CPU_ActionFlags; break;
            case 52: data = &CPU_Random; break;
            case 53: data = &CPU_VideoCyclesRemaining; break;
            case 70: data = &FlashStatus; break;
            case 71: data = &FlashCommand; break;
            case 72: data = &FlashRamAddress; break;
            case 73: data = &FlashEnabled; break;
            case 80:
                data = EEPROMData;
                EEPROMSize = length;
                EEPROMUsed = 1;
                break;
            case 81: data = MemPackData; MemPackUsed = 1; break;
            case 82: data = SRAMData; SRAMUsed = 1; break;
            case 90:
                movieFile = fopen("$$$temp.tmp", "wb");
                fseek(movieFile, 0, SEEK_SET);
                movieData = malloc(length);
                MoviePlay = 1;
                data = movieData;
                break;
            }
        }
        if (!data) {
            fseek(file, packedLength ? packedLength : length, SEEK_CUR);
        } else if (!packedLength) {
            fread(data, length, 1, file);
        } else {
            void* packed = malloc(packedLength);
            fread(packed, packedLength, 1, file);
            uncompress((unsigned char*)data, &length, (const unsigned char*)packed, packedLength);
            free(packed);
            if (data == movieData) {
                fwrite(movieData, 1, length, movieFile);
                fclose(movieFile);
                free(movieData);
            }
        }
        fgetpos(file, &position);
    }
    fclose(file);
    if (previewOnly)
        return foundPreview;
    CPU_ActionFlags &= ~0x00400000;
    if (EEPROMUsed || MemPackUsed || SRAMUsed) {
        FlushStateSaves();
        DisableSaveWrites = 0;
    }
    CPU_FRMode = (CPU_COP0[CP0_Status] >> 26) & 1;
    CPU_AudioEnabled = 1;
    CPU_AudioCyclesRemaining = 0;
    CPU_FpuModeChanges = 0;
    Dynarec_DynamicFpuMode = 0;
    SetStatusText(0, "State loaded");
    return true;
}

void FlushStateSaves()
{
    EnableMenuItem(GetMenu(MainWindow), ID_USE_VIRTUAL_SAVE_FILES, MF_GRAYED);
    DisableSaveWrites = 0;
    if (EEPROMUsed)
        SaveEEPROM(0, EEPROMSize);
    if (MemPackUsed)
        SaveMemPack(0, 0x8000);
    if (SRAMUsed)
        SaveSRAM(0, 0x40000);
}
