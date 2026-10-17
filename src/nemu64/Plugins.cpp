#include "Plugins.h"
#include "CPU.h"
#include "Memory.h"
#include "RSP.h"
#include "RomBrowser.h"
#include "WinMain.h"
#include "FrontendIni.h"
#include <stdio.h>
#include <string.h>

char PluginDirectory[MAX_PATH];
PluginDescriptor PluginCatalog[40];
unsigned int PluginCount;
ToolDescriptor ToolCatalog[40];
unsigned int ToolCount;
HMODULE PluginModules[30];
unsigned int PluginModuleCount;
PluginDescriptor *SelectedGraphicsPlugin;
PluginDescriptor *PendingGraphicsPlugin;
void *GraphicsCombiner;
void (__cdecl *CIInit)(HWND, void *);
void (__cdecl *CIFree)();
void (__cdecl *CIGetJoyData)();
void (__cdecl *CIRumble)(unsigned int);
void (__cdecl *CIConfigure)(HWND);
void (__cdecl *AIInit)(HWND, unsigned char *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
void (__cdecl *AIFree)();
void (__cdecl *AISetReg)(unsigned int, unsigned int);
unsigned int (__cdecl *AIGetReg)(unsigned int);
void (__cdecl *AIConfigure)(HWND);
void (__cdecl *AUDList)(unsigned int);
void (__cdecl *AISetUCodeID)(unsigned int);
void (__cdecl *NemuSettings)(unsigned int, PluginDescriptor *);
void (__cdecl *DCTList)(unsigned int);

bool LoadPlugin(char *filename)
{
    char path[MAX_PATH];
    char error[200];
    strcpy(path, PluginDirectory);
    strcat(path, filename);
    HMODULE module = LoadLibraryA(path);
    if (module) {
        if (GetProcAddress(module, "GetDllInfo")) {
            PluginDescriptor *plugin = &PluginCatalog[PluginCount];
            plugin->Size = sizeof(PluginDescriptor);
            plugin->AiDacrateChanged = (void (__cdecl *)(int))GetProcAddress(module, "AiDacrateChanged");
            plugin->AiLenChanged = (void (__cdecl *)())GetProcAddress(module, "AiLenChanged");
            plugin->AiReadLength = (unsigned int (__cdecl *)())GetProcAddress(module, "AiReadLength");
            plugin->AiUpdate = (void (__cdecl *)(BOOL))GetProcAddress(module, "AiUpdate");
            plugin->ChangeWindow = (void (__cdecl *)())GetProcAddress(module, "ChangeWindow");
            plugin->CloseDLL = (void (__cdecl *)())GetProcAddress(module, "CloseDLL");
            plugin->DllAbout = (void (__cdecl *)(HWND))GetProcAddress(module, "DllAbout");
            plugin->DllConfig = (void (__cdecl *)(HWND))GetProcAddress(module, "DllConfig");
            plugin->DllTest = (void (__cdecl *)(HWND))GetProcAddress(module, "DllTest");
            plugin->DrawScreen = (void (__cdecl *)())GetProcAddress(module, "DrawScreen");
            plugin->FillRectHack = (void (__cdecl *)())GetProcAddress(module, "FillRectHack");
            plugin->GetCombinerPointer = (void (__cdecl *)(void **))GetProcAddress(module, "GetCombinerPointer");
            plugin->GetDllInfo = (void (__cdecl *)(PluginInfo *))GetProcAddress(module, "GetDllInfo");
            plugin->InitiateAudio = (BOOL (__cdecl *)(AudioInfo))GetProcAddress(module, "InitiateAudio");
            plugin->InitiateGFX = (BOOL (__cdecl *)(GraphicsInfo))GetProcAddress(module, "InitiateGFX");
            plugin->MoveScreen = (void (__cdecl *)(int, int))GetProcAddress(module, "MoveScreen");
            plugin->ProcessAList = (void (__cdecl *)())GetProcAddress(module, "ProcessAList");
            plugin->ProcessDList = (void (__cdecl *)())GetProcAddress(module, "ProcessDList");
            plugin->RomClosed = (void (__cdecl *)())GetProcAddress(module, "RomClosed");
            plugin->RomOpen = (void (__cdecl *)())GetProcAddress(module, "RomOpen");
            plugin->UpdateScreen = (void (__cdecl *)())GetProcAddress(module, "UpdateScreen");
            plugin->ViStatusChanged = (void (__cdecl *)())GetProcAddress(module, "ViStatusChanged");
            plugin->ViWidthChanged = (void (__cdecl *)())GetProcAddress(module, "ViWidthChanged");
            plugin->VISetUCodeID = (void (__cdecl *)(unsigned int))GetProcAddress(module, "VISetUCodeID");
            plugin->SetResolution = (void (__cdecl *)(int, int))GetProcAddress(module, "SetResolution");
            plugin->GetDllInfo(&plugin->Info);
            strcpy(plugin->Filename, filename);
            ++PluginCount;
        }
        if (GetProcAddress(module, "CIInit")) {
            CIInit = (void (__cdecl *)(HWND, void *))GetProcAddress(module, "CIInit");
            CIFree = (void (__cdecl *)())GetProcAddress(module, "CIFree");
            CIGetJoyData = (void (__cdecl *)())GetProcAddress(module, "CIGetJoyData");
            CIRumble = (void (__cdecl *)(unsigned int))GetProcAddress(module, "CIRumble");
            CIConfigure = (void (__cdecl *)(HWND))GetProcAddress(module, "CIConfigure");
        }
        if (GetProcAddress(module, "AIInit")) {
            AIInit = (void (__cdecl *)(HWND, unsigned char *, unsigned int *, unsigned int *, unsigned int *, unsigned int *))GetProcAddress(module, "AIInit");
            AIFree = (void (__cdecl *)())GetProcAddress(module, "AIFree");
            AISetReg = (void (__cdecl *)(unsigned int, unsigned int))GetProcAddress(module, "AISetReg");
            AIGetReg = (unsigned int (__cdecl *)(unsigned int))GetProcAddress(module, "AIGetReg");
            AIConfigure = (void (__cdecl *)(HWND))GetProcAddress(module, "AIConfigure");
            AUDList = (void (__cdecl *)(unsigned int))GetProcAddress(module, "AUDList");
            AISetUCodeID = (void (__cdecl *)(unsigned int))GetProcAddress(module, "AISetUCodeID");
        }
        if (GetProcAddress(module, "ToolInit")) {
            ToolCatalog[ToolCount].Init = (unsigned int (__cdecl *)(NemuHostInterface *))GetProcAddress(module, "ToolInit");
            ToolCatalog[ToolCount].Free = (void (__cdecl *)())GetProcAddress(module, "ToolFree");
            ToolCatalog[ToolCount].Invoke = (void (__cdecl *)(unsigned int, unsigned int))GetProcAddress(module, "ToolFunc");
            ToolCatalog[ToolCount].GetMenuText = (void (__cdecl *)(char *))GetProcAddress(module, "ToolGetMenuText");
            sprintf(ToolCatalog[ToolCount].MenuText, "%s...", filename);
            ++ToolCount;
        }
        if (GetProcAddress(module, "NemuSettings"))
            NemuSettings = (void (__cdecl *)(unsigned int, PluginDescriptor *))GetProcAddress(module, "NemuSettings");
        if (GetProcAddress(module, "DCTList"))
            DCTList = (void (__cdecl *)(unsigned int))GetProcAddress(module, "DCTList");
        PluginModules[PluginModuleCount++] = module;
        return true;
    }
    else {
        sprintf(error, "Error loading library '%s'.\n\nExecution will continue but you won't be able to use this plugin", filename);
        MessageBoxA(MainWindow, error, "Error", MB_ICONERROR);
        return false;
    }
}

bool LoadPlugins()
{
    char search[300];
    WIN32_FIND_DATAA data;
    UnloadPlugins();
    PluginModuleCount = 0;
    strcpy(PluginDirectory, ApplicationDirectory);
    strcat(PluginDirectory, "Plugin\\");
    strcpy(search, PluginDirectory);
    strcat(search, "*.dll");
    HANDLE handle = FindFirstFileA(search, &data);
    if (handle != INVALID_HANDLE_VALUE) {
        do {
            LoadPlugin(data.cFileName);
        } while (FindNextFileA(handle, &data));
        FindClose(handle);
        return true;
    }
    return false;
}

void UnloadPlugins()
{
    ToolCount = 0;
    if (PluginModuleCount) do {
        --PluginModuleCount;
        if (PluginModules[PluginModuleCount])
            FreeLibrary(PluginModules[PluginModuleCount]);
    } while (PluginModuleCount > 0);
}

void FreeTools()
{
    for (unsigned int i = 0; i < ToolCount; ++i)
        if (ToolCatalog[i].Free)
            ToolCatalog[i].Free();
}


unsigned int NetPlayActive;
int NetPlayPlayer;
int NetPlayPlayerCount;
unsigned char RomHeader[64];
unsigned int GraphicsInterruptRegister;
int ShowErrorMessages;
char SaveStateDirectory[MAX_PATH];
extern char SelectedRomFilename[256];

void InitializeTools()
{
    MENUITEMINFOA menu;
    NemuHostInterface host;
    char text[100];
    host.MainWindow = &MainWindow;
    host.RDRAM = RDRAM;
    host.RamSize = &RDRAMSize;
    host.PeripheralMemory = SPMemory;
    host.ROM = ROMImage;
    host.RomSize = &ROMSize;
    host.PIFRam = PIFRam;
    host.RomName = CurrentRom.InternalName;
    host.RomInternalName = CurrentRom.DisplayName;
    host.CPUProgramCounter = &CPU_PC;
    host.CPURegisters = CPU_GPR;
    host.COP0Registers = CPU_COP0;
    host.FPURegisters32 = CPU_FPR.words;
    host.FCR31 = &CPU_FCR31;
    host.RSPRegisters = RSPRegisters;
    host.RSPFlags = RSPFlags;
    host.RSPAccumulator = RSPAccumulator;
    host.RSPVectors = RSPVectors;
    host.MultLO = &CPU_LO;
    host.MultHI = &CPU_HI;
    host.CPUInstruction = &CPU_Instruction;
    host.DebugFlags = &CPU_ActionFlags;
    host.EmulationThread = &CPU_Thread;
    host.RegisterStatsEnabled = &RegisterStatsEnabled;
    host.OpcodeStatsEnabled = &OpcodeStatsEnabled;
    host.RegisterStats = RegisterStats;
    host.OpcodeStats = OpcodeStats;
    host.VICount = &VICount;
    host.OpcodeBreakpoints = OpcodeBreakpoints;
    host.OpcodeBreakpointCount = &OpcodeBreakpointCount;
    host.RSPBreakpoints = RSPBreakpoints;
    host.RSPBreakpointCount = &RSPBreakpointCount;
    host.Watches = reinterpret_cast<DebugWatch *>(MemoryBreakpoints);
    host.WatchCount = &MemoryBreakpointCount;
    host.LastProgramCounter = &CPU_LastPC;
    host.TranslateAddress = TranslateTLB;
    host.TranslateError = &TranslationError;
    host.ReadWord = ReadPhysicalWord;
    host.WriteWord = WritePhysicalWord;
    host.ReadError = &ReadError;
    host.ShowErrors = &ShowErrors;
    host.Breakpoints = CPUBreakpoints;
    host.BreakpointCount = &CPUBreakpointCount;
    host.RSPControlRegisters = &SPReg.MemAddress;
    host.MIRegisters = &MIReg.Mode;
    host.RomFilename = CurrentRom.FullPath;
    host.ControllerData = Controllers;
    host.FPURegisters64 = CPU_FPR64;
    host.RSPTask = &HLETaskAddress;
    host.Running = &CPU_Running;
    host.RomCRC = CurrentRom.IniSection;
    host.GraphicsCombiner = &GraphicsCombiner;
    host.UndoEnabled = &UndoEnabled;
    host.UndoData = UndoData;
    host.UndoPosition = &UndoPosition;
    host.BlockExecutionCounts = BlockExecutionCounts;
    host.BlockCycleCounts = BlockCycleCounts;
    host.BlockStatsEnabled = &BlockStatsEnabled;
    host.DPCRegisters = &DPCReg.Start;
    host.RDPHistory = RDPHistory;
    host.RDPHistoryPosition = &RDPHistoryPosition;
    for (unsigned int i = 0; i < ToolCount; ++i) {
        int position = 0;
        ToolCatalog[i].Flags = ToolCatalog[i].Init(&host);
        if (ToolCatalog[i].GetMenuText)
            ToolCatalog[i].GetMenuText(ToolCatalog[i].MenuText);
        if (ToolCatalog[i].Flags & 1) {
            menu.cbSize = sizeof(menu);
            menu.fMask = MIIM_TYPE | MIIM_STATE | MIIM_ID;
            menu.fType = MFT_STRING;
            menu.fState = MFS_ENABLED;
            menu.dwTypeData = text;
            menu.wID = 65001 + i * 30;
            int first = 0;
            do {
                if (ToolCatalog[i].MenuText[position] == '\r') {
                    lstrcpynA(text, ToolCatalog[i].MenuText + first, position - first + 1);
                    InsertMenuItemA(GetMenu(MainWindow), 0x8008, FALSE, &menu);
                    ++menu.wID;
                    first = position + 1;
                }
                ++position;
            } while (position < lstrlenA(ToolCatalog[i].MenuText));
            lstrcpynA(text, ToolCatalog[i].MenuText + first, position - first + 1);
            InsertMenuItemA(GetMenu(MainWindow), 0x8008, FALSE, &menu);
            menu.cbSize = sizeof(menu);
            menu.fMask = MIIM_TYPE;
            menu.fType = MFT_SEPARATOR;
            InsertMenuItemA(GetMenu(MainWindow), 0x8008, FALSE, &menu);
        }
    }
}

void NotifyTools(unsigned int mask, unsigned int reason)
{
    for (unsigned int i = 0; i < ToolCount; ++i)
        if ((ToolCatalog[i].Flags & mask) && ToolCatalog[i].Invoke)
            ToolCatalog[i].Invoke(mask, reason);
}

void __stdcall NetplayClientDropped(char *, int)
{
}

int __stdcall NetplayStartGame(char *game, int player, int playerCount)
{
    LVFINDINFOA find;
    LVITEMA item;
    char filename[256];
    NetPlayPlayer = player;
    NetPlayPlayerCount = playerCount;
    NetPlayActive = 1;
    find.flags = LVFI_STRING;
    find.psz = game;
    int index = SendMessageA(BrowserWindow, LVM_FINDITEMA, -1, (LPARAM)&find);
    item.iSubItem = 1;
    item.pszText = filename;
    item.cchTextMax = 256;
    SendMessageA(BrowserWindow, LVM_GETITEMTEXTA, index, (LPARAM)&item);
    strcpy(SelectedRomFilename, RomDirectory);
    strcat(SelectedRomFilename, filename);
    Controllers[0].Present = true;
    Controllers[1].Present = false;
    Controllers[2].Present = false;
    Controllers[3].Present = false;
    StartCPUThread();
    return 0;
}

void CheckGraphicsInterrupts()
{
}

void CommitRDRAM()
{
 if (!VirtualAlloc(RDRAM, RDRAMSize, MEM_COMMIT, PAGE_READWRITE))
  MessageBoxA(MainWindow, "Couldn't commit rdram-memory", "Error", MB_ICONERROR);
}
void DecommitRDRAM()
{
 if (!VirtualFree(RDRAM, RDRAMSize, MEM_DECOMMIT))
  MessageBoxA(MainWindow, "Couldn't decommit rdram-memory", "Error", MB_ICONERROR);
}
bool SelectGraphicsPlugin()
{
 SelectedGraphicsPlugin = NULL;
 if (PluginCount == 0)
  return false;
 char filename[256];
 ReadIniString("Configuration", "GFXDLL", "", filename, ConfigurationIniPath);
 for (unsigned i = 0; i < PluginCount; ++i)
  if (PluginCatalog[i].Info.Type == 2)
   if (strlen(filename) == 0 || strcmp(filename, PluginCatalog[i].Filename) == 0)
   {
    SelectedGraphicsPlugin = &PluginCatalog[i];
    break;
   }
 if (SelectedGraphicsPlugin == NULL)
  for (unsigned i = 0; i < PluginCount; ++i)
   if (PluginCatalog[i].Info.Type == 2)
   {
    SelectedGraphicsPlugin = &PluginCatalog[i];
    break;
   }
 return true;
}
void ReloadConfiguration()
{
 unsigned size = ReadIniInteger("Configuration", "RDRAMSize", 4, ConfigurationIniPath) << 20;
 if (size != RDRAMSize)
 {
  if (RDRAMSize != 0)
   DecommitRDRAM();
  RDRAMSize = size;
  CommitRDRAM();
 }
 ShowErrorMessages = ReadIniInteger("Configuration", "ShowErrors", 0, ConfigurationIniPath) != 0;
 ReadIniString("Configuration", "SAVESTATE_PATH", InitialDirectory, SaveStateDirectory, ConfigurationIniPath);
 if (lstrlenA(SaveStateDirectory) != 0)
  if (SaveStateDirectory[lstrlenA(SaveStateDirectory) - 1] != '\\')
   lstrcatA(SaveStateDirectory, "\\");
 SelectGraphicsPlugin();
}
void InitializeGraphicsPlugin()
{
 if (SelectedGraphicsPlugin->InitiateGFX)
 {
  GraphicsInfo info;
  info.StatusBar = StatusWindow;
  info.Window = MainWindow;
  info.CheckInterrupts = CheckGraphicsInterrupts;
  info.MemoryBswapped = TRUE;
  info.RDRAM = RDRAM;
  info.DMEM = RSPShadowMemory;
  info.IMEM = RSPShadowMemory + 0x1000;
  info.Header = RomHeader;
  info.MIInterrupt = &GraphicsInterruptRegister;
  info.DPCStart = &DPCReg.Start;
  info.DPCEnd = &DPCReg.End;
  info.DPCCurrent = &DPCReg.Current;
  info.DPCStatus = &DPCReg.Status;
  info.DPCClock = &DPCReg.Clock;
  info.DPCBufferBusy = &DPCReg.BufferBusy;
  info.DPCPipeBusy = &DPCReg.PipeBusy;
  info.DPCTMEM = &DPCReg.Tmem;
  info.VIStatus = &VIReg.Status;
  info.VIOrigin = &VIReg.Origin;
  info.VIWidth = &VIReg.Width;
  info.VIInterrupt = &VIReg.Interrupt;
  info.VICurrent = &VIReg.Current;
  info.VITiming = &VIReg.Burst;
  info.VIVSync = &VIReg.VSync;
  info.VIHSync = &VIReg.HSync;
  info.VILeap = &VIReg.Leap;
  info.VIHStart = &VIReg.HStart;
  info.VIVStart = &VIReg.VStart;
  info.VIBurst = &VIReg.VBurst;
  info.VIXScale = &VIReg.XScale;
  info.VIYScale = &VIReg.YScale;
  SelectedGraphicsPlugin->InitiateGFX(info);
 }
 if (SelectedGraphicsPlugin->GetCombinerPointer)
  SelectedGraphicsPlugin->GetCombinerPointer(&GraphicsCombiner);
}
