unit UnitDebugCommands;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls, Menus, Buttons, ShellAPI;

type
  TFormDebug = class(TForm)
    PageControl: TPageControl;
    TabSheetCPU: TTabSheet;
    PanelCPUCommands: TPanel;
    PanelCPUDown: TPanel;
    SpeedButtonCPUDown: TSpeedButton;
    PanelCPUUp: TPanel;
    SpeedButtonCPUUp: TSpeedButton;
    GroupBoxCPUJumpPC: TGroupBox;
    EditCPUPC: TEdit;
    ButtonCPUStep: TButton;
    ButtonCPUSkip: TButton;
    ButtonCPUGo: TButton;
    ButtonCPUBreak: TButton;
    PanelCPUCommandList: TPanel;
    PaintBoxCPU: TPaintBox;
    HeaderControlCPUCommands: THeaderControl;
    Timer: TTimer;
    PopupMenu: TPopupMenu;
    MenuItemClearallPCbreakpoints: TMenuItem;
    PanelBreakpoints: TPanel;
    SpeedButtonBreakpoints: TSpeedButton;
    SpeedButtonSymbols: TSpeedButton;
    PopupMenuSymbol: TPopupMenu;
    MenuItemLoadsymbolfile: TMenuItem;
    OpenDialogSymbolFile: TOpenDialog;
    SpeedButtonCPUSwitchTo: TSpeedButton;
    ButtonClose: TButton;
    TabSheetRSP: TTabSheet;
    PanelRSPCommandList: TPanel;
    PaintBoxRSP: TPaintBox;
    HeaderControlRSPCommands: THeaderControl;
    PanelRSPRight: TPanel;
    GroupBoxRSPJump: TGroupBox;
    SpeedButtonRSPSwitchTo: TSpeedButton;
    EditRSPPC: TEdit;
    ButtonRSPStep: TButton;
    ButtonRSPSkip: TButton;
    ButtonRSPGo: TButton;
    ButtonRSPBreak: TButton;
    ImageBreakpoint: TImage;
    MenuItemLibraryoptions: TMenuItem;
    MenuItemGeneratecodfilerunlfe: TMenuItem;
    MenuItemRemovesymbolinfofrommemory: TMenuItem;
    N2: TMenuItem;
    MenuItemViewsymbols: TMenuItem;
    N3: TMenuItem;
    ButtonCPUStepOver: TButton;
    N4: TMenuItem;
    MenuItemBreakafterdisplaylist: TMenuItem;
    MenuItemBreakafteraudiolist: TMenuItem;
    PopupMenuCommands: TPopupMenu;
    MenuItemFollowJump: TMenuItem;
    MenuItemSetNewPC: TMenuItem;
    MenuItemMemorydebugger: TMenuItem;
    PanelOpcodeInfo: TPanel;
    NotebookOpcodeinfo: TNotebook;
    LabelOpcodeInfoAddress: TLabel;
    EditOpcodeinfoAddress: TEdit;
    ButtonViewMemory: TButton;
    LabelOpcodeInfoConditionalJump: TLabel;
    LabelOpcodeInfoReg1: TLabel;
    LabelOpcodeInfoReg2: TLabel;
    LabelOpcodeInfoCondition: TLabel;
    LabelOpcodeInfoCondJumpDelaySlot: TLabel;
    ButtonOpcodeInfoTargetAddress: TButton;
    LabelOpcodeInfoUncoditionalDelay: TLabel;
    ButtonOpcodeInfoTargetAddressJR: TButton;
    ButtonRSPStepOver: TButton;
    SpeedButtonCPUBack: TSpeedButton;
    SpeedButtonCPUForward: TSpeedButton;
    N6: TMenuItem;
    ButtonStepBack: TButton;
    MenuItemSaveundoinformation: TMenuItem;
    N1: TMenuItem;
    MenuItemCreatebookmark: TMenuItem;
    TabSheetRDP: TTabSheet;
    ListBoxRDP: TListBox;
    N7: TMenuItem;
    ImageNote: TImage;
    SpeedButtonRSPBack: TSpeedButton;
    SpeedButtonRSPForward: TSpeedButton;
    ScrollBarRSP: TScrollBar;
    ButtonBookmarks: TButton;
    LabelOpcodeInfoMemDesc: TLabel;
    procedure EditCPUPCChange(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure ButtonCPUStepClick(Sender: TObject);
    procedure ButtonCPUBreakClick(Sender: TObject);
    procedure ButtonCPUGoClick(Sender: TObject);
    procedure PaintBoxCPUPaint(Sender: TObject);
    procedure HeaderControlCPUCommandsSectionResize(Sender: TObject; Section: THeaderSection);
    procedure TimerTimer(Sender: TObject);
    procedure SpeedButtonCPUUpMouseDown(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
    procedure SpeedButtonCPUDownMouseDown(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
    procedure SpeedButtonCPUUpMouseUp(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
    procedure SpeedButtonCPUDownMouseUp(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
    procedure PaintBoxCPUMouseDown(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
    procedure SpeedButtonBreakpointsClick(Sender: TObject);
    procedure MenuItemClearallPCbreakpointsClick(Sender: TObject);
    procedure ButtonCPUSkipClick(Sender: TObject);
    procedure SpeedButtonSymbolsClick(Sender: TObject);
    procedure MenuItemLoadsymbolfileClick(Sender: TObject);
    procedure ButtonCloseClick(Sender: TObject);
    procedure HeaderControlRSPCommandsSectionResize(Sender: TObject; Section: THeaderSection);
    procedure ButtonRSPSkipClick(Sender: TObject);
    procedure ButtonRSPGoClick(Sender: TObject);
    procedure PaintBoxRSPPaint(Sender: TObject);
    procedure EditRSPPCChange(Sender: TObject);
    procedure ButtonRSPBreakClick(Sender: TObject);
    procedure PaintBoxRSPMouseDown(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure ButtonViewMemoryClick(Sender: TObject);
    procedure ButtonOpcodeInfoTargetAddressClick(Sender: TObject);
    procedure MenuItemLibraryoptionsClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure MenuItemRemovesymbolinfofrommemoryClick(Sender: TObject);
    procedure MenuItemGeneratecodfilerunlfeClick(Sender: TObject);
    procedure MenuItemViewsymbolsClick(Sender: TObject);
    procedure ButtonCPUStepOverClick(Sender: TObject);
    procedure MenuItemBreakafterdisplaylistClick(Sender: TObject);
    procedure MenuItemBreakafteraudiolistClick(Sender: TObject);
    procedure MenuItemSetNewPCClick(Sender: TObject);
    procedure MenuItemFollowJumpClick(Sender: TObject);
    procedure MenuItemMemorydebuggerClick(Sender: TObject);
    procedure FormResize(Sender: TObject);
    procedure ButtonRSPStepOverClick(Sender: TObject);
    procedure SpeedButtonCPUBackClick(Sender: TObject);
    procedure SpeedButtonCPUForwardClick(Sender: TObject);
    procedure MenuItemSaveundoinformationClick(Sender: TObject);
    procedure ButtonStepBackClick(Sender: TObject);
    procedure MenuItemCreatebookmarkClick(Sender: TObject);
    procedure PageControlChange(Sender: TObject);
    procedure ListBoxRDPDrawItem(Control: TWinControl; Index: Integer; Rect: TRect; State: TOwnerDrawState);
    procedure PaintBoxRSPMouseMove(Sender: TObject; Shift: TShiftState; X, Y: Integer);
    procedure PaintBoxCPUMouseMove(Sender: TObject; Shift: TShiftState; X, Y: Integer);
    procedure SpeedButtonRSPBackClick(Sender: TObject);
    procedure SpeedButtonRSPForwardClick(Sender: TObject);
    procedure ScrollBarRSPChange(Sender: TObject);
    procedure ButtonBookmarksClick(Sender: TObject);
    procedure FormKeyUp(Sender: TObject; var Key: Word; Shift: TShiftState);
  private
    CPUViewAddress, RSPViewAddress: Cardinal;
    CurrentInstruction: Boolean;
    LastSymbolFile: string;
    CPUHistory: array of Cardinal;
    CPUHistoryIndex: Cardinal;
    RSPHistory: array of Cardinal;
    RSPHistoryIndex: Cardinal;
    procedure WMShowDebugger(var Message: TMessage); message $401;
    procedure UpdateHistoryButtons;
    procedure UpdateOpcodeInfo(IsRSP: Boolean; Opcode: Cardinal; Text: string; Address: Cardinal);
  public
    function GetSymbolName(Address: Cardinal): string;
    procedure RefreshRDP;
    procedure RefreshDebugger;
    procedure ShowDebugger;
    procedure LoadSymbols(FileName: string);
  end;

  TTemporaryBreakpoint = record
    Address: Cardinal;
    Temporary: Boolean;
  end;

var
  FormDebug: TFormDebug;
  DebugFormHandle: THandle;
  SymbolCount: Cardinal;
  Symbols: array of record
    Name: ShortString;
    Address: Cardinal;
  end;
  BreakpointCount: Cardinal;
  CPUBreakpoints: array[0..99] of TTemporaryBreakpoint;
  RSPTemporaryBreakpoints: array[0..99] of Boolean;
  BreakOnDList, BreakOnAList: Boolean;
  NemuFileDir: string;

function HasCPUBreakpoint(Address: Cardinal): Boolean;

implementation

uses UnitDecs, RCP, UnitDisassembly, UnitDebugMemory,
  UnitDebugRegisters, UnitLibOptions, UnitSymbolViewer, UnitBookmarks;

{$R *.dfm}

{$W+}
function FindSymbol(Address: Cardinal): string;
var
  I: Cardinal;
begin
  Result := '';
  if SymbolCount <> 0 then
    for I := 0 to SymbolCount - 1 do
      if Address = Symbols[I].Address then
      begin
        Result := Symbols[I].Name;
        Break;
      end;
end;
{$W-}

function TFormDebug.GetSymbolName(Address: Cardinal): string;
var
  PhysicalAddress: Cardinal;
  I: Integer;
begin
  Result := FindSymbol(Address);
  if Result <> '' then Exit;
  PhysicalAddress := ResolvePhysicalAddress(Address);
  if PhysicalAddress = 0 then
  begin
    Result := '';
    Exit;
  end;
  with FormBookmarks.TreeViewBookmarks do
    for I := 0 to Items.Count - 1 do
      if ResolvePhysicalAddress(StrToIntDef('$' + Copy(Items[I].Text, 7, 8), 0)) = PhysicalAddress then
      begin
        Result := Copy(Items[I].Text, 17, 10000);
        Exit;
      end;
  Result := '';
end;

procedure TFormDebug.RefreshRDP;
var
  I, Position, Word3, Word2, Word1, Word0: Cardinal;
begin
  ListBoxRDP.Items.Clear;
  Position := Nemu64_API.RDPHistoryPosition^;
  I := $800;
  repeat
    Dec(I);
    Word3 := Nemu64_API.RDPHistory^[(I + Position) and $7FF];
    Dec(I);
    Word2 := Nemu64_API.RDPHistory^[(I + Position) and $7FF];
    Word1 := Nemu64_API.RDPHistory^[(I + Position - 1) and $7FF];
    Word0 := Nemu64_API.RDPHistory^[(I + Position - 2) and $7FF];
    if (Word0 shr 24) or $C0 = $E4 then
    begin
      Dec(I, 2);
      ListBoxRDP.Items.Add(IntToHex(Int64(Word0), 8) + ' ' +
        IntToHex(Int64(Word1), 8) + ' - ' + IntToHex(Int64(Word2), 8) + ' ' +
        IntToHex(Int64(Word3), 8) + ': ' + DisassembleRDP(Position,
        Word1 or (Int64(Word0) shl 32), Word3 or (Int64(Word3) shl 32)));
    end
    else
      ListBoxRDP.Items.Add(IntToHex(Int64(Word2), 8) + ' ' +
        IntToHex(Int64(Word3), 8) + ': ' + DisassembleRDP(Position,
        Word3 or (Int64(Word2) shl 32), 0));
  until I <= 0;
end;

procedure TFormDebug.ShowDebugger;
begin
  FormDebug.RefreshDebugger;
  if not FormDebug.Visible then FormDebug.Show;
  if FormMemory.Visible then FormMemory.RefreshMemory;
  if FormRegisters.Visible then FormRegisters.RefreshRegisters;
end;

procedure TFormDebug.WMShowDebugger(var Message: TMessage);
begin
  if Message.WParam = 1 then
    FormDebug.PageControl.ActivePage := TabSheetRSP
  else
    FormDebug.PageControl.ActivePage := TabSheetCPU;
  ShowDebugger;
end;

function HasCPUBreakpoint(Address: Cardinal): Boolean;
var
  I: Cardinal;
begin
  Result := False;
  I := 0;
  while I < BreakpointCount do
  begin
    if Address = CPUBreakpoints[I].Address then
    begin
      Result := True;
      Exit;
    end;
    Inc(I);
  end;
end;

function RemoveCPUBreakpoint(Address: Cardinal; TemporaryOnly: Boolean): Boolean;
var
  Found, I: Integer;
begin
  Found := -1;
  for I := 0 to Integer(BreakpointCount) - 1 do
    if Address = CPUBreakpoints[I].Address then
      if TemporaryOnly then
      begin
        if CPUBreakpoints[I].Temporary then
        begin
          Found := I;
          Break;
        end;
      end
      else
      begin
        Found := I;
        Break;
      end;
  if Found = -1 then
    Result := False
  else
  begin
    Result := True;
    Dec(BreakpointCount);
    if BreakpointCount - Cardinal(Found) <> 0 then
      for I := Found + 1 to Integer(BreakpointCount) do
        CPUBreakpoints[I - 1] := CPUBreakpoints[I];
  end;
end;

function RemoveRSPBreakpoint(Address: Cardinal; TemporaryOnly: Boolean): Boolean;
var
  Found, I: Integer;
begin
  with PNemu(@Nemu64_API)^ do
  begin
    Found := -1;
    { Delphi 6 counts these references before eliminating the assignments.
      They reproduce the original allocation: Found in EDI, host pointer in EDX.
      This is inferred compiler guidance; the discarded original expressions
      cannot be recovered uniquely. See analysis/debugger-allocator.json. }
    Found := Found;
    Found := Found;
    Found := Found;
    for I := 0 to Integer(RSPBreakpointCount^) - 1 do
      if Address = RSPBreakpoints^[I] then
        if TemporaryOnly then
        begin
          if RSPTemporaryBreakpoints[I] then
          begin
            Found := I;
            Break;
          end;
        end
        else
        begin
          Found := I;
          Break;
        end;
    if Found = -1 then
      Result := False
    else
    begin
      Result := True;
      Dec(RSPBreakpointCount^);
      if RSPBreakpointCount^ - Cardinal(Found) <> 0 then
        for I := Found + 1 to Integer(RSPBreakpointCount^) do
          RSPBreakpoints^[I - 1] := RSPBreakpoints^[I];
      if RSPBreakpointCount^ - Cardinal(Found) <> 0 then
        for I := Found + 1 to Integer(RSPBreakpointCount^) do
          RSPTemporaryBreakpoints[I - 1] := RSPTemporaryBreakpoints[I];
    end;
  end;
end;

function HasRSPBreakpoint(Address: Cardinal): Boolean;
var
  I: Cardinal;
begin
  Result := False;
  I := 0;
  while I < Nemu64_API.RSPBreakpointCount^ do
  begin
    if Address = Nemu64_API.RSPBreakpoints^[I] then
    begin
      Result := True;
      Break;
    end;
    Inc(I);
  end;
end;

procedure CenterAddress(Address: Cardinal; var ViewAddress: Cardinal;
  Height, TextHeight: Cardinal);
begin
  if (Address < ViewAddress) or
    (Address >= ViewAddress + ((Height - 1) div TextHeight) * 4) then
    ViewAddress := Address;
end;

procedure TFormDebug.EditCPUPCChange(Sender: TObject);
var
  Address: Cardinal;
begin
  if Length(EditCPUPC.Text) = 8 then
  begin
    Address := StrToIntDef('$' + EditCPUPC.Text, 0);
    if Address <> 0 then
    begin
      with PaintBoxCPU do
        CenterAddress(Address, CPUViewAddress, Height, Canvas.TextHeight('A'));
      PaintBoxCPUPaint(PaintBoxCPU);
      if (EditCPUPC.Tag = 0) and ((CPUHistoryIndex = 0) or
        (Address <> CPUHistory[CPUHistoryIndex - 1])) then
        begin
          Inc(CPUHistoryIndex);
          SetLength(CPUHistory, CPUHistoryIndex);
          CPUHistory[CPUHistoryIndex - 1] := Address;
        end;
    end;
  end;
  UpdateHistoryButtons;
end;

procedure TFormDebug.FormShow(Sender: TObject);
begin
  EditCPUPC.Text := IntToHex(Int64(Nemu64_API.CPUProgramCounter^), 8);
  EditRSPPC.Text := IntToHex(Int64(Nemu64_API.RSPControlRegisters^.PC), 3);
  RefreshDebugger;
end;

procedure TFormDebug.ButtonCPUStepClick(Sender: TObject);
begin
  if Nemu64_API.EmulationThread^ <> 0 then ResumeThread(Nemu64_API.EmulationThread^);
end;

procedure TFormDebug.ButtonCPUBreakClick(Sender: TObject);
begin
  Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $10000;
end;

procedure TFormDebug.ButtonCPUGoClick(Sender: TObject);
begin
  Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ and not $10000;
  if Nemu64_API.EmulationThread^ <> 0 then ResumeThread(Nemu64_API.EmulationThread^);
end;

procedure TFormDebug.RefreshDebugger;
begin
  if RemoveCPUBreakpoint(Nemu64_API.CPUProgramCounter^, True) then
    Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $400;
  if EditCPUPC.Text = IntToHex(Nemu64_API.CPUProgramCounter^, 8) then
    EditCPUPCChange(EditCPUPC)
  else
    EditCPUPC.Text := IntToHex(Nemu64_API.CPUProgramCounter^, 8);
  RemoveRSPBreakpoint(Nemu64_API.RSPControlRegisters^.PC, True);
  if EditRSPPC.Text = IntToHex(Nemu64_API.RSPControlRegisters^.PC, 3) then
    EditRSPPCChange(EditRSPPC)
  else
    EditRSPPC.Text := IntToHex(Nemu64_API.RSPControlRegisters^.PC, 3);
  if PageControl.ActivePage = TabSheetCPU then
    PaintBoxCPUPaint(PaintBoxCPU);
  if PageControl.ActivePage = TabSheetRSP then
    PaintBoxRSPPaint(PaintBoxRSP);
  if PageControl.ActivePage = TabSheetRDP then
    RefreshRDP;
end;

procedure TFormDebug.UpdateHistoryButtons;
begin
  SpeedButtonCPUBack.Enabled := CPUHistoryIndex > 1;
  SpeedButtonCPUForward.Enabled := Cardinal(Length(CPUHistory)) > CPUHistoryIndex;
  SpeedButtonRSPBack.Enabled := RSPHistoryIndex > 1;
  SpeedButtonRSPForward.Enabled := Cardinal(Length(RSPHistory)) > RSPHistoryIndex;
end;

procedure TFormDebug.UpdateOpcodeInfo(IsRSP: Boolean; Opcode: Cardinal;
  Text: string; Address: Cardinal);
var
  TakesJump, ExecutesDelay: Boolean;
  Reg1, Reg2: Int64;
begin
  if IsRSP then
  begin
    if UnitDisassembly.Reg1 <> -1 then
      FormRegisters.GetRegisterLabel(UnitDisassembly.Reg1 + 32).Font.Color := clBlue;
    if UnitDisassembly.Reg2 <> -1 then
      FormRegisters.GetRegisterLabel(UnitDisassembly.Reg2 + 32).Font.Color := clBlue;
    if UnitDisassembly.Reg3 <> -1 then
      FormRegisters.GetRegisterLabel(UnitDisassembly.Reg3 + 32).Font.Color := clBlue;
  end
  else
  begin
    if UnitDisassembly.Reg1 <> -1 then
      FormRegisters.GetRegisterLabel(UnitDisassembly.Reg1).Font.Color := clBlue;
    if UnitDisassembly.Reg2 <> -1 then
      FormRegisters.GetRegisterLabel(UnitDisassembly.Reg2).Font.Color := clBlue;
    if UnitDisassembly.Reg3 <> -1 then
      FormRegisters.GetRegisterLabel(UnitDisassembly.Reg3).Font.Color := clBlue;
  end;
  if MemAccess then
  begin
    NotebookOpcodeinfo.PageIndex := 1;
    if IsRSP then
      EditOpcodeinfoAddress.Text := IntToHex(
        ((Nemu64_API.RSPRegisters^[DebuggerAddressReg] + Cardinal(ImmValue))
        and $FFF) + $A4000000, 8)
    else
      EditOpcodeinfoAddress.Text := IntToHex(
        Cardinal(Nemu64_API.CPURegisters^[DebuggerAddressReg]) + Cardinal(ImmValue), 8);
    LabelOpcodeInfoMemDesc.Caption := GetSymbolName(StrToInt('$' + EditOpcodeinfoAddress.Text));
  end
  else if (Copy(Text, 1, 2) = 'JR') or (Copy(Text, 1, 4) = 'JALR') then
  begin
    NotebookOpcodeinfo.PageIndex := 3;
    if IsRSP then
      ButtonOpcodeInfoTargetAddressJR.Caption := IntToHex(
        Nemu64_API.RSPRegisters^[(Opcode shr 21) and $1F], 8)
    else
      ButtonOpcodeInfoTargetAddressJR.Caption := IntToHex(
        Cardinal(Nemu64_API.CPURegisters^[(Opcode shr 21) and $1F]), 8);
  end
  else if (Copy(Text, 1, 2) = 'J ') or (Copy(Text, 1, 3) = 'JAL') then
  begin
    NotebookOpcodeinfo.PageIndex := 3;
    if IsRSP then
      ButtonOpcodeInfoTargetAddressJR.Caption := IntToHex((Opcode shl 2) and $FFF, 3)
    else
      ButtonOpcodeInfoTargetAddressJR.Caption := IntToHex(
        (Address and $F0000000) or ((Opcode shl 2) and $FFFFFFF), 8);
  end
  else if (Copy(Text, 1, 3) = 'BEQ') or (Copy(Text, 1, 3) = 'BNE') or
    (Copy(Text, 1, 2) = 'BG') or (Copy(Text, 1, 2) = 'BL') then
  begin
    NotebookOpcodeinfo.PageIndex := 2;
    TakesJump := True;
    if IsRSP then
      Reg1 := Integer(Nemu64_API.RSPRegisters^[(Opcode shr 21) and $1F])
    else
      Reg1 := Nemu64_API.CPURegisters^[(Opcode shr 21) and $1F];
    if IsRSP then
      Reg2 := Integer(Nemu64_API.RSPRegisters^[(Opcode shr 16) and $1F])
    else
      Reg2 := Nemu64_API.CPURegisters^[(Opcode shr 16) and $1F];
    LabelOpcodeInfoReg1.Caption := GPRNames[(Opcode shr 21) and $1F] +
      ': ' + IntToHex(Cardinal(Reg1), 8);
    LabelOpcodeInfoReg2.Visible := False;
    if Copy(Text, 1, 3) = 'BEQ' then
    begin
      LabelOpcodeInfoConditionalJump.Caption := GPRNames[(Opcode shr 21) and $1F] +
        ' == ' + GPRNames[(Opcode shr 16) and $1F] + ' ?';
      TakesJump := Reg1 = Reg2;
      LabelOpcodeInfoReg2.Caption := GPRNames[(Opcode shr 16) and $1F] +
        ': ' + IntToHex(Cardinal(Reg2), 8);
      LabelOpcodeInfoReg2.Visible := True;
    end;
    if Copy(Text, 1, 3) = 'BNE' then
    begin
      LabelOpcodeInfoConditionalJump.Caption := GPRNames[(Opcode shr 21) and $1F] +
        ' != ' + GPRNames[(Opcode shr 16) and $1F] + ' ?';
      TakesJump := Reg1 <> Reg2;
      LabelOpcodeInfoReg2.Caption := GPRNames[(Opcode shr 16) and $1F] +
        ': ' + IntToHex(Cardinal(Reg2), 8);
      LabelOpcodeInfoReg2.Visible := True;
    end;
    if Copy(Text, 1, 4) = 'BGTZ' then
    begin
      LabelOpcodeInfoConditionalJump.Caption := GPRNames[(Opcode shr 21) and $1F] + ' > 0 ?';
      TakesJump := Reg1 > 0;
    end;
    if Copy(Text, 1, 4) = 'BLEZ' then
    begin
      LabelOpcodeInfoConditionalJump.Caption := GPRNames[(Opcode shr 21) and $1F] + ' <= 0 ?';
      TakesJump := Reg1 <= 0;
    end;
    if Copy(Text, 1, 4) = 'BLTZ' then
    begin
      LabelOpcodeInfoConditionalJump.Caption := GPRNames[(Opcode shr 21) and $1F] + ' < 0 ?';
      TakesJump := Reg1 < 0;
    end;
    if Copy(Text, 1, 4) = 'BGEZ' then
    begin
      LabelOpcodeInfoConditionalJump.Caption := GPRNames[(Opcode shr 21) and $1F] + ' >= 0 ?';
      TakesJump := Reg1 >= 0;
    end;
    if (Text[Pos(' ', Text) - 1] = 'L') and
      (Text[Pos(' ', Text) - 2] <> 'A') then
      ExecutesDelay := False
    else
      ExecutesDelay := True;
    ButtonOpcodeInfoTargetAddress.Visible := TakesJump;
    if TakesJump then
    begin
      LabelOpcodeInfoCondition.Caption := 'Takes jump';
      LabelOpcodeInfoCondJumpDelaySlot.Caption := 'Executes delay';
      ButtonOpcodeInfoTargetAddress.Caption := IntToHex(
        Address + 4 + Cardinal(SmallInt(Opcode) shl 2), 8);
    end
    else
    begin
      LabelOpcodeInfoCondition.Caption := 'No jump';
      if ExecutesDelay then
        LabelOpcodeInfoCondJumpDelaySlot.Caption := 'Executes delay'
      else
        LabelOpcodeInfoCondJumpDelaySlot.Caption := 'Skips delay';
    end;
  end;
  NotebookOpcodeinfo.Repaint;
end;

procedure ResetRegisterColors;
var
  I: Integer;
begin
  for I := 0 to 31 do
    FormRegisters.GetRegisterLabel(I).Font.Color := clBlack;
  for I := 0 to 31 do
    FormRegisters.GetRegisterLabel(I + 32).Font.Color := clBlack;
  for I := 0 to 31 do
    FormRegisters.GetRegisterLabel(I + 64).Font.Color := clBlack;
end;

procedure TFormDebug.PaintBoxCPUPaint(Sender: TObject);
var
  Width, Height, RowHeight: Integer;
  Row: Cardinal;
  AddressX, OpcodeX, OperandsX: Integer;
  Text, SymbolName: string;
  Opcode, SelectedAddress, Address: Cardinal;
begin
  if PageControl.ActivePage <> TabSheetCPU then Exit;
  SelectedAddress := StrToIntDef('$' + EditCPUPC.Text, 0);
  Width := (Sender as TPaintBox).Width;
  Height := (Sender as TPaintBox).Height;
  AddressX := HeaderControlCPUCommands.Sections[0].Width;
  OpcodeX := HeaderControlCPUCommands.Sections[1].Width + AddressX;
  OperandsX := HeaderControlCPUCommands.Sections[2].Width + OpcodeX;
  ResetRegisterColors;
  NotebookOpcodeinfo.PageIndex := 0;
  with (Sender as TPaintBox).Canvas do
  begin
    RowHeight := TextHeight('A');
    Brush.Color := clBtnFace;
    FillRect(Rect(0, 0, AddressX - 2, Height));
    Brush.Color := clWhite;
    FillRect(Rect(AddressX - 2, 0, Width, Height));
    MoveTo(AddressX - 2, 0);
    LineTo(AddressX - 2, Height);
    if (Height - 1) div RowHeight - 1 >= 0 then
    for Row := 0 to (Height - 1) div RowHeight - 1 do
    begin
      Address := CPUViewAddress + Row * 4;
      Brush.Color := clBtnFace;
      if HasCPUBreakpoint(Address) then
        BrushCopy(Rect(AddressX - 14, Row * RowHeight + 1,
          AddressX - 14 + ImageBreakpoint.Picture.Width,
          Row * RowHeight + 1 + ImageBreakpoint.Picture.Height),
          ImageBreakpoint.Picture.Bitmap,
          Rect(0, 0, ImageBreakpoint.Picture.Width, ImageBreakpoint.Picture.Height), clSilver)
      else if GetSymbolName(Address) <> '' then
        BrushCopy(Rect(AddressX - 14, Row * RowHeight + 1,
          AddressX - 14 + ImageNote.Picture.Width,
          Row * RowHeight + 1 + ImageNote.Picture.Height),
          ImageNote.Picture.Bitmap,
          Rect(0, 0, ImageNote.Picture.Width, ImageNote.Picture.Height), clSilver);
      Brush.Color := clWhite;
      if Address = SelectedAddress then
      begin
        Brush.Color := $00FFCACA;
        FillRect(Rect(AddressX - 1, Row * RowHeight, Width, (Row + 1) * RowHeight));
      end;
      if Address = Nemu64_API.CPUProgramCounter^ then
      begin
        Font.Color := clRed;
        CurrentInstruction := True;
      end
      else
      begin
        Font.Color := clBlack;
        CurrentInstruction := False;
      end;
      TextOut(AddressX, Row * RowHeight, IntToHex(Address, 8));
      Opcode := ReadWord(ResolvePhysicalAddress(Address));
      if Nemu64_API.TranslateError^ or Nemu64_API.ReadError^ then
      begin
        Text := '*';
        Nemu64_API.TranslateError^ := False;
        Nemu64_API.ReadError^ := False;
      end
      else
      begin
        Text := DisassembleCPU(Address, Opcode, True);
        if CurrentInstruction then
          UpdateOpcodeInfo(False, Opcode, Text, Address);
      end;
      if (Copy(Text, 1, 2) = 'J ') or (Copy(Text, 1, 4) = 'JAL ') then
      begin
        SymbolName := GetSymbolName((Address and $F0000000) or ((Opcode shl 2) and $FFFFFFF));
        if SymbolName <> '' then
          Text := Copy(Text, 1, Pos(' ', Text)) + SymbolName + ' (0x' +
            IntToHex((Address and $F0000000) or ((Opcode shl 2) and $FFFFFFF), 8) + ')';
      end;
      if (Copy(Text, 1, 3) = 'BEQ') or (Copy(Text, 1, 3) = 'BNE') or
        (Copy(Text, 1, 3) = 'BLE') or (Copy(Text, 1, 3) = 'BLT') or
        (Copy(Text, 1, 3) = 'BGT') or (Copy(Text, 1, 3) = 'BGE') then
      begin
        Address := Address + 4 + Cardinal(SmallInt(Opcode) shl 2);
        SymbolName := GetSymbolName(Address);
        if SymbolName <> '' then
        begin
          SetLength(Text, Pos('0x', Text) - 1);
          Text := Text + SymbolName + ' (' + IntToHex(Address, 8) + ')';
        end;
      end;
      if (Text = 'NOP') and (Font.Color = clBlack) then
        Font.Color := clGray;
      if Pos(' ', Text) > 0 then
      begin
        TextOut(OpcodeX, Row * RowHeight, Copy(Text, 1, Pos(' ', Text) - 1));
        Delete(Text, 1, Pos(' ', Text));
        TextOut(OperandsX, Row * RowHeight, Text);
      end
      else
        TextOut(OpcodeX, Row * RowHeight, Text);
      Brush.Color := clWhite;
    end;
  end;
end;

procedure TFormDebug.HeaderControlCPUCommandsSectionResize(Sender: TObject; Section: THeaderSection);
begin
  PaintBoxCPU.Refresh;
end;

procedure TFormDebug.TimerTimer(Sender: TObject);
begin
  if PageControl.ActivePage = TabSheetCPU then
  begin
    Inc(CPUViewAddress, Timer.Tag);
    PaintBoxCPUPaint(PaintBoxCPU);
  end;
end;

procedure TFormDebug.SpeedButtonCPUUpMouseDown(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  if Button = mbLeft then
  begin
    Timer.Tag := -4;
    Timer.Enabled := True;
    TimerTimer(Timer);
  end;
end;

procedure TFormDebug.SpeedButtonCPUDownMouseDown(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  if Button = mbLeft then
  begin
    Timer.Tag := 4;
    Timer.Enabled := True;
    TimerTimer(Timer);
  end;
end;

procedure TFormDebug.SpeedButtonCPUUpMouseUp(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  if Button = mbLeft then Timer.Enabled := False;
end;

procedure TFormDebug.SpeedButtonCPUDownMouseUp(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  if Button = mbLeft then Timer.Enabled := False;
end;

procedure TFormDebug.PaintBoxCPUMouseDown(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
var
  Address, Instruction, SelectedAddress: Cardinal;
begin
  if (HeaderControlCPUCommands.Sections[0].Width >= X) and (Button = mbLeft) then
  begin
    Address := CPUViewAddress + (Y div PaintBoxCPU.Canvas.TextHeight('A')) * 4;
    if HasCPUBreakpoint(Address) then
      RemoveCPUBreakpoint(Address, False)
    else
    begin
      CPUBreakpoints[BreakpointCount].Address := Address;
      CPUBreakpoints[BreakpointCount].Temporary := False;
      Inc(BreakpointCount);
    end;
    Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $400;
    PaintBoxCPUPaint(PaintBoxCPU);
  end;
  if Button = mbRight then
  begin
    SelectedAddress := CPUViewAddress + (Y div PaintBoxCPU.Canvas.TextHeight('A')) * 4;
    MenuItemSetNewPC.Tag := Integer(SelectedAddress);
    MenuItemMemorydebugger.Tag := Integer(SelectedAddress);
    MenuItemCreatebookmark.Tag := Integer(SelectedAddress);
    Instruction := ReadWord(TranslateAddress(SelectedAddress));
    if (Instruction shr 26 = 2) or (Instruction shr 26 = 3) then
    begin
      MenuItemFollowJump.Tag := (SelectedAddress and $F0000000) or ((Instruction shl 2) and $FFFFFFF);
      MenuItemFollowJump.Enabled := True;
    end
    else if (Instruction shr 26 in [4..7, 20..23]) or
      ((Instruction shr 26 = 1) and ((Instruction shr 16) and 31 in [0..3, 16..19])) then
    begin
      MenuItemFollowJump.Tag := SelectedAddress + 4 + SmallInt(Instruction) * 4;
      MenuItemFollowJump.Enabled := True;
    end
    else MenuItemFollowJump.Enabled := False;
  end;
end;

procedure TFormDebug.SpeedButtonBreakpointsClick(Sender: TObject);
var
  P: TPoint;
begin
  P := (Sender as TSpeedButton).ClientToScreen(
    Point(0, (Sender as TSpeedButton).ClientRect.Bottom));
  PopupMenu.Popup(P.X, P.Y);
end;

procedure TFormDebug.MenuItemClearallPCbreakpointsClick(Sender: TObject);
begin
  BreakpointCount := 0;
  PaintBoxCPUPaint(PaintBoxCPU);
end;

procedure TFormDebug.ButtonCPUSkipClick(Sender: TObject);
begin
  Inc(Nemu64_API.CPUProgramCounter^, 4);
  RefreshDebugger;
end;

procedure TFormDebug.SpeedButtonSymbolsClick(Sender: TObject);
var
  P: TPoint;
begin
  P := (Sender as TSpeedButton).ClientToScreen(
    Point(0, (Sender as TSpeedButton).ClientRect.Bottom));
  PopupMenuSymbol.Popup(P.X, P.Y);
end;

procedure TFormDebug.LoadSymbols(FileName: string);
var
  S: string;
  F: TextFile;
begin
  if FileName = '' then
  begin
    if (FormLibOptions.EditCodFolder.Text <> '') and FormLibOptions.CheckBoxAutoloadCodFiles.Checked then
    begin
      S := Nemu64_API.RomFilename;
      if S <> '' then
      begin
        S := ExtractFileName(S);
        SetLength(S, Length(S) - Length(ExtractFileExt(S)));
        if FormLibOptions.EditCodFolder.Text[Length(FormLibOptions.EditCodFolder.Text)] = '\' then
          FileName := FormLibOptions.EditCodFolder.Text + S + '.cod'
        else
          FileName := FormLibOptions.EditCodFolder.Text + '\' + S + '.cod';
      end;
    end
    else FileName := '';
  end;
  if (FileName <> '') and (FileName <> LastSymbolFile) then
  begin
    if not FileExists(FileName) then
    begin
      if SymbolCount <> 0 then
        if MessageBox(Nemu64_API.MainWindow^,
          'New rom loaded and no .cod file? Want to remove symbol info in memory?',
          'New rom loaded', MB_YESNO or MB_ICONQUESTION) = IDYES then
        begin
          SymbolCount := 0;
          FormSymbols.ListBoxSymbols.Items.Clear;
        end;
    end
    else
    begin
      LastSymbolFile := FileName;
      AssignFile(F, FileName);
      Reset(F);
      SymbolCount := 0;
      FormSymbols.ListBoxSymbols.Items.Clear;
      while not Eof(F) do
      begin
        SetLength(Symbols, SymbolCount + 1);
        ReadLn(F, S);
        if Copy(UpperCase(S), 1, 2) = 'F ' then
        begin
          Delete(S, 1, 2);
          Symbols[SymbolCount].Name := Copy(S, 1, Pos(' ', S) - 1);
          Delete(S, 1, Pos(' ', S) + 1);
          Symbols[SymbolCount].Address := StrToIntDef('$' + S, 0);
        end
        else
        begin
          Symbols[SymbolCount].Address := StrToIntDef('$' + Copy(S, 3, 8), 0);
          Delete(S, 1, 11);
          Symbols[SymbolCount].Name := S;
        end;
        FormSymbols.ListBoxSymbols.Items.Add(string(Symbols[SymbolCount].Name) +
          ' - 0x' + IntToHex(Int64(Symbols[SymbolCount].Address), 8));
        Inc(SymbolCount);
      end;
      CloseFile(F);
    end;
  end;
end;

procedure TFormDebug.MenuItemLoadsymbolfileClick(Sender: TObject);
begin
  OpenDialogSymbolFile.InitialDir := FormLibOptions.EditCodFolder.Text;
  if OpenDialogSymbolFile.Execute then
  begin
    LoadSymbols(OpenDialogSymbolFile.FileName);
    RefreshDebugger;
  end;
end;

procedure TFormDebug.ButtonCloseClick(Sender: TObject);
begin
  Close;
end;

procedure TFormDebug.HeaderControlRSPCommandsSectionResize(Sender: TObject;
  Section: THeaderSection);
begin
  PaintBoxRSP.Refresh;
end;

procedure TFormDebug.ButtonRSPSkipClick(Sender: TObject);
begin
  Inc(Nemu64_API.RSPControlRegisters^.PC, 4);
  RefreshDebugger;
end;

procedure TFormDebug.ButtonRSPGoClick(Sender: TObject);
begin
  Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ and not $20000;
  if Nemu64_API.EmulationThread^ <> 0 then
    ResumeThread(Nemu64_API.EmulationThread^);
end;

procedure TFormDebug.PaintBoxRSPPaint(Sender: TObject);
var
  Width, Height, RowHeight: Integer;
  Row: Cardinal;
  AddressX, OpcodeX, OperandsX: Integer;
  Text, SymbolName: string;
  Opcode, SelectedAddress, Address, TargetAddress: Cardinal;
begin
  if PageControl.ActivePage <> TabSheetRSP then Exit;
  SelectedAddress := StrToIntDef('$' + EditRSPPC.Text, 0);
  Width := (Sender as TPaintBox).Width;
  Height := (Sender as TPaintBox).Height;
  AddressX := HeaderControlRSPCommands.Sections[0].Width;
  OpcodeX := HeaderControlRSPCommands.Sections[1].Width + AddressX;
  OperandsX := HeaderControlRSPCommands.Sections[2].Width + OpcodeX;
  ResetRegisterColors;
  NotebookOpcodeinfo.PageIndex := 0;
  with (Sender as TPaintBox).Canvas do
  begin
    RowHeight := TextHeight('A');
    Brush.Color := clBtnFace;
    FillRect(Rect(0, 0, AddressX - 2, Height));
    Brush.Color := clWhite;
    FillRect(Rect(AddressX - 2, 0, Width, Height));
    MoveTo(AddressX - 2, 0);
    LineTo(AddressX - 2, Height);
    if (Height - 1) div RowHeight - 1 >= 0 then
    for Row := 0 to (Height - 1) div RowHeight - 1 do
    begin
      Address := (RSPViewAddress + Row * 4) and $FFF;
      Brush.Color := clBtnFace;
      if HasRSPBreakpoint(Address) then
        BrushCopy(Rect(AddressX - 14, Row * RowHeight + 1,
          AddressX - 14 + ImageBreakpoint.Picture.Width,
          Row * RowHeight + 1 + ImageBreakpoint.Picture.Height),
          ImageBreakpoint.Picture.Bitmap,
          Rect(0, 0, ImageBreakpoint.Picture.Width, ImageBreakpoint.Picture.Height), clSilver)
      else if GetSymbolName(Address + $A4001000) <> '' then
        BrushCopy(Rect(AddressX - 14, Row * RowHeight + 1,
          AddressX - 14 + ImageNote.Picture.Width,
          Row * RowHeight + 1 + ImageNote.Picture.Height),
          ImageNote.Picture.Bitmap,
          Rect(0, 0, ImageNote.Picture.Width, ImageNote.Picture.Height), clSilver);
      Brush.Color := clWhite;
      if Address = SelectedAddress then
      begin
        Brush.Color := $00FFCACA;
        FillRect(Rect(AddressX - 1, Row * RowHeight, Width, (Row + 1) * RowHeight));
      end;
      if Address = (Nemu64_API.RSPControlRegisters^.PC and $FFF) then
      begin
        Font.Color := clRed;
        CurrentInstruction := True;
      end
      else
      begin
        Font.Color := clBlack;
        CurrentInstruction := False;
      end;
      TextOut(AddressX, Row * RowHeight, IntToHex(Address, 3));
      Opcode := ReadWord(Address + $04001000);
      Text := DisassembleRSP(Address, Opcode, True);
      if (Copy(Text, 1, 2) = 'J ') or (Copy(Text, 1, 4) = 'JAL ') then
      begin
        SymbolName := GetSymbolName(((Opcode shl 2) and $FFF) + $A4001000);
        if SymbolName <> '' then
          Text := Copy(Text, 1, Pos(' ', Text)) + #1 + SymbolName + ' (0x' +
            IntToHex((Opcode shl 2) and $FFF, 3) + ')';
      end;
      if (Copy(Text, 1, 3) = 'BEQ') or (Copy(Text, 1, 3) = 'BNE') or
        (Copy(Text, 1, 3) = 'BLE') or (Copy(Text, 1, 3) = 'BLT') or
        (Copy(Text, 1, 3) = 'BGT') or (Copy(Text, 1, 3) = 'BGE') then
      begin
        TargetAddress := (Address + 4 + Cardinal(SmallInt(Opcode) shl 2)) and $FFF;
        SymbolName := GetSymbolName(TargetAddress + $A4001000);
        if SymbolName <> '' then
        begin
          SetLength(Text, Pos('0x', Text) - 1);
          Text := Text + SymbolName + ' (0x' + IntToHex(TargetAddress, 3) + ')';
        end;
      end;
      if CurrentInstruction then
        UpdateOpcodeInfo(True, Opcode, Text, Address);
      if (Text = 'NOP') and (Font.Color = clBlack) then
        Font.Color := clGray;
      if Pos(#1, Text) > 0 then
      begin
        TextOut(OpcodeX, Row * RowHeight, Copy(Text, 1, Pos(#1, Text) - 1));
        Delete(Text, 1, Pos(#1, Text));
        TextOut(OperandsX, Row * RowHeight, Text);
      end
      else
        TextOut(OpcodeX, Row * RowHeight, Text);
      Brush.Color := clWhite;
    end;
  end;
end;

procedure TFormDebug.EditRSPPCChange(Sender: TObject);
var
  Address: Cardinal;
begin
  if Length(EditRSPPC.Text) = 3 then
  begin
    Address := StrToIntDef('$' + EditRSPPC.Text, 0);
    CenterAddress(Address, RSPViewAddress, PaintBoxRSP.Height,
      PaintBoxRSP.Canvas.TextHeight('A'));
    if Cardinal(ScrollBarRSP.Position) <> RSPViewAddress shr 2 then
      ScrollBarRSP.Position := RSPViewAddress shr 2
    else
      PaintBoxRSPPaint(PaintBoxRSP);
    if (EditRSPPC.Tag = 0) and ((RSPHistoryIndex = 0) or
      (RSPHistory[RSPHistoryIndex - 1] <> Address)) then
      begin
        Inc(RSPHistoryIndex);
        SetLength(RSPHistory, RSPHistoryIndex);
        RSPHistory[RSPHistoryIndex - 1] := Address;
      end;
  end;
  UpdateHistoryButtons;
end;

procedure TFormDebug.ButtonRSPBreakClick(Sender: TObject);
begin
  Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $20000;
end;

procedure TFormDebug.PaintBoxRSPMouseDown(Sender: TObject; Button: TMouseButton;
  Shift: TShiftState; X, Y: Integer);
var
  Address, Instruction: Cardinal;
begin
  if (X <= HeaderControlRSPCommands.Sections[0].Width) and (Button = mbLeft) then
  begin
    Address := RSPViewAddress + (Y div PaintBoxRSP.Canvas.TextHeight('A')) * 4;
    if HasRSPBreakpoint(Address) then
      RemoveRSPBreakpoint(Address, False)
    else
    begin
      Nemu64_API.RSPBreakpoints^[Nemu64_API.RSPBreakpointCount^] := Address;
      RSPTemporaryBreakpoints[Nemu64_API.RSPBreakpointCount^] := False;
      Inc(Nemu64_API.RSPBreakpointCount^);
    end;
    PaintBoxRSPPaint(PaintBoxRSP);
  end;
  if Button = mbRight then
  begin
    Instruction := (RSPViewAddress + (Y div PaintBoxRSP.Canvas.TextHeight('A')) * 4) and $FFF;
    MenuItemSetNewPC.Tag := Instruction;
    MenuItemMemorydebugger.Tag := Instruction + $A4001000;
    MenuItemCreatebookmark.Tag := Instruction + $A4001000;
    Instruction := ReadWord(Instruction + $04001000);
    if (Instruction shr 26 = 2) or (Instruction shr 26 = 3) then
    begin
      MenuItemFollowJump.Tag := (Instruction shl 2) and $FFF;
      MenuItemFollowJump.Enabled := True;
    end
    else
      MenuItemFollowJump.Enabled := False;
  end;
end;

procedure TFormDebug.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Action := caHide;
end;

procedure TFormDebug.ButtonViewMemoryClick(Sender: TObject);
begin
  FormMemory.EditMemAddress.Text := EditOpcodeinfoAddress.Text;
  FormMemory.Show;
end;

procedure TFormDebug.ButtonOpcodeInfoTargetAddressClick(Sender: TObject);
begin
  if PageControl.ActivePage = TabSheetCPU then
    EditCPUPC.Text := (Sender as TButton).Caption
  else
    EditRSPPC.Text := (Sender as TButton).Caption;
end;

procedure TFormDebug.MenuItemLibraryoptionsClick(Sender: TObject);
begin
  FormLibOptions.Show;
end;

procedure TFormDebug.FormCreate(Sender: TObject);
begin
  CPUHistoryIndex := 0;
  RSPHistoryIndex := 0;
  SetLength(CPUHistory, CPUHistoryIndex);
  SetLength(RSPHistory, RSPHistoryIndex);
  LastSymbolFile := '';
  BreakOnDList := False;
  BreakOnAList := False;
  Application.HintPause := 50;
  Application.HintHidePause := 100000;
  Application.HintShortPause := 50;
end;

procedure TFormDebug.MenuItemRemovesymbolinfofrommemoryClick(Sender: TObject);
begin
  if MessageBox(Handle, 'Remove symbol info?', 'Remove symbol info',
    MB_YESNO or MB_ICONQUESTION) = IDYES then
  begin
    SymbolCount := 0;
    FormSymbols.ListBoxSymbols.Items.Clear;
  end;
end;

procedure TFormDebug.MenuItemGeneratecodfilerunlfeClick(Sender: TObject);
var
  CommandLine: string;
begin
  if (Cardinal(Nemu64_API.RomFilename) = 0) or
    (Pos('.ZIP', UpperCase(Nemu64_API.RomFilename)) <> 0) then
    MessageBox(Handle, 'No rom loaded or rom is zipped', 'No rom loaded', MB_ICONERROR)
  else
  begin
    CommandLine := FormLibOptions.EditLFECommandline.Text;
    while Pos('%1', CommandLine) <> 0 do
    begin
      Insert(Nemu64_API.RomFilename, CommandLine, Pos('%1', CommandLine) + 2);
      Delete(CommandLine, Pos('%1', CommandLine), 2);
    end;
    if ShellExecute(Handle, 'open', PChar(FormLibOptions.EditLFE.Text),
      PChar(CommandLine), PChar(FormLibOptions.EditCodFolder.Text), SW_SHOW) = 2 then
      MessageBox(Handle, 'Didn''t find executable', 'Error', MB_ICONERROR);
  end;
end;

procedure TFormDebug.MenuItemViewsymbolsClick(Sender: TObject);
begin
  FormSymbols.Show;
end;

procedure TFormDebug.ButtonCPUStepOverClick(Sender: TObject);
var
  Instruction: Cardinal;
begin
  Instruction := ReadWord(TranslateAddress(Nemu64_API.CPUProgramCounter^));
  if (Instruction shr 26 = 3) or
    ((Instruction shr 26 = 0) and (Instruction and $3F = 9)) then
  begin
    if not HasCPUBreakpoint(Nemu64_API.CPUProgramCounter^ + 8) then
    begin
      CPUBreakpoints[BreakpointCount].Address := Nemu64_API.CPUProgramCounter^ + 8;
      CPUBreakpoints[BreakpointCount].Temporary := True;
      Inc(BreakpointCount);
    end;
    Nemu64_API.DebugFlags^ := (Nemu64_API.DebugFlags^ and not $10000) or $400;
  end;
  if Nemu64_API.EmulationThread^ <> 0 then
    ResumeThread(Nemu64_API.EmulationThread^);
end;

procedure TFormDebug.MenuItemBreakafterdisplaylistClick(Sender: TObject);
begin
  MenuItemBreakafterdisplaylist.Checked := not MenuItemBreakafterdisplaylist.Checked;
  BreakOnDList := MenuItemBreakafterdisplaylist.Checked;
end;

procedure TFormDebug.MenuItemBreakafteraudiolistClick(Sender: TObject);
begin
  MenuItemBreakafteraudiolist.Checked := not MenuItemBreakafteraudiolist.Checked;
  BreakOnAList := MenuItemBreakafteraudiolist.Checked;
end;

procedure TFormDebug.MenuItemSetNewPCClick(Sender: TObject);
begin
  if PageControl.ActivePage = TabSheetCPU then
    Nemu64_API.CPUProgramCounter^ := (Sender as TMenuItem).Tag
  else
    Nemu64_API.RSPControlRegisters^.PC := (Sender as TMenuItem).Tag;
  RefreshDebugger;
end;

procedure TFormDebug.MenuItemFollowJumpClick(Sender: TObject);
begin
  if PageControl.ActivePage = TabSheetCPU then
    EditCPUPC.Text := IntToHex((Sender as TMenuItem).Tag, 8)
  else
    EditRSPPC.Text := IntToHex((Sender as TMenuItem).Tag, 3);
end;

procedure TFormDebug.MenuItemMemorydebuggerClick(Sender: TObject);
begin
  FormMemory.EditMemAddress.Text := IntToHex((Sender as TMenuItem).Tag, 8);
  FormMemory.Show;
end;

procedure TFormDebug.FormResize(Sender: TObject);
begin
  PanelOpcodeInfo.Left := ClientWidth - 109;
  ScrollBarRSP.LargeChange := (PaintBoxRSP.Height - 1) div
    PaintBoxRSP.Canvas.TextHeight('A') - 1;
end;

procedure TFormDebug.ButtonRSPStepOverClick(Sender: TObject);
var
  Instruction: Cardinal;
begin
  Instruction := ReadWord(Nemu64_API.RSPControlRegisters^.PC + $04001000);
  if (Instruction shr 26 = 3) or
    ((Instruction shr 26 = 0) and (Instruction and $3F = 9)) then
  begin
    if not HasRSPBreakpoint(Nemu64_API.RSPControlRegisters^.PC + 8) then
    begin
      Nemu64_API.RSPBreakpoints^[Nemu64_API.RSPBreakpointCount^] := Nemu64_API.RSPControlRegisters^.PC + 8;
      RSPTemporaryBreakpoints[Nemu64_API.RSPBreakpointCount^] := True;
      Inc(Nemu64_API.RSPBreakpointCount^);
    end;
    Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ and not $20000;
  end;
  if Nemu64_API.EmulationThread^ <> 0 then
    ResumeThread(Nemu64_API.EmulationThread^);
end;

procedure TFormDebug.SpeedButtonCPUBackClick(Sender: TObject);
begin
  if CPUHistoryIndex > 1 then
  begin
    Dec(CPUHistoryIndex);
    EditCPUPC.Tag := 1;
    EditCPUPC.Text := IntToHex(CPUHistory[CPUHistoryIndex - 1], 8);
    EditCPUPC.Tag := 0;
    UpdateHistoryButtons;
  end;
end;

procedure TFormDebug.SpeedButtonCPUForwardClick(Sender: TObject);
begin
  if Cardinal(Length(CPUHistory)) > CPUHistoryIndex then
  begin
    Inc(CPUHistoryIndex);
    EditCPUPC.Tag := 1;
    EditCPUPC.Text := IntToHex(CPUHistory[CPUHistoryIndex - 1], 8);
    EditCPUPC.Tag := 0;
    UpdateHistoryButtons;
  end;
end;

procedure TFormDebug.MenuItemSaveundoinformationClick(Sender: TObject);
begin
  MenuItemSaveundoinformation.Checked := not MenuItemSaveundoinformation.Checked;
  Nemu64_API.UndoEnabled^ := MenuItemSaveundoinformation.Checked;
end;

procedure PreviousUndoWord;
begin
  if Nemu64_API.UndoPos^ = 0 then
    Nemu64_API.UndoPos^ := $3FFFFC
  else
    Dec(Nemu64_API.UndoPos^, 4);
end;

function ReadUndoWord: Cardinal;
begin
  PreviousUndoWord;
  Result := PCardinal(@PChar(Nemu64_API.UndoData)[Nemu64_API.UndoPos^])^;
end;

{ 004B4FE4 }

{$W+}
function ReadUndoInt64: Int64;
begin
  Result := (Int64(ReadUndoWord) shl 32) or ReadUndoWord;
end;

{$W-}

procedure TFormDebug.ButtonStepBackClick(Sender: TObject);
var
  Instruction: Cardinal;
begin
  Instruction := ReadUndoWord;
  case Instruction shr 26 of
    8..15, 24..27, 32..39, 55:
      begin
        Nemu64_API.CPURegisters^[(Instruction shr 16) and 31] := ReadUndoInt64;
        ShowMessage(DisassembleCPU(Nemu64_API.CPUProgramCounter^, Instruction, True));
        Dec(Nemu64_API.CPUProgramCounter^, 4);
      end;
    3:
      begin
        Nemu64_API.CPUProgramCounter^ := ReadUndoWord;
        Nemu64_API.CPURegisters^[31] := ReadUndoInt64;
        ShowMessage(DisassembleCPU(Nemu64_API.CPUProgramCounter^, Instruction, True));
      end;
    2, 4..7, 20..23:
      begin
        Nemu64_API.CPUProgramCounter^ := ReadUndoWord;
        ShowMessage(DisassembleCPU(Nemu64_API.CPUProgramCounter^, Instruction, True));
      end;
  else
    ShowMessage('Case else (' +
      DisassembleCPU(Nemu64_API.CPUProgramCounter^, Instruction, True) + ') at PC: ' +
      IntToHex(Int64(PC), 8) + ' at Ptr: ' +
      IntToHex(Int64(Nemu64_API.UndoPos^), 8));
    Dec(Nemu64_API.CPUProgramCounter^, 4);
  end;
  ShowDebugger;
end;

procedure TFormDebug.MenuItemCreatebookmarkClick(Sender: TObject);
var
  Node: TTreeNode;
begin
  FormBookmarks.Show;
  if FormBookmarks.Notebook.PageIndex = 1 then
    FormBookmarks.ButtonCreate.Click;
  if FormBookmarks.TreeViewBookmarks.Selected = nil then
    Node := FormBookmarks.TreeViewBookmarks.Items.AddFirst(nil, '')
  else
    Node := FormBookmarks.TreeViewBookmarks.Items.AddChildFirst(
      FormBookmarks.TreeViewBookmarks.Selected, '');
  if Node.Parent <> nil then
    Node.Parent.Expand(False);
  if PageControl.ActivePageIndex = 0 then
    Node.Text := 'CPU 0x' + IntToHex((Sender as TMenuItem).Tag, 8) + ': '
  else
    Node.Text := 'RSP 0x' + IntToHex((Sender as TMenuItem).Tag, 8) + ': ';
  Node.EditText;
end;

procedure TFormDebug.PageControlChange(Sender: TObject);
begin
  if PageControl.ActivePage = TabSheetRDP then
  begin
    RefreshRDP;
    PanelOpcodeInfo.Visible := False;
  end
  else
    PanelOpcodeInfo.Visible := True;
end;

procedure TFormDebug.ListBoxRDPDrawItem(Control: TWinControl; Index: Integer;
  Rect: TRect; State: TOwnerDrawState);
begin
  with Control as TListBox do
  begin
    if Pos('TRI_', ListBoxRDP.Items[Index]) <> 0 then
      Canvas.Font.Color := clRed;
    Canvas.TextRect(Rect, Rect.Left + 2, Rect.Top, ListBoxRDP.Items[Index]);
  end;
end;

procedure TFormDebug.PaintBoxRSPMouseMove(Sender: TObject; Shift: TShiftState;
  X, Y: Integer);
var
  Text: string;
  PC, Instruction, Address, Value: Cardinal;
begin
  PC := RSPViewAddress + $A4001000 +
    Cardinal((Y div PaintBoxRSP.Canvas.TextHeight('A')) shl 2);
  if GetSymbolName(PC) <> '' then
    Text := GetSymbolName(PC) + #13
  else
    Text := '';
  Instruction := ReadWord(TranslateAddress(PC));
  DisassembleRSP(PC, Instruction, True);
  if Reg1 > 0 then
    if Reg1 >= 32 then
      Text := Text + '$v' + IntToStr(Reg1 - 32) + #13
    else
      Text := Text + GPRNames[Reg1] + ': 0x' +
        IntToHex(Int64(Nemu64_API.RSPRegisters^[Reg1]), 8) + ' (' +
        IntToStr(Int64(Nemu64_API.RSPRegisters^[Reg1])) + ')'#13;
  if (Reg2 > 0) and (Reg2 <> Reg1) then
    if Reg2 >= 32 then
      Text := Text + '$v' + IntToStr(Reg2 - 32) + #13
    else
      Text := Text + GPRNames[Reg2] + ': 0x' +
        IntToHex(Int64(Nemu64_API.RSPRegisters^[Reg2]), 8) + ' (' +
        IntToStr(Int64(Nemu64_API.RSPRegisters^[Reg2])) + ')'#13;
  if (Reg3 > 0) and (Reg3 <> Reg2) and
    (Reg3 <> Reg1) then
    if Reg3 >= 32 then
      Text := Text + '$v' + IntToStr(Reg3 - 32) + #13
    else
      Text := Text + GPRNames[Reg3] + ': 0x' +
        IntToHex(Int64(Nemu64_API.RSPRegisters^[Reg3]), 8) + ' (' +
        IntToStr(Int64(Nemu64_API.RSPRegisters^[Reg3])) + ')'#13;
  if MemAccess then
  begin
    Address := Nemu64_API.RSPRegisters^[DebuggerAddressReg] + ImmValue;
    Value := ReadWord((Address and $FFF) + $04000000);
    Text := Text + 'Memory address: 0x' + IntToHex(Int64(Address and $FFF), 3) +
      #13 + 'Memory contents: 0x' + IntToHex(Int64(Value), 8) + ' (' +
      IntToStr(Int64(Value)) + ')'#13;
  end;
  if (Length(Text) > 0) and (Text[Length(Text)] = #13) then
    SetLength(Text, Length(Text) - 1);
  if PaintBoxRSP.Hint <> Text then
  begin
    Application.CancelHint;
    PaintBoxRSP.Hint := Text;
  end;
end;

procedure TFormDebug.PaintBoxCPUMouseMove(Sender: TObject; Shift: TShiftState;
  X, Y: Integer);
var
  Text: string;
  PC, Instruction, Address, Value: Cardinal;
begin
  PC := CPUViewAddress +
    Cardinal((Y div PaintBoxCPU.Canvas.TextHeight('A')) shl 2);
  if GetSymbolName(PC) <> '' then
    Text := GetSymbolName(PC) + #13
  else
    Text := '';
  Instruction := ReadWord(TranslateAddress(PC));
  DisassembleCPU(PC, Instruction, True);
  if Reg1 > 0 then
    Text := Text + GPRNames[Reg1] + ': 0x' +
      IntToHex(Nemu64_API.CPURegisters^[Reg1], 16) + ' (' +
      IntToStr(Nemu64_API.CPURegisters^[Reg1]) + ')'#13;
  if (Reg2 > 0) and (Reg2 <> Reg1) then
    Text := Text + GPRNames[Reg2] + ': 0x' +
      IntToHex(Nemu64_API.CPURegisters^[Reg2], 16) + ' (' +
      IntToStr(Nemu64_API.CPURegisters^[Reg2]) + ')'#13;
  if (Reg3 > 0) and (Reg3 <> Reg2) and
    (Reg3 <> Reg1) then
    Text := Text + GPRNames[Reg3] + ': 0x' +
      IntToHex(Nemu64_API.CPURegisters^[Reg3], 16) + ' (' +
      IntToStr(Nemu64_API.CPURegisters^[Reg3]) + ')'#13;
  if MemAccess then
  begin
    Address := Nemu64_API.CPURegisters^[DebuggerAddressReg] + ImmValue;
    Value := ReadWord(TranslateAddress(Address and $FFFFFFFC));
    Text := Text + 'Memory address: 0x' +
      IntToHex(Int64(Address and $FFFFFFFC), 8) + #13 +
      'Memory contents: 0x' + IntToHex(Int64(Value), 8) + ' (' +
      IntToStr(Int64(Value)) + ')'#13;
  end;
  if (Length(Text) > 0) and (Text[Length(Text)] = #13) then
    SetLength(Text, Length(Text) - 1);
  if PaintBoxCPU.Hint <> Text then
  begin
    Application.CancelHint;
    PaintBoxCPU.Hint := Text;
  end;
end;

procedure TFormDebug.SpeedButtonRSPBackClick(Sender: TObject);
begin
  if RSPHistoryIndex > 1 then
  begin
    Dec(RSPHistoryIndex);
    EditRSPPC.Tag := 1;
    EditRSPPC.Text := IntToHex(Int64(RSPHistory[RSPHistoryIndex - 1]), 3);
    EditRSPPC.Tag := 0;
    UpdateHistoryButtons;
  end;
end;

procedure TFormDebug.SpeedButtonRSPForwardClick(Sender: TObject);
begin
  if Cardinal(Length(RSPHistory)) > RSPHistoryIndex then
  begin
    Inc(RSPHistoryIndex);
    EditRSPPC.Tag := 1;
    EditRSPPC.Text := IntToHex(Int64(RSPHistory[RSPHistoryIndex - 1]), 3);
    EditRSPPC.Tag := 0;
    UpdateHistoryButtons;
  end;
end;

procedure TFormDebug.ScrollBarRSPChange(Sender: TObject);
begin
  RSPViewAddress := ScrollBarRSP.Position shl 2;
  PaintBoxRSPPaint(PaintBoxRSP);
end;

procedure TFormDebug.ButtonBookmarksClick(Sender: TObject);
begin
  FormBookmarks.Show;
end;

procedure TFormDebug.FormKeyUp(Sender: TObject; var Key: Word;
  Shift: TShiftState);
var
  Button: TButton;
begin
  Button := nil;
  case Key of
    VK_F5:
      if PageControl.ActivePage = TabSheetCPU then
        Button := ButtonCPUGo
      else if PageControl.ActivePage = TabSheetRSP then
        Button := ButtonRSPGo;
    VK_F10:
      if PageControl.ActivePage = TabSheetCPU then
        Button := ButtonCPUStepOver
      else if PageControl.ActivePage = TabSheetRSP then
        Button := ButtonRSPStepOver;
    VK_F11:
      if PageControl.ActivePage = TabSheetCPU then
        Button := ButtonCPUStep
      else if PageControl.ActivePage = TabSheetRSP then
        Button := ButtonRSPStep;
  end;
  if Button <> nil then
  begin
    Button.Click;
    Button.SetFocus;
  end;
end;

end.
