unit UnitSelectInputDevices;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls, Buttons, CheckLst;

type
  TFormSelectInputDevices = class(TForm)
    ButtonOK: TButton;
    ButtonCancel: TButton;
    GroupBoxTop: TGroupBox;
    LabelTitle: TLabel;
    LabelMsg: TLabel;
    ImageStick: TImage;
    BevelBottom: TBevel;
    LabelName: TLabel;
    CheckListBox: TCheckListBox;
    GroupBox1: TGroupBox;
    LabelDeadzone: TLabel;
    EditDeadzone: TEdit;
    UpDownDeadzone: TUpDown;
    CheckBoxForceFeedback: TCheckBox;
    procedure ButtonOKClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure CheckListBoxClick(Sender: TObject);
    procedure UpDownDeadzoneClick(Sender: TObject; Button: TUDBtnType);
    procedure CheckBoxForceFeedbackClick(Sender: TObject);
  private
    DeviceSettings: array of record
      ForceFeedback: Boolean;
      DeadZone: Cardinal;
    end;
  end;

var
  FormSelectInputDevices: TFormSelectInputDevices;

implementation

uses Registry, UnitDIStuff, UnitSettings, DInput;

{$R *.dfm}

procedure TFormSelectInputDevices.ButtonOKClick(Sender: TObject);
var
  Reg: TRegistry;
  Value: string;
  I: Integer;
begin
  Reg := TRegistry.Create;
  Reg.RootKey := HKEY_CURRENT_USER;
  Reg.DeleteKey('\Software\Nemu\0.8\Input.5\Use');
  Reg.DeleteKey('\Software\Nemu\0.8\Input.5\Use');
  Reg.OpenKey('\Software\Nemu\0.8\Input.5\Use', True);
  for I := 0 to CheckListBox.Items.Count - 1 do
    if CheckListBox.Checked[I] then
    begin
      if DeviceSettings[I].ForceFeedback then
        Value := 'f' + IntToStr(DeviceSettings[I].DeadZone)
      else
        Value := ' ' + IntToStr(DeviceSettings[I].DeadZone);
      Reg.WriteString(CheckListBox.Items[I], Value);
    end;
  Reg.Free;
  CurrentlyConfiguring := True;
  FreeDirectInput;
  InitDirectInput;
  CurrentlyConfiguring := False;
  FillButtonList(FormSettings.ComboBoxButton.Items);
  FillAxisList(FormSettings.ComboBoxAxis.Items);
  FillButtonList(FormSettings.ComboBoxAxisButton1.Items);
  FillButtonList(FormSettings.ComboBoxAxisButton2.Items);
  FillButtonList(FormSettings.ComboBoxModifier1.Items);
  FillButtonList(FormSettings.ComboBoxModifier2.Items);
  FillButtonList(FormSettings.ComboBoxModifier3.Items);
  FillDeviceList(FormSettings.ComboBoxDDevice.Items);
  FormSettings.TabControlController.TabIndex := 0;
  FormSettings.TabControlControllerChange(FormSettings.TabControlController);
end;

procedure TFormSelectInputDevices.FormCreate(Sender: TObject);
var
  Reg: TRegistry;
  Value: string;
  I: Integer;
begin
  Reg := TRegistry.Create;
  Reg.RootKey := HKEY_CURRENT_USER;
  Reg.OpenKey('\Software\Nemu\0.8\Input.5\Use', True);
  SetLength(DeviceSettings, DeviceCount);
  for I := 0 to DeviceCount - 1 do
  begin
    CheckListBox.Items.Add(DIDevices[I].Name);
    if Reg.ValueExists(DIDevices[I].Name) then
    begin
      CheckListBox.Checked[CheckListBox.Count - 1] := True;
      Value := Reg.ReadString(DIDevices[I].Name);
      DeviceSettings[CheckListBox.Count - 1].ForceFeedback := Pos('f', Value) <> 0;
      DeviceSettings[CheckListBox.Count - 1].DeadZone := StrToIntDef(Copy(Value, 2, 10), 0);
    end
    else
      CheckListBox.Checked[CheckListBox.Count - 1] := False;
  end;
  Reg.Free;
  CheckListBox.ItemIndex := 0;
  CheckListBox.OnClick(CheckListBox);
end;

procedure TFormSelectInputDevices.CheckListBoxClick(Sender: TObject);
begin
  CheckBoxForceFeedback.Checked := DeviceSettings[CheckListBox.ItemIndex].ForceFeedback;
  UpDownDeadzone.Position := DeviceSettings[CheckListBox.ItemIndex].DeadZone;
  CheckBoxForceFeedback.Enabled := (DIDevices[CheckListBox.ItemIndex].Caps.dwFlags and DIDC_FORCEFEEDBACK) <> 0;
end;

procedure TFormSelectInputDevices.UpDownDeadzoneClick(Sender: TObject; Button: TUDBtnType);
begin
  DeviceSettings[CheckListBox.ItemIndex].DeadZone := UpDownDeadzone.Position;
end;

procedure TFormSelectInputDevices.CheckBoxForceFeedbackClick(Sender: TObject);
begin
  DeviceSettings[CheckListBox.ItemIndex].ForceFeedback := CheckBoxForceFeedback.Checked;
end;

initialization
finalization
end.
