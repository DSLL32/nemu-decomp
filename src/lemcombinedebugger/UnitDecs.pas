unit UnitDecs;

interface

uses Windows, RCP;

type
  TByteArray = array[0..$7FFFFFFE] of Byte;
  PByteArray = ^TByteArray;
  TTranslateAddress = function(Address: Cardinal; WriteAccess: LongBool): Cardinal; stdcall;
  TWriteWord = procedure(Address, Value: Cardinal); stdcall;
  PInt64RegisterArray = ^TInt64RegisterArray;
  TInt64RegisterArray = array[0..31] of Int64;
  PCardinalRegisterArray = ^TCardinalRegisterArray;
  TCardinalRegisterArray = array[0..31] of Cardinal;
  PCardinalArray = ^TCardinalArray;
  TCardinalArray = array[0..$FFFF] of Cardinal;
  TIntegerRegisterArray = array[0..31] of Integer;
  PIntegerRegisterArray = ^TIntegerRegisterArray;
  TRSPFlags = array[0..2] of Word;
  PRSPFlags = ^TRSPFlags;
  TRSPVectors = array[0..31] of array[0..7] of Word;
  PRSPVectors = ^TRSPVectors;
  TMIRegisters = record
    Unknown00, InterruptMask, Interrupt: Cardinal;
  end;
  PMIRegisters = ^TMIRegisters;
  TBlockCounts = array[0..$1FFFFF] of Cardinal;
  PBlockCounts = ^TBlockCounts;
  TOpcodeBreakpoint = packed record
    Mask, Opcode: Cardinal;
  end;
  TOpcodeBreakpointArray = array[0..$FFFF] of TOpcodeBreakpoint;
  POpcodeBreakpointArray = ^TOpcodeBreakpointArray;
  TOpcodeStats = array[0..$21F] of Cardinal;
  POpcodeStats = ^TOpcodeStats;
  TRDPHistory = array[0..2047] of Cardinal;
  PRDPHistory = ^TRDPHistory;
  TWatch = packed record
    Address, WatchType: Cardinal;
  end;
  PWatchArray = ^TWatchArray;
  TWatchArray = array[0..$FFFF] of TWatch;
  TRSPControlRegisters = record
    Unknown00: array[$00..$13] of Byte;
    Semaphore: Cardinal;
    PC: Cardinal;
  end;
  PRSPControlRegisters = ^TRSPControlRegisters;
  TReadWord = function(Address: Cardinal): Cardinal; stdcall;
  TBreakpoint = packed record
    Instruction: Cardinal;
    Address: Cardinal;
    Unknown08: Cardinal;
  end;
  PBreakpointArray = ^TBreakpointArray;
  TBreakpointArray = array[0..$FFFF] of TBreakpoint;

  { Field names are inferred from their uses. Unused ABI slots retain their
    observed offsets until the corresponding host declarations are recovered. }
  PCombineModes = ^TCombineModes;
  TCombineMode = record
    Lo, Hi, Extras: Cardinal;
    Highlight: LongBool;
    Age, EmulationMode: Cardinal;
  end;
  TCombineModes = record
    ReloadDatFile: Integer;
    AddX, AddY, AddZ, RotX, RotY, RotZ: Single;
    Count: Cardinal;
    Modes: array[0..299] of TCombineMode;
  end;
  TNemu = record
    MainWindow: PHandle;                     { $00 }
    RDRAM: PByteArray;                       { $04 }
    RamSize: PCardinal;                      { $08 }
    Unknown0C: array[$0C..$13] of Byte;
    RomSize: PCardinal;                      { $14 }
    Unknown18: Cardinal;
    RomName: PChar;                          { $1C }
    RomInternalName: PChar;                 { $20 }
    CPUProgramCounter: PCardinal;           { $24 }
    CPURegisters: PInt64RegisterArray;       { $28 }
    COP0Registers: PCardinalRegisterArray;   { $2C }
    FPURegisters32: PIntegerRegisterArray;   { $30 }
    FCR31: PCardinal;                       { $34 }
    RSPRegisters: PCardinalRegisterArray;    { $38 }
    RSPFlags: PRSPFlags;                    { $3C }
    RSPAccumulator: PInt64RegisterArray;     { $40 }
    RSPVectors: PRSPVectors;                { $44 }
    MultLO: PInt64;                         { $48 }
    MultHI: PInt64;                         { $4C }
    Unknown50: Cardinal;
    DebugFlags: PCardinal;                  { $54 }
    EmulationThread: PHandle;               { $58 }
    RegisterStatsEnabled: PBoolean;         { $5C }
    OpcodeStatsEnabled: PBoolean;           { $60 }
    RegisterStats: PCardinalRegisterArray;  { $64 }
    OpcodeStats: POpcodeStats;              { $68 }
    VICount: PCardinal;                     { $6C }
    OpcodeBreakpoints: POpcodeBreakpointArray; { $70 }
    OpcodeBreakpointCount: PCardinal;       { $74 }
    RSPBreakpoints: PCardinalArray;         { $78 }
    RSPBreakpointCount: PCardinal;          { $7C }
    Watches: PWatchArray;                  { $80 }
    WatchCount: PCardinal;                 { $84 }
    LastProgramCounter: PCardinal;          { $88 }
    TranslateAddress: TTranslateAddress;     { $8C }
    TranslateError: PBoolean;               { $90 }
    ReadWord: TReadWord;                     { $94 }
    WriteWord: TWriteWord;                  { $98 }
    ReadError: PBoolean;                     { $9C }
    ShowErrors: PBoolean;                    { $A0 }
    Breakpoints: PBreakpointArray;           { $A4 }
    BreakpointCount: PCardinal;              { $A8 }
    RSPControlRegisters: PRSPControlRegisters; { $AC }
    MIRegisters: PMIRegisters;             { $B0 }
    RomFilename: PChar;                      { $B4 }
    UnknownB8: Cardinal;
    FPURegisters64: PInt64RegisterArray;     { $BC }
    RSPTask: PCardinal;                     { $C0 }
    Running: PBoolean;                       { $C4 }
    RomCRC: PChar;                           { $C8 }
    CombineModes: ^PCombineModes;
    UndoEnabled: PBoolean;                 { $D0 }
    UndoData: Pointer;                     { $D4 }
    UndoPos: PCardinal;                    { $D8 }
    BlockExecutionCounts: PBlockCounts;      { $DC }
    BlockCycleCounts: PBlockCounts;          { $E0 }
    BlockStatsEnabled: PBoolean;             { $E4 }
    UnknownE8: Cardinal;
    RDPHistory: PRDPHistory;                { $EC }
    RDPHistoryPosition: PCardinal;          { $F0 }
  end;
  PNemu = ^TNemu;

{ Global and ABI field names are inferred. The host interface layout is
  shared with Nemu's debugger plugin. Imported-data hashes constrain names
  but do not establish their original spelling. }
var
  Nemu64_API: TNemu;
  NemuMainWinHandle: THandle;


implementation

end.
