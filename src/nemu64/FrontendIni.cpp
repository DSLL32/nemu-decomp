#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <string.h>
#include "FrontendIni.h"
#include "RomBrowser.h"
#include "WinMain.h"
#include "CPU.h"
#include "Memory.h"
#include "Dynarec.h"
#include "Plugins.h"
#include "PIF.h"
#include "Rom.h"
#ifdef NEMU_STATIC
#define NEMU_STATIC_PROFILE_REDIRECT
#include "../static/StaticFiles.h"
#endif

char NemuIniPath[260];
char ConfigurationIniPath[260];
char CheatsIniPath[260];
char InitialDirectory[260];


void ReadIniString(const char* section, const char* key, const char* defaultValue,
                   char* destination, const char* filename)
{
    GetPrivateProfileStringA(section, key, defaultValue, destination, 256, filename);
}

int ReadIniInteger(const char* section, const char* key, int defaultValue,
                   const char* filename)
{
    char defaultText[32];
    char value[32];
    itoa(defaultValue, defaultText, 10);
    GetPrivateProfileStringA(section, key, defaultText, value, 256, filename);
    return atoi(value);
}

void WriteIniString(const char* section, const char* key, const char* value,
                    const char* filename)
{
    WritePrivateProfileStringA(section, key, value, filename);
}

void WriteIniInteger(const char* section, const char* key, int value,
                     const char* filename)
{
    char text[32];
    itoa(value, text, 10);
    WritePrivateProfileStringA(section, key, text, filename);
}

void InitializeIniPaths()
{
    lstrcpyA(NemuIniPath, "");
    GetCurrentDirectoryA(260, NemuIniPath);
    GetCurrentDirectoryA(260, InitialDirectory);
    if (NemuIniPath[lstrlenA(NemuIniPath) - 1] != '\\')
        lstrcatA(NemuIniPath, "\\");
    lstrcpyA(ConfigurationIniPath, NemuIniPath);
    lstrcpyA(CheatsIniPath, NemuIniPath);
    lstrcatA(NemuIniPath, "Nemu64.ini");
    lstrcatA(ConfigurationIniPath, "Config.ini");
    lstrcatA(CheatsIniPath, "Cheats.ini");
}

void ReplaceExtension(char* filename, const char* extension)
{
    int position;
    for (position = strlen(filename); position >= 0 && filename[position] != '.'; --position)
        ;
    // Preserve the original test, including its behavior for names without '.'.
    if (position != 0)
        filename[position] = '\0';
    lstrcatA(filename, extension);
}

int ReadRomInteger(const char* key, int defaultValue)
{
    int value = ReadIniInteger("default", key, defaultValue, NemuIniPath);
    value = ReadIniInteger(CurrentRom.InternalName, key, value, NemuIniPath);
    return ReadIniInteger(CurrentRom.IniSection, key, value, NemuIniPath);
}

void ReadRomString(const char* key, const char* defaultValue, char* destination)
{
    char value[300];
    ReadIniString("default", key, defaultValue, value, NemuIniPath);
    ReadIniString(CurrentRom.InternalName, key, value, value, NemuIniPath);
    ReadIniString(CurrentRom.IniSection, key, value, destination, NemuIniPath);
}

void ReadRomConfiguration()
{
    ReadIniString("Configuration", "CART_EEPROM_PATH", InitialDirectory, EEPROMFilename, ConfigurationIniPath);
    if (lstrlenA(EEPROMFilename) && EEPROMFilename[lstrlenA(EEPROMFilename) - 1] != '\\')
        lstrcatA(EEPROMFilename, "\\");
    lstrcatA(EEPROMFilename, CurrentRom.FileName);
    ReplaceExtension(EEPROMFilename, ".sav");
    ReadIniString("Configuration", "MEMPACK_EEPROM_PATH", InitialDirectory, MemPackFilename, ConfigurationIniPath);
    if (lstrlenA(MemPackFilename) && MemPackFilename[lstrlenA(MemPackFilename) - 1] != '\\')
        lstrcatA(MemPackFilename, "\\");
    lstrcatA(MemPackFilename, CurrentRom.FileName);
    ReplaceExtension(MemPackFilename, ".mpk");
    ReadIniString("Configuration", "SRAM_PATH", InitialDirectory, SRAMFilename, ConfigurationIniPath);
    if (lstrlenA(SRAMFilename) && SRAMFilename[lstrlenA(SRAMFilename) - 1] != '\\')
        lstrcatA(SRAMFilename, "\\");
    lstrcatA(SRAMFilename, CurrentRom.FileName);
    ReplaceExtension(SRAMFilename, ".sra");
    CurrentRom.VideoHLE = ReadRomInteger("GBI", 1);
    if (CurrentRom.VideoHLE && SelectedGraphicsPlugin->VISetUCodeID)
        SelectedGraphicsPlugin->VISetUCodeID(CurrentRom.VideoHLE);
    CurrentRom.AudioHLE = ReadRomInteger("ABI", 1);
    AISetUCodeID(CurrentRom.AudioHLE);
    EEPROMEnabled = ReadRomInteger("EEProm", 1) != 0;
    CPU_Use32BitMemoryAccess = ReadRomInteger("Optimize", 1) != 0;
    Dynarec_Mode = ReadRomInteger("CompilerMode", 1);
    TLBRamMapped = ReadRomInteger("SlowTLB", 0) != 0;
    CPU_TLBEnabled = ReadRomInteger("EnableTLB", 0) != 0;
    CPU_InterpreterMode = ReadRomInteger("Interpreter", 0) != 0;
    ROMCacheEnabled = ReadRomInteger("ROMCache", 0) != 0;
    Dynarec_Inlining = ReadRomInteger("inlining", 1) != 0;
    EEPROMSize = ReadRomInteger("EEPromSize", 512);
    if (ROMCacheEnabled)
        ROMCache = (unsigned char*)malloc(ROMSize);
    ReadRomString("FriendlyName", CurrentRom.InternalName, CurrentRom.DisplayName);
}

void SaveBrowserSettings()
{
    int order[7];
    WriteIniString("Configuration", "Folder", RomDirectory, ConfigurationIniPath);
    WriteIniInteger("Configuration", "SortMode", BrowserSortMode, ConfigurationIniPath);
    WriteIniInteger("Configuration", "Column0Width", ListView_GetColumnWidth(BrowserWindow, 0), ConfigurationIniPath);
    WriteIniInteger("Configuration", "Column1Width", ListView_GetColumnWidth(BrowserWindow, 1), ConfigurationIniPath);
    WriteIniInteger("Configuration", "Column2Width", ListView_GetColumnWidth(BrowserWindow, 2), ConfigurationIniPath);
    WriteIniInteger("Configuration", "Column3Width", ListView_GetColumnWidth(BrowserWindow, 3), ConfigurationIniPath);
    WriteIniInteger("Configuration", "Column4Width", ListView_GetColumnWidth(BrowserWindow, 4), ConfigurationIniPath);
    WriteIniInteger("Configuration", "Column5Width", ListView_GetColumnWidth(BrowserWindow, 5), ConfigurationIniPath);
    WriteIniInteger("Configuration", "Column6Width", ListView_GetColumnWidth(BrowserWindow, 6), ConfigurationIniPath);
    ListView_GetColumnOrderArray(BrowserWindow, 7, order);
    WritePrivateProfileStructA("Configuration", "ColumnOrder", order, sizeof(order), ConfigurationIniPath);
}
