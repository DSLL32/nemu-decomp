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

void SaveFunctionSignatures(unsigned *candidate)
{
    // Adjacent FILE, length and filename slots suggest this local aggregate;
    // its original type and name are unknown.
    struct {
        FILE *file;
        int length;
        char filename[260];
    } output;
    char status[256];
    unsigned *lastMatch = 0;
    if (candidate < (unsigned *)RDRAM)
        candidate = (unsigned *)RDRAM;
    unsigned *start = candidate;
    strcpy(output.filename, ApplicationDirectory);
    if (output.filename[strlen(output.filename) - 1] != '\\')
        strcat(output.filename, "\\cod\\");
    else
        strcat(output.filename, "cod\\");
    strcat(output.filename, CurrentRom.FileName);
    ReplaceExtension(output.filename, ".cod");
    output.file = fopen(output.filename, "wb");
    if (!output.file)
        return;
    unsigned hash = 0;
    unsigned signature = 0;
    for (;;) {
        for (;; hash = 0) {
            if (!RuntimeSignatures[signature].name)
                goto finished;
            if (!*candidate) {
                do {
                    ++candidate;
                    if (candidate >= start + 0x40000)
                        break;
                } while (!*candidate);
            }
            output.length = RuntimeSignatures[signature].length;
            if (output.length > 256)
                output.length = 256;
            if ((unsigned char *)candidate + output.length < (unsigned char *)(start + 0x40000)) {
                unsigned *functionStart = candidate;
                if ((*candidate >> 16) != RuntimeSignatures[signature].firstWord) hash = 0;
                else {
                    for (int offset = 0; offset < output.length; offset += 4)
                        hash = HashSignatureInstruction(hash, signature, offset, candidate++);
                }
                if (hash == RuntimeSignatures[signature].hash) {
                    fprintf(output.file, "0x%X,%s\r\n", (unsigned)functionStart - (unsigned)RDRAM - 0x80000000,
                        RuntimeSignatures[signature].name);
                    fflush(output.file);
                    unsigned *previousMatch = lastMatch;
                    lastMatch = start;
                    if (previousMatch) lastMatch = candidate;
                    candidate = start;
                    unsigned address = ((unsigned)functionStart - (unsigned)RDRAM) | 0x80000000;
                    RuntimeReplacements[signature].function = RuntimeSignatures[signature].replacement;
                    RuntimeReplacements[signature].address = address;
                    sprintf(status, "Searching... 0x%X, %s", address, RuntimeSignatures[signature].name);
                    SetStatusText(0, status);
                    RuntimeReplacementCount = ++signature;
                    if (signature == 512)
                        goto finished;
                    continue;
                } else {
                    candidate = functionStart + 1;
                    continue;
                }
            } else {
                break;
            }
        }
        {
            RuntimeReplacements[signature].address = 0;
            RuntimeReplacements[signature].function = 0;
            RuntimeReplacementCount = ++signature;
            candidate = start;
        }
    }
finished:
    fclose(output.file);
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
