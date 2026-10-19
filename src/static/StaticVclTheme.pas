{ Static user-build palette bridge. Reconstructed plugin sources stay intact. }
unit StaticVclTheme;
interface
uses Graphics;
function StaticThemeColor(Color: TColor): TColor;
procedure NemuStaticVclThemeRefresh; cdecl;
implementation
uses Windows, Messages, Classes, Controls, Forms, StdCtrls, ExtCtrls;

function NemuStaticThemeIsDark: LongBool; cdecl;
  external 'nemu64-static.exe' name 'NemuStaticThemeIsDark';

{ Only explicit UI colors in the generated debugger painters use this mapping.
  Sprite transparency and emulated image data never pass through it. }
function StaticThemeColor(Color: TColor): TColor;
begin
  Result := Color;
  if not NemuStaticThemeIsDark then Exit;
  case Color of
    clWhite: Result := clWindow;
    clBlack: Result := clWindowText;
    clBlue: Result := RGB(128,176,255);
    clRed: Result := RGB(255,110,110);
    clGray: Result := RGB(160,160,160);
    $00FFCACA: Result := RGB(42,65,100); { selected instruction }
    $AEFFFF: Result := RGB(90,75,22);   { read watch: yellow }
    $FFAEAE: Result := RGB(35,65,105);  { write watch: blue }
    $D7FFAE: Result := RGB(45,85,40);   { read/write watch: green }
    $40FFFB: Result := RGB(105,72,12);  { search match: amber }
  end;
end;

type
  TControlAccess = class(TControl);
  { Owned by the control, so recreated/destroyed forms cannot leave stale
    pointers. Keep literals for restoring the original light appearance. }
  TStaticPalette = class(TComponent)
  public
    Color, FontColor: TColor;
  end;

procedure RefreshControl(Control: TControl);
var
  I: Integer;
  Access: TControlAccess;
  Palette: TStaticPalette;
  IsDark: Boolean;
begin
  Access := TControlAccess(Control);
  Palette := nil;
  for I := 0 to Control.ComponentCount - 1 do
    if Control.Components[I] is TStaticPalette then
      Palette := TStaticPalette(Control.Components[I]);
  if Palette = nil then
  begin
    Palette := TStaticPalette.Create(Control);
    Palette.Color := Access.Color;
    Palette.FontColor := Access.Font.Color;
  end;
  IsDark := NemuStaticThemeIsDark;
  if (Palette.Color = clWhite) and
     ((Control is TCustomEdit) or (Control is TCustomListBox) or
      (Control is TCustomComboBox) or (Control is TCustomPanel)) then
    if IsDark then Access.Color := clWindow else Access.Color := Palette.Color;
  if Palette.FontColor = clBlack then
    if IsDark then Access.Font.Color := clWindowText else Access.Font.Color := clBlack;
  { Banner headings keep their blue/red meaning, with legible dark contrast. }
  if (Control is TCustomLabel) and (Control.Parent is TCustomPanel) then
    if Palette.FontColor = clNavy then
      if IsDark then Access.Font.Color := RGB(128,176,255) else Access.Font.Color := clNavy
    else if Palette.FontColor = clMaroon then
      if IsDark then Access.Font.Color := RGB(255,150,150) else Access.Font.Color := clMaroon;
  if Control is TWinControl then
    for I := 0 to TWinControl(Control).ControlCount - 1 do
      RefreshControl(TWinControl(Control).Controls[I]);
  Control.Invalidate;
end;

procedure NemuStaticVclThemeRefresh;
var I: Integer;
begin
  Graphics.PaletteChanged;
  if Screen = nil then Exit;
  for I := 0 to Screen.CustomFormCount - 1 do
  begin
    Screen.CustomForms[I].Perform(CM_SYSCOLORCHANGE, 0, 0);
    RefreshControl(Screen.CustomForms[I]);
  end;
end;
end.
