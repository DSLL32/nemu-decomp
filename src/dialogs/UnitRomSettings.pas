unit UnitRomSettings;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls;

type
  TFormRomSettings = class(TForm)
    ButtonOK: TButton;
    ButtonCancel: TButton;
    ButtonApply: TButton;
    GroupBoxDefault: TGroupBox;
    MemoDefault: TMemo;
    GroupBoxName: TGroupBox;
    MemoName: TMemo;
    GroupBoxCRC: TGroupBox;
    MemoCRC: TMemo;
    PanelUnusedCRC: TPanel;
    PanelUnusedName: TPanel;
    PanelUnusedDefault: TPanel;
    GroupBoxTop: TGroupBox;
    LabelTitle: TLabel;
    LabelMsg: TLabel;
    ImageMario: TImage;
    procedure ButtonOKClick(Sender: TObject);
    procedure ButtonCancelClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure FormShow(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure ButtonApplyClick(Sender: TObject);
    procedure MemoDefaultChange(Sender: TObject);
    procedure PanelUnusedDefaultClick(Sender: TObject);
    procedure PanelUnusedNameClick(Sender: TObject);
    procedure PanelUnusedCRCClick(Sender: TObject);
    procedure MemoNameChange(Sender: TObject);
    procedure MemoCRCChange(Sender: TObject);
  end;

var
  FormRomSettings: TFormRomSettings;

implementation

uses UnitMain, UnitDecs, UnitNemuSettings, UnitHeader, IniFiles;

{$R *.dfm}

procedure TFormRomSettings.ButtonOKClick(Sender: TObject);
begin
  ButtonApply.Click;
  Close;
end;

procedure TFormRomSettings.ButtonCancelClick(Sender: TObject);
begin
  Close;
end;

procedure TFormRomSettings.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Action := caFree;
end;

procedure TFormRomSettings.FormShow(Sender: TObject);
begin
  MemoName.SetFocus;
end;

{ Helper names are inferred; their bodies are reconstructed from the DLL. }
procedure ReadSection(Section: string; Ini: TIniFile; Lines: TStrings);
var
  Keys: TStringList;
  I: Integer;
begin
  Keys := TStringList.Create;
  Ini.ReadSection(Section, Keys);
  for I := 0 to Keys.Count - 1 do
    Lines.Add(Keys[I] + '=' + Ini.ReadString(Section, Keys[I], ''));
  Keys.Free;
end;

procedure TFormRomSettings.FormCreate(Sender: TObject);
var
  Ini: TIniFile;
begin
  GroupBoxName.Caption := 'Settings for ''' + string(Nemu64_API.RomName) +
    ''' - this can include other country codes';
  GroupBoxCRC.Caption := 'Settings for this specific rom (CRC is ' +
    string(Nemu64_API.RomCRC) + ')';
  Ini := TIniFile.Create(NemuFileDir + 'Nemu64.ini');
  ReadSection('Default', Ini, MemoDefault.Lines);
  ReadSection(Nemu64_API.RomName, Ini, MemoName.Lines);
  ReadSection(Nemu64_API.RomCRC, Ini, MemoCRC.Lines);
  MemoDefaultChange(MemoDefault);
  MemoNameChange(MemoName);
  MemoCRCChange(MemoCRC);
  Ini.Free;
end;

procedure WriteSection(Section: string; Ini: TIniFile; Lines: TStrings);
var
  I: Integer;
  Line, Key: string;
begin
  if Ini.SectionExists(Section) then
    Ini.EraseSection(Section);
  if Lines.Count <> 0 then
    for I := 0 to Lines.Count - 1 do
    begin
      Line := Lines[I];
      if Pos('=', Line) <> 0 then
      begin
        Key := Copy(Line, 1, Pos('=', Line) - 1);
        Delete(Line, 1, Pos('=', Line));
        Ini.WriteString(Section, Key, Line);
      end;
    end;
end;

procedure TFormRomSettings.ButtonApplyClick(Sender: TObject);
var
  Ini: TIniFile;
begin
  Ini := TIniFile.Create(NemuFileDir + 'Nemu64.ini');
  WriteSection('Default', Ini, MemoDefault.Lines);
  WriteSection(Nemu64_API.RomName, Ini, MemoName.Lines);
  WriteSection(Nemu64_API.RomCRC, Ini, MemoCRC.Lines);
  Ini.Free;
end;

procedure TFormRomSettings.MemoDefaultChange(Sender: TObject);
begin
  if MemoDefault.Lines.Count = 0 then
  begin
    MemoDefault.Color := clBtnFace;
    PanelUnusedDefault.Visible := True;
  end
  else
  begin
    MemoDefault.Color := clWindow;
    PanelUnusedDefault.Visible := False;
  end;
end;

procedure TFormRomSettings.PanelUnusedDefaultClick(Sender: TObject);
begin
  MemoDefault.SetFocus;
end;

procedure TFormRomSettings.PanelUnusedNameClick(Sender: TObject);
begin
  MemoName.SetFocus;
end;

procedure TFormRomSettings.PanelUnusedCRCClick(Sender: TObject);
begin
  MemoCRC.SetFocus;
end;

procedure TFormRomSettings.MemoNameChange(Sender: TObject);
begin
  if MemoName.Lines.Count = 0 then
  begin
    MemoName.Color := clBtnFace;
    PanelUnusedName.Visible := True;
  end
  else
  begin
    MemoName.Color := clWindow;
    PanelUnusedName.Visible := False;
  end;
end;

procedure TFormRomSettings.MemoCRCChange(Sender: TObject);
begin
  if MemoCRC.Lines.Count = 0 then
  begin
    MemoCRC.Color := clBtnFace;
    PanelUnusedCRC.Visible := True;
  end
  else
  begin
    MemoCRC.Color := clWindow;
    PanelUnusedCRC.Visible := False;
  end;
end;

end.
