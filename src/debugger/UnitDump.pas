unit UnitDump;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls, Menus, Buttons;

type
  TFormDump = class(TForm)
    GroupBoxRange: TGroupBox;
    LabelStart: TLabel;
    LabelEnd: TLabel;
    EditStart: TEdit;
    EditEnd: TEdit;
    GroupBoxFile: TGroupBox;
    LabelFilename: TLabel;
    EditFilename: TEdit;
    ButtonBrowse: TButton;
    SaveDialogDumpfile: TSaveDialog;
    LabelFormat: TLabel;
    ComboBoxEndian: TComboBox;
    ButtonDump: TButton;
    ButtonClose: TButton;
    ButtonAutoRSPUcode: TButton;
    CheckBoxAlternatePC: TCheckBox;
    EditAlternateStartPC: TEdit;
    procedure ButtonBrowseClick(Sender: TObject);
    procedure ButtonCloseClick(Sender: TObject);
    procedure ButtonDumpClick(Sender: TObject);
    procedure EditStartChange(Sender: TObject);
    procedure EditEndChange(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure ButtonAutoRSPUcodeClick(Sender: TObject);
    procedure CheckBoxAlternatePCClick(Sender: TObject);
  end;

var
  FormDump: TFormDump;

implementation

uses UnitDecs, UnitDebugMemory, UnitDisassembly, UnitMIPSToCCode;

{$R *.dfm}

procedure TFormDump.ButtonBrowseClick(Sender: TObject);
begin
  if SaveDialogDumpfile.Execute then
    EditFilename.Text := SaveDialogDumpfile.FileName;
end;

procedure TFormDump.ButtonCloseClick(Sender: TObject);
begin
  Close;
end;

procedure TFormDump.ButtonDumpClick(Sender: TObject);
var
  Address, Instruction, Count, LastAddress, PC: Cardinal;
  Line: string;
  SwapEndian, DelaySlot: Boolean;
  BinaryFile: file;
  OutputFile: TextFile;
begin
  if CheckBoxAlternatePC.Checked then
    PC := StrToIntDef('$' + EditAlternateStartPC.Text, 0)
  else
    PC := StrToIntDef('$' + EditStart.Text, 0);

  if (ComboBoxEndian.ItemIndex = 0) or (ComboBoxEndian.ItemIndex = 1) then
  begin
    AssignFile(BinaryFile, EditFilename.Text);
    Rewrite(BinaryFile, 1);
    Address := ResolvePhysicalAddress(StrToIntDef('$' + EditStart.Text, 0));
    LastAddress := ResolvePhysicalAddress(StrToIntDef('$' + EditEnd.Text, 0));
    Count := 0;
    SwapEndian := ComboBoxEndian.ItemIndex = 1;
    while Address < LastAddress do
    begin
      Instruction := ReadWord(Address);
      if SwapEndian then
      asm
        MOV EAX, Instruction
        BSWAP EAX
        MOV Instruction, EAX
      end;
      BlockWrite(BinaryFile, Instruction, 4);
      Inc(Address, 4);
      Inc(Count, 4);
    end;
    CloseFile(BinaryFile);
    Application.MessageBox(PChar('Dumped ' + IntToStr(Count) + ' bytes'),
      'Success', MB_ICONINFORMATION);
  end;

  if (ComboBoxEndian.ItemIndex = 2) or (ComboBoxEndian.ItemIndex = 3) then
  begin
    AssignFile(OutputFile, EditFilename.Text);
    Rewrite(OutputFile);
    Address := StrToIntDef('$' + EditStart.Text, 0);
    LastAddress := TranslateAddress(StrToIntDef('$' + EditEnd.Text, 0));
    Address := TranslateAddress(Address);
    Count := 0;
    while Address <= LastAddress do
    begin
      Instruction := ReadWord(Address);
      Line := DisassembleCPU(PC, Instruction, True);
      while Pos(#1, Line) <> 0 do
        Line[Pos(#1, Line)] := #9;
      if ComboBoxEndian.ItemIndex = 3 then
        WriteLn(OutputFile, IntToHex(PC, 8) + ': ' + Line)
      else
        WriteLn(OutputFile, Line);
      Inc(Address, 4);
      Inc(Count, 4);
      Inc(PC, 4);
    end;
    CloseFile(OutputFile);
    Application.MessageBox(PChar('Dumped ' + IntToStr(Count div 4) + ' commands'),
      'Success', MB_ICONINFORMATION);
  end;

  if (ComboBoxEndian.ItemIndex = 4) or (ComboBoxEndian.ItemIndex = 5) then
  begin
    AssignFile(OutputFile, EditFilename.Text);
    Rewrite(OutputFile);
    Address := StrToIntDef('$' + EditStart.Text, 0);
    LastAddress := TranslateAddress(StrToIntDef('$' + EditEnd.Text, 0));
    Address := TranslateAddress(Address);
    Count := 0;
    while Address <= LastAddress do
    begin
      Instruction := ReadWord(Address);
      Line := DisassembleRSP(PC, Instruction, True);
      while Pos(#1, Line) <> 0 do
        Line[Pos(#1, Line)] := #9;
      if ComboBoxEndian.ItemIndex = 5 then
        WriteLn(OutputFile, IntToHex(PC, 8) + ': ' + Line)
      else
        WriteLn(OutputFile, Line);
      Inc(Address, 4);
      Inc(Count, 4);
      Inc(PC, 4);
    end;
    CloseFile(OutputFile);
    Application.MessageBox(PChar('Dumped ' + IntToStr(Count div 4) + ' commands'),
      'Success', MB_ICONINFORMATION);
  end;

  if ComboBoxEndian.ItemIndex = 6 then
  begin
    AssignFile(OutputFile, EditFilename.Text);
    Rewrite(OutputFile);
    Address := StrToIntDef('$' + EditStart.Text, 0);
    LastAddress := TranslateAddress(StrToIntDef('$' + EditEnd.Text, 0));
    Address := TranslateAddress(Address);
    Count := 0;
    while Address <= LastAddress do
    begin
      Instruction := ReadHalfWord(Address);
      WriteLn(OutputFile, IntToHex(PC, 8) + ': ' + IntToHex(Instruction, 4));
      Inc(Address, 2);
      Inc(Count, 2);
      Inc(PC, 2);
    end;
    CloseFile(OutputFile);
    Application.MessageBox(PChar('Dumped ' + IntToStr(Count div 2) + ' halfwords'),
      'Success', MB_ICONINFORMATION);
  end;

  if ComboBoxEndian.ItemIndex = 7 then
  begin
    AssignFile(OutputFile, EditFilename.Text);
    Rewrite(OutputFile);
    Address := StrToIntDef('$' + EditStart.Text, 0);
    LastAddress := TranslateAddress(StrToIntDef('$' + EditEnd.Text, 0));
    Address := TranslateAddress(Address);
    Count := 0;
    while Address <= LastAddress do
    begin
      Instruction := ReadWord(Address);
      WriteLn(OutputFile, IntToHex(PC, 8) + ': ' + IntToHex(Instruction, 8));
      Inc(Address, 4);
      Inc(Count, 4);
      Inc(PC, 4);
    end;
    CloseFile(OutputFile);
    Application.MessageBox(PChar('Dumped ' + IntToStr(Count div 4) + ' dwords'),
      'Success', MB_ICONINFORMATION);
  end;

  if ComboBoxEndian.ItemIndex = 8 then
  begin
    AssignFile(OutputFile, EditFilename.Text);
    Rewrite(OutputFile);
    Address := StrToIntDef('$' + EditStart.Text, 0);
    LastAddress := TranslateAddress(StrToIntDef('$' + EditEnd.Text, 0));
    WriteLn(OutputFile, '// Generated By Nemu Debugger from ' + Nemu64_API.RomInternalName);
    WriteLn(OutputFile, '#include "stdafx.h"');
    WriteLn(OutputFile, '// ToDo: Include files for Registers and stuff in here:');
    WriteLn(OutputFile, '#include "cpu.h"');
    WriteLn(OutputFile, '#include "decs.h"');
    WriteLn(OutputFile, '#include "mem.h"');
    WriteLn(OutputFile);
    WriteLn(OutputFile);
    WriteLn(OutputFile, 'void func' + IntToHex(PC, 8) + '()');
    WriteLn(OutputFile, '{');
    Address := TranslateAddress(Address);
    Count := 0;
    while Address <= LastAddress do
    begin
      Instruction := ReadWord(Address);
      WriteLn(OutputFile, 'pc' + IntToHex(PC, 8) + ':    ' +
        MIPSToCCode(PC, Instruction, DelaySlot));
      if DelaySlot then
      begin
        Inc(Address, 4);
        Inc(Count, 4);
        Inc(PC, 4);
        Instruction := ReadWord(Address);
        WriteLn(OutputFile, 'pc' + IntToHex(PC, 8) + ':    // 0x' +
          IntToHex(Instruction, 8) + ' ' + DisassembleCPU(PC, Instruction, True));
      end;
      Inc(Address, 4);
      Inc(Count, 4);
      Inc(PC, 4);
    end;
    WriteLn(OutputFile, #9';');
    WriteLn(OutputFile, '}  // done');
    CloseFile(OutputFile);
    Application.MessageBox(PChar('Dumped ' + IntToStr(Count div 4) + ' commands'),
      'Success', MB_ICONINFORMATION);
  end;
end;

procedure TFormDump.EditStartChange(Sender: TObject);
begin
  if StrToIntDef('$' + EditStart.Text, -1) = -1 then
    LabelStart.Font.Color := clRed
  else
    LabelStart.Font.Color := clWindowText;
end;

procedure TFormDump.EditEndChange(Sender: TObject);
begin
  if StrToIntDef('$' + EditEnd.Text, -1) = -1 then
    LabelEnd.Font.Color := clRed
  else
    LabelEnd.Font.Color := clWindowText;
end;

procedure TFormDump.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Action := caHide;
end;

procedure TFormDump.ButtonAutoRSPUcodeClick(Sender: TObject);
var
  StartAddress: Cardinal;
begin
  StartAddress := ReadWord(Nemu64_API.RSPTask^ + $10) + $80000000;
  EditStart.Text := IntToHex(StartAddress, 8);
  EditEnd.Text := IntToHex(StartAddress + $1000, 8);
  ComboBoxEndian.ItemIndex := 5;
  CheckBoxAlternatePC.Checked := True;
  EditAlternateStartPC.Text := '80';
end;

procedure TFormDump.CheckBoxAlternatePCClick(Sender: TObject);
begin
  EditAlternateStartPC.Enabled := CheckBoxAlternatePC.Checked;
end;

end.
