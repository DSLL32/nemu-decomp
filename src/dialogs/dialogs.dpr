library dialogs;

uses
  Windows, SysUtils, Classes, Forms, IniFiles,
  UnitRomSettings in 'UnitRomSettings.pas',
  UnitMain in 'UnitMain.pas',
  UnitDecs in 'UnitDecs.pas',
  UnitNemuSettings in 'UnitNemuSettings.pas',
  UnitHeader in 'UnitHeader.pas';

{$R XPManifest.res}

type
  PPlugin = ^TPlugin;
  TPlugin = record
    Size: Cardinal;
    Version: Word;
    PluginType: Word;
    Name: array[0..99] of Char;
    NormalMemory: LongBool;
    MemoryBswapped: LongBool;
    Filename: array[0..MAX_PATH - 1] of Char;
  end;

function ToolInit(Info: PNemu): Integer; cdecl;
begin
  Result := 1;
  Move(Info^, Nemu, SizeOf(Nemu));
  NemuInstance := Nemu.Instance;
  GetDir(0, NemuPath);
  if NemuPath[Length(NemuPath)] <> '\' then
    NemuPath := NemuPath + '\';
end;

procedure ToolFree; cdecl;
begin
end;

procedure ToolFunc(Reason, Index: Integer); cdecl;
begin
  case Reason of
    1:
      case Index of
        0:
          begin
            FormHeader := TFormHeader.Create(nil);
            FormHeader.ParentWindow := Nemu.MainWindow^;
            FormHeader.ShowModal;
          end;
        1:
          if Length(string(Nemu.RomName)) > 0 then
          begin
            FormRomSettings := TFormRomSettings.Create(nil);
            FormRomSettings.ParentWindow := Nemu.MainWindow^;
            FormRomSettings.ShowModal;
          end
          else
            MessageBox(Nemu.MainWindow^, 'Please select a rom', 'No rom selected', MB_ICONERROR);
      end;
  end;
end;

procedure ToolGetMenuText(Text: PChar); cdecl;
begin
  StrCopy(Text, 'Show Rom Info...'#13'Rom settings...');
end;

procedure NemuSettings(Count: Integer; Plugins: PPlugin); cdecl;
var
  Running: Boolean;
  Ini: TIniFile;
  Name, Selected: string;
  Plugin: PPlugin;
  I: Integer;
begin
  Running := Nemu.Running^;
  FormNemuSettings := TFormNemuSettings.Create(nil);
  FormNemuSettings.ParentWindow := Nemu.MainWindow^;
  FormNemuSettings.ComboBoxRDRAMSize.Enabled := not Running;
  FormNemuSettings.LabelRDRAMSizeRunning.Visible := Running;
  Ini := TIniFile.Create(NemuPath + 'config.ini');
  case Ini.ReadInteger('Configuration', 'RDRAMSize', 8) of
    4: FormNemuSettings.ComboBoxRDRAMSize.ItemIndex := 0;
    8: FormNemuSettings.ComboBoxRDRAMSize.ItemIndex := 1;
  else
    FormNemuSettings.ComboBoxRDRAMSize.ItemIndex := 1;
  end;
  FormNemuSettings.EditMempack.Text := Ini.ReadString('Configuration', 'MEMPACK_EEPROM_PATH', '');
  FormNemuSettings.EditEEProm.Text := Ini.ReadString('Configuration', 'CART_EEPROM_PATH', '');
  FormNemuSettings.EditSRAM.Text := Ini.ReadString('Configuration', 'SRAM_PATH', '');
  FormNemuSettings.EditSavestate.Text := Ini.ReadString('Configuration', 'SAVESTATE_PATH', '');
  if (FormNemuSettings.EditMempack.Text = FormNemuSettings.EditEEProm.Text) and
     (FormNemuSettings.EditMempack.Text = FormNemuSettings.EditSRAM.Text) and
     (FormNemuSettings.EditMempack.Text = FormNemuSettings.EditSavestate.Text) then
    FormNemuSettings.CheckBoxUseOneFolder.Checked := True;
  FormNemuSettings.LabelEmulationRunning.Visible := Running;
  FormNemuSettings.ComboBoxPlugins.Items.Clear;
  Selected := Ini.ReadString('Configuration', 'GFXDLL', '');
  Plugin := Plugins;
  if Count <> 0 then
    for I := 0 to Count - 1 do
    begin
      if Plugin^.PluginType = 2 then
      begin
        Name := Trim(Plugin^.Name);
        FormNemuSettings.ComboBoxPlugins.Items.Add(Name + ' <' + Trim(Plugin^.Filename) + '>');
        if Selected <> '' then
          if UpperCase(Trim(Selected)) = UpperCase(Trim(Plugin^.Filename)) then
            FormNemuSettings.ComboBoxPlugins.ItemIndex := FormNemuSettings.ComboBoxPlugins.Items.Count - 1;
      end;
      Plugin := Pointer(Cardinal(Plugin) + Plugin^.Size);
    end;
  if FormNemuSettings.ComboBoxPlugins.ItemIndex = -1 then
    if FormNemuSettings.ComboBoxPlugins.Items.Count <> 0 then
      FormNemuSettings.ComboBoxPlugins.ItemIndex := 0;
  FormNemuSettings.ShowModal;
  Ini.Free;
end;

exports
  NemuSettings index 1,
  ToolFunc index 2,
  ToolGetMenuText index 3,
  ToolFree index 4,
  ToolInit index 5;

begin
end.
