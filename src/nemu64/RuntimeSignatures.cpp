// Inferred separate unit for runtime library signatures and replacement lookup.
// Original source filenames are unknown; helpers occupy 0041FF00..004202CF.
#include "Dynarec.h"
#include "CPU.h"
#include "PIF.h"
#include "CpuControl.h"
#include "Rom.h"
#include "RSP.h"
#include "DynarecFPU.h"
#include "DynarecMemoryOps.h"
#include "Memory.h"
#include "Plugins.h"
#include "FrontendIni.h"
#include "WinMain.h"
#include "RomBrowser.h"
#include <string.h>

#include "DynarecSignatures.inc"

int RuntimeLibraryEnabled;
int RuntimeReplacementCount;
RuntimeReplacement RuntimeReplacements[512];

unsigned HashSignatureInstruction(unsigned hash, unsigned signature,
    unsigned offset, const unsigned *instruction)
{
    int ordinary = 1;
    for (int i = 0; i < RuntimeRelocations[signature].count; ++i) {
        if (RuntimeRelocations[signature].offsets[i] == offset) {
            unsigned opcode = (*instruction) >> 26;
            ordinary = 0;
            if (opcode == 3 || opcode == 2)
                hash ^= opcode;
            else if (opcode == 15) {
                unsigned immediate = (*instruction) & 0xffff;
                if (immediate >= 0xa400 && immediate <= 0xafff)
                    hash += (*instruction);
                else
                    hash ^= (*instruction) >> 16;
            } else if (opcode == 13)
                hash ^= (*instruction) >> 16;
            else if (opcode == 35)
                hash ^= (*instruction) >> 16;
            else
                hash += (*instruction) >> 16;
        }
    }
    if (ordinary) {
        if (*instruction == 0)
            hash ^= 0xdeadbeef;
        else
            hash += *instruction;
    }
    return hash;
}

void SaveFunctionSignatures(unsigned *start)
{
    char filename[260];
    char status[256];
    unsigned *lastMatch = 0;
    if (start < (unsigned *)RDRAM)
        start = (unsigned *)RDRAM;
    strcpy(filename, ApplicationDirectory);
    if (filename[strlen(filename) - 1] == '\\')
        strcat(filename, "cod\\");
    else
        strcat(filename, "\\cod\\");
    strcat(filename, CurrentRom.FileName);
    ReplaceExtension(filename, ".cod");
    FILE *file = fopen(filename, "wb");
    if (!file)
        return;
    unsigned *candidate = start;
    unsigned signature = 0;
    for (;;) {
        FunctionSignature &entry = RuntimeSignatures[signature];
        if (!entry.name)
            break;
        if (!*candidate) {
            do {
                ++candidate;
                if (candidate >= start + 0x40000)
                    break;
            } while (!*candidate);
        }
        int length = entry.length;
        if (length > 256)
            length = 256;
        if ((unsigned char *)candidate + length < (unsigned char *)(start + 0x40000)) {
            unsigned hash = 0;
            unsigned *instruction = candidate;
            if ((*candidate >> 16) == entry.firstWord) {
                for (int offset = 0; offset < length; offset += 4)
                    hash = HashSignatureInstruction(hash, signature, offset, instruction++);
            }
            if (hash == entry.hash) {
                fprintf(file, "0x%X,%s\r\n", (unsigned)candidate - (unsigned)RDRAM - 0x80000000,
                    entry.name);
                fflush(file);
                if (lastMatch) lastMatch = instruction;
                else lastMatch = start;
                unsigned address = ((unsigned)candidate - (unsigned)RDRAM) | 0x80000000;
                RuntimeReplacements[signature].function = entry.replacement;
                RuntimeReplacements[signature].address = address;
                sprintf(status, "Searching... 0x%X, %s", address, entry.name);
                SetStatusText(0, status);
                candidate = start;
                RuntimeReplacementCount = ++signature;
                if (signature == 512)
                    break;
            } else
                ++candidate;
        } else {
            RuntimeReplacements[signature].address = 0;
            RuntimeReplacements[signature].function = 0;
            RuntimeReplacementCount = ++signature;
            candidate = start;
        }
    }
    fclose(file);
    SetStatusText(0, "Nemulating...");
}

void ResetRuntimeReplacements()
{
    RuntimeLibraryEnabled = 1;
    for (int i = 0; i < 512; ++i) {
        RuntimeReplacements[i].address = 0;
        RuntimeReplacements[i].function = 0;
    }
}

void (*FindRuntimeReplacement(unsigned address))()
{
    for (int i = 0; i < RuntimeReplacementCount; ++i)
        if ((RuntimeReplacements[i].address & 0xffffff) == (address & 0xffffff))
            return RuntimeReplacements[i].function;
    return 0;
}

void EnablePIFSuccess()
{
    ForcePIFSuccess = 1;
}
