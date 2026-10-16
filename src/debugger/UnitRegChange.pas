unit UnitRegChange;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls, Menus, Buttons, CheckLst;

type
  TFormDebugRegisterChange = class(TForm)
    PageControlWhichAccess: TPageControl;
    TabSheetRead: TTabSheet;
    TabSheet1: TTabSheet;
    CheckListBoxWrite: TCheckListBox;
    CheckListBoxRead: TCheckListBox;
    procedure CheckListBoxClickCheck(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
  end;

var
  FormDebugRegisterChange: TFormDebugRegisterChange;
  RegBreakpointEnabled: Boolean;
  RegisterReadBreakpoints: array[0..31] of Boolean;
  RegisterWriteBreakpoints: array[0..31] of Boolean;

procedure CheckRegisterAccess(Opcode: Cardinal);

implementation

uses UnitDecs;

{$R *.dfm}

procedure CheckRegisters(WriteReg, ReadReg1, ReadReg2: Cardinal);
begin
  if ((WriteReg <> 34) and RegisterWriteBreakpoints[WriteReg]) or
     ((ReadReg1 <> 34) and RegisterReadBreakpoints[ReadReg1]) or
     ((ReadReg2 <> 34) and RegisterReadBreakpoints[ReadReg2]) then
    Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $100000;
end;

procedure CheckRegisterAccess(Opcode: Cardinal);

  function RD: Cardinal;
  begin
    Result := (Opcode shr 11) and 31;
  end;

  function RS: Cardinal;
  begin
    Result := (Opcode shr 21) and 31;
  end;

  function RT: Cardinal;
  begin
    Result := (Opcode shr 16) and 31;
  end;

begin
  case Opcode shr 26 of
    $00:
      case Opcode and $3F of
        $20: CheckRegisters(RD, RS, RS);
        $21: CheckRegisters(RD, RS, RS);
        $24: CheckRegisters(RD, RS, RS);
        $2C: CheckRegisters(RD, RS, RS);
        $2D: CheckRegisters(RD, RS, RS);
        $1E: CheckRegisters($22, RS, RS);
        $1F: CheckRegisters($22, RS, RS);
        $1A: CheckRegisters($22, RS, RS);
        $1B: CheckRegisters($22, RS, RS);
        $1C: CheckRegisters($22, RS, RS);
        $1D: CheckRegisters($22, RS, RS);
        $38: CheckRegisters(RD, RS, $22);
        $3C: CheckRegisters(RD, RS, $22);
        $14: CheckRegisters(RD, RS, RS);
        $3B: CheckRegisters(RD, RS, $22);
        $3F: CheckRegisters(RD, RT, $22);
        $17: CheckRegisters(RD, RT, RS);
        $3A: CheckRegisters(RD, RT, $22);
        $3E: CheckRegisters(RD, RT, $22);
        $16: CheckRegisters(RD, RT, RS);
        $2E: CheckRegisters(RD, RT, RS);
        $2F: CheckRegisters(RD, RT, RS);
        $09: CheckRegisters(RD, $22, RS);
        $08: CheckRegisters($22, RS, $22);
        $10: CheckRegisters($22, RD, $22);
        $12: CheckRegisters(RD, $22, $22);
        $11: CheckRegisters($22, RS, $22);
        $13: CheckRegisters(RS, $22, $22);
        $18: CheckRegisters($22, RS, RT);
        $19: CheckRegisters($22, RS, RT);
        $27: CheckRegisters(RD, RS, RT);
        $25: CheckRegisters(RD, RS, RT);
        $00: CheckRegisters(RD, RT, $22);
        $04: CheckRegisters(RD, RT, RS);
        $2A: CheckRegisters(RD, RS, RT);
        $2B: CheckRegisters(RD, RS, RT);
        $03: CheckRegisters(RD, RT, $22);
        $07: CheckRegisters(RD, RT, RS);
        $02: CheckRegisters(RD, RT, $22);
        $06: CheckRegisters(RD, RT, RS);
        $22: CheckRegisters(RD, RS, RT);
        $23: CheckRegisters(RD, RS, RT);
        $34: CheckRegisters($22, RS, RT);
        $30: CheckRegisters($22, RS, RT);
        $31: CheckRegisters($22, RS, RT);
        $32: CheckRegisters($22, RS, RT);
        $33: CheckRegisters($22, RS, RT);
        $36: CheckRegisters($22, RS, RT);
        $26: CheckRegisters(RD, RS, RT);
      end;
    $01: CheckRegisters($22, RS, $22);
    $10:
      case RS of
        0: CheckRegisters(RT, $22, $22);
        4: CheckRegisters($22, RT, $22);
      end;
    $11:
      case RS of
        $01: CheckRegisters(RT, $22, $22);
        $05: CheckRegisters($22, RT, $22);
        $00: CheckRegisters(RT, $22, $22);
        $04: CheckRegisters($22, RT, $22);
        $02: CheckRegisters(RT, $22, $22);
        $06: CheckRegisters($22, RT, $22);
      end;
    $08: CheckRegisters(RT, RS, $22);
    $09: CheckRegisters(RT, RS, $22);
    $0C: CheckRegisters(RT, RS, $22);
    $04: CheckRegisters($22, RS, RT);
    $14: CheckRegisters($22, RS, RT);
    $07: CheckRegisters($22, RS, $22);
    $17: CheckRegisters($22, RS, RT);
    $06: CheckRegisters($22, RS, RT);
    $16: CheckRegisters($22, RS, RT);
    $05: CheckRegisters($22, RS, RT);
    $15: CheckRegisters($22, RS, RT);
    $18: CheckRegisters(RT, RS, $22);
    $19: CheckRegisters(RT, RS, $22);
    $20: CheckRegisters(RT, RS, $22);
    $24: CheckRegisters(RT, RS, $22);
    $37: CheckRegisters(RT, RS, $22);
    $1A: CheckRegisters(RT, RS, $22);
    $1B: CheckRegisters(RT, RS, $22);
    $21: CheckRegisters(RT, RS, $22);
    $25: CheckRegisters(RT, RS, $22);
    $0F: CheckRegisters(RT, $22, $22);
    $23: CheckRegisters(RT, RS, $22);
    $22: CheckRegisters(RT, RS, $22);
    $26: CheckRegisters(RT, RS, $22);
    $27: CheckRegisters(RT, RS, $22);
    $0D: CheckRegisters(RT, RS, $22);
    $28: CheckRegisters($22, RT, RS);
    $3F: CheckRegisters($22, RT, RS);
    $2C: CheckRegisters($22, RT, RS);
    $2D: CheckRegisters($22, RT, RS);
    $29: CheckRegisters($22, RT, RS);
    $0A: CheckRegisters(RT, RS, $22);
    $0B: CheckRegisters(RT, RS, $22);
    $2B: CheckRegisters($22, RT, RS);
    $2E: CheckRegisters($22, RT, RS);
    $2A: CheckRegisters($22, RT, RS);
    $0E: CheckRegisters(RT, RS, $22);
  end;
end;

procedure TFormDebugRegisterChange.CheckListBoxClickCheck(Sender: TObject);
var
  I: Integer;
begin
  RegBreakpointEnabled := False;
  for I := 0 to 31 do
  begin
    if CheckListBoxRead.Checked[I] then
    begin
      RegBreakpointEnabled := True;
      RegisterReadBreakpoints[I] := True;
    end
    else
      RegisterReadBreakpoints[I] := False;
    if CheckListBoxWrite.Checked[I] then
    begin
      RegBreakpointEnabled := True;
      RegisterWriteBreakpoints[I] := True;
    end
    else
      RegisterWriteBreakpoints[I] := False;
  end;
  Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $400;
end;

procedure TFormDebugRegisterChange.FormCreate(Sender: TObject);
begin
  FillChar(RegisterReadBreakpoints, SizeOf(RegisterReadBreakpoints), 0);
  FillChar(RegisterWriteBreakpoints, SizeOf(RegisterWriteBreakpoints), 0);
  RegBreakpointEnabled := False;
end;

procedure TFormDebugRegisterChange.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Action := caHide;
end;

initialization
finalization
end.
