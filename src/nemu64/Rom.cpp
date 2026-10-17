#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Rom.h"
#include "RomBrowser.h"
#include "FrontendIni.h"
#include "Memory.h"
#include "WinMain.h"
#include "CPU.h"
#include "Dynarec.h"
#include "Plugins.h"
#include "PIF.h"
#include "vendor/zlib-1.1.3/contrib/minizip/unzip.h"

unsigned char* ROMCache;
int ROMCacheEnabled;
unsigned RomEntryPoint;
unsigned BootCicSeed;
unsigned VideoNTSC;
unsigned BootVersion;


bool ReadZipRom(const char* filename, unsigned maximumBytes, bool showProgress)
{
    char name[256], status[100], loading[100];
    unz_file_info info;
    bool result = false;
    unzFile file = unzOpen(filename);
    if (file) {
        if (unzGoToFirstFile(file) == UNZ_OK) {
            do {
                if (unzGetCurrentFileInfo(file, &info, name, 256, NULL, 0, NULL, 0) != UNZ_OK)
                    continue;
                if (_stricmp(name + strlen(name) - 4, ".bin") &&
                    _stricmp(name + strlen(name) - 4, ".rom") &&
                    _stricmp(name + strlen(name) - 4, ".pal") &&
                    _stricmp(name + strlen(name) - 4, ".usa") &&
                    _stricmp(name + strlen(name) - 4, ".eur") &&
                    _stricmp(name + strlen(name) - 4, ".v64") &&
                    _stricmp(name + strlen(name) - 4, ".z64") &&
                    _stricmp(name + strlen(name) - 4, ".n64"))
                    continue;
                CurrentRom.FileSize = info.uncompressed_size;
                CurrentRom.CompressedSize = info.compressed_size;
                ROMSize = info.uncompressed_size;
                if (maximumBytes && maximumBytes < ROMSize)
                    ROMSize = maximumBytes;
                if (showProgress) {
                    sprintf(status, "Allocating %i mb of memory", (ROMSize + 0x80000) >> 20);
                    SetStatusText(0, status);
                }
                ROMImage = (unsigned char*)malloc(ROMSize);
                if (unzOpenCurrentFile(file) != UNZ_OK)
                    continue;
                unsigned done = 0;
                if (showProgress) {
                    sprintf(loading, "Unzipping and loading %s...", name);
                    SetStatusText(0, loading);
                }
                result = true;
                do {
                    unsigned count = done + 0x100000 < ROMSize ? 0x100000 : ROMSize - done;
                    unsigned read = unzReadCurrentFile(file, ROMImage + done, count);
                    if (read < count)
                        result = false;
                    done += read;
                    if (showProgress) {
                        sprintf(status, "%d%%", (done >> 2) * 100 / (ROMSize >> 2));
                        SetStatusText(1, status);
                    }
                } while (done < ROMSize);
                unzCloseCurrentFile(file);
                break;
            } while (unzGoToNextFile(file) == UNZ_OK);
        }
        unzClose(file);
    }
    return result;
}

void FreeRomImage()
{
    if (CPU_Running) {
        free(ROMImage);
        if (ROMCacheEnabled) {
            free(ROMCache);
            ROMCacheEnabled = 0;
        }
    }
    ROMSize = 0;
    CPU_Running = 0;
}

bool ReadRomImage(HWND parent, const char* filename, unsigned maximumBytes, bool showProgress)
{
    char status[100], loading[100];
    OFSTRUCT openInfo;
    if (!_stricmp(filename + strlen(filename) - 4, ".zip")) {
        if (!ReadZipRom(filename, maximumBytes, showProgress))
            return false;
    } else {
        HANDLE file = (HANDLE)OpenFile(filename, &openInfo, OF_SHARE_DENY_WRITE);
        if (file == INVALID_HANDLE_VALUE) {
            if (showProgress)
                MessageBoxA(parent, "Error opening file", "Error", MB_ICONERROR);
            return false;
        }
        CurrentRom.FileSize = GetFileSize(file, NULL);
        CurrentRom.CompressedSize = 0;
        ROMSize = CurrentRom.FileSize;
        if (maximumBytes && maximumBytes < ROMSize)
            ROMSize = maximumBytes;
        if (showProgress) {
            sprintf(status, "Allocating %i mb of memory", (ROMSize + 0x80000) >> 20);
            SetStatusText(0, status);
        }
        ROMImage = (unsigned char*)malloc(ROMSize);
        unsigned done = 0;
        if (showProgress) {
            sprintf(loading, "Loading %s...", filename);
            SetStatusText(0, loading);
        }
        do {
            DWORD count = done + 0x100000 < ROMSize ? 0x100000 : ROMSize - done;
            DWORD read;
            ReadFile(file, ROMImage + done, count, &read, NULL);
            done += read;
            if (showProgress) {
                sprintf(status, "%d%%", (done >> 2) * 100 / (ROMSize >> 2));
                SetStatusText(1, status);
            }
        } while (done < ROMSize);
        CloseHandle(file);
    }
    if (showProgress)
        SetStatusText(0, "Byteswapping...");
    unsigned magic = ReadPhysicalWord(0x10000000);
    unsigned offset;
    switch (magic) {
    case 0x12408037: case 0x12418037:
        for (offset = 0; offset < ROMSize; offset += 4) {
            unsigned short value = *(unsigned short*)(ROMImage + offset);
            *(unsigned short*)(ROMImage + offset) = *(unsigned short*)(ROMImage + offset + 2);
            *(unsigned short*)(ROMImage + offset + 2) = value;
            if (!(offset & 0xffff) && showProgress) {
                sprintf(status, "%d%%", (offset >> 2) * 100 / (ROMSize >> 2));
                SetStatusText(1, status);
            }
        }
        break;
    case 0x37804012: case 0x37804112:
        for (offset = 0; offset < ROMSize; offset += 2) {
            unsigned char value = ROMImage[offset];
            ROMImage[offset] = ROMImage[offset + 1];
            ROMImage[offset + 1] = value;
            if (!(offset & 0xffff) && showProgress) {
                sprintf(status, "%d%%", (offset >> 2) * 100 / (ROMSize >> 2));
                SetStatusText(1, status);
            }
        }
        break;
    case 0x40123780: case 0x41123780:
        for (offset = 0; offset < ROMSize; offset += 4) {
            unsigned* word = (unsigned*)(ROMImage + offset);
            unsigned value = *word;
            *word = (value >> 24) | ((value & 0xff0000) >> 8) |
                    ((value & 0xff00) << 8) | (value << 24);
            if (!(offset & 0xffff) && showProgress) {
                sprintf(status, "%d%%", (offset >> 2) * 100 / (ROMSize >> 2));
                SetStatusText(1, status);
            }
        }
        break;
    case 0x80371240: case 0x80371241:
        break;
    default:
        if (showProgress)
            MessageBoxA(MainWindow, "This is most likely not a n64 rom", "Unknown rom type", MB_ICONERROR);
        FreeRomImage();
        return false;
    }
    if (showProgress) {
        SetStatusText(0, "");
        SetStatusText(1, "");
    }
    int position = strlen(filename);
    while (filename[position] != '\\')
        --position;
    strcpy(CurrentRom.FileName, filename + position + 1);
    strcpy(CurrentRom.FullPath, filename);
    CopyByteSwapped(CurrentRom.InternalName, ROMImage + 0x20, 0x20);
    for (position = 25; CurrentRom.InternalName[position] == ' ' || CurrentRom.InternalName[position] == '\0'; --position)
        CurrentRom.InternalName[position] = '\0';
    if (!strlen(CurrentRom.InternalName)) {
        strcpy(CurrentRom.InternalName, CurrentRom.FileName);
        ReplaceExtension(CurrentRom.InternalName, "");
    }
    CurrentRom.CRC1 = ReadPhysicalWord(0x10000010);
    CurrentRom.CRC2 = ReadPhysicalWord(0x10000014);
    sprintf(CurrentRom.IniSection, "0x%8X", ReadPhysicalWord(0x10000010));
    return true;
}

bool LoadRom(HWND parent, const char* filename)
{
    char title[100];
    FreeRomImage();
    CPU_Running = 1;
    if (!ReadRomImage(parent, filename, 0, true))
        return false;
    ReadRomConfiguration();
    RomEntryPoint = ReadPhysicalWord(0x10000008);
    unsigned checksum = 0;
    for (unsigned index = 0x40; index < 0x1000; ++index)
        checksum += ROMImage[index] * (index ^ 3);
    switch (checksum) {
    case 0x18ed367a: case 0x18decfbd:
        BootCicSeed = 0x3f;
        break;
    case 0x15886c07:
        BootCicSeed = 0x85;
        RomEntryPoint -= 0x200000;
        break;
    case 0x19f83396:
        BootCicSeed = 0x78;
        RomEntryPoint -= 0x100000;
        break;
    case 0x2976f0dc:
        BootCicSeed = 0x91;
        break;
    default:
        MessageBoxA(parent, "Unknown bootcode. Chances that this game works aren't high", "Unknown bootcode", MB_ICONWARNING);
    }
    switch (ROMImage[0x3d]) {
    case 'D': case 'P': case 'U': VideoNTSC = 0; break;
    default: VideoNTSC = 1;
    }
    RuntimeLibraryEnabled = ReadRomInteger("osHLE", 0) != 0;
    if (RuntimeLibraryEnabled)
        ResetRuntimeReplacements();
    VideoNTSC = ReadRomInteger("TVMode", VideoNTSC);
    BootVersion = VideoNTSC ? 1 : 6;
    BootVersion = ReadRomInteger("Version", BootVersion);
    FlashEnabled = 0;
    FlashStatus = 0;
    FlashCommand = 0;
    SetEmulationMenus(1);
    sprintf(title, "Nemu64 - %s", CurrentRom.DisplayName);
    SetWindowTextA(MainWindow, title);
    NotifyTools(0x80, 0);
    return true;
}
