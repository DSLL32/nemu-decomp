unit UnitDecs;

interface

uses Windows, RCP;

type
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
  TNemu = record
    MainWindow: PHandle;                     { $00 }
    Instance: THandle;                       { $04 }
    Unknown08: array[$08..$13] of Byte;
    RomSize: PCardinal;                      { $14 }
    Unknown18: Cardinal;
    RomName: PChar;                          { $1C }
    Unknown20: array[$20..$93] of Byte;
    ReadWord: TReadWord;                     { $94 }
    Unknown98: Cardinal;
    ReadError: PBoolean;                     { $9C }
    ShowErrors: PBoolean;                    { $A0 }
    Breakpoints: PBreakpointArray;           { $A4 }
    BreakpointCount: PCardinal;              { $A8 }
    UnknownAC: array[$AC..$B3] of Byte;
    RomFilename: PChar;                      { $B4 }
    UnknownB8: array[$B8..$C3] of Byte;
    Running: PBoolean;                       { $C4 }
    RomCRC: PChar;                           { $C8 }
    UnknownCC: array[$CC..$F3] of Byte;
  end;
  PNemu = ^TNemu;

var
  Nemu: TNemu;
  NemuInstance: THandle;

function ReadWord(Address: Cardinal): Cardinal;

implementation

function ReadWord(Address: Cardinal): Cardinal;
var
  ShowErrors: Boolean;
  I: Cardinal;
begin
  ShowErrors := Nemu.ShowErrors^;
  Nemu.ReadError^ := False;
  Nemu.ShowErrors^ := False;
  Result := Nemu.ReadWord(Address);
  Nemu.ShowErrors^ := ShowErrors;
  if Result shr 26 = $3B then
    if Nemu.BreakpointCount^ > 0 then
      for I := Nemu.BreakpointCount^ - 1 downto 0 do
        if Nemu.Breakpoints^[I].Address = Address then
        begin
          Result := Nemu.Breakpoints^[I].Instruction;
          Break;
        end;
end;

end.
