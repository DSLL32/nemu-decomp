unit StaticDebuggerProbe;
interface
function ProbeDebugger(Action: Integer): Integer; cdecl;
implementation
uses Windows, Graphics, Static_debugger_UnitDecs,
  Static_debugger_UnitDebugCommands, Static_debugger_UnitDebugMemory,
  Static_debugger_UnitDebugRegisters, Static_debugger_UnitSearch;
var Saved: TNemu; Seeded: Boolean;
function ReadTestWord(Address: Cardinal): Cardinal; stdcall;
begin Result := $24010001; end;
function TranslateTest(Address: Cardinal; WriteAccess: LongBool): Cardinal; stdcall;
begin Result := Address and $1FFFFFFF; end;
function ProbeDebugger(Action: Integer): Integer;
var
  X, H, I: Integer; Expected, Background, Foreground: TColor;
  ReadColor, WriteColor, CombinedColor, SearchColor: TColor;
begin
  Result := 1;
  case Action of
    0: begin
      if not Seeded then begin Saved := Nemu64_API; Seeded := True; end;
      Nemu64_API.ReadWord := ReadTestWord;
      Nemu64_API.TranslateAddress := TranslateTest;
      Nemu64_API.WatchCount^ := 3;
      for I := 0 to 2 do begin
        Nemu64_API.Watches^[I].Address := I * 4;
        Nemu64_API.Watches^[I].WatchType := I + 1;
      end;
      FormSearch.ListBoxSearchResults.Items.Add('0x8000000C');
      FormMemory.EditMemAddress.Text := '80000000';
      FormMemory.Show; FormMemory.BringToFront; FormMemory.RefreshMemory;
    end;
    1: begin FormDebug.Show; FormDebug.BringToFront; FormDebug.RefreshDebugger; end;
    2: begin FormRegisters.Show; FormRegisters.BringToFront; FormDebug.RefreshDebugger; end;
    9: begin
      FormMemory.Hide; FormDebug.Hide; FormRegisters.Hide;
      Nemu64_API.WatchCount^ := 0; Nemu64_API := Saved;
    end;
    10,20: begin
      Background := clWhite; Foreground := clBlack;
      ReadColor := $AEFFFF; WriteColor := $FFAEAE; CombinedColor := $D7FFAE; SearchColor := $40FFFB;
      if Action = 10 then begin
        Background := RGB(25,25,25); Foreground := RGB(232,232,232);
        ReadColor := RGB(90,75,22); WriteColor := RGB(35,65,105);
        CombinedColor := RGB(45,85,40); SearchColor := RGB(105,72,12);
      end;
      FormMemory.RefreshMemory;
      with FormMemory.PaintBox.Canvas do begin
        if ColorToRGB(Brush.Color) <> Background then Result := 27;
        if Pixels[2,TextHeight('A')*2-1] <> Background then Result := 28;
        X := TextWidth('00000000:'); H := TextHeight('A');
        if Pixels[X+2,H-1] <> ReadColor then Result := 21;
        if Pixels[X+74,H-1] <> WriteColor then Result := 22;
        if Pixels[X+162,H-1] <> CombinedColor then Result := 23;
        if Pixels[X+234,H-1] <> SearchColor then Result := 24;
        if Pixels[FormMemory.PaintBox.Width-2,FormMemory.PaintBox.Height-2] <>
          Background then Result := 25;
        if ColorToRGB(Font.Color) <> Foreground then Result := 26;
      end;
    end;
    11: begin
      FormDebug.RefreshDebugger;
      with FormDebug.PaintBoxCPU do
        if Canvas.Pixels[Width-2,Height-2] <> RGB(25,25,25) then Result := 31;
    end;
    12: begin
      FormDebug.RefreshDebugger;
      Expected := RGB(232,232,232);
      if ColorToRGB(FormRegisters.GetRegisterLabel(15).Font.Color) <> Expected then Result := 32;
    end;
  end;
end;
end.
