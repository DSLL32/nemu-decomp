unit UnitCombiners;

interface

uses Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ExtCtrls, CheckLst;

type
  TFormCombiners = class(TForm)
    CheckListBox: TCheckListBox;
    PanelTop: TPanel;
    ButtonEdit: TButton;
    LabelColor: TLabel;
    LabelAlpha: TLabel;
    EditColor: TEdit;
    EditAlpha: TEdit;
    ButtonDelete: TButton;
    Label1: TLabel;
    EditExtras: TEdit;
    procedure CheckListBoxClickCheck(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure CheckListBoxDrawItem(Control: TWinControl; Index: Integer; Rect: TRect; State: TOwnerDrawState);
    procedure ButtonEditClick(Sender: TObject);
    procedure FormResize(Sender: TObject);
    procedure CheckListBoxClick(Sender: TObject);
    procedure ButtonDeleteClick(Sender: TObject);
  private
    FRefreshCount: Integer;
    procedure WMRefresh(var Message: TMessage); message WM_USER + 1;
    procedure WMShow(var Message: TMessage); message WM_USER + 2;
  end;

var
  FormCombiners: TFormCombiners;
  CombinerFormHandle: THandle;

implementation

uses UnitDecs, UnitCombineEditor;

{$R *.dfm}

var
  CombineHi, CombineLo: Cardinal;



const
  ColorAText: array[0..15] of string = (
    'CombinedColor', 'Tex0Color', 'Tex1Color', 'PrimColor', 'ShadeColor', 'EnvColor', '1', 'Noise', '0', '0', '0', '0', '0', '0', '0', '0');
  ColorBText: array[0..15] of string = (
    'CombinedColor', 'Tex0Color', 'Tex1Color', 'PrimColor', 'ShadeColor', 'EnvColor', 'Key Center', 'K4', '0', '0', '0', '0', '0', '0', '0', '0');
  ColorCText: array[0..31] of string = (
    'CombinedColor', 'Tex0Color', 'Tex1Color', 'PrimColor', 'ShadeColor', 'EnvColor', 'Key Scale', 'CombinedAlpha', 'Tex0Alpha', 'Tex1Alpha', 'PrimAlpha', 'ShadedAlpha', 'EnvAlpha', 'LODFraction', 'PrimLODFrac', 'K5', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0');
  ColorDText: array[0..7] of string = (
    'CombinedColor', 'Tex0Color', 'Tex1Color', 'PrimColor', 'ShadeColor', 'EnvColor', '1', '0');
  AlphaAText: array[0..7] of string = (
    'CombinedAlpha', 'Tex0Alpha', 'Tex1Alpha', 'PrimAlpha', 'ShadedAlpha', 'EnvAlpha', '1', '0');
  AlphaBText: array[0..7] of string = (
    'CombinedAlpha', 'Tex0Alpha', 'Tex1Alpha', 'PrimAlpha', 'ShadedAlpha', 'EnvAlpha', '1', '0');
  AlphaCText: array[0..7] of string = (
    'LODFraction', 'Tex0Alpha', 'Tex1Alpha', 'PrimAlpha', 'ShadedAlpha', 'EnvAlpha', 'PrimLODFrac', '0');
  AlphaDText: array[0..7] of string = (
    'CombinedAlpha', 'Tex0Alpha', 'Tex1Alpha', 'PrimAlpha', 'ShadedAlpha', 'EnvAlpha', '1', '0');

function AlphaD1: Cardinal;
begin
  Result := CombineLo and 7;
end;

function AlphaD1Str: string;
begin
  Result := AlphaDText[AlphaD1];
end;

function AlphaB1: Cardinal;
begin
  Result := CombineLo shr 3 and 7;
end;

function AlphaB1Str: string;
begin
  Result := AlphaBText[AlphaB1];
end;

function ColorD1: Cardinal;
begin
  Result := CombineLo shr 6 and 7;
end;

function ColorD1Str: string;
begin
  Result := ColorDText[ColorD1];
end;

function AlphaD0: Cardinal;
begin
  Result := CombineLo shr 9 and 7;
end;

function AlphaD0Str: string;
begin
  Result := AlphaDText[AlphaD0];
end;

function AlphaB0: Cardinal;
begin
  Result := CombineLo shr 12 and 7;
end;

function AlphaB0Str: string;
begin
  Result := AlphaBText[AlphaB0];
end;

function ColorD0: Cardinal;
begin
  Result := CombineLo shr 15 and 7;
end;

function ColorD0Str: string;
begin
  Result := ColorDText[ColorD0];
end;

function AlphaC1: Cardinal;
begin
  Result := CombineLo shr 18 and 7;
end;

function AlphaC1Str: string;
begin
  Result := AlphaCText[AlphaC1];
end;

function AlphaA1: Cardinal;
begin
  Result := CombineLo shr 21 and 7;
end;

function AlphaA1Str: string;
begin
  Result := AlphaAText[AlphaA1];
end;

function ColorB1: Cardinal;
begin
  Result := CombineLo shr 24 and 15;
end;

function ColorB1Str: string;
begin
  Result := ColorBText[ColorB1];
end;

function ColorB0: Cardinal;
begin
  Result := CombineLo shr 28 and 15;
end;

function ColorB0Str: string;
begin
  Result := ColorBText[ColorB0];
end;

function ColorC1: Cardinal;
begin
  Result := CombineHi and 31;
end;

function ColorC1Str: string;
begin
  Result := ColorCText[ColorC1];
end;

function ColorA1: Cardinal;
begin
  Result := CombineHi shr 5 and 15;
end;

function ColorA1Str: string;
begin
  Result := ColorAText[ColorA1];
end;

function AlphaC0: Cardinal;
begin
  Result := CombineHi shr 9 and 7;
end;

function AlphaC0Str: string;
begin
  Result := AlphaCText[AlphaC0];
end;

function AlphaA0: Cardinal;
begin
  Result := CombineHi shr 12 and 7;
end;

function AlphaA0Str: string;
begin
  Result := AlphaAText[AlphaA0];
end;

function ColorC0: Cardinal;
begin
  Result := CombineHi shr 15 and 31;
end;

function ColorC0Str: string;
begin
  Result := ColorCText[ColorC0];
end;

function ColorA0: Cardinal;
begin
  Result := CombineHi shr 20 and 15;
end;

function ColorA0Str: string;
begin
  Result := ColorAText[ColorA0];
end;

procedure TFormCombiners.WMShow(var Message: TMessage);
begin
  Visible := True;
end;

procedure TFormCombiners.WMRefresh(var Message: TMessage);
var
  Line: string;
  Count, I: Cardinal;
begin
  if (Nemu64_API.CombineModes <> nil) and (Nemu64_API.CombineModes^ <> nil) and
     (Nemu64_API.CombineModes^^.Count = 0) then
  begin
    CheckListBox.Clear;
    Caption := 'New game started';
    Exit;
  end;
  if not Visible then Exit;
  if Nemu64_API.CombineModes = nil then
  begin
    Caption := 'Not using a graphics plugin that supports the combine interface';
    Exit;
  end;
  if Nemu64_API.CombineModes^ = nil then
  begin
    Caption := 'Not using a graphics plugin that supports the combine interface';
    Exit;
  end;
  Count := CheckListBox.Items.Count;
  Caption := IntToStr(Int64(Nemu64_API.CombineModes^^.Count)) + ' combiners used since the game was started';
  I := 0;
  Inc(FRefreshCount);
  if FRefreshCount > 5 then FRefreshCount := 0;
  while I < Nemu64_API.CombineModes^^.Count do
  begin
    CombineHi := Nemu64_API.CombineModes^^.Modes[I].Hi;
    CombineLo := Nemu64_API.CombineModes^^.Modes[I].Lo;
    if (ColorA0 = ColorA1) and (ColorB0 = ColorB1) and
       (ColorC0 = ColorC1) and (ColorD0 = ColorD1) and
       (AlphaA0 = AlphaA1) and (AlphaB0 = AlphaB1) and
       (AlphaC0 = AlphaC1) and (AlphaD0 = AlphaD1) then
      Line := IntToStr(Int64(I)) + ': (' + ColorA0Str + ', ' + ColorB0Str + ', ' + ColorC0Str + ', ' + ColorD0Str + ', ' + AlphaA0Str + ', ' + AlphaB0Str + ', ' + AlphaC0Str + ', ' + AlphaD0Str
    else
      Line := IntToStr(Int64(I)) + ': (' + ColorA0Str + ', ' + ColorB0Str + ', ' + ColorC0Str + ', ' + ColorD0Str + ', ' + AlphaA0Str + ', ' + AlphaB0Str + ', ' + AlphaC0Str + ', ' + AlphaD0Str + ', ' + ColorA1Str + ', ' + ColorB1Str + ', ' + ColorC1Str + ', ' + ColorD1Str + ', ' + AlphaA1Str + ', ' + AlphaB1Str + ', ' + AlphaC1Str + ', ' + AlphaD1Str;
    if I >= Count then
    begin
      CheckListBox.Items.Add(Line);
      Inc(Count);
    end
    else
    begin
      CheckListBox.Checked[I] := Nemu64_API.CombineModes^^.Modes[I].Highlight;
      if Nemu64_API.CombineModes^^.Modes[I].Age > 5 then
      begin
        if CheckListBox.Items[I][1] <> '/' then CheckListBox.Items[I] := '//' + Line;
      end
      else if CheckListBox.Items[I][1] = '/' then CheckListBox.Items[I] := Line;
    end;
    Inc(I);
  end;
end;

procedure TFormCombiners.CheckListBoxClickCheck(Sender: TObject);
begin
  if CheckListBox.ItemIndex <> -1 then
    Nemu64_API.CombineModes^^.Modes[CheckListBox.ItemIndex].Highlight := CheckListBox.Checked[CheckListBox.ItemIndex];
end;

procedure TFormCombiners.FormCreate(Sender: TObject);
begin
  CombinerFormHandle := Handle;
end;

procedure TFormCombiners.CheckListBoxDrawItem(Control: TWinControl; Index: Integer; Rect: TRect; State: TOwnerDrawState);
var
  Box: TCheckListBox;
begin
  Box := Control as TCheckListBox;
  if not (odSelected in State) then
    case Nemu64_API.CombineModes^^.Modes[Index].EmulationMode of
      0: Box.Canvas.Brush.Color := $FFFFFF;
      1: Box.Canvas.Brush.Color := $FFFF;
      2: Box.Canvas.Brush.Color := $8080FF;
      3: Box.Canvas.Brush.Color := $FF00;
    end;
  Box.Canvas.TextRect(Rect, Rect.Left + 2, Rect.Top, Box.Items[Index]);
end;

procedure TFormCombiners.ButtonEditClick(Sender: TObject);
var
  Line: string;
begin
  if CheckListBox.ItemIndex <> -1 then
  begin
    Line := CheckListBox.Items[CheckListBox.ItemIndex];
    Delete(Line, 1, Pos(' ', Line));
    FormEditor.LoadScript(Nemu64_API.CombineModes^^.Modes[CheckListBox.ItemIndex].Lo,
      Nemu64_API.CombineModes^^.Modes[CheckListBox.ItemIndex].Hi,
      Nemu64_API.CombineModes^^.Modes[CheckListBox.ItemIndex].Extras);
    FormEditor.Show;
    FormEditor.EditAlpha.Text := EditAlpha.Text;
    FormEditor.EditColor.Text := EditColor.Text;
    FormEditor.EditExtras.Text := EditExtras.Text;
  end;
end;

procedure TFormCombiners.FormResize(Sender: TObject);
begin
  EditColor.Width := Width - 138;
  EditAlpha.Width := Width - 138;
  EditExtras.Width := Width - 138;
end;

procedure TFormCombiners.CheckListBoxClick(Sender: TObject);
var
  Color0, Color1, Alpha0, Alpha1: string;
begin
  CombineLo := Nemu64_API.CombineModes^^.Modes[CheckListBox.ItemIndex].Lo;
  CombineHi := Nemu64_API.CombineModes^^.Modes[CheckListBox.ItemIndex].Hi;
  if ColorA0Str <> '0' then
  begin
    if ColorB0Str <> '0' then Color0 := '(' + ColorA0Str + ' - ' + ColorB0Str + ')'
    else Color0 := ColorA0Str;
  end
  else
  begin
    if ColorB0Str <> '0' then Color0 := '(0 - ' + ColorB0Str
    else Color0 := '';
  end;
  if ColorC0Str <> '0' then
  begin
    if Color0 <> '' then
    begin
      if ColorC0Str <> '1' then Color0 := Color0 + ' * ' + ColorC0Str
      else Color0 := Color0;
    end
    else Color0 := ColorC0Str;
  end
  else Color0 := '';
  if ColorD0Str <> '0' then
  begin
    if Color0 <> '' then Color0 := Color0 + ' + ' + ColorD0Str
    else Color0 := ColorD0Str;
  end;
  if AlphaA0Str <> '0' then
  begin
    if AlphaB0Str <> '0' then Alpha0 := '(' + AlphaA0Str + ' - ' + AlphaB0Str + ')'
    else Alpha0 := AlphaA0Str;
  end
  else
  begin
    if AlphaB0Str <> '0' then Alpha0 := '(0 - ' + AlphaB0Str
    else Alpha0 := '';
  end;
  if AlphaC0Str <> '0' then
  begin
    if Alpha0 <> '' then
    begin
      if AlphaC0Str <> '1' then Alpha0 := Alpha0 + ' * ' + AlphaC0Str
      else Alpha0 := Alpha0;
    end
    else Alpha0 := AlphaC0Str;
  end
  else Alpha0 := '';
  if AlphaD0Str <> '0' then
  begin
    if Alpha0 <> '' then Alpha0 := Alpha0 + ' + ' + AlphaD0Str
    else Alpha0 := AlphaD0Str;
  end;
  if ColorA1Str <> '0' then
  begin
    if ColorB1Str <> '0' then
    begin
      if ColorA1Str = 'CombinedColor' then Color1 := '[' + Color0 + ' - ' + ColorB1Str + ']'
      else if ColorB1Str = 'CombinedColor' then Color1 := '[' + ColorA1Str + ' - [' + Color0 + ']]'
      else Color1 := '(' + ColorA1Str + ' - ' + ColorB1Str + ')';
    end
    else
    begin
      if ColorA1Str = 'CombinedColor' then Color1 := '[' + Color0 + ']'
      else Color1 := ColorA1Str;
    end;
  end
  else
  begin
    if ColorB1Str <> '0' then
    begin
      if ColorB1Str = 'CombinedColor' then Color1 := '(0 - [' + Color0 + '])'
      else Color1 := '(0 - ' + ColorB1Str + ')';
    end
    else Color1 := '';
  end;
  if ColorC1Str <> '0' then
  begin
    if Color1 <> '' then
    begin
      if ColorC1Str <> '1' then
      begin
        if ColorC1Str = 'CombinedColor' then Color1 := Color1 + ' * [' + Color0 + ']'
        else if ColorC1Str = 'CombinedAlpha' then Color1 := Color1 + ' * [' + Alpha0 + ']'
        else Color1 := Color1 + ' * ' + ColorC1Str;
      end
      else Color1 := Color1;
    end
    else
    begin
      if ColorC1Str = 'CombinedColor' then Color1 := Color0
      else if ColorC1Str = 'CombinedAlpha' then Color1 := Alpha0
      else Color1 := ColorC1Str;
    end;
  end
  else Color1 := '';
  if ColorD1Str <> '0' then
  begin
    if Color1 <> '' then
    begin
      if ColorD1Str = 'CombinedColor' then Color1 := Color1 + ' + ' + Color0
      else Color1 := Color1 + ' + ' + ColorD1Str;
    end
    else
    begin
      if ColorD1Str = 'CombinedColor' then Color1 := Color0
      else Color1 := ColorD1Str;
    end;
  end;
  if AlphaA1Str <> '0' then
  begin
    if AlphaB1Str <> '0' then
    begin
      if AlphaA1Str = 'CombinedAlpha' then Alpha1 := '[' + Alpha0 + ' - ' + AlphaB1Str + ']'
      else if AlphaB1Str = 'CombinedAlpha' then Alpha1 := '[' + AlphaA1Str + ' - [' + Alpha0 + ']]'
      else Alpha1 := '(' + AlphaA1Str + ' - ' + AlphaB1Str + ')';
    end
    else
    begin
      if AlphaA1Str = 'CombinedAlpha' then Alpha1 := '[' + Alpha0 + ']'
      else Alpha1 := AlphaA1Str;
    end;
  end
  else
  begin
    if AlphaB1Str <> '0' then
    begin
      if AlphaB1Str = 'CombinedAlpha' then Alpha1 := '(0 - [' + Alpha0 + '])'
      else Alpha1 := '(0 - ' + AlphaB1Str + ')';
    end
    else Alpha1 := '';
  end;
  if AlphaC1Str <> '0' then
  begin
    if Alpha1 <> '' then
    begin
      if AlphaC1Str <> '1' then
      begin
        if AlphaC1Str = 'CombinedAlpha' then Alpha1 := Alpha1 + ' * [' + Alpha0 + ']'
        else Alpha1 := Alpha1 + ' * ' + AlphaC1Str;
      end
      else Alpha1 := Alpha1;
    end
    else
    begin
      if AlphaC1Str = 'CombinedAlpha' then Alpha1 := Alpha0
      else Alpha1 := AlphaC1Str;
    end;
  end
  else Alpha1 := '';
  if AlphaD1Str <> '0' then
  begin
    if Alpha1 <> '' then
    begin
      if AlphaD1Str = 'CombinedAlpha' then Alpha1 := Alpha1 + ' + ' + Alpha0
      else Alpha1 := Alpha1 + ' + ' + AlphaD1Str;
    end
    else
    begin
      if AlphaD1Str = 'CombinedAlpha' then Alpha1 := Alpha0
      else Alpha1 := AlphaD1Str;
    end;
  end;
  EditAlpha.Text := Alpha1;
  EditColor.Text := Color1;
  ButtonDelete.Enabled := Nemu64_API.CombineModes^^.Modes[CheckListBox.ItemIndex].EmulationMode = 1;
  EditExtras.Text := '';
  if Nemu64_API.CombineModes^^.Modes[CheckListBox.ItemIndex].Extras and 1 <> 0 then
    EditExtras.Text := EditExtras.Text + 'Transparency disabled';
end;

procedure TFormCombiners.ButtonDeleteClick(Sender: TObject);
var
  Line, Key: string;
begin
  if MessageBox(Handle, 'Really delete this custom combiner mode?', 'Delete combine mode', MB_YESNO or MB_ICONQUESTION) = IDNO then Exit;
  CombineLo := Nemu64_API.CombineModes^^.Modes[CheckListBox.ItemIndex].Lo;
  CombineHi := Nemu64_API.CombineModes^^.Modes[CheckListBox.ItemIndex].Hi;
  ButtonDelete.Enabled := False;
  try
    Reset(CombineFile);
    try
      Rewrite(NewFile);
    except
      MessageBox(Nemu64_API.MainWindow^, 'Error writing combiner data file', 'Erorr', MB_ICONERROR);
      Exit;
    end;
  except
    try
      Rewrite(NewFile);
    except
      MessageBox(Nemu64_API.MainWindow^, 'Error writing combiner data file', 'Erorr', MB_ICONERROR);
      Exit;
    end;
  end;
  while not Eof(CombineFile) do
  begin
    ReadLn(CombineFile, Line);
    if (Length(Line) > 0) and (Line[1] = '[') then
    begin
      Key := Line;
      Delete(Key, 1, 1);
      SetLength(Key, Pos(']', Key) - 1);
      if (Copy(Key, 1, 6) = IntToHex(Int64(CombineHi), 6)) and
         (Copy(Key, 7, 8) = IntToHex(Int64(CombineLo), 8)) then
      begin
        repeat
          ReadLn(CombineFile, Line);
        until Eof(CombineFile) or ((Length(Line) > 0) and (Line[1] = '['));
        if not Eof(CombineFile) then WriteLn(NewFile, Line);
      end
      else WriteLn(NewFile, Line);
    end
    else WriteLn(NewFile, Line);
  end;
  CloseFile(NewFile);
  CloseFile(CombineFile);
  DeleteFile(CombineFilename);
  RenameFile(NewCombineFilename, CombineFilename);
  Nemu64_API.CombineModes^^.Modes[CheckListBox.ItemIndex].EmulationMode := 2;
  Nemu64_API.CombineModes^^.ReloadDatFile := -1;
end;



end.
