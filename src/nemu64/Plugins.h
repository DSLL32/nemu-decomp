#ifndef NEMU64_PLUGINS_H
#define NEMU64_PLUGINS_H
#ifndef WINVER
#define WINVER 0x0500
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#include <windows.h>

// Names are inferred. Original storage locations and ABI offsets are recorded
// in analysis/nemu-plugins.json; these are ordinary linker-resolved objects.
struct PluginInfo {
    WORD Version;
    WORD Type;
    char Name[100];
    BOOL NormalMemory;
    BOOL MemoryBswapped;
};
struct GraphicsInfo {
    HWND Window;
    HWND StatusBar;
    BOOL MemoryBswapped;
    unsigned char *Header;
    unsigned char *RDRAM;
    unsigned char *DMEM;
    unsigned char *IMEM;
    unsigned int *MIInterrupt;
    unsigned int *DPCStart, *DPCEnd, *DPCCurrent, *DPCStatus;
    unsigned int *DPCClock, *DPCBufferBusy, *DPCPipeBusy, *DPCTMEM;
    unsigned int *VIStatus, *VIOrigin, *VIWidth, *VIInterrupt;
    unsigned int *VICurrent, *VITiming, *VIVSync, *VIHSync;
    unsigned int *VILeap, *VIHStart, *VIVStart, *VIBurst;
    unsigned int *VIXScale, *VIYScale;
    void (__cdecl *CheckInterrupts)();
};
struct AudioInfo;
struct PluginDescriptor {
    unsigned int Size;
    PluginInfo Info;
    char Filename[MAX_PATH];
    void (__cdecl *AiDacrateChanged)(int);
    void (__cdecl *AiLenChanged)();
    unsigned int (__cdecl *AiReadLength)();
    void (__cdecl *AiUpdate)(BOOL);
    void (__cdecl *ChangeWindow)();
    void (__cdecl *CloseDLL)();
    void (__cdecl *DllAbout)(HWND);
    void (__cdecl *DllConfig)(HWND);
    void (__cdecl *DllTest)(HWND);
    void (__cdecl *DrawScreen)();
    void (__cdecl *GetDllInfo)(PluginInfo *);
    BOOL (__cdecl *InitiateGFX)(GraphicsInfo);
    BOOL (__cdecl *InitiateAudio)(AudioInfo);
    void (__cdecl *MoveScreen)(int, int);
    void (__cdecl *ProcessAList)();
    void (__cdecl *ProcessDList)();
    void (__cdecl *RomClosed)();
    void (__cdecl *RomOpen)();
    void (__cdecl *UpdateScreen)();
    void (__cdecl *ViStatusChanged)();
    void (__cdecl *ViWidthChanged)();
    void (__cdecl *SetResolution)(int, int);
    void (__cdecl *FillRectHack)();
    void (__cdecl *VISetUCodeID)(unsigned int);
    void (__cdecl *GetCombinerPointer)(void **);
};
struct DebugBreakpoint {
    unsigned int Instruction, Address, CodeOffset;
};
struct DebugWatch { unsigned int Address, Type; };
struct DebugOpcodeBreakpoint { unsigned int Mask, Opcode; };
struct NemuHostInterface {
    HWND *MainWindow;
    unsigned char *RDRAM;
    unsigned int *RamSize;
    unsigned char *PeripheralMemory;
    unsigned char *ROM;
    unsigned int *RomSize;
    unsigned char *PIFRam;
    char *RomName;
    char *RomInternalName;
    int *CPUProgramCounter;
    __int64 *CPURegisters;
    unsigned int *COP0Registers;
    unsigned int *FPURegisters32;
    unsigned int *FCR31;
    unsigned int *RSPRegisters;
    unsigned short *RSPFlags;
    __int64 *RSPAccumulator;
    unsigned short (*RSPVectors)[8];
    __int64 *MultLO, *MultHI;
    unsigned int *CPUInstruction;
    unsigned int *DebugFlags;
    HANDLE *EmulationThread;
    unsigned int *RegisterStatsEnabled;
    unsigned int *OpcodeStatsEnabled;
    unsigned int *RegisterStats;
    unsigned int *OpcodeStats;
    unsigned int *VICount;
    DebugOpcodeBreakpoint *OpcodeBreakpoints;
    unsigned int *OpcodeBreakpointCount;
    unsigned int *RSPBreakpoints;
    unsigned int *RSPBreakpointCount;
    DebugWatch *Watches;
    unsigned int *WatchCount;
    unsigned int *LastProgramCounter;
    unsigned int (__stdcall *TranslateAddress)(unsigned int, unsigned char);
    unsigned int *TranslateError;
    unsigned int (__stdcall *ReadWord)(unsigned int);
    void (__stdcall *WriteWord)(unsigned int, unsigned int);
    int *ReadError;
    int *ShowErrors;
    DebugBreakpoint *Breakpoints;
    unsigned int *BreakpointCount;
    unsigned int *RSPControlRegisters;
    unsigned int *MIRegisters;
    char *RomFilename;
    void *ControllerData;
    __int64 *FPURegisters64;
    unsigned int *RSPTask;
    unsigned int *Running;
    char *RomCRC;
    void **GraphicsCombiner;
    bool *UndoEnabled;
    unsigned char *UndoData;
    unsigned int *UndoPosition;
    unsigned int *BlockExecutionCounts;
    unsigned int *BlockCycleCounts;
    unsigned int *BlockStatsEnabled;
    unsigned int *DPCRegisters;
    unsigned int *RDPHistory;
    unsigned int *RDPHistoryPosition;
};
struct ToolDescriptor {
    unsigned int Flags;
    char MenuText[1000];
    unsigned int (__cdecl *Init)(NemuHostInterface *);
    void (__cdecl *Free)();
    void (__cdecl *Invoke)(unsigned int, unsigned int);
    void (__cdecl *GetMenuText)(char *);
};
extern HWND MainWindow;
extern char ApplicationDirectory[MAX_PATH];
extern char PluginDirectory[MAX_PATH];
extern PluginDescriptor PluginCatalog[40];
extern unsigned int PluginCount;
extern ToolDescriptor ToolCatalog[40];
extern unsigned int ToolCount;
extern HMODULE PluginModules[30];
extern unsigned int PluginModuleCount;
extern PluginDescriptor *SelectedGraphicsPlugin;
extern PluginDescriptor *PendingGraphicsPlugin;
extern void *GraphicsCombiner;
extern void (__cdecl *CIInit)(HWND, void *);
extern void (__cdecl *CIFree)();
extern void (__cdecl *CIGetJoyData)();
extern void (__cdecl *CIRumble)(unsigned int);
extern void (__cdecl *CIConfigure)(HWND);
extern void (__cdecl *AIInit)(HWND, unsigned char *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
extern void (__cdecl *AIFree)();
extern void (__cdecl *AISetReg)(unsigned int, unsigned int);
extern unsigned int (__cdecl *AIGetReg)(unsigned int);
extern void (__cdecl *AIConfigure)(HWND);
extern void (__cdecl *AUDList)(unsigned int);
extern void (__cdecl *AISetUCodeID)(unsigned int);
extern void (__cdecl *NemuSettings)(unsigned int, PluginDescriptor *);
extern void (__cdecl *DCTList)(unsigned int);

bool LoadPlugin(char *filename);
bool LoadPlugins();
void UnloadPlugins();
void InitializeTools();
void FreeTools();
bool SelectGraphicsPlugin();
void ReloadConfiguration();
void InitializeGraphicsPlugin();
void CommitRDRAM();
void DecommitRDRAM();

struct ControllerState {
    bool Present, MemPak, RumblePak, RumbleActive;
    unsigned int Buttons;
};
extern ControllerState Controllers[4];
extern unsigned int NetPlayActive;
extern int NetPlayPlayer;
extern int NetPlayPlayerCount;
extern unsigned int VICount;
extern unsigned int RSPBreakpointCount;
extern unsigned int RSPBreakpoints[100];
extern unsigned int RegisterStatsEnabled;
extern unsigned int OpcodeStatsEnabled;
extern unsigned int RegisterStats[32];
extern unsigned int OpcodeStats[0x220];
extern DebugOpcodeBreakpoint OpcodeBreakpoints[256];
extern unsigned int OpcodeBreakpointCount;
extern DebugBreakpoint CPUBreakpoints[100000];
extern unsigned int CPUBreakpointCount;
extern bool UndoEnabled;
extern unsigned char UndoData[0x400000];
extern unsigned int UndoPosition;
extern unsigned int BlockExecutionCounts[0x200000];
extern unsigned int BlockCycleCounts[0x200000];
extern unsigned int BlockStatsEnabled;
extern unsigned int RDPHistory[2048];
extern unsigned int RDPHistoryPosition;
extern char SaveStateDirectory[260];
extern unsigned char RomHeader[64];
extern unsigned int GraphicsInterruptRegister;
extern int ShowErrorMessages;
extern char SelectedRomFilename[256];

void NotifyTools(unsigned int mask, unsigned int reason);
int __stdcall NetplayStartGame(char *game, int player, int playerCount);
void __stdcall NetplayChatReceived(char *nick, char *text);
void __stdcall NetplayClientDropped(char *nick, int player);
void CheckGraphicsInterrupts();

// These interfaces cross DLL boundaries and must retain their 32-bit layouts.
typedef char PluginInfoSizeCheck[sizeof(PluginInfo) == 112 ? 1 : -1];
typedef char GraphicsInfoSizeCheck[sizeof(GraphicsInfo) == 124 ? 1 : -1];
typedef char PluginDescriptorSizeCheck[sizeof(PluginDescriptor) == 476 ? 1 : -1];
typedef char NemuHostInterfaceSizeCheck[sizeof(NemuHostInterface) == 244 ? 1 : -1];
typedef char ToolDescriptorSizeCheck[sizeof(ToolDescriptor) == 1020 ? 1 : -1];
typedef char ControllerStateSizeCheck[sizeof(ControllerState) == 8 ? 1 : -1];

#endif
