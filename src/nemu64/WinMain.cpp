#include "WinMain.h"
#include "resource.h"
#include "Plugins.h"
#include "CPU.h"
#include "Memory.h"
#include "RSP.h"
#include "FrontendIni.h"
#include "RomBrowser.h"
#include "Rom.h"
#include "State.h"
#include "PIF.h"
#include "Kaillera.h"
#include "Dynarec.h"
#include <shellapi.h>
#include <commdlg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "EasterText.h"
#ifdef NEMU_STATIC
#include "../static/StaticRuntime.h"
#include "../static/StaticTheme.h"
#define NEMU_STATIC_PROFILE_REDIRECT
#include "../static/StaticFiles.h"
#endif

HWND MainWindow;
HINSTANCE ApplicationInstance;
char ApplicationDirectory[MAX_PATH];
HWND ToolbarWindow;
HWND StatusWindow;
HWND BrowserWindow;
POINT MainWindowPosition;
int DisplayResolution;
unsigned short StatePreview[120][160];
char StateFileName[256];
char EasterText[10000];
int EasterTextLength;
HIMAGELIST BrowserImageList;
HMENU DriveMenu;
HMENU FavouritesMenu;
bool BrowserInitialized;
bool Closing;
bool FullScreen;
unsigned int EasterTextPosition;
int ActivePause;
bool FrontendInitialized;

// Browser helpers use an internal register calling convention in the linked
// program. The declarations here describe their C++ parameters; /GL and LTCG
// decide that private convention when the complete program is linked.
extern int WindowHeight(HWND window);
void GSConfigure();
extern char SelectedRomFilename[256];

// Main-window integration, menu and dialog definitions are grouped by the
// recovered entrypoint order. Original source-unit names remain inferred.

int WindowHeight(HWND window)
{
    RECT rectangle;
    GetWindowRect(window, &rectangle);
    return rectangle.bottom - rectangle.top;
}

void SetStatusText(unsigned int part, const char* text)
{
    SendMessageA(StatusWindow, SB_SETTEXTA, part, (LPARAM)text);
}

void __stdcall NetplayClientDropped(char *, int)
{
}

void __stdcall NetplayMessage(const char* text)
{
    MessageBoxA(MainWindow, text, "", 0);
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

void RebuildFavouritesMenu()
{
    MENUITEMINFOA item;
    char text[256], filename[256], firstLine[256];
    if (FavouritesMenu)
        DestroyMenu(FavouritesMenu);
    FavouritesMenu = CreatePopupMenu();
    item.cbSize = sizeof(item);
    item.fMask = MIIM_TYPE | MIIM_ID;
    item.fType = MFT_STRING;
    item.wID = 1;
    item.dwTypeData = text;
    strcpy(text, "Add this folder to favourites");
    InsertMenuItemA(FavouritesMenu, 0, TRUE, &item);
    strcpy(filename, ApplicationDirectory);
    strcat(filename, "fav.dat");
    FILE* file = fopen(filename, "rt");
    bool first = true;
    if (file) {
        while (!feof(file) && fgets(text, 256, file)) {
            if (first) {
                strcpy(firstLine, text);
                first = false;
                item.wID = 2;
                strcpy(text, "Remove a folder from favourites...");
                InsertMenuItemA(FavouritesMenu, 1, TRUE, &item);
                item.fType = MFT_SEPARATOR;
                item.wID = 3;
                InsertMenuItemA(FavouritesMenu, 2, TRUE, &item);
                item.wID = 4;
                item.fType = MFT_STRING;
                strcpy(text, firstLine);
            }
            if (text[strlen(text) - 1] == '\n')
                text[strlen(text) - 1] = '\0';
            InsertMenuItemA(FavouritesMenu, item.wID, TRUE, &item);
            ++item.wID;
        }
        fclose(file);
    }
}

void ShowFavouritesMenu()
{
    POINT point;
    RECT button;
    MENUITEMINFOA item;
    char folder[256], filename[256];
    RebuildFavouritesMenu();
    int index = SendMessageA(ToolbarWindow, TB_COMMANDTOINDEX, 0x801f, 0);
    SendMessageA(ToolbarWindow, TB_GETITEMRECT, index, (LPARAM)&button);
    point.x = button.left;
    point.y = button.bottom + 2;
    ClientToScreen(MainWindow, &point);
    int selection = TrackPopupMenuEx(FavouritesMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                     point.x, point.y, MainWindow, NULL);
    switch (selection) {
    case 0:
    case 3:
        return;
    case 1: {
        strcpy(filename, ApplicationDirectory);
        strcat(filename, "fav.dat");
        FILE* file = fopen(filename, "at");
        if (file) {
            strcpy(folder, RomDirectory);
            if (strlen(folder) != 3)
                folder[strlen(folder) - 1] = '\0';
            fprintf(file, "%s\n", folder);
            fclose(file);
        } else {
            MessageBoxA(MainWindow, "There was an error opening the favourites file", "Error", 0);
        }
        return;
    }
    case 2:
        DialogBoxParamA(ApplicationInstance, MAKEINTRESOURCEA(IDD_REMOVE_FAVOURITES),
                        MainWindow, RemoveFavouritesDialogProc, 0);
        return;
    default:
        item.cbSize = sizeof(item);
        item.fMask = MIIM_TYPE;
        item.dwTypeData = RomDirectory;
        item.cch = 256;
        GetMenuItemInfoA(FavouritesMenu, selection - 1, TRUE, &item);
        if (lstrlenA(RomDirectory) && RomDirectory[lstrlenA(RomDirectory) - 1] != '\\')
            lstrcatA(RomDirectory, "\\");
        RefreshRomBrowser();
        return;
    }
}

void ShowDriveMenu()
{
    POINT point;
    RECT button;
    int index = SendMessageA(ToolbarWindow, TB_COMMANDTOINDEX, 0x801e, 0);
    SendMessageA(ToolbarWindow, TB_GETITEMRECT, index, (LPARAM)&button);
    point.x = button.left;
    point.y = button.bottom + 2;
    ClientToScreen(MainWindow, &point);
    int selection = TrackPopupMenuEx(DriveMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                     point.x, point.y, MainWindow, NULL);
    if (selection) {
        strcpy(RomDirectory, "X:\\");
        RomDirectory[0] = selection + '@';
        RefreshRomBrowser();
    }
}

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

void FreeTools()
{
    for (unsigned int i = 0; i < ToolCount; ++i)
        if (ToolCatalog[i].Free)
            ToolCatalog[i].Free();
}

void ResizeMainWindow(unsigned int width, int height)
{
    RECT rect;
    rect.left = MainWindowPosition.x;
    rect.top = MainWindowPosition.y;
    rect.right = rect.left + width;
    rect.bottom = rect.top + height;
    if (SelectedGraphicsPlugin->SetResolution)
        SelectedGraphicsPlugin->SetResolution(width, height);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);
    MoveWindow(MainWindow, rect.left, rect.top - 27,
        rect.right - rect.left,
        rect.bottom - rect.top + WindowHeight(StatusWindow) + WindowHeight(ToolbarWindow), TRUE);
    switch (width)
    {
    case 320: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_320X240, MF_BYCOMMAND); break;
    case 400: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_400X300, MF_BYCOMMAND); break;
    case 640: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_640X480, MF_BYCOMMAND); break;
    case 800: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_800X600, MF_BYCOMMAND); break;
    case 1024: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_1024X768, MF_BYCOMMAND); break;
    case 1152: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_1152X864, MF_BYCOMMAND); break;
    case 1280: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_1280X960, MF_BYCOMMAND); break;
    case 1600: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_1600X1200, MF_BYCOMMAND); break;
    }
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

void SetEmulationMenus(unsigned char running)
{
    unsigned int stopped = !running;
    EnableMenuItem(GetMenu(MainWindow), ID_CLOSE_ROM, stopped);
    EnableMenuItem(GetMenu(MainWindow), 0x8026, running);
    EnableMenuItem(GetMenu(MainWindow), ID_RESET, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_PAUSE, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_ACTIVE_PAUSE, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_SAVE_STATE, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_LOAD_STATE, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_QUICK_SAVESTATE, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_QUICK_LOADSTATE, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_START_RECORD, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_PLAY_START, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_STOP_RECORD, MF_GRAYED);
    EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_PLAY_STOP, MF_GRAYED);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, ID_DIRECTORY_UP, stopped);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, 0x801e, stopped);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, 0x801f, stopped);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, 0x8026, stopped);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, ID_CLOSE_ROM, running);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, ID_PAUSE, running);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, ID_CLOSE_ROM, running);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, ID_RESET, running);
    if (running)
    {
        ShowWindow(BrowserWindow, SW_HIDE);
        EnableWindow(BrowserWindow, FALSE);
        HWND junk = CreateWindowExA(0, "EDIT", "Junk", 0, 0, 0, 0, 0,
            MainWindow, NULL, ApplicationInstance, NULL);
        SetFocus(junk);
        DestroyWindow(junk);
    }
    else
    {
        ShowWindow(BrowserWindow, SW_SHOW);
        EnableWindow(BrowserWindow, TRUE);
        SetFocus(BrowserWindow);
    }
    DrawMenuBar(MainWindow);
    InvalidateRect(MainWindow, NULL, TRUE);
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

void DrawStatePreview(HWND dialog)
{
    HWND child = GetWindow(dialog, GW_CHILD);
    HDC dc = GetDC(child);
    for (unsigned int y = 0; y < 120; y++)
        for (unsigned int x = 0; x < 160; x++)
        {
            unsigned int value = StatePreview[y][x] >> 1;
            BYTE red = (BYTE)(value >> 10);
            BYTE green = (BYTE)(value >> 5);
            BYTE blue = (BYTE)value;
            red <<= 3;
            green <<= 3;
            blue <<= 3;
            SetPixelV(dc, x, y, RGB(red, green, blue));
        }
    // The original releases this DC with a null HWND.
    ReleaseDC(NULL, dc);
}

UINT CALLBACK StatePreviewHook(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message != WM_PAINT)
    {
        if (message != WM_NOTIFY) return 0;
        OFNOTIFYA* notification = (OFNOTIFYA*)lParam;
        if (notification->hdr.code != CDN_SELCHANGE)
        {
            if (notification->hdr.code != CDN_INITDONE) return 0;
            SetWindowPos(GetWindow(dialog, GW_CHILD), NULL, 0, 0, 164, 124,
                SWP_NOMOVE | SWP_NOZORDER);
            return 0;
        }
        SendMessageA(notification->hdr.hwndFrom, CDM_GETFILEPATH, MAX_PATH, (LPARAM)StateFileName);
        if (!LoadState(true)) memset(StatePreview, 0, sizeof(StatePreview));
    }
    DrawStatePreview(dialog);
    return 0;
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand)
{
#ifdef NEMU_STATIC
    if (!NemuStaticInitialize(instance)) {
        MessageBoxA(NULL, "Error initializing built-in components", "Error", MB_ICONSTOP);
        return 1;
    }
#endif
    ApplicationInstance = instance;
    WNDCLASSEXA windowClass;
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = MainWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(IDI_NEMU64));
    windowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    windowClass.lpszMenuName = MAKEINTRESOURCEA(IDR_MAINMENU);
    windowClass.lpszClassName = "Nemu64";
    windowClass.hIconSm = LoadIconA(windowClass.hInstance, MAKEINTRESOURCEA(108));
    if (!RegisterClassExA(&windowClass))
    {
        MessageBoxA(NULL, "Error initializing class", "Error", MB_ICONSTOP);
        return 0;
    }
    if (!CIC6105Table)
    {
        CIC6105Table = (unsigned int*)malloc(0x400010);
        if (CIC6105Table)
        {
            HRSRC resource = FindResourceA(instance, MAKEINTRESOURCEA(IDR_CIC6105_TABLE), RT_RCDATA);
            GetLastError();
            HGLOBAL loaded = LoadResource(instance, resource);
            void* data = LockResource(loaded);
            if (data)
                memcpy(CIC6105Table, data, 0x400000);
        }
    }
    sprintf(EasterText, EasterTextFirst);
    strcat(EasterText, EasterTextSecond);
    EasterTextLength = strlen(EasterText);
    kailleraInit();
    NetPlayPlayer = 0;
    NetPlayActive = 0;
    GetCurrentDirectoryA(MAX_PATH, ApplicationDirectory);
    if (ApplicationDirectory[strlen(ApplicationDirectory) - 1] != '\\')
        strcat(ApplicationDirectory, "\\");
    InitializeIniPaths();
    RDRAMSize = 0;
#ifdef NEMU_STATIC
    RDRAM = (unsigned char*)NemuStaticRdram();
#else
    RDRAM = (unsigned char*)VirtualAlloc(NULL, 0x20000000, MEM_RESERVE, PAGE_READWRITE);
#endif
    if (!RDRAM)
        MessageBoxA(MainWindow, "Couldn't reserve rdram-memory", "Error", MB_ICONSTOP);
    ReloadConfiguration();
    ReadIniString("Configuration", "Folder", "C:\\", RomDirectory, ConfigurationIniPath);
    if (RomDirectory[strlen(RomDirectory) - 1] != '\\')
        strcat(RomDirectory, "\\");
    BrowserSortMode = ReadIniInteger("Configuration", "SortMode", 1, ConfigurationIniPath);
    DisplayResolution = ReadIniInteger("Configuration", "Resolution", 640, ConfigurationIniPath);
    FrontendInitialized = false;
    ReadIniString("Configuration", "PifNTSC", "", PIFNTSCFilename, ConfigurationIniPath);
    ReadIniString("Configuration", "PifPAL", "", PIFPALFilename, ConfigurationIniPath);
    if (!LoadPlugins())
    {
        MessageBoxA(NULL, "Error initializing plugins - can't continue execution", "Error", MB_ICONSTOP);
        return 0;
    }
    if (!CIInit)
    {
        MessageBoxA(NULL, "No input plugin found - can't continue execution", "Error", MB_ICONSTOP);
        return 0;
    }
    if (!AIInit)
    {
        MessageBoxA(NULL, "No audio plugin found - can't continue execution", "Error", MB_ICONSTOP);
        return 0;
    }
    if (!DCTList)
    {
        MessageBoxA(NULL, "No dct (jpeg) plugin found - jpeg disabled", "Error", MB_ICONSTOP);
        return 0;
    }
    if (!PluginCount)
    {
        MessageBoxA(NULL, "No zilmar-standard compatible plugin found", "Error", MB_ICONSTOP);
        return 0;
    }
    SelectGraphicsPlugin();
    if (!SelectedGraphicsPlugin)
    {
        MessageBoxA(NULL, "No zilmar-standard compatible graphics plugin found", "Error", MB_ICONSTOP);
        return 0;
    }
    CPU_Running = 0;
    MainWindow = CreateWindowExA(0, "Nemu64", "Nemu64", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, instance, NULL);
    if (!MainWindow)
        return 0;
    SetEmulationMenus(0);
    ShowWindow(MainWindow, showCommand);
    UpdateWindow(MainWindow);
    ResizeMainWindow(640, 480);
    InitializeGraphicsPlugin();
    if (SelectedGraphicsPlugin->MoveScreen)
    {
        RECT rect;
        GetWindowRect(MainWindow, &rect);
        SelectedGraphicsPlugin->MoveScreen(rect.left + 4, rect.top + 69);
    }
    int parts[3] = {350, 420, -1};
    SendMessageA(StatusWindow, SB_SETPARTS, 3, (LPARAM)parts);
    HACCEL accelerators = LoadAcceleratorsA(instance, MAKEINTRESOURCEA(IDR_MAINMENU));
    RSPBreakpointCount = 0;
    OpcodeBreakpointCount = 0;
    InitializeTools();
    CIInit(MainWindow, Controllers);
    MSG message;
    while (GetMessageA(&message, NULL, 0, 0))
    {
        if (!TranslateAcceleratorA(message.hwnd, accelerators, &message))
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
#ifdef NEMU_STATIC
    // Plugin cleanup destroys Delphi forms and may pump more window messages.
    NemuStaticThemeShutdown();
#endif
    CIFree();
    if (SelectedGraphicsPlugin->CloseDLL)
        SelectedGraphicsPlugin->CloseDLL();
    FreeTools();
    kailleraShutdown();
    UnloadPlugins();
#ifdef NEMU_STATIC
    NemuStaticShutdown();
#endif
    DecommitRDRAM();
    if (!VirtualFree(RDRAM, 0, MEM_RELEASE))
        MessageBoxA(MainWindow, "Couldn't release rdram-memory", "Error", MB_ICONSTOP);
    return message.wParam;
}

void CaptureStatePreview()
{
    if (SelectedGraphicsPlugin->DrawScreen) SelectedGraphicsPlugin->DrawScreen();
    Sleep(100);
    SetStatusText(0, "Creating screenshot...");
    HDC dc = GetDC(MainWindow);
    RECT rect;
    GetClientRect(MainWindow, &rect);
    unsigned int width = rect.right - rect.left;
    unsigned int height = rect.bottom - rect.top - 45;
    unsigned int scaledX = 0;
    unsigned short* column = &StatePreview[0][0];
    for (int x = 0; x < 160; x++)
    {
        unsigned int pixelX = scaledX / 160;
        unsigned int scaledY = 0;
        unsigned short* pixel = column;
        for (int y = 0; y < 120; y++)
        {
            COLORREF value = GetPixel(dc, pixelX, scaledY / 120 + 27);
            *pixel = (unsigned short)(((GetRValue(value) >> 3) << 11) |
                ((GetGValue(value) >> 3) << 6) | ((GetBValue(value) >> 3) << 1));
            scaledY += height;
            pixel += 160;
        }
        scaledX += width;
        column++;
    }
    ReleaseDC(MainWindow, dc);
    SetStatusText(0, "Compressing and saving state...");
    CPU_ActionFlags |= 0x1000;
}

LRESULT MainWindowCommand(HWND window, UINT message, WPARAM wParam, LPARAM lParam, int unused)
{
    int toolResult = 1;
    OPENFILENAMEA file;
    KailleraInfos infos;
    LVITEMA item;
    char name[512];
    unsigned int capacity;
    char* games;
    int position;
    int index;
    int command = LOWORD(wParam);
    if (command > 65000)
    {
        ToolCatalog[(int)(command - 65001) / 30].Invoke(toolResult, (int)(command - 65001) % 30);
    }
    else switch (command)
    {
    case ID_START_NETPLAY:
        {
            infos.AppName = "Nemu64 0.8";
            capacity = 8192;
            games = (char*)malloc(capacity);
            games[0] = 0;
            games[1] = 0;
            position = 0;
            for (index = 0; index < ListView_GetItemCount(BrowserWindow); ++index)
            {
                item.mask = LVIF_TEXT | LVIF_IMAGE;
                item.iItem = index;
                item.iSubItem = 0;
                item.pszText = name;
                item.cchTextMax = 511;
                SendMessageA(BrowserWindow, LVM_GETITEMA, 0, (LPARAM)&item);
                if (item.iImage == 6)
                {
                    while (position + strlen(name) + 4 > capacity)
                    {
                        capacity *= 2;
                        games = (char*)realloc(games, capacity);
                    }
                    strcpy(games + position, name);
                    char* next = games + position;
                    unsigned int length = strlen(next) + 1;
                    next[length] = 0;
                    position += length;
                }
            }
            infos.GameList = games;
            infos.GameCallback = NetplayStartGame;
            infos.ChatReceivedCallback = NetplayChatReceived;
            infos.ClientDroppedCallback = NetplayClientDropped;
            infos.MoreInfosCallback = NetplayMessage;
            kailleraSetInfos(&infos);
            kailleraSelectServerDialog(NULL);
            free(games);
        }
        break;
    case ID_ABOUT:
        DialogBoxParamA(ApplicationInstance, MAKEINTRESOURCEA(IDD_ABOUT), window, AboutDialogProc, 0);
        break;
    case ID_EXIT:
        DestroyWindow(MainWindow);
        break;
    case ID_CONFIGURE_VIDEO_PLUGIN:
        if (SelectedGraphicsPlugin->DllConfig)
            SelectedGraphicsPlugin->DllConfig(MainWindow);
        break;
    case ID_CONFIGURE_AUDIO_PLUGIN:
        AIConfigure(window);
        break;
    case ID_CONFIGURE_INPUT_PLUGIN:
        CIConfigure(window);
        break;
    case ID_CHEAT_DP_INT:
        CPU_ActionFlags |= 0x20;
        break;
    case ID_SETTINGS:
        {
            if (!NemuSettings)
                break;
            NemuSettings(PluginCount, PluginCatalog);
            PluginDescriptor* previous = SelectedGraphicsPlugin;
            if (CPU_Running)
                SuspendThread(CPU_Thread);
            ReloadConfiguration();
            if (previous != SelectedGraphicsPlugin)
            {
                if (CPU_Running)
                {
                    PendingGraphicsPlugin = SelectedGraphicsPlugin;
                    SelectedGraphicsPlugin = previous;
                    CPU_ActionFlags |= 0x4000000;
                }
                else
                {
                    if (previous->CloseDLL)
                        previous->CloseDLL();
                    InitializeGraphicsPlugin();
                }
            }
            if (CPU_Running)
                ResumeThread(CPU_Thread);
        }
        break;
    case ID_RESET_DYNREC:
        CPU_ActionFlags |= 0x400;
        break;
    case ID_320X240: ResizeMainWindow(320, 240); break;
    case ID_400X300: ResizeMainWindow(400, 300); break;
    case ID_640X480: ResizeMainWindow(640, 480); break;
    case ID_800X600: ResizeMainWindow(800, 600); break;
    case ID_DIRECTORY_UP:
        if (strlen(RomDirectory) != 3)
        {
            int i = strlen(RomDirectory) - 2;
            while (i >= 0 && RomDirectory[i] != '\\')
                --i;
            RomDirectory[i + 1] = 0;
            RefreshRomBrowser();
        }
        break;
    case ID_RESET:
        CPU_ActionFlags |= 0x800;
        break;
    case ID_PAUSE:
        if (CPU_ActionFlags & 0x200000)
        {
            CPU_ActionFlags &= ~0x200000;
            ResumeThread(CPU_Thread);
            SendMessageA(ToolbarWindow, TB_SETSTATE, ID_PAUSE, TBSTATE_ENABLED);
        }
        else
        {
            ActivePause = 0;
            CPU_ActionFlags |= 0x200000;
            SendMessageA(ToolbarWindow, TB_SETSTATE, ID_PAUSE, TBSTATE_ENABLED | TBSTATE_CHECKED);
        }
        break;
    case ID_1024X768: ResizeMainWindow(1024, 768); break;
    case ID_1152X864: ResizeMainWindow(1152, 864); break;
    case ID_1280X960: ResizeMainWindow(1280, 960); break;
    case ID_1600X1200: ResizeMainWindow(1600, 1200); break;
    case 0x801f:
        ShowFavouritesMenu();
        break;
    case 0x801e:
        ShowDriveMenu();
        break;
    case ID_FULL_SCREEN:
        FullScreen = !FullScreen;
        if (SelectedGraphicsPlugin->ChangeWindow)
            SelectedGraphicsPlugin->ChangeWindow();
        break;
    case ID_1: CPU_CycleShift = 0; break;
    case ID_GAMESHARK_CHEATS:
        GSConfigure();
        break;
    case ID_ABOUT_VIDEO_PLUGIN:
        if (SelectedGraphicsPlugin->DllAbout)
            SelectedGraphicsPlugin->DllAbout(MainWindow);
        break;
    case ID_SHOW_ERRORS:
        ShowErrorMessages = !ShowErrorMessages;
        if (ShowErrorMessages)
            CheckMenuItem(GetMenu(MainWindow), ID_SHOW_ERRORS, MF_CHECKED);
        else
            CheckMenuItem(GetMenu(MainWindow), ID_SHOW_ERRORS, MF_UNCHECKED);
        WriteIniInteger("Configuration", "ShowErrors", ShowErrorMessages, ConfigurationIniPath);
        break;
    case 0x8026:
    case ID_LOAD_ROM:
        SelectedRomFilename[0] = 0;
        memset(&file, 0, sizeof(file));
        file.lStructSize = sizeof(file);
        file.hwndOwner = window;
        file.nMaxFile = 256;
        file.lpstrTitle = "Open rom...";
        file.lpstrFilter = "Rom and Zip files\0*.zip;*.?64;*.rom;*.usa;*.pal;*.eur;*.bin\0All files\0*.*\0\0";
        file.lpstrFile = SelectedRomFilename;
        if (GetOpenFileNameA(&file))
            StartCPUThread();
        break;
    case ID_CLOSE_ROM:
        CPU_ActionFlags = (CPU_ActionFlags & ~0x10000) | 0x400000;
        ResumeThread(CPU_Thread);
        SetStatusText(0, "Closing rom...");
        break;
    case ID_2: CPU_CycleShift = 1; break;
    case ID_4: CPU_CycleShift = 2; break;
    case ID_8: CPU_CycleShift = 3; break;
    case ID_DISABLE_EXCEPTION:
        CPU_DisableExceptions = !CPU_DisableExceptions;
        CheckMenuItem(GetMenu(MainWindow), ID_DISABLE_EXCEPTION, CPU_DisableExceptions == 1 ? MF_CHECKED : MF_UNCHECKED);
        break;
    case ID_DISABLE_VIDEO_HLE_1:
        CPU_DisableVideoHLE = !CPU_DisableVideoHLE;
        CheckMenuItem(GetMenu(MainWindow), ID_DISABLE_VIDEO_HLE_1, CPU_DisableVideoHLE == 1 ? MF_CHECKED : MF_UNCHECKED);
        break;
    case ID_FORCE_CFB:
        CPU_ForceCFB = !CPU_ForceCFB;
        CheckMenuItem(GetMenu(MainWindow), ID_FORCE_CFB, CPU_ForceCFB == 1 ? MF_CHECKED : MF_UNCHECKED);
        break;
    case ID_DISABLE_AUDIO_HLE_2:
        CPU_DisableAudioHLE = !CPU_DisableAudioHLE;
        CheckMenuItem(GetMenu(MainWindow), ID_DISABLE_AUDIO_HLE_2, CPU_DisableAudioHLE == 1 ? MF_CHECKED : MF_UNCHECKED);
        break;
    case ID_SAVE_STATE:
        StateFileName[0] = 0;
        memset(&file, 0, sizeof(file));
        file.lStructSize = sizeof(file);
        file.hwndOwner = window;
        file.nMaxFile = 256;
        file.lpstrTitle = "Save state...";
        file.lpstrFilter = "Nemu save state (*.nss)\0*.nss\0All files\0*.*\0\0";
        file.lpstrFile = StateFileName;
        file.lpstrDefExt = "nss";
        file.lpstrInitialDir = SaveStateDirectory;
        if (GetSaveFileNameA(&file))
            CaptureStatePreview();
        break;
    case ID_NEMU_MOVIE_START_RECORD:
        StateFileName[0] = 0;
        memset(&file, 0, sizeof(file));
        file.lStructSize = sizeof(file);
        file.hwndOwner = window;
        file.nMaxFile = 256;
        file.lpstrTitle = "Select movie file...";
        file.lpstrFilter = "Nemu movie file (*.nmf)\0*.nmf\0All files\0*.*\0\0";
        file.lpstrFile = StateFileName;
        file.lpstrDefExt = "nmf";
        file.lpstrInitialDir = SaveStateDirectory;
        if (GetSaveFileNameA(&file))
        {
            CaptureStatePreview();
            MovieRecord = 1;
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_START_RECORD, MF_GRAYED);
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_STOP_RECORD, MF_ENABLED);
        }
        break;
    case ID_NEMU_MOVIE_STOP_RECORD:
        if (MovieRecord == 1)
        {
            StopMovie();
            MovieRecord = 0;
            SaveMovieState();
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_START_RECORD, MF_ENABLED);
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_STOP_RECORD, MF_GRAYED);
        }
        break;
    case ID_NEMU_MOVIE_PLAY_START:
        StateFileName[0] = 0;
        memset(&file, 0, sizeof(file));
        file.lStructSize = sizeof(file);
        file.hwndOwner = window;
        file.nMaxFile = 256;
        file.lpstrTitle = "Select movie file...";
        file.lpstrFilter = "Nemu movie file (*.nmf)\0*.nmf\0All files\0*.*\0\0";
        file.lpstrFile = StateFileName;
        file.lpstrDefExt = "nmf";
        file.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
        file.lpfnHook = StatePreviewHook;
        file.hInstance = ApplicationInstance;
        file.lpTemplateName = MAKEINTRESOURCEA(IDD_ROM_PREVIEW);
        file.lpstrInitialDir = SaveStateDirectory;
        if (GetOpenFileNameA(&file))
        {
            SetStatusText(0, "Loading movie...");
            CPU_ActionFlags |= 0x2000;
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_START_RECORD, MF_GRAYED);
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_PLAY_START, MF_GRAYED);
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_PLAY_STOP, MF_ENABLED);
        }
        break;
    case ID_NEMU_MOVIE_PLAY_STOP:
        MoviePlay = 0;
        StopMovie();
        EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_START_RECORD, MF_ENABLED);
        EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_PLAY_START, MF_ENABLED);
        EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_PLAY_STOP, MF_GRAYED);
        break;
    case ID_QUICK_SAVESTATE:
        strcpy(StateFileName, SaveStateDirectory);
        strcat(StateFileName, "_");
        strcat(StateFileName, CurrentRom.FileName);
        ReplaceExtension(StateFileName, ".nss");
        CaptureStatePreview();
        break;
    case ID_LOAD_STATE:
        StateFileName[0] = 0;
        memset(&file, 0, sizeof(file));
        file.lStructSize = sizeof(file);
        file.hwndOwner = window;
        file.nMaxFile = 256;
        file.lpstrTitle = "Load state...";
        file.lpstrFilter = "Nemu save state (*.nss)\0*.nss\0All files\0*.*\0\0";
        file.lpstrFile = StateFileName;
        file.lpstrDefExt = "nss";
        file.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
        file.lpfnHook = StatePreviewHook;
        file.hInstance = ApplicationInstance;
        file.lpTemplateName = MAKEINTRESOURCEA(IDD_ROM_PREVIEW);
        file.lpstrInitialDir = SaveStateDirectory;
        if (GetOpenFileNameA(&file))
        {
            SetStatusText(0, "Loading state...");
            CPU_ActionFlags |= 0x2000;
        }
        break;
    case ID_QUICK_LOADSTATE:
        strcpy(StateFileName, SaveStateDirectory);
        strcat(StateFileName, "_");
        strcat(StateFileName, CurrentRom.FileName);
        ReplaceExtension(StateFileName, ".nss");
        SetStatusText(0, "Loading state...");
        CPU_ActionFlags |= 0x2000;
        break;
    case ID_USE_VIRTUAL_SAVE_FILES:
        if (MessageBoxA(MainWindow,
            "This will overwrite your own save files and replace them by the ones in memory. Proceed?",
            "Replace save files", MB_YESNO | MB_ICONQUESTION) == IDYES)
            FlushStateSaves();
        break;
    case ID_ACTIVE_PAUSE:
        if (CPU_ActionFlags & 0x200000)
        {
            CPU_ActionFlags &= ~0x200000;
            ResumeThread(CPU_Thread);
            SendMessageA(ToolbarWindow, TB_SETSTATE, ID_PAUSE, TBSTATE_ENABLED);
        }
        else
        {
            ActivePause = 1;
            CPU_ActionFlags |= 0x200000;
            SendMessageA(ToolbarWindow, TB_SETSTATE, ID_PAUSE, TBSTATE_ENABLED | TBSTATE_CHECKED);
        }
        break;
    case ID_TEST_PRE_MNI:
        CPU_ActionFlags |= 0x8000000;
        EnablePIFSuccess();
        break;
    default:
        return DefWindowProcA(window, message, wParam, lParam);
    }
    return toolResult;
}

void AddToolbarButton(TBBUTTON* buttons, int* count, BYTE style, int bitmap,
                      BYTE state, int command)
{
    buttons[*count].fsState = state;
    buttons[*count].fsStyle = style;
    buttons[*count].iBitmap = bitmap;
    buttons[*count].idCommand = command;
    buttons[*count].dwData = 0;
    buttons[*count].iString = *count;
    ++*count;
}

LRESULT CALLBACK MainWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND notificationWindow;
    NMHDR* notification;
    NMLISTVIEW* list;
    NMTOOLBARA* toolbar;
    int count;
    LVCOLUMNA column;
    LVITEMA item;
    struct { DWORD maximumNameLength; DWORD fileSystemFlags; } volumeInformation;
    char columnName[100];
    LVITEMA selected;
    MENUITEMINFOA menuItem;
    int columnOrder[7];
    TBBUTTON buttons[20];
    char text[500];
    PAINTSTRUCT paint;
    char menuText[256], filename[256], drivePath[256], volumeName[256], fileSystemName[256];
    switch (message)
    {
    case WM_DESTROY:
        SaveBrowserSettings();
        Closing = true;
        if (CPU_Running)
        {
            CPU_ActionFlags |= 0x400000;
            ResumeThread(CPU_Thread);
            int timeout = 30;
            do
            {
                Sleep(100);
                timeout--;
                if (!CPU_Thread) break;
            } while (timeout >= 0);
        }
        DestroyMenu(DriveMenu);
        if (FavouritesMenu)
        {
            DestroyMenu(FavouritesMenu);
            FavouritesMenu = NULL;
        }
        PostQuitMessage(0);
        break;
    case WM_MOVE:
        {
            PluginDescriptor* graphics = SelectedGraphicsPlugin;
            MainWindowPosition.x = LOWORD(lParam);
            MainWindowPosition.y = HIWORD(lParam) + 27;
            if (graphics->MoveScreen)
                graphics->MoveScreen(MainWindowPosition.x, MainWindowPosition.y);
            break;
        }
    case WM_SIZE:
        {
            int toolbarHeight = WindowHeight(ToolbarWindow);
            int statusHeight = WindowHeight(StatusWindow);
            MoveWindow(BrowserWindow, 0, toolbarHeight, LOWORD(lParam),
                HIWORD(lParam) - statusHeight - toolbarHeight, TRUE);
            MoveWindow(StatusWindow, 0, 0, 0, 0, FALSE);
            if (!BrowserInitialized)
            {
                BrowserInitialized = true;
                RefreshRomBrowser();
            }
            break;
        }
    case WM_CREATE:
        {
            MainWindow = window;
            InitCommonControls();
            BrowserWindow = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "RomSelector",
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_AUTOARRANGE, 0, 0, 100, 100,
                MainWindow, NULL, ApplicationInstance, NULL);
            StatusWindow = CreateWindowExA(0, STATUSCLASSNAMEA, "StatusBar", WS_CHILD | WS_VISIBLE,
                0, 0, 100, 100, MainWindow, NULL, ApplicationInstance, NULL);
            ToolbarWindow = CreateWindowExA(0, TOOLBARCLASSNAMEA, "ToolBar", WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT,
                0, 0, 100, 100, MainWindow, NULL, ApplicationInstance, NULL);
            BrowserImageList = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 15, 1);
            HBITMAP bitmap = (HBITMAP)LoadImageA(ApplicationInstance, MAKEINTRESOURCEA(IDB_BITMAP143),
                IMAGE_BITMAP, 0, 0, LR_COLOR);
            ImageList_AddMasked(BrowserImageList, bitmap, RGB(255, 0, 255));
            DeleteObject(bitmap);
            DriveMenu = CreatePopupMenu();
            menuItem.cbSize = sizeof(menuItem);
            menuItem.fMask = MIIM_TYPE | MIIM_ID;
            menuItem.fType = MFT_STRING;
            menuItem.dwTypeData = menuText;
            DWORD drives = GetLogicalDrives();
            for (unsigned int drive = 0; drive <= 26; drive++)
                if (drives & (1 << drive))
                {
                    menuItem.wID = drive + 1;
                    strcpy(text, "A:");
                    text[0] = (char)(drive + 'A');
                    strcpy(drivePath, text);
                    strcat(drivePath, "\\");
                    UINT type = GetDriveTypeA(drivePath);
                    if (type >= DRIVE_FIXED && type <= DRIVE_CDROM &&
                        GetVolumeInformationA(drivePath, volumeName, 256, NULL,
                            &volumeInformation.maximumNameLength, &volumeInformation.fileSystemFlags, fileSystemName, 256))
                    {
                        strcpy(menuText, volumeName);
                        strcat(menuText, " (");
                        strcat(menuText, text);
                        strcat(menuText, ")");
                    }
                    else strcpy(menuText, text);
                    InsertMenuItemA(DriveMenu, drive, TRUE, &menuItem);
                }
            FavouritesMenu = NULL;
            SendMessageA(ToolbarWindow, TB_SETIMAGELIST, 0, (LPARAM)BrowserImageList);
            memset(buttons, 0, sizeof(buttons));
            count = 0;
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 4, TBSTATE_ENABLED, ID_DIRECTORY_UP);
            AddToolbarButton(buttons, &count, TBSTYLE_DROPDOWN, 1, TBSTATE_ENABLED, 0x801e);
            AddToolbarButton(buttons, &count, TBSTYLE_DROPDOWN, 7, TBSTATE_ENABLED, 0x801f);
            AddToolbarButton(buttons, &count, TBSTYLE_SEP, 0, TBSTATE_ENABLED, ID_DIRECTORY_UP);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 12, TBSTATE_ENABLED, 0x8026);
            AddToolbarButton(buttons, &count, TBSTYLE_CHECK, 13, TBSTATE_ENABLED, ID_PAUSE);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 14, TBSTATE_ENABLED, ID_CLOSE_ROM);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 15, TBSTATE_ENABLED, ID_RESET);
            AddToolbarButton(buttons, &count, TBSTYLE_SEP, 0, TBSTATE_ENABLED, ID_DIRECTORY_UP);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 8, TBSTATE_ENABLED, ID_SETTINGS);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 9, TBSTATE_ENABLED, ID_CONFIGURE_AUDIO_PLUGIN);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 10, TBSTATE_ENABLED, ID_CONFIGURE_INPUT_PLUGIN);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 11, TBSTATE_ENABLED, ID_CONFIGURE_VIDEO_PLUGIN);
            AddToolbarButton(buttons, &count, TBSTYLE_SEP, 0, TBSTATE_ENABLED, ID_DIRECTORY_UP);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 16, TBSTATE_ENABLED, ID_ABOUT);
            SendMessageA(ToolbarWindow, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
            SendMessageA(ToolbarWindow, TB_ADDBUTTONSA, count, (LPARAM)buttons);
            SendMessageA(ToolbarWindow, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
            SendMessageA(ToolbarWindow, TB_AUTOSIZE, 0, 0);
            SendMessageA(BrowserWindow, LVM_SETEXTENDEDLISTVIEWSTYLE, 0x30, 0x30);
            SendMessageA(BrowserWindow, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)BrowserImageList);
            SendMessageA(BrowserWindow, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)BrowserImageList);
            column.mask = LVCF_TEXT | LVCF_WIDTH;
            column.pszText = columnName;
            strcpy(columnName, "Rom name");
            column.cx = ReadIniInteger("Configuration", "Column0Width", 120, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 0, (LPARAM)&column);
            strcpy(columnName, "Filename");
            column.cx = ReadIniInteger("Configuration", "Column1Width", 120, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 1, (LPARAM)&column);
            strcpy(columnName, "Size");
            column.cx = ReadIniInteger("Configuration", "Column2Width", 59, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 2, (LPARAM)&column);
            strcpy(columnName, "Rating");
            column.cx = ReadIniInteger("Configuration", "Column3Width", 43, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 3, (LPARAM)&column);
            strcpy(columnName, "Comment");
            column.cx = ReadIniInteger("Configuration", "Column4Width", 240, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 4, (LPARAM)&column);
            strcpy(columnName, "INI Section");
            column.cx = ReadIniInteger("Configuration", "Column5Width", 0, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 5, (LPARAM)&column);
            strcpy(columnName, "CRC1");
            column.cx = ReadIniInteger("Configuration", "Column6Width", 0, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 6, (LPARAM)&column);
            if (GetPrivateProfileStructA("Configuration", "ColumnOrder", columnOrder,
                sizeof(columnOrder), ConfigurationIniPath))
                SendMessageA(BrowserWindow, LVM_SETCOLUMNORDERARRAY, 7, (LPARAM)columnOrder);
            if (ShowErrorMessages)
                CheckMenuItem(GetMenu(MainWindow), ID_SHOW_ERRORS, MF_CHECKED);
            else
                CheckMenuItem(GetMenu(MainWindow), ID_SHOW_ERRORS, MF_UNCHECKED);
            EnableMenuItem(GetMenu(MainWindow), ID_USE_VIRTUAL_SAVE_FILES, MF_GRAYED);
            DragAcceptFiles(MainWindow, TRUE);
            return 1;
        }
    case WM_PAINT:
        {
            BeginPaint(window, &paint);
            EndPaint(window, &paint);
            if (CPU_Running && SelectedGraphicsPlugin->DrawScreen)
                SelectedGraphicsPlugin->DrawScreen();
            break;
        }
    case WM_COMMAND:
        return MainWindowCommand(window, message, wParam, lParam, 0);
    case WM_NOTIFY:
        {
            notification = (NMHDR*)lParam;
            notificationWindow = notification->hwndFrom;
            if (notificationWindow == BrowserWindow)
            {
                list = (NMLISTVIEW*)notification;
                switch ((int)notification->code)
                {
                case LVN_COLUMNCLICK:
                    {
                        int order = list->iSubItem + 1;
                        if (BrowserSortMode == order)
                            BrowserSortMode = -order;
                        else
                            BrowserSortMode = order;
                        SendMessageA(BrowserWindow, LVM_SORTITEMS, BrowserSortMode, (LPARAM)CompareBrowserItems);
                        break;
                    }
                case LVN_ITEMCHANGED:
                    {
                        item.iSubItem = 5;
                        item.cchTextMax = 100;
                        item.pszText = CurrentRom.InternalName;
                        SendMessageA(BrowserWindow, LVM_GETITEMTEXTA, list->iItem, (LPARAM)&item);
                        item.iSubItem = 6;
                        item.cchTextMax = 11;
                        item.pszText = CurrentRom.IniSection;
                        SendMessageA(BrowserWindow, LVM_GETITEMTEXTA, list->iItem, (LPARAM)&item);
                        break;
                    }
                case NM_RETURN:
                case NM_DBLCLK:
                    if (list->iItem != -1)
                    {
                        selected.iItem = SendMessageA(BrowserWindow, LVM_GETSELECTIONMARK, 0, 0);
                        selected.iSubItem = 0;
                        selected.mask = LVIF_IMAGE;
                        SendMessageA(BrowserWindow, LVM_GETITEMA, 0, (LPARAM)&selected);
                        if (selected.iImage == 6)
                        {
                            if (!CPU_Running)
                            {
                                item.pszText = text;
                                item.iSubItem = 1;
                                item.cchTextMax = 256;
                                SendMessageA(BrowserWindow, LVM_GETITEMTEXTA, selected.iItem, (LPARAM)&item);
                                SetCurrentDirectoryA(RomDirectory);
                                strcpy(SelectedRomFilename, RomDirectory);
                                strcat(SelectedRomFilename, text);
                                StartCPUThread();
                            }
                        }
                        else
                        {
                            item.pszText = text;
                            item.iSubItem = 0;
                            item.cchTextMax = MAX_PATH;
                            SendMessageA(BrowserWindow, LVM_GETITEMTEXTA, selected.iItem, (LPARAM)&item);
                            strcat(RomDirectory, text);
                            strcat(RomDirectory, "\\");
                            RefreshRomBrowser();
                        }
                    }
                    break;
                }
            }
            else if (notificationWindow == ToolbarWindow)
            {
                toolbar = (NMTOOLBARA*)notification;
                if (notification->code != TBN_DROPDOWN)
                {
                    if (notification->code == NM_RCLICK && toolbar->iItem == ID_ABOUT)
                    {
                        unsigned int position = EasterTextPosition;
                        do
                        {
                            position++;
                            if (position >= (unsigned int)EasterTextLength) break;
                        } while (EasterText[position] != '\n');
                        strncpy(text, EasterText + EasterTextPosition, position - EasterTextPosition);
                        text[position - EasterTextPosition] = 0;
                        EasterTextPosition = position + 1;
                        if (EasterTextPosition > (unsigned int)EasterTextLength) EasterTextPosition = 0;
                        SetStatusText(0, text);
                    }
                }
                else
                {
                    switch (toolbar->iItem)
                    {
                    case 0x801e: ShowDriveMenu(); break;
                    case 0x801f: ShowFavouritesMenu(); break;
                    }
                }
            }
            break;
        }
    case WM_DROPFILES:
        {
            DragQueryFileA((HDROP)wParam, 0, filename, 256);
            DragFinish((HDROP)wParam);
            if (CPU_Running)
            {
                sprintf(text, "Load savestate file %s?", filename);
                if (MessageBoxA(MainWindow, text, "Load state file", MB_YESNO | MB_ICONQUESTION) == IDYES)
                {
                    strcpy(StateFileName, filename);
                    SetStatusText(0, "Loading state...");
                    CPU_ActionFlags |= 0x2000;
                }
            }
            else
            {
                sprintf(text, "Load rom file %s?", filename);
                if (MessageBoxA(MainWindow, text, "Load rom file", MB_YESNO | MB_ICONQUESTION) == IDYES)
                {
                    strcpy(SelectedRomFilename, filename);
                    StartCPUThread();
                }
            }
            break;
        }
    case WM_USER + 10:
        break;
    default:
        return DefWindowProcA(window, message, wParam, lParam);
    }
    return 0;
}

BOOL CALLBACK RemoveFavouritesDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM)
{
    char oldName[256], newName[256], text[256], filename[256], line[256];
    switch (message) {
    case WM_INITDIALOG: {
        strcpy(filename, ApplicationDirectory);
        strcat(filename, "fav.dat");
        FILE* file = fopen(filename, "rt");
        if (file) {
            while (!feof(file)) {
                if (!fgets(text, 256, file))
                    break;
                if (text[strlen(text) - 1] == '\n')
                    text[strlen(text) - 1] = '\0';
                SendMessageA(FindWindowExA(window, NULL, "LISTBOX", NULL), LB_ADDSTRING, 0, (LPARAM)text);
            }
            fclose(file);
        }
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1035:
            EndDialog(window, 1035);
            return TRUE;
        case 1034: {
            strcpy(oldName, ApplicationDirectory);
            strcat(oldName, "fav.dat");
            FILE* input = fopen(oldName, "rt");
            strcpy(newName, oldName);
            newName[strlen(newName) - 1] = 'n';
            FILE* output = fopen(newName, "w+t");
            int index = 0;
            if (input && output) {
                while (!feof(input)) {
                    if (!fgets(line, 256, input))
                        break;
                    if (SendMessageA(FindWindowExA(window, NULL, "LISTBOX", NULL), LB_GETSEL, index, 0) <= 0)
                        fprintf(output, "%s", line);
                    ++index;
                }
                fclose(output);
                fclose(input);
                DeleteFileA(oldName);
                MoveFileA(newName, oldName);
            }
            EndDialog(window, 1034);
            return TRUE;
        }
        }
    }
    return FALSE;
}

BOOL CALLBACK AboutDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(window, LOWORD(wParam));
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_DISCLAIMER) {
            DialogBoxParamA(ApplicationInstance, MAKEINTRESOURCEA(IDD_DISCLAIMER), window, DisclaimerDialogProc, 0);
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_WEBSITE) {
            ShellExecuteA(window, "open", "http://www.nemu.com", "", "", SW_SHOW);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CALLBACK DisclaimerDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM)
{
    char text[1000];
    switch (message) {
    case WM_INITDIALOG:
        LoadStringA(ApplicationInstance, IDS_STRING110, text, 1000);
        SendMessageA(FindWindowExA(window, NULL, "Edit", NULL), WM_SETTEXT, 0, (LPARAM)text);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            EndDialog(window, IDOK);
            return TRUE;
        }
    }
    return FALSE;
}
