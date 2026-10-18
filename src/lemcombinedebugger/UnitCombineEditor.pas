unit UnitCombineEditor;

interface

uses Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ExtCtrls;

type
  TFormEditor = class(TForm)
    ListBoxCommands: TListBox;
    PanelTop: TPanel;
    GroupBoxEditor: TGroupBox;
    ButtonNewCommand: TButton;
    ComboBoxCommand: TComboBox;
    Panel1: TPanel;
    NotebookCommand: TNotebook;
    LabelOperation: TLabel;
    ComboBoxOperation: TComboBox;
    LabelSource1: TLabel;
    ComboBoxSource1: TComboBox;
    LabelSource2: TLabel;
    ComboBoxSource2: TComboBox;
    LabelSource3: TLabel;
    ComboBoxSource3: TComboBox;
    ButtonDeleteCommand: TButton;
    ButtonUp: TButton;
    ButtonDown: TButton;
    RadioGroupTFactorColor: TRadioGroup;
    RadioGroupTFactorAlpha: TRadioGroup;
    RadioGroupTextureSource: TRadioGroup;
    ComboBoxTextureStage: TComboBox;
    LabelAssignTextureStage: TLabel;
    LabelSrcBlend: TLabel;
    LabelDstBlend: TLabel;
    ComboBoxSrcBlend: TComboBox;
    ComboBoxDstBlend: TComboBox;
    ComboBoxPrerenderTexture: TComboBox;
    LabelSource: TLabel;
    ComboBoxPrerenderer: TComboBox;
    LabelPrerenderer: TLabel;
    RadioGroupPrerenderArg1: TRadioGroup;
    RadioGroupPrerenderArg2: TRadioGroup;
    LabelTempRegOpSource1: TLabel;
    ComboBoxTempRegOpSource1: TComboBox;
    LabelTempRegOpTarget: TLabel;
    ComboBoxTempRegOpTarget: TComboBox;
    LabelTempRegOpOperation: TLabel;
    ComboBoxTempRegOpOperation: TComboBox;
    LabelTempRegOpSource2: TLabel;
    ComboBoxTempRegOpSource2: TComboBox;
    EditTempRegOpImm: TEdit;
    LabelTempRegOpImm: TLabel;
    ButtonSave: TButton;
    ButtonLoad: TButton;
    LabelColor: TLabel;
    EditColor: TEdit;
    EditAlpha: TEdit;
    LabelAlpha: TLabel;
    Label1: TLabel;
    EditExtras: TEdit;
    ButtonCopy: TButton;
    ButtonPaste: TButton;
    CheckBoxConditional: TCheckBox;
    PanelConditional: TPanel;
    LabelConditional: TLabel;
    LabelConditionalConstant: TLabel;
    LabelConditionalReg1: TLabel;
    LabelConditionalReg2: TLabel;
    ComboBoxConditionalReg1: TComboBox;
    ComboBoxConditionalReg2: TComboBox;
    EditConditionalConstant: TEdit;
    ComboBoxCondition: TComboBox;
    LabelRead: TLabel;
    ComboBoxQueryDevice: TComboBox;
    LabelQueryTargetReg: TLabel;
    ComboBoxQueryTargetReg: TComboBox;
    procedure ButtonNewCommandClick(Sender: TObject);
    procedure ListBoxCommandsClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure ComboBoxCommandChange(Sender: TObject);
    procedure ComboBoxOperationChange(Sender: TObject);
    procedure ButtonDeleteCommandClick(Sender: TObject);
    procedure ButtonUpClick(Sender: TObject);
    procedure ButtonDownClick(Sender: TObject);
    procedure ComboBoxTempRegOpOperationChange(Sender: TObject);
    procedure ComboBoxTempRegOpSource2Change(Sender: TObject);
    procedure ParamaterChanged(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure NotebookCommandPageChanged(Sender: TObject; PageIndex: Integer);
    procedure ButtonLoadClick(Sender: TObject);
    procedure ButtonSaveClick(Sender: TObject);
    procedure FormResize(Sender: TObject);
    procedure ButtonCopyClick(Sender: TObject);
    procedure ButtonPasteClick(Sender: TObject);
    procedure CheckBoxConditionalClick(Sender: TObject);
    procedure ComboBoxConditionalReg2Change(Sender: TObject);
  public
    procedure LoadScript(Color, Alpha, Extras: Cardinal);
    procedure SaveScript(Color, Alpha, Extras: Cardinal);
  private
    FColor, FAlpha, FExtras: Cardinal;
    FChanging: Boolean;
    procedure Renumber;
    procedure UpdateCommand;
    procedure WMShow(var Message: TMessage); message WM_USER + 2;
  end;

var
  FormEditor: TFormEditor;
  EditorFormHandle: THandle;

  { The retained string slot is cleared by the original unit finalizer. }
  EditorText: string;
  CombineFile, NewFile: TextFile;
  CombineFilename, NewCombineFilename: string;

implementation

uses UnitDecs, Clipbrd;

{$R *.dfm}

procedure TFormEditor.LoadScript(Color, Alpha, Extras: Cardinal);
var
  Line, Key: string;
begin
  FormEditor.FColor := Color;
  FormEditor.FAlpha := Alpha;
  FormEditor.FExtras := Extras;
  try
    Reset(CombineFile);
  except
    try
      Rewrite(CombineFile);
    except
      MessageBox(Nemu64_API.MainWindow^, 'Error loading combiner data file', 'Erorr', MB_ICONERROR);
      Exit;
    end;
  end;
  ListBoxCommands.Items.Clear;
  while not Eof(CombineFile) do
  begin
    ReadLn(CombineFile, Line);
    if Length(Line) > 0 then
      if Line[1] = '[' then
      begin
        Key := Line;
        Delete(Key, 1, 1);
        SetLength(Key, Pos(']', Key) - 1);
        if (Copy(Key, 1, 6) = IntToHex(Int64(Alpha), 6)) and
           (Copy(Key, 7, 8) = IntToHex(Int64(Color), 8)) and
           (Copy(Key, 15, 8) = IntToHex(Int64(Extras), 8)) then
        repeat
          ReadLn(CombineFile, Line);
          if (Length(Line) > 0) and (Line[1] <> '[') then
            ListBoxCommands.Items.Add(Line);
        until Eof(CombineFile) or ((Length(Line) > 0) and (Line[1] = '['));
      end;
  end;
  CloseFile(CombineFile);
  if ListBoxCommands.Items.Count = 0 then
    ListBoxCommands.Items.Add('NewPass 5, 6');
end;

procedure TFormEditor.SaveScript(Color, Alpha, Extras: Cardinal);
var
  Line, Key: string;
  Found: Boolean;
  I: Integer;
begin
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
  Found := False;
  while not Eof(CombineFile) do
  begin
    ReadLn(CombineFile, Line);
    if Line <> '' then WriteLn(NewFile, Line);
    if Length(Line) > 0 then
      if Line[1] = '[' then
      begin
        Key := Line;
        Delete(Key, 1, 1);
        SetLength(Key, Pos(']', Key) - 1);
        if (Copy(Key, 1, 6) = IntToHex(Int64(Alpha), 6)) and
           (Copy(Key, 7, 8) = IntToHex(Int64(Color), 8)) and
           (Copy(Key, 15, 8) = IntToHex(Int64(Extras), 8)) then
        begin
          repeat
            ReadLn(CombineFile, Line);
          until Eof(CombineFile) or ((Length(Line) > 0) and (Line[1] = '['));
          Found := True;
          for I := 0 to ListBoxCommands.Items.Count - 1 do
            WriteLn(NewFile, ListBoxCommands.Items[I]);
          if not Eof(CombineFile) then WriteLn(NewFile, Line);
        end;
      end;
  end;
  if not Found then
  begin
    WriteLn(NewFile, '[' + IntToHex(Int64(Alpha), 6) + IntToHex(Int64(Color), 8) + IntToHex(Int64(Extras), 8) + ']');
    for I := 0 to ListBoxCommands.Items.Count - 1 do
      WriteLn(NewFile, ListBoxCommands.Items[I]);
  end;
  CloseFile(NewFile);
  CloseFile(CombineFile);
  DeleteFile(CombineFilename);
  RenameFile(NewCombineFilename, CombineFilename);
end;

procedure TFormEditor.Renumber;
var
  I, Pass, ColorStage, AlphaStage: Integer;
  Line: string;
begin
  I := 0;
  Pass := -1;
  ColorStage := -1;
  AlphaStage := -1;
  while I < ListBoxCommands.Items.Count do
  begin
    Line := ListBoxCommands.Items[I];
    if (Pos(',', Line) = 0) or (Pos(',', Line) > Pos(':', Line)) then
    begin
      if Pos('NewPass', Line) <> 0 then
      begin
        Inc(Pass);
        ColorStage := -1;
        AlphaStage := -1;
      end
      else if Pos(':', Line) <> 0 then
      begin
        Delete(Line, 1, Pos(':', Line) - 1);
        Insert(IntToStr(Pass), Line, 1);
      end;
    end
    else
    begin
      if Pos('SetColorSources', Line) <> 0 then
      begin
        Inc(ColorStage);
        Delete(Line, Pos(',', Line) + 1, Pos(':', Line) - Pos(',', Line) - 1);
        Insert(IntToStr(ColorStage), Line, Pos(',', Line) + 1);
      end;
      if Pos('SetAlphaSources', Line) <> 0 then
      begin
        Inc(AlphaStage);
        Delete(Line, Pos(',', Line) + 1, Pos(':', Line) - Pos(',', Line) - 1);
        Insert(IntToStr(AlphaStage), Line, Pos(',', Line) + 1);
      end;
      Delete(Line, 1, Pos(',', Line) - 1);
      Insert(IntToStr(Pass), Line, 1);
    end;
    if Line <> ListBoxCommands.Items[I] then ListBoxCommands.Items[I] := Line;
    Inc(I);
  end;
end;

function ItemValue(Text: string): string;
begin
  Result := Copy(Text, 1, Pos(': ', Text) - 1);
end;

procedure TFormEditor.UpdateCommand;
var
  Conditional: string;
begin
  if not FChanging and (ListBoxCommands.ItemIndex <> -1) then
  begin
    case ComboBoxCommand.ItemIndex of
      0:
      begin
        if CheckBoxConditional.Checked then
          Conditional := ', ' + IntToHex(ComboBoxCondition.ItemIndex, 0) + ', ' +
            IntToHex(ComboBoxConditionalReg1.ItemIndex, 0) + ', ' +
            IntToHex(ComboBoxConditionalReg2.ItemIndex, 0) + ', ' + EditConditionalConstant.Text
        else Conditional := '';
        ListBoxCommands.Items[ListBoxCommands.ItemIndex] := 'NewPass ' +
          ItemValue(ComboBoxSrcBlend.Items[ComboBoxSrcBlend.ItemIndex]) + ', ' +
          ItemValue(ComboBoxDstBlend.Items[ComboBoxDstBlend.ItemIndex]) + Conditional;
      end;
      1: ListBoxCommands.Items[ListBoxCommands.ItemIndex] := '0, 0: SetColorSources ' +
          ItemValue(ComboBoxOperation.Items[ComboBoxOperation.ItemIndex]) + ', ' +
          ItemValue(ComboBoxSource1.Items[ComboBoxSource1.ItemIndex]) + ', ' +
          ItemValue(ComboBoxSource2.Items[ComboBoxSource2.ItemIndex]) + ', ' +
          ItemValue(ComboBoxSource3.Items[ComboBoxSource3.ItemIndex]);
      2: ListBoxCommands.Items[ListBoxCommands.ItemIndex] := '0, 0: SetAlphaSources ' +
          ItemValue(ComboBoxOperation.Items[ComboBoxOperation.ItemIndex]) + ', ' +
          ItemValue(ComboBoxSource1.Items[ComboBoxSource1.ItemIndex]) + ', ' +
          ItemValue(ComboBoxSource2.Items[ComboBoxSource2.ItemIndex]) + ', ' +
          ItemValue(ComboBoxSource3.Items[ComboBoxSource3.ItemIndex]);
      3: ListBoxCommands.Items[ListBoxCommands.ItemIndex] := '0: AssignTexture ' +
          IntToHex(ComboBoxTextureStage.ItemIndex, 0) + ', ' + IntToHex(RadioGroupTextureSource.ItemIndex, 0);
      4: ListBoxCommands.Items[ListBoxCommands.ItemIndex] := '0: SetTFactor ' +
          IntToHex(RadioGroupTFactorColor.ItemIndex, 0) + ', ' + IntToHex(RadioGroupTFactorAlpha.ItemIndex, 0);
      5: ListBoxCommands.Items[ListBoxCommands.ItemIndex] := 'TempRegOperation ' +
          IntToHex(ComboBoxTempRegOpOperation.ItemIndex, 0) + ', ' +
          IntToHex(ComboBoxTempRegOpSource1.ItemIndex, 0) + ', ' +
          IntToHex(ComboBoxTempRegOpSource2.ItemIndex, 0) + ', ' + EditTempRegOpImm.Text + ', ' +
          IntToHex(ComboBoxTempRegOpTarget.ItemIndex, 0);
      6: ListBoxCommands.Items[ListBoxCommands.ItemIndex] := 'PrerenderTexture ' +
          IntToHex(ComboBoxPrerenderTexture.ItemIndex, 0) + ', ' +
          IntToHex(ComboBoxPrerenderer.ItemIndex, 0) + ', ' +
          IntToHex(RadioGroupPrerenderArg1.ItemIndex, 0) + ', ' + IntToHex(RadioGroupPrerenderArg2.ItemIndex, 0);
      7: ListBoxCommands.Items[ListBoxCommands.ItemIndex] := 'QueryDevice ' +
          IntToHex(ComboBoxQueryDevice.ItemIndex, 0) + ', ' + IntToHex(ComboBoxQueryTargetReg.ItemIndex, 0);
    end;
    Renumber;
  end;
end;

procedure TFormEditor.WMShow(var Message: TMessage);
begin
  Visible := True;
end;

procedure TFormEditor.ButtonNewCommandClick(Sender: TObject);
begin
  ListBoxCommands.Items.Add('0, 0: SetColorSources 2, 3, 0, 0');
  Renumber;
  ListBoxCommands.ItemIndex := ListBoxCommands.Items.Count - 1;
  ListBoxCommandsClick(ListBoxCommands);
end;

function ItemIndex(Text: string; Items: TStrings): Integer;
var
  I: Integer;
begin
  Result := 0;
  for I := 0 to Items.Count - 1 do
    if Text = Copy(Items[I], 1, Length(Text)) then
    begin
      Result := I;
      Break;
    end;
end;

function NextParameter(var Line: string): string;
begin
  if Pos(', ', Line) <> 0 then
  begin
    Result := Copy(Line, 1, Pos(',', Line) - 1);
    Delete(Line, 1, Pos(',', Line) + 1);
    Line := TrimLeft(Line);
  end
  else
  begin
    Result := Line;
    Line := '';
  end;
end;

procedure TFormEditor.ListBoxCommandsClick(Sender: TObject);
var
  Line, Command: string;
begin
  FChanging := True;
  Line := ListBoxCommands.Items[ListBoxCommands.ItemIndex];
  if Pos(': ', Line) <> 0 then Delete(Line, 1, Pos(': ', Line) + 1);
  if Pos(' ', Line) <> 0 then
  begin
    Command := Copy(Line, 1, Pos(' ', Line) - 1);
    Delete(Line, 1, Length(Command) + 1);
  end
  else
  begin
    Command := Line;
    Line := '';
  end;
  if Command = 'NewPass' then
  begin
    ComboBoxCommand.ItemIndex := 0;
    ComboBoxSrcBlend.ItemIndex := ItemIndex(NextParameter(Line), ComboBoxSrcBlend.Items);
    ComboBoxDstBlend.ItemIndex := ItemIndex(NextParameter(Line), ComboBoxDstBlend.Items);
    if Pos(', ', Line) <> 0 then
    begin
      CheckBoxConditional.Checked := True;
      ComboBoxCondition.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
      ComboBoxConditionalReg1.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
      ComboBoxConditionalReg2.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
      EditConditionalConstant.Text := NextParameter(Line);
      ComboBoxConditionalReg2Change(ComboBoxConditionalReg2);
    end
    else CheckBoxConditional.Checked := False;
  end;
  if Command = 'SetColorSources' then
  begin
    ComboBoxCommand.ItemIndex := 1;
    ComboBoxOperation.ItemIndex := ItemIndex(NextParameter(Line), ComboBoxOperation.Items);
    ComboBoxSource1.ItemIndex := ItemIndex(NextParameter(Line), ComboBoxSource1.Items);
    ComboBoxSource2.ItemIndex := ItemIndex(NextParameter(Line), ComboBoxSource2.Items);
    ComboBoxSource3.ItemIndex := ItemIndex(NextParameter(Line), ComboBoxSource3.Items);
    ComboBoxOperationChange(ComboBoxOperation);
  end;
  if Command = 'SetAlphaSources' then
  begin
    ComboBoxCommand.ItemIndex := 2;
    ComboBoxOperation.ItemIndex := ItemIndex(NextParameter(Line), ComboBoxOperation.Items);
    ComboBoxSource1.ItemIndex := ItemIndex(NextParameter(Line), ComboBoxSource1.Items);
    ComboBoxSource2.ItemIndex := ItemIndex(NextParameter(Line), ComboBoxSource2.Items);
    ComboBoxSource3.ItemIndex := ItemIndex(NextParameter(Line), ComboBoxSource3.Items);
    ComboBoxOperationChange(ComboBoxOperation);
  end;
  if Command = 'AssignTexture' then
  begin
    ComboBoxCommand.ItemIndex := 3;
    ComboBoxTextureStage.ItemIndex := StrToIntDef(NextParameter(Line), 0);
    RadioGroupTextureSource.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
  end;
  if Command = 'SetTFactor' then
  begin
    ComboBoxCommand.ItemIndex := 4;
    RadioGroupTFactorColor.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
    RadioGroupTFactorAlpha.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
  end;
  if Command = 'TempRegOperation' then
  begin
    ComboBoxCommand.ItemIndex := 5;
    ComboBoxTempRegOpOperation.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
    ComboBoxTempRegOpSource1.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
    ComboBoxTempRegOpSource2.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
    EditTempRegOpImm.Text := NextParameter(Line);
    ComboBoxTempRegOpTarget.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
    ComboBoxTempRegOpOperationChange(ComboBoxTempRegOpOperation);
    ComboBoxTempRegOpSource2Change(ComboBoxTempRegOpSource2);
  end;
  if Command = 'PrerenderTexture' then
  begin
    ComboBoxCommand.ItemIndex := 6;
    ComboBoxPrerenderTexture.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
    ComboBoxPrerenderer.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
    RadioGroupPrerenderArg1.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
    RadioGroupPrerenderArg2.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
  end;
  if Command = 'QueryDevice' then
  begin
    ComboBoxCommand.ItemIndex := 7;
    ComboBoxQueryDevice.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
    ComboBoxQueryTargetReg.ItemIndex := StrToIntDef('$' + NextParameter(Line), 0);
  end;
  ComboBoxCommandChange(ComboBoxCommand);
  FChanging := False;
end;

procedure TFormEditor.FormCreate(Sender: TObject);
begin
  FChanging := True;
  EditorFormHandle := Handle;
  if ComboBoxOperation.ItemIndex = -1 then ComboBoxOperation.ItemIndex := 0;
  if ComboBoxSource1.ItemIndex = -1 then ComboBoxSource1.ItemIndex := 0;
  if ComboBoxSource2.ItemIndex = -1 then ComboBoxSource2.ItemIndex := 0;
  if ComboBoxSource3.ItemIndex = -1 then ComboBoxSource3.ItemIndex := 0;
  if ComboBoxTextureStage.ItemIndex = -1 then ComboBoxTextureStage.ItemIndex := 0;
  if ComboBoxTempRegOpOperation.ItemIndex = -1 then ComboBoxTempRegOpOperation.ItemIndex := 0;
  if ComboBoxTempRegOpSource1.ItemIndex = -1 then ComboBoxTempRegOpSource1.ItemIndex := 0;
  if ComboBoxTempRegOpSource2.ItemIndex = -1 then ComboBoxTempRegOpSource2.ItemIndex := 0;
  if ComboBoxTempRegOpTarget.ItemIndex = -1 then ComboBoxTempRegOpTarget.ItemIndex := 0;
  if ComboBoxPrerenderTexture.ItemIndex = -1 then ComboBoxPrerenderTexture.ItemIndex := 0;
  if ComboBoxPrerenderer.ItemIndex = -1 then ComboBoxPrerenderer.ItemIndex := 0;

end;

procedure TFormEditor.ComboBoxCommandChange(Sender: TObject);
begin
  case ComboBoxCommand.ItemIndex of
    0: NotebookCommand.PageIndex := 0;
    1, 2: NotebookCommand.PageIndex := 1;
    3: NotebookCommand.PageIndex := 2;
    4: NotebookCommand.PageIndex := 3;
    5: NotebookCommand.PageIndex := 4;
    6: NotebookCommand.PageIndex := 5;
    7: NotebookCommand.PageIndex := 6;
  end;
  UpdateCommand;
end;

procedure TFormEditor.ComboBoxOperationChange(Sender: TObject);
var
  Source1, Source2, Source3: Boolean;
begin
  Source1 := False;
  Source2 := False;
  Source3 := False;
  case StrToIntDef(Copy(ComboBoxOperation.Text, 0, Pos(':', ComboBoxOperation.Text) - 1), 0) of
    1: ;
    2: Source1 := True;
    3: Source2 := True;
    25, 26:
      begin
        Source1 := True;
        Source2 := True;
        Source3 := True;
      end;
  else
    Source1 := True;
    Source2 := True;
  end;
  LabelSource1.Enabled := Source1;
  ComboBoxSource1.Enabled := Source1;
  LabelSource2.Enabled := Source2;
  ComboBoxSource2.Enabled := Source2;
  LabelSource3.Enabled := Source3;
  ComboBoxSource3.Enabled := Source3;
  UpdateCommand;
end;

procedure TFormEditor.ButtonDeleteCommandClick(Sender: TObject);
begin
  ListBoxCommands.Items.Delete(ListBoxCommands.ItemIndex);
  Renumber;
end;

procedure TFormEditor.ButtonUpClick(Sender: TObject);
begin
  if ListBoxCommands.ItemIndex > 0 then
  begin
    ListBoxCommands.Items.Exchange(ListBoxCommands.ItemIndex, ListBoxCommands.ItemIndex - 1);
    Renumber;
  end;
end;

procedure TFormEditor.ButtonDownClick(Sender: TObject);
begin
  if (ListBoxCommands.ItemIndex >= 0) and
    (ListBoxCommands.ItemIndex < ListBoxCommands.Items.Count - 1) then
  begin
    ListBoxCommands.Items.Exchange(ListBoxCommands.ItemIndex, ListBoxCommands.ItemIndex + 1);
    Renumber;
  end;
end;

procedure TFormEditor.ComboBoxTempRegOpOperationChange(Sender: TObject);
begin
  if ComboBoxTempRegOpOperation.ItemIndex <= 1 then
  begin
    ComboBoxTempRegOpSource1.Visible := False;
    LabelTempRegOpSource1.Visible := False;
    LabelTempRegOpSource2.Caption := 'Source Reg:';
  end
  else
  begin
    ComboBoxTempRegOpSource1.Visible := True;
    LabelTempRegOpSource1.Visible := True;
    LabelTempRegOpSource2.Caption := 'Source Reg 2:';
  end;
  UpdateCommand;
end;

procedure TFormEditor.ComboBoxTempRegOpSource2Change(Sender: TObject);
begin
  EditTempRegOpImm.Visible := ComboBoxTempRegOpSource2.ItemIndex = 0;
  LabelTempRegOpImm.Visible := ComboBoxTempRegOpSource2.ItemIndex = 0;
  UpdateCommand;
end;

procedure TFormEditor.ParamaterChanged(Sender: TObject);
begin
  UpdateCommand;
end;

procedure TFormEditor.FormShow(Sender: TObject);
begin
  if ListBoxCommands.Items.Count > 0 then
  begin
    ListBoxCommands.ItemIndex := 0;
    ListBoxCommandsClick(ListBoxCommands);
  end;
end;

procedure TFormEditor.NotebookCommandPageChanged(Sender: TObject; PageIndex: Integer);
begin
  ComboBoxTempRegOpOperationChange(ComboBoxTempRegOpOperation);
  ComboBoxTempRegOpSource2Change(ComboBoxTempRegOpSource2);
  ComboBoxOperationChange(ComboBoxOperation);
end;

procedure TFormEditor.ButtonLoadClick(Sender: TObject);
begin
  LoadScript(FColor, FAlpha, FExtras);
end;

procedure TFormEditor.ButtonSaveClick(Sender: TObject);
begin
  SaveScript(FColor, FAlpha, FExtras);
  Nemu64_API.CombineModes^^.ReloadDatFile := -1;
end;

procedure TFormEditor.FormResize(Sender: TObject);
begin
  EditColor.Width := Width - 330;
  EditAlpha.Width := Width - 330;
  EditExtras.Width := Width - 330;
end;

procedure TFormEditor.ButtonCopyClick(Sender: TObject);
begin
  Clipboard.AsText := ListBoxCommands.Items.CommaText;
end;

procedure TFormEditor.ButtonPasteClick(Sender: TObject);
begin
  if Clipboard.HasFormat(CF_TEXT) then
    ListBoxCommands.Items.CommaText := Clipboard.AsText
  else
    MessageBox(Handle, 'Clipboard does not contain text', 'Error', MB_ICONERROR);
end;

procedure TFormEditor.CheckBoxConditionalClick(Sender: TObject);
begin
  PanelConditional.Visible := CheckBoxConditional.Checked;
  UpdateCommand;
end;

procedure TFormEditor.ComboBoxConditionalReg2Change(Sender: TObject);
begin
  EditConditionalConstant.Visible := ComboBoxConditionalReg2.ItemIndex = 0;
  LabelConditionalConstant.Visible := ComboBoxConditionalReg2.ItemIndex = 0;
  UpdateCommand;
end;


end.
