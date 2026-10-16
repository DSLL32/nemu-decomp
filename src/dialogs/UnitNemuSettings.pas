unit UnitNemuSettings;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ThemeMgr, Verlauf, ComCtrls;

type
  TFormNemuSettings = class(TForm)
    PageControl: TPageControl;
    TabSheetGeneral: TTabSheet;
    GroupBoxRDRAM: TGroupBox;
    LabelRDRAMSize: TLabel;
    ComboBoxRDRAMSize: TComboBox;
    TabSheetFolders: TTabSheet;
    LabelMempackFolder: TLabel;
    LabelEEPromFolder: TLabel;
    Bevel1: TBevel;
    LabelFolderDesc: TLabel;
    Bevel2: TBevel;
    LabelSRAMFolder: TLabel;
    EditMempack: TEdit;
    EditEEProm: TEdit;
    ButtonBrowseMempack: TButton;
    ButtonBrowseEEProm: TButton;
    EditSRAM: TEdit;
    ButtonBrowseSRAM: TButton;
    TabSheetShellIntegration: TTabSheet;
    GroupBoxFiletypes: TGroupBox;
    CheckBoxN: TCheckBox;
    CheckBoxU: TCheckBox;
    CheckBoxV: TCheckBox;
    CheckBoxBin: TCheckBox;
    CheckBoxZ: TCheckBox;
    CheckBoxROM: TCheckBox;
    ButtonOK: TButton;
    ButtonCancel: TButton;
    ButtonApply: TButton;
    SaveDialogBrowse: TSaveDialog;
    CheckBoxUseOneFolder: TCheckBox;
    LabelRDRAMSizeRunning: TLabel;
    Bevel3: TBevel;
    LabelSavestateFolder: TLabel;
    EditSavestate: TEdit;
    ButtonBrowseSavestate: TButton;
    GroupBoxPlugins: TGroupBox;
    Label1: TLabel;
    LabelEmulationRunning: TLabel;
    ComboBoxPlugins: TComboBox;
    ThemeManager: TThemeManager;
    PanelTop: TPanel;
    LabelTitle: TLabel;
    LabelMsg: TLabel;
    procedure ButtonOKClick(Sender: TObject);
    procedure ButtonCancelClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure CheckBoxUseOneFolderClick(Sender: TObject);
    procedure EditMempackChange(Sender: TObject);
    procedure ButtonApplyClick(Sender: TObject);
    procedure ButtonBrowseMempackClick(Sender: TObject);
    procedure ButtonBrowseEEPromClick(Sender: TObject);
    procedure ButtonBrowseSRAMClick(Sender: TObject);
    procedure ButtonBrowseSavestateClick(Sender: TObject);
  end;

var
  FormNemuSettings: TFormNemuSettings;
  NemuFileDir: string;

implementation

uses UnitMain, UnitDecs, IniFiles;

{$R *.dfm}

procedure TFormNemuSettings.ButtonOKClick(Sender: TObject);
begin
  ButtonApply.Click;
  Close;
end;

procedure TFormNemuSettings.ButtonCancelClick(Sender: TObject);
begin
  Close;
end;

procedure TFormNemuSettings.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Action := caFree;
end;

procedure TFormNemuSettings.CheckBoxUseOneFolderClick(Sender: TObject);
begin
  LabelEEPromFolder.Enabled := not CheckBoxUseOneFolder.Checked;
  LabelSRAMFolder.Enabled := not CheckBoxUseOneFolder.Checked;
  LabelSavestateFolder.Enabled := not CheckBoxUseOneFolder.Checked;
  EditEEProm.Enabled := not CheckBoxUseOneFolder.Checked;
  EditSRAM.Enabled := not CheckBoxUseOneFolder.Checked;
  EditSavestate.Enabled := not CheckBoxUseOneFolder.Checked;
  ButtonBrowseEEProm.Enabled := not CheckBoxUseOneFolder.Checked;
  ButtonBrowseSRAM.Enabled := not CheckBoxUseOneFolder.Checked;
  ButtonBrowseSavestate.Enabled := not CheckBoxUseOneFolder.Checked;
  if CheckBoxUseOneFolder.Checked then
  begin
    LabelMempackFolder.Caption := '&Save folder:';
    EditMempackChange(EditMempack);
  end
  else
    LabelMempackFolder.Caption := '&Mempack folder:';
end;

procedure TFormNemuSettings.EditMempackChange(Sender: TObject);
begin
  if CheckBoxUseOneFolder.Checked then
  begin
    EditEEProm.Text := EditMempack.Text;
    EditSRAM.Text := EditMempack.Text;
    EditSavestate.Text := EditMempack.Text;
  end;
end;

procedure TFormNemuSettings.ButtonApplyClick(Sender: TObject);
var
  Ini: TIniFile;
  Plugin: string;
begin
  Ini := TIniFile.Create(NemuFileDir + 'config.ini');
  case ComboBoxRDRAMSize.ItemIndex of
    0: Ini.WriteInteger('Configuration', 'RDRAMSize', 4);
    1: Ini.WriteInteger('Configuration', 'RDRAMSize', 8);
  end;
  Ini.WriteString('Configuration', 'MEMPACK_EEPROM_PATH', EditMempack.Text);
  Ini.WriteString('Configuration', 'CART_EEPROM_PATH', EditEEProm.Text);
  Ini.WriteString('Configuration', 'SRAM_PATH', EditSRAM.Text);
  Ini.WriteString('Configuration', 'SAVESTATE_PATH', EditSavestate.Text);
  Plugin := ComboBoxPlugins.Text;
  SetLength(Plugin, Length(Plugin) - 1);
  while Pos('<', Plugin) <> 0 do
    Delete(Plugin, 1, Pos('<', Plugin));
  Ini.WriteString('Configuration', 'GFXDLL', Plugin);
  Ini.Free;
end;

procedure TFormNemuSettings.ButtonBrowseMempackClick(Sender: TObject);
begin
  SaveDialogBrowse.InitialDir := EditMempack.Text;
  SaveDialogBrowse.Title := 'Choose Mempack folder';
  SaveDialogBrowse.Filter := 'Mempack files|*.mpk';
  SaveDialogBrowse.DefaultExt := 'mpk';
  SaveDialogBrowse.FileName := 'default.mpk';
  if SaveDialogBrowse.Execute then
    EditMempack.Text := ExtractFilePath(SaveDialogBrowse.FileName);
end;

procedure TFormNemuSettings.ButtonBrowseEEPromClick(Sender: TObject);
begin
  SaveDialogBrowse.InitialDir := EditEEProm.Text;
  SaveDialogBrowse.Title := 'Choose EEProm folder';
  SaveDialogBrowse.Filter := 'EEProm files|*.sav';
  SaveDialogBrowse.FileName := 'default.sav';
  SaveDialogBrowse.DefaultExt := 'sav';
  if SaveDialogBrowse.Execute then
    EditEEProm.Text := ExtractFilePath(SaveDialogBrowse.FileName);
end;

procedure TFormNemuSettings.ButtonBrowseSRAMClick(Sender: TObject);
begin
  SaveDialogBrowse.InitialDir := EditSRAM.Text;
  SaveDialogBrowse.Title := 'Choose SRAM folder';
  SaveDialogBrowse.Filter := 'SRAM files|*.sra';
  SaveDialogBrowse.FileName := 'default.sra';
  SaveDialogBrowse.DefaultExt := 'sra';
  if SaveDialogBrowse.Execute then
    EditSRAM.Text := ExtractFilePath(SaveDialogBrowse.FileName);
end;

procedure TFormNemuSettings.ButtonBrowseSavestateClick(Sender: TObject);
begin
  SaveDialogBrowse.InitialDir := EditSavestate.Text;
  SaveDialogBrowse.Title := 'Choose Savestate folder';
  SaveDialogBrowse.Filter := 'Nemu64 Savestate files (*.nss)|*.nss';
  SaveDialogBrowse.FileName := 'default.nss';
  SaveDialogBrowse.DefaultExt := 'nss';
  if SaveDialogBrowse.Execute then
    EditSavestate.Text := ExtractFilePath(SaveDialogBrowse.FileName);
end;

end.
