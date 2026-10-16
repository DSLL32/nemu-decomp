library debugger;

{$IMPLICITBUILD OFF}

uses
  SysUtils,
  Windows,
  Messages,
  Classes,
  Registry,
  UnitMain in 'UnitMain.pas',
  UnitDecs in 'UnitDecs.pas',
  RCP in 'RCP.pas',
  UnitDisassembly in 'UnitDisassembly.pas',
  UnitOpcode in 'UnitOpcode.pas',
  UnitDebugCommands in 'UnitDebugCommands.pas',
  UnitBookmarks in 'UnitBookmarks.pas',
  UnitDebugMemory in 'UnitDebugMemory.pas',
  UnitDump in 'UnitDump.pas',
  UnitMIPSToCCode in 'UnitMIPSToCCode.pas',
  UnitSearch in 'UnitSearch.pas',
  UnitDebugRegisters in 'UnitDebugRegisters.pas',
  r4300 in 'r4300.pas',
  UnitLibOptions in 'UnitLibOptions.pas',
  UnitSymbolViewer in 'UnitSymbolViewer.pas',
  UnitDebugBreakOpcode in 'UnitDebugBreakOpcode.pas',
  UnitRegChange in 'UnitRegChange.pas',
  UnitStats in 'UnitStats.pas',
  UnitImageView in 'UnitImageView.pas';

{$R XPManifest.res}

function ReadIntegerDef(Registry: TRegistry; Name: string; Default: Integer): Integer;
begin
  if Registry.ValueExists(Name) then
    Result := Registry.ReadInteger(Name)
  else
    Result := Default;
end;

function ToolInit(Info: PNemu): Integer; cdecl;
var
  Reg: TRegistry;
begin
  GetDir(0, NemuFileDir);
  if NemuFileDir <> '' then
    if NemuFileDir[Length(NemuFileDir)] <> '\' then
      NemuFileDir := NemuFileDir + '\';
  Result := $18F;
  BreakpointCount := 0;
  Move(Info^, Nemu64_API, SizeOf(Nemu64_API));
  NemuMainWinHandle := THandle(Nemu64_API.RDRAM);
  SymbolCount := 0;
  FormDebug := TFormDebug.Create(nil);
  FormMemory := TFormMemory.Create(nil);
  FormRegisters := TFormRegisters.Create(nil);
  FormDump := TFormDump.Create(nil);
  FormBookmarks := TFormBookmarks.Create(nil);
  FormDebugBreakOpcode := TFormDebugBreakOpcode.Create(nil);
  FormDebugRegisterChange := TFormDebugRegisterChange.Create(nil);
  DebugFormHandle := FormDebug.Handle;
  FormStats := TFormStats.Create(nil);
  FormSearch := TFormSearch.Create(nil);
  FormLibOptions := TFormLibOptions.Create(nil);
  FormSymbols := TFormSymbols.Create(nil);
  FormImageView := TFormImageView.Create(nil);
  FormSymbols.Left := -1000;
  FormSymbols.Visible := True;
  FormSymbols.Visible := False;
  FormSymbols.Left := 192;
  FormBookmarks.Left := -1000;
  FormBookmarks.Visible := True;
  FormBookmarks.Visible := False;
  FormBookmarks.Left := 192;
  FormImageView.Left := -1000;
  FormImageView.Visible := True;
  FormImageView.Visible := False;
  FormImageView.Left := 192;
  SetActiveWindow(Nemu64_API.MainWindow^);
  Reg := TRegistry.Create;
  Reg.RootKey := HKEY_CURRENT_USER;
  Reg.OpenKey('\Software\Nemu\0.8\Debugger', True);
  FormDebug.Left := ReadIntegerDef(Reg, 'CommandsLeft', 531);
  FormDebug.Top := ReadIntegerDef(Reg, 'CommandsTop', 108);
  FormDebug.Width := ReadIntegerDef(Reg, 'CommandsWidth', 390);
  FormDebug.Height := ReadIntegerDef(Reg, 'CommandsHeight', 567);
  FormDebug.PageControl.ActivePageIndex := 0;
  FormMemory.Left := ReadIntegerDef(Reg, 'MemoryLeft', 258);
  FormMemory.Top := ReadIntegerDef(Reg, 'MemoryTop', 114);
  FormMemory.Width := ReadIntegerDef(Reg, 'MemoryWidth', 531);
  FormMemory.Height := ReadIntegerDef(Reg, 'MemoryHeight', 226);
  FormRegisters.Left := ReadIntegerDef(Reg, 'RegistersLeft', 196);
  FormRegisters.Top := ReadIntegerDef(Reg, 'RegistersTop', 100);
  FormRegisters.Width := ReadIntegerDef(Reg, 'RegistersWidth', 452);
  FormRegisters.Height := ReadIntegerDef(Reg, 'RegistersHeight', 541);
  FormBookmarks.Left := ReadIntegerDef(Reg, 'BookmarksLeft', 242);
  FormBookmarks.Top := ReadIntegerDef(Reg, 'BookmarksTop', 101);
  FormBookmarks.Width := ReadIntegerDef(Reg, 'BookmarksWidth', 448);
  FormBookmarks.Height := ReadIntegerDef(Reg, 'BookmarksHeight', 242);
  FormSymbols.Left := ReadIntegerDef(Reg, 'SymbolsLeft', 242);
  FormSymbols.Top := ReadIntegerDef(Reg, 'SymbolsTop', 101);
  FormSymbols.Width := ReadIntegerDef(Reg, 'SymbolsWidth', 307);
  FormSymbols.Height := ReadIntegerDef(Reg, 'SymbolsHeight', 224);
  FormImageView.Left := ReadIntegerDef(Reg, 'ImageLeft', 242);
  FormImageView.Top := ReadIntegerDef(Reg, 'ImageTop', 101);
  FormImageView.Width := ReadIntegerDef(Reg, 'ImageWidth', 307);
  FormImageView.Height := ReadIntegerDef(Reg, 'ImageHeight', 224);
  Reg.Free;
end;

procedure ToolFree; cdecl;
var
  Registry: TRegistry;
begin
  Registry := TRegistry.Create;
  Registry.RootKey := HKEY_CURRENT_USER;
  Registry.OpenKey('\Software\Nemu\0.8\Debugger', True);
  Registry.WriteInteger('CommandsLeft', FormDebug.Left);
  Registry.WriteInteger('CommandsTop', FormDebug.Top);
  Registry.WriteInteger('CommandsWidth', FormDebug.Width);
  Registry.WriteInteger('CommandsHeight', FormDebug.Height);
  Registry.WriteInteger('MemoryLeft', FormMemory.Left);
  Registry.WriteInteger('MemoryTop', FormMemory.Top);
  Registry.WriteInteger('MemoryWidth', FormMemory.Width);
  Registry.WriteInteger('MemoryHeight', FormMemory.Height);
  Registry.WriteInteger('RegistersLeft', FormRegisters.Left);
  Registry.WriteInteger('RegistersTop', FormRegisters.Top);
  Registry.WriteInteger('RegistersWidth', FormRegisters.Width);
  Registry.WriteInteger('RegistersHeight', FormRegisters.Height);
  Registry.WriteInteger('BookmarksLeft', FormBookmarks.Left);
  Registry.WriteInteger('BookmarksTop', FormBookmarks.Top);
  Registry.WriteInteger('BookmarksWidth', FormBookmarks.Width);
  Registry.WriteInteger('BookmarksHeight', FormBookmarks.Height);
  Registry.WriteInteger('SymbolsLeft', FormSymbols.Left);
  Registry.WriteInteger('SymbolsTop', FormSymbols.Top);
  Registry.WriteInteger('SymbolsWidth', FormSymbols.Width);
  Registry.WriteInteger('SymbolsHeight', FormSymbols.Height);
  Registry.WriteInteger('ImageLeft', FormImageView.Left);
  Registry.WriteInteger('ImageTop', FormImageView.Top);
  Registry.WriteInteger('ImageWidth', FormImageView.Width);
  Registry.WriteInteger('ImageHeight', FormImageView.Height);
  Registry.Free;
  FormDebug.Free;
  FormMemory.Free;
  FormRegisters.Free;
  FormBookmarks.Free;
  FormDump.Free;
  FormDebugBreakOpcode.Free;
  FormDebugRegisterChange.Free;
  FormStats.Free;
  FormSearch.Free;
  FormLibOptions.Free;
  FormSymbols.Free;
end;

procedure ToolGetMenuText(Text: PChar); cdecl;
begin
  StrCopy(Text,
    'Debugger: Commands...'#13 +
    'Debugger: Memory...'#13 +
    'Debugger: Registers...'#13 +
    'Debugger: Bookmarks...'#13 +
    'Debugger: Opcode breaks...'#13 +
    'Debugger: Register change...'#13 +
    'Debugger: Dump memory...'#13 +
    'Debugger: Search...'#13 +
    'Debugger: Statistics...'#13 +
    'Debugger: Image viewer...');
end;

procedure ToolFunc(Reason, Index: Integer); cdecl;
begin
  case Reason of
    1:
      case Index of
        0:
          begin
            FormDebug.Show;
            FormDebug.LoadSymbols('');
          end;
        1: FormMemory.Show;
        2: FormRegisters.Show;
        3: FormBookmarks.Show;
        4: FormDebugBreakOpcode.Show;
        5: FormDebugRegisterChange.Show;
        6: FormDump.Show;
        7: FormSearch.Show;
        8: FormStats.Show;
        9: FormImageView.Show;
      end;
    $80:
      begin
        if FormDebug.Visible then
          FormDebug.LoadSymbols('');
        SendMessage(BookmarksFormHandle, WM_USER + 1, 0, 0);
      end;
    2:
      SendMessage(DebugFormHandle, WM_USER + 1, Index, 0);
    $100:
      begin
        if HasCPUBreakpoint(Nemu64_API.LastProgramCounter^) then
          Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $100000;
        if RegBreakpointEnabled then
          CheckRegisterAccess(Index);
      end;
    4:
      begin
        if BreakOnDList and (Index = 0) then
          Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $10000;
        if RefreshAfterEveryFrame and (Index = 0) then
          FormImageView.PaintBoxPaint(FormImageView.PaintBox);
      end;
    8:
      if BreakOnAList and (Index = 0) then
        Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $10000;
  end;
end;

exports
  ToolInit,
  ToolFree,
  ToolGetMenuText,
  ToolFunc;

begin
end.
