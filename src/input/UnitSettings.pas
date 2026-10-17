unit UnitSettings;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ExtCtrls, Menus, ComCtrls, UnitDecs;

type
  PButtonMapping = ^TButtonMapping;
  TButtonMapping = record
    Turbo: Boolean;
    Device: Integer;
    Button: Integer;
  end;
  PAxisMapping = ^TAxisMapping;
  TAxisMapping = record
    Analog: Boolean;
    Negative: TButtonMapping;
    Positive: TButtonMapping;
    Device: Integer;
    Axis: Cardinal;
  end;
  TControllerSettings = record
    Enabled: Boolean;
    MemPack: Boolean;
    RumblePack: Boolean;
    Buttons: array[0..5] of TButtonMapping;
    Modifiers: array[0..2] of TButtonMapping;
    Axes: array[0..1] of TAxisMapping;
    DigitalAxes: array[0..3] of TAxisMapping;
    ForceFeedbackDevice: Cardinal;
  end;
  TInputSettings = record
    DefaultSensitivity: Integer;
    Modifier1Sensitivity: Integer;
    Modifier2Sensitivity: Integer;
    Modifier3Sensitivity: Integer;
    Controllers: array[0..3] of TControllerSettings;
  end;

  TFormSettings = class(TForm)
    ButtonOK: TButton;
    ButtonSelectInput: TButton;
    GroupBoxSettings: TGroupBox;
    GroupBoxGeneral: TGroupBox;
    CheckBoxMemPack: TCheckBox;
    CheckBoxRumblePack: TCheckBox;
    CheckBoxPluggedIn: TCheckBox;
    LabelSelect: TLabel;
    GroupBoxMapping: TGroupBox;
    ListBoxButtons: TListBox;
    NotebookMap: TNotebook;
    RadioGroupAxis: TRadioGroup;
    ComboBoxButton: TComboBox;
    NotebookAxis: TNotebook;
    ComboBoxAxis: TComboBox;
    LabelButtonButton: TLabel;
    LabelAxisButton1: TLabel;
    ComboBoxAxisButton1: TComboBox;
    LabelAxisButton2: TLabel;
    ComboBoxAxisButton2: TComboBox;
    LabelAxis: TLabel;
    CheckBoxTurboFire: TCheckBox;
    LabelAxisModifier2: TLabel;
    ComboBoxModifier2: TComboBox;
    ComboBoxModifier1: TComboBox;
    LabelAxisModifier1: TLabel;
    LabelAxisModifier3: TLabel;
    ComboBoxModifier3: TComboBox;
    LabelModifierExplanation: TLabel;
    ButtonSensitivity: TButton;
    ButtonAutoConfigureAdaptoid: TButton;
    ButtonAutoconfigureKeyboard: TButton;
    PopupMenuAdaptoid: TPopupMenu;
    TabControlController: TTabControl;
    Image1: TImage;
    ButtonWaitButton: TButton;
    ButtonWaitAxisButton1: TButton;
    ButtonWaitAxisButton2: TButton;
    ButtonWaitAxisModifier1: TButton;
    ButtonWaitAxisModifier2: TButton;
    ButtonWaitAxisModifier3: TButton;
    Label1: TLabel;
    ComboBoxDDevice: TComboBox;
    procedure CheckBoxPluggedInClick(Sender: TObject);
    procedure ButtonSelectInputClick(Sender: TObject);
    procedure ListBoxButtonsDrawItem(Control: TWinControl; Index: Integer; Rect: TRect; State: TOwnerDrawState);
    procedure ListBoxButtonsClick(Sender: TObject);
    procedure RadioGroupAxisClick(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure ComboBoxAxisChange(Sender: TObject);
    procedure ComboBoxButtonChange(Sender: TObject);
    procedure ComboBoxAxisButton1Change(Sender: TObject);
    procedure ComboBoxAxisButton2Change(Sender: TObject);
    procedure CheckBoxMemPackClick(Sender: TObject);
    procedure CheckBoxRumblePackClick(Sender: TObject);
    procedure ButtonOKClick(Sender: TObject);
    procedure GroupBoxSettingsClick(Sender: TObject);
    procedure CheckBoxTurboFireClick(Sender: TObject);
    procedure ComboBoxModifier2Change(Sender: TObject);
    procedure ComboBoxModifier3Change(Sender: TObject);
    procedure ComboBoxModifier1Change(Sender: TObject);
    procedure ButtonSensitivityClick(Sender: TObject);
    procedure ButtonAutoConfigureAdaptoidClick(Sender: TObject);
    procedure ButtonAutoconfigureKeyboardClick(Sender: TObject);
    procedure MenuItemAdaptoidClick(Sender: TObject);
    procedure TabControlControllerChange(Sender: TObject);
    procedure ButtonWaitButtonClick(Sender: TObject);
    procedure ButtonWaitAxisButton2Click(Sender: TObject);
    procedure ButtonWaitAxisButton1Click(Sender: TObject);
    procedure ButtonWaitAxisModifier1Click(Sender: TObject);
    procedure ButtonWaitAxisModifier2Click(Sender: TObject);
    procedure ButtonWaitAxisModifier3Click(Sender: TObject);
    procedure ComboBoxDDeviceChange(Sender: TObject);
  private
    ButtonMapping: PButtonMapping;
    Modifier1Mapping: PButtonMapping;
    Modifier2Mapping: PButtonMapping;
    Modifier3Mapping: PButtonMapping;
    AxisMapping: PAxisMapping;
  public
    procedure UpdateLabels;
    procedure SetControllerEnabled(Enabled: Boolean);
    procedure UpdateController;
    procedure SaveSettings;
  end;

var
  FormSettings: TFormSettings;
  LastForegroundWindow: HWND;
  ControllerData: PControllerDataArray;
  InputSettings: TInputSettings;

procedure FillButtonList(Items: TStrings);
procedure FillAxisList(Items: TStrings);
procedure FillDeviceList(Items: TStrings);
function GetDeviceIndex(Name: string): Integer;
function GetKeyCode(Name: string): Integer;
function GetButtonIndex(Name: string; Device: Integer): Integer;
function GetKeyName(Code: Integer): string;
procedure AutoConfigureKeyboard(Controller: Integer);

implementation

uses UnitDIStuff, UnitSelectInputDevices, UnitSensitivity, UnitWaitKey, Registry;

{$R *.dfm}

procedure TFormSettings.UpdateLabels;
begin
  LabelModifierExplanation.Caption := 'Without modifiers, analog stick is at ' +
    IntToStr(InputSettings.DefaultSensitivity) + '% when button is pressed';
  LabelAxisModifier1.Caption := IntToStr(InputSettings.Modifier1Sensitivity) + '%:';
  LabelAxisModifier2.Caption := IntToStr(InputSettings.Modifier2Sensitivity) + '%:';
  LabelAxisModifier3.Caption := IntToStr(InputSettings.Modifier3Sensitivity) + '%:';
  FormSettings.ButtonAutoconfigureKeyboard.Enabled := HasKeyboard;
  FormSettings.ButtonAutoConfigureAdaptoid.Enabled := HasAdaptoid;
end;

procedure FillButtonList(Items: TStrings);
var
  I, J: Integer;
begin
  Items.Clear;
  Items.Add('(none)');
  for I := 0 to ActiveDeviceCount - 1 do
    case DIDevices[ActiveDevices[I].DeviceIndex].DeviceType and $FF of
      3:
        for J := 0 to 71 do
          Items.Add(DIDevices[ActiveDevices[I].DeviceIndex].Name + '.' + KeyDefs[J].Name);
      4:
        for J := 0 to DIDevices[ActiveDevices[I].DeviceIndex].Caps.dwButtons - 1 do
          Items.Add(DIDevices[ActiveDevices[I].DeviceIndex].Name + '.' + ActiveDevices[I].ButtonNames[J]);
      2:
        for J := 0 to DIDevices[ActiveDevices[I].DeviceIndex].Caps.dwButtons - 1 do
          Items.Add(DIDevices[ActiveDevices[I].DeviceIndex].Name + '.' + ActiveDevices[I].ButtonNames[J]);
    end;
end;

procedure FillAxisList(Items: TStrings);
var
  I, J: Integer;
begin
  Items.Clear;
  Items.Add('(none)');
  for I := 0 to ActiveDeviceCount - 1 do
    for J := 1 to DIDevices[ActiveDevices[I].DeviceIndex].Caps.dwAxes do
      Items.Add(DIDevices[ActiveDevices[I].DeviceIndex].Name + '.Axis ' + IntToStr(J));
end;

procedure FillDeviceList(Items: TStrings);
var
  I: Integer;
begin
  Items.Clear;
  Items.Add('(none)');
  for I := 0 to ActiveDeviceCount - 1 do
    if ActiveDevices[I].ForceFeedback then
      Items.Add(DIDevices[ActiveDevices[I].DeviceIndex].Name);
end;

function GetDeviceIndex(Name: string): Integer;
var
  I: Integer;
begin
  for I := 0 to ActiveDeviceCount - 1 do
    if DIDevices[ActiveDevices[I].DeviceIndex].Name = Name then
    begin
      Result := I;
      Exit;
    end;
  Result := 0;
end;

function GetKeyCode(Name: string): Integer;
var
  I: Integer;
begin
  for I := 0 to 71 do
    if KeyDefs[I].Name = Name then
    begin
      Result := KeyDefs[I].Code;
      Exit;
    end;
  Result := 0;
end;

function GetButtonIndex(Name: string; Device: Integer): Integer;
var
  I: Integer;
begin
  for I := 0 to ActiveDevices[Device].ButtonCount - 1 do
    if StrComp(ActiveDevices[Device].ButtonNames[I], PChar(Name)) = 0 then
    begin
      Result := I + 1;
      Exit;
    end;
  Result := 0;
end;

function GetKeyName(Code: Integer): string;
var
  I: Integer;
begin
  for I := 0 to 71 do
    if KeyDefs[I].Code = Code then
    begin
      Result := KeyDefs[I].Name;
      Exit;
    end;
  Result := '';
end;

procedure TFormSettings.SetControllerEnabled(Enabled: Boolean);
begin
  if Enabled then
  begin
    CheckBoxMemPack.Enabled := False;
    CheckBoxRumblePack.Enabled := False;
    if CheckBoxMemPack.Checked then
      CheckBoxMemPack.Enabled := True
    else
      CheckBoxRumblePack.Enabled := True;
    if CheckBoxRumblePack.Checked then
      CheckBoxRumblePack.Enabled := True
    else
      CheckBoxMemPack.Enabled := True;
  end
  else
  begin
    CheckBoxRumblePack.Enabled := False;
    CheckBoxMemPack.Enabled := False;
  end;
  GroupBoxMapping.Visible := Enabled;
end;

procedure TFormSettings.UpdateController;
begin
  CheckBoxPluggedIn.OnClick := nil;
  CheckBoxPluggedIn.Checked := InputSettings.Controllers[TabControlController.TabIndex].Enabled;
  CheckBoxPluggedIn.OnClick := CheckBoxPluggedInClick;
  if CheckBoxPluggedIn.Checked then
  begin
    CheckBoxMemPack.Enabled := True;
    CheckBoxRumblePack.Enabled := True;
    GroupBoxMapping.Visible := True;
  end
  else
  begin
    CheckBoxMemPack.Enabled := False;
    CheckBoxRumblePack.Enabled := False;
    GroupBoxMapping.Visible := False;
  end;
  CheckBoxMemPack.OnClick := nil;
  CheckBoxMemPack.Checked := InputSettings.Controllers[TabControlController.TabIndex].MemPack;
  CheckBoxMemPack.OnClick := CheckBoxMemPackClick;
  CheckBoxRumblePack.OnClick := nil;
  CheckBoxRumblePack.Checked := InputSettings.Controllers[TabControlController.TabIndex].RumblePack;
  CheckBoxRumblePack.OnClick := CheckBoxRumblePackClick;
  ListBoxButtonsClick(ListBoxButtons);
  if CheckBoxRumblePack.Checked then
    CheckBoxMemPack.Enabled := False
  else if CheckBoxMemPack.Checked then
    CheckBoxRumblePack.Enabled := False;
end;

procedure TFormSettings.CheckBoxPluggedInClick(Sender: TObject);
begin
  SetControllerEnabled(CheckBoxPluggedIn.Checked);
  InputSettings.Controllers[TabControlController.TabIndex].Enabled := (Sender as TCheckBox).Checked;
end;

procedure TFormSettings.ButtonSelectInputClick(Sender: TObject);
begin
  SaveSettings;
  FormSelectInputDevices := TFormSelectInputDevices.Create(FormSettings);
  FormSelectInputDevices.ParentWindow := FormSettings.Handle;
  if FormSelectInputDevices.ShowModal = mrOk then
    GroupBoxMapping.Visible := True;
  FormSelectInputDevices.Free;
  UpdateLabels;
end;

procedure TFormSettings.ListBoxButtonsDrawItem(Control: TWinControl;
  Index: Integer; Rect: TRect; State: TOwnerDrawState);
var
  S: string;
begin
  with Control as TListBox do
  begin
    S := Items[Index];
    Delete(S, 1, 1);
    Canvas.TextRect(Rect, Rect.Left + 2, Rect.Top, S);
  end;
end;

procedure TFormSettings.ListBoxButtonsClick(Sender: TObject);
var
  i, Count, Selection: Cardinal;
begin
  if ListBoxButtons.ItemIndex = -1 then Exit;
  case ListBoxButtons.Items[ListBoxButtons.ItemIndex][1] of
    'B':
    begin
      NotebookMap.PageIndex := 1;
      case ListBoxButtons.ItemIndex of
        0: ButtonMapping := @InputSettings.Controllers[TabControlController.TabIndex].Buttons[0];
        1: ButtonMapping := @InputSettings.Controllers[TabControlController.TabIndex].Buttons[1];
        2: ButtonMapping := @InputSettings.Controllers[TabControlController.TabIndex].Buttons[2];
        3: ButtonMapping := @InputSettings.Controllers[TabControlController.TabIndex].Buttons[3];
        4: ButtonMapping := @InputSettings.Controllers[TabControlController.TabIndex].Buttons[4];
        5: ButtonMapping := @InputSettings.Controllers[TabControlController.TabIndex].Buttons[5];
      else
        ButtonMapping := @InputSettings.Controllers[TabControlController.TabIndex].Buttons[0];
      end;
      if ButtonMapping^.Device = -1 then ComboBoxButton.ItemIndex := 0;
      case DIDevices[ActiveDevices[ButtonMapping^.Device].DeviceIndex].DeviceType and $FF of
        4:
          ComboBoxButton.ItemIndex := ComboBoxButton.Items.IndexOf(
            DIDevices[ActiveDevices[ButtonMapping^.Device].DeviceIndex].Name + '.' +
            ActiveDevices[ButtonMapping^.Device].ButtonNames[ButtonMapping^.Button - 1]);
        2:
          ComboBoxButton.ItemIndex := ComboBoxButton.Items.IndexOf(
            DIDevices[ActiveDevices[ButtonMapping^.Device].DeviceIndex].Name + '.' +
            ActiveDevices[ButtonMapping^.Device].ButtonNames[ButtonMapping^.Button - 1]);
        3:
          ComboBoxButton.ItemIndex := ComboBoxButton.Items.IndexOf(
            DIDevices[ActiveDevices[ButtonMapping^.Device].DeviceIndex].Name + '.' +
            GetKeyName(ButtonMapping^.Button));
      end;
      if ComboBoxButton.ItemIndex = -1 then ComboBoxButton.ItemIndex := 0;
      CheckBoxTurboFire.Checked := ButtonMapping^.Turbo;
    end;
    'A':
    begin
      NotebookMap.PageIndex := 0;
      Modifier1Mapping := nil;
      Modifier2Mapping := nil;
      Modifier3Mapping := nil;
      LabelModifierExplanation.Visible := False;
      case ListBoxButtons.ItemIndex of
        6:
        begin
          AxisMapping := @InputSettings.Controllers[TabControlController.TabIndex].Axes[0];
          Modifier1Mapping := @InputSettings.Controllers[TabControlController.TabIndex].Modifiers[0];
          Modifier2Mapping := @InputSettings.Controllers[TabControlController.TabIndex].Modifiers[1];
          Modifier3Mapping := @InputSettings.Controllers[TabControlController.TabIndex].Modifiers[2];
          LabelModifierExplanation.Visible := True;
        end;
        7:
        begin
          AxisMapping := @InputSettings.Controllers[TabControlController.TabIndex].Axes[1];
          Modifier1Mapping := @InputSettings.Controllers[TabControlController.TabIndex].Modifiers[0];
          Modifier2Mapping := @InputSettings.Controllers[TabControlController.TabIndex].Modifiers[1];
          Modifier3Mapping := @InputSettings.Controllers[TabControlController.TabIndex].Modifiers[2];
          LabelModifierExplanation.Visible := True;
        end;
        8: AxisMapping := @InputSettings.Controllers[TabControlController.TabIndex].DigitalAxes[0];
        9: AxisMapping := @InputSettings.Controllers[TabControlController.TabIndex].DigitalAxes[1];
        10: AxisMapping := @InputSettings.Controllers[TabControlController.TabIndex].DigitalAxes[2];
        11: AxisMapping := @InputSettings.Controllers[TabControlController.TabIndex].DigitalAxes[3];
      else
        AxisMapping := @InputSettings.Controllers[TabControlController.TabIndex].Axes[0];
      end;
      ComboBoxModifier1.Visible := Modifier1Mapping <> nil;
      LabelAxisModifier1.Visible := Modifier1Mapping <> nil;
      ButtonWaitAxisModifier1.Visible := Modifier1Mapping <> nil;
      ComboBoxModifier2.Visible := Modifier2Mapping <> nil;
      LabelAxisModifier2.Visible := Modifier2Mapping <> nil;
      ButtonWaitAxisModifier2.Visible := Modifier2Mapping <> nil;
      ComboBoxModifier3.Visible := Modifier3Mapping <> nil;
      LabelAxisModifier3.Visible := Modifier3Mapping <> nil;
      ButtonWaitAxisModifier3.Visible := Modifier3Mapping <> nil;
      if AxisMapping^.Analog then RadioGroupAxis.ItemIndex := 0
      else RadioGroupAxis.ItemIndex := 1;
      ComboBoxAxis.ItemIndex := ComboBoxAxis.Items.IndexOf(
        DIDevices[ActiveDevices[AxisMapping^.Device].DeviceIndex].Name + '.Axis ' +
        IntToStr(AxisMapping^.Axis));
      if ComboBoxAxis.ItemIndex = -1 then ComboBoxAxis.ItemIndex := 0;
      case DIDevices[ActiveDevices[AxisMapping^.Negative.Device].DeviceIndex].DeviceType and $FF of
        4:
          ComboBoxAxisButton1.ItemIndex := ComboBoxAxisButton1.Items.IndexOf(
            DIDevices[ActiveDevices[AxisMapping^.Negative.Device].DeviceIndex].Name + '.' +
            ActiveDevices[AxisMapping^.Negative.Device].ButtonNames[AxisMapping^.Negative.Button - 1]);
        2:
          ComboBoxAxisButton1.ItemIndex := ComboBoxAxisButton1.Items.IndexOf(
            DIDevices[ActiveDevices[AxisMapping^.Negative.Device].DeviceIndex].Name + '.' +
            ActiveDevices[AxisMapping^.Negative.Device].ButtonNames[AxisMapping^.Negative.Button - 1]);
        3:
          ComboBoxAxisButton1.ItemIndex := ComboBoxAxisButton1.Items.IndexOf(
            DIDevices[ActiveDevices[AxisMapping^.Negative.Device].DeviceIndex].Name + '.' +
            GetKeyName(AxisMapping^.Negative.Button));
      end;
      if ComboBoxAxisButton1.ItemIndex = -1 then ComboBoxAxisButton1.ItemIndex := 0;
      case DIDevices[ActiveDevices[AxisMapping^.Positive.Device].DeviceIndex].DeviceType and $FF of
        4:
          ComboBoxAxisButton2.ItemIndex := ComboBoxAxisButton2.Items.IndexOf(
            DIDevices[ActiveDevices[AxisMapping^.Positive.Device].DeviceIndex].Name + '.' +
            ActiveDevices[AxisMapping^.Positive.Device].ButtonNames[AxisMapping^.Positive.Button - 1]);
        2:
          ComboBoxAxisButton2.ItemIndex := ComboBoxAxisButton2.Items.IndexOf(
            DIDevices[ActiveDevices[AxisMapping^.Positive.Device].DeviceIndex].Name + '.' +
            ActiveDevices[AxisMapping^.Positive.Device].ButtonNames[AxisMapping^.Positive.Button - 1]);
        3:
          ComboBoxAxisButton2.ItemIndex := ComboBoxAxisButton2.Items.IndexOf(
            DIDevices[ActiveDevices[AxisMapping^.Positive.Device].DeviceIndex].Name + '.' +
            GetKeyName(AxisMapping^.Positive.Button));
      end;
      if ComboBoxAxisButton2.ItemIndex = -1 then ComboBoxAxisButton2.ItemIndex := 0;
      if Modifier1Mapping <> nil then
      begin
        case DIDevices[ActiveDevices[Modifier1Mapping^.Device].DeviceIndex].DeviceType and $FF of
          4:
            ComboBoxModifier1.ItemIndex := ComboBoxModifier1.Items.IndexOf(
              DIDevices[ActiveDevices[Modifier1Mapping^.Device].DeviceIndex].Name + '.' +
              ActiveDevices[Modifier1Mapping^.Device].ButtonNames[Modifier1Mapping^.Button - 1]);
          2:
            ComboBoxModifier1.ItemIndex := ComboBoxModifier1.Items.IndexOf(
              DIDevices[ActiveDevices[Modifier1Mapping^.Device].DeviceIndex].Name + '.' +
              ActiveDevices[Modifier1Mapping^.Device].ButtonNames[Modifier1Mapping^.Button - 1]);
          3:
            ComboBoxModifier1.ItemIndex := ComboBoxModifier1.Items.IndexOf(
              DIDevices[ActiveDevices[Modifier1Mapping^.Device].DeviceIndex].Name + '.' +
              GetKeyName(Modifier1Mapping^.Button));
        end;
        if ComboBoxModifier1.ItemIndex = -1 then ComboBoxModifier1.ItemIndex := 0;
      end;
      if Modifier2Mapping <> nil then
      begin
        case DIDevices[ActiveDevices[Modifier2Mapping^.Device].DeviceIndex].DeviceType and $FF of
          4:
            ComboBoxModifier2.ItemIndex := ComboBoxModifier2.Items.IndexOf(
              DIDevices[ActiveDevices[Modifier2Mapping^.Device].DeviceIndex].Name + '.' +
              ActiveDevices[Modifier2Mapping^.Device].ButtonNames[Modifier2Mapping^.Button - 1]);
          2:
            ComboBoxModifier2.ItemIndex := ComboBoxModifier2.Items.IndexOf(
              DIDevices[ActiveDevices[Modifier2Mapping^.Device].DeviceIndex].Name + '.' +
              ActiveDevices[Modifier2Mapping^.Device].ButtonNames[Modifier2Mapping^.Button - 1]);
          3:
            ComboBoxModifier2.ItemIndex := ComboBoxModifier2.Items.IndexOf(
              DIDevices[ActiveDevices[Modifier2Mapping^.Device].DeviceIndex].Name + '.' +
              GetKeyName(Modifier2Mapping^.Button));
        end;
        if ComboBoxModifier2.ItemIndex = -1 then ComboBoxModifier2.ItemIndex := 0;
      end;
      if Modifier3Mapping <> nil then
      begin
        case DIDevices[ActiveDevices[Modifier3Mapping^.Device].DeviceIndex].DeviceType and $FF of
          4:
            ComboBoxModifier3.ItemIndex := ComboBoxModifier3.Items.IndexOf(
              DIDevices[ActiveDevices[Modifier3Mapping^.Device].DeviceIndex].Name + '.' +
              ActiveDevices[Modifier3Mapping^.Device].ButtonNames[Modifier3Mapping^.Button - 1]);
          2:
            ComboBoxModifier3.ItemIndex := ComboBoxModifier3.Items.IndexOf(
              DIDevices[ActiveDevices[Modifier3Mapping^.Device].DeviceIndex].Name + '.' +
              ActiveDevices[Modifier3Mapping^.Device].ButtonNames[Modifier3Mapping^.Button - 1]);
          3:
            ComboBoxModifier3.ItemIndex := ComboBoxModifier3.Items.IndexOf(
              DIDevices[ActiveDevices[Modifier3Mapping^.Device].DeviceIndex].Name + '.' +
              GetKeyName(Modifier3Mapping^.Button));
        end;
        if ComboBoxModifier3.ItemIndex = -1 then ComboBoxModifier3.ItemIndex := 0;
      end;
    end;
    'D':
    begin
      NotebookMap.PageIndex := 2;
      Count := 0;
      Selection := 0;
      for i := 0 to ActiveDeviceCount - 1 do
        if ActiveDevices[i].ForceFeedback then
        begin
          Inc(Count);
          if InputSettings.Controllers[TabControlController.TabIndex].ForceFeedbackDevice = i then
          begin
            Selection := Count;
            Break;
          end;
        end;
      ComboBoxDDevice.ItemIndex := Selection;
    end;
  end;
end;


procedure TFormSettings.RadioGroupAxisClick(Sender: TObject);
begin
  NotebookAxis.PageIndex := RadioGroupAxis.ItemIndex;
  if NotebookAxis.PageIndex = 0 then
    AxisMapping.Analog := True
  else
    AxisMapping.Analog := False;
end;

procedure TFormSettings.FormShow(Sender: TObject);
begin
  UpdateLabels;
  ListBoxButtons.ItemIndex := 0;
  ListBoxButtonsClick(ListBoxButtons);
  SetControllerEnabled(CheckBoxPluggedIn.Checked);
end;

procedure TFormSettings.ComboBoxAxisChange(Sender: TObject);
var
  S, DeviceName: string;
begin
  S := ComboBoxAxis.Items[ComboBoxAxis.ItemIndex];
  if S = '(none)' then
    AxisMapping.Device := -1
  else
  begin
    DeviceName := Copy(S, 1, Pos('.', S) - 1);
    Delete(S, 1, Pos('.', S));
    AxisMapping.Device := GetDeviceIndex(DeviceName);
    Delete(S, 1, 5);
    AxisMapping.Axis := StrToIntDef(S, 1);
  end;
end;

procedure TFormSettings.ComboBoxButtonChange(Sender: TObject);
var
  S, DeviceName: string;
  Device: Integer;
begin
  S := ComboBoxButton.Items[ComboBoxButton.ItemIndex];
  if S = '(none)' then
    ButtonMapping.Device := -1
  else
  begin
    DeviceName := Copy(S, 1, Pos('.', S) - 1);
    Delete(S, 1, Pos('.', S));
    Device := GetDeviceIndex(DeviceName);
    ButtonMapping.Device := Device;
    if DIDevices[ActiveDevices[Device].DeviceIndex].DeviceType and $FF = 3 then
      ButtonMapping.Button := GetKeyCode(S)
    else
      ButtonMapping.Button := GetButtonIndex(S, Device);
  end;
end;

procedure TFormSettings.ComboBoxAxisButton1Change(Sender: TObject);
var
  S, DeviceName: string;
  Device: Integer;
begin
  S := ComboBoxAxisButton1.Items[ComboBoxAxisButton1.ItemIndex];
  if S = '(none)' then
    AxisMapping.Negative.Device := -1
  else
  begin
    DeviceName := Copy(S, 1, Pos('.', S) - 1);
    Delete(S, 1, Pos('.', S));
    Device := GetDeviceIndex(DeviceName);
    AxisMapping.Negative.Device := Device;
    if DIDevices[ActiveDevices[Device].DeviceIndex].DeviceType and $FF = 3 then
      AxisMapping.Negative.Button := GetKeyCode(S)
    else
      AxisMapping.Negative.Button := GetButtonIndex(S, Device);
  end;
end;

procedure TFormSettings.ComboBoxAxisButton2Change(Sender: TObject);
var
  S, DeviceName: string;
  Device: Integer;
begin
  S := ComboBoxAxisButton2.Items[ComboBoxAxisButton2.ItemIndex];
  if S = '(none)' then
    AxisMapping.Positive.Device := -1
  else
  begin
    DeviceName := Copy(S, 1, Pos('.', S) - 1);
    Delete(S, 1, Pos('.', S));
    Device := GetDeviceIndex(DeviceName);
    AxisMapping.Positive.Device := Device;
    if DIDevices[ActiveDevices[Device].DeviceIndex].DeviceType and $FF = 3 then
      AxisMapping.Positive.Button := GetKeyCode(S)
    else
      AxisMapping.Positive.Button := GetButtonIndex(S, Device);
  end;
end;

procedure TFormSettings.CheckBoxMemPackClick(Sender: TObject);
begin
  InputSettings.Controllers[TabControlController.TabIndex].MemPack := (Sender as TCheckBox).Checked;
  if CheckBoxMemPack.Checked then
  begin
    CheckBoxRumblePack.Checked := False;
    CheckBoxRumblePack.Enabled := False;
  end
  else
    CheckBoxRumblePack.Enabled := True;
end;

procedure TFormSettings.CheckBoxRumblePackClick(Sender: TObject);
begin
  InputSettings.Controllers[TabControlController.TabIndex].RumblePack := (Sender as TCheckBox).Checked;
  if CheckBoxRumblePack.Checked then
  begin
    CheckBoxMemPack.Checked := False;
    CheckBoxMemPack.Enabled := False;
  end
  else
    CheckBoxMemPack.Enabled := True;
end;

procedure TFormSettings.SaveSettings;
var
  Reg: TRegistry;
begin
  Reg := TRegistry.Create;
  Reg.RootKey := HKEY_CURRENT_USER;
  Reg.OpenKey('\Software\Nemu\0.8\Input.5', True);
  Reg.WriteBinaryData('Data', InputSettings, SizeOf(InputSettings));
  Reg.Free;
end;

procedure TFormSettings.ButtonOKClick(Sender: TObject);
begin
  SaveSettings;
end;

procedure TFormSettings.GroupBoxSettingsClick(Sender: TObject);
begin
  LastForegroundWindow := GetForegroundWindow;
end;

procedure TFormSettings.CheckBoxTurboFireClick(Sender: TObject);
begin
  ButtonMapping.Turbo := CheckBoxTurboFire.Checked;
end;

procedure TFormSettings.ComboBoxModifier2Change(Sender: TObject);
var
  S, DeviceName: string;
  Device: Integer;
begin
  S := ComboBoxModifier2.Items[ComboBoxModifier2.ItemIndex];
  if S = '(none)' then
    Modifier2Mapping.Device := -1
  else
  begin
    DeviceName := Copy(S, 1, Pos('.', S) - 1);
    Delete(S, 1, Pos('.', S));
    Modifier2Mapping.Device := GetDeviceIndex(DeviceName);
    Device := ButtonMapping.Device;
    if DIDevices[ActiveDevices[Device].DeviceIndex].DeviceType and $FF = 3 then
      Modifier2Mapping.Button := GetKeyCode(S)
    else
      Modifier2Mapping.Button := GetButtonIndex(S, Device);
  end;
end;

procedure TFormSettings.ComboBoxModifier3Change(Sender: TObject);
var
  S, DeviceName: string;
  Device: Integer;
begin
  S := ComboBoxModifier3.Items[ComboBoxModifier3.ItemIndex];
  if S = '(none)' then
    Modifier3Mapping.Device := -1
  else
  begin
    DeviceName := Copy(S, 1, Pos('.', S) - 1);
    Delete(S, 1, Pos('.', S));
    Modifier3Mapping.Device := GetDeviceIndex(DeviceName);
    Device := ButtonMapping.Device;
    if DIDevices[ActiveDevices[Device].DeviceIndex].DeviceType and $FF = 3 then
      Modifier3Mapping.Button := GetKeyCode(S)
    else
      Modifier3Mapping.Button := GetButtonIndex(S, Device);
  end;
end;

procedure TFormSettings.ComboBoxModifier1Change(Sender: TObject);
var
  S, DeviceName: string;
  Device: Integer;
begin
  S := ComboBoxModifier1.Items[ComboBoxModifier1.ItemIndex];
  if S = '(none)' then
    Modifier1Mapping.Device := -1
  else
  begin
    DeviceName := Copy(S, 1, Pos('.', S) - 1);
    Delete(S, 1, Pos('.', S));
    Modifier1Mapping.Device := GetDeviceIndex(DeviceName);
    Device := ButtonMapping.Device;
    if DIDevices[ActiveDevices[Device].DeviceIndex].DeviceType and $FF = 3 then
      Modifier1Mapping.Button := GetKeyCode(S)
    else
      Modifier1Mapping.Button := GetButtonIndex(S, Device);
  end;
end;

{ $483140 }
procedure TFormSettings.ButtonSensitivityClick(Sender: TObject);
begin
  FormSensitivity := TFormSensitivity.Create(Self);
  FormSensitivity.UpDownDefault.Position := InputSettings.DefaultSensitivity;
  FormSensitivity.UpDown1.Position := InputSettings.Modifier1Sensitivity;
  FormSensitivity.UpDown2.Position := InputSettings.Modifier2Sensitivity;
  FormSensitivity.UpDown3.Position := InputSettings.Modifier3Sensitivity;
  FormSensitivity.ParentWindow := FormSettings.Handle;
  if FormSensitivity.ShowModal = mrOK then
  begin
    InputSettings.DefaultSensitivity := FormSensitivity.UpDownDefault.Position;
    InputSettings.Modifier1Sensitivity := FormSensitivity.UpDown1.Position;
    InputSettings.Modifier2Sensitivity := FormSensitivity.UpDown2.Position;
    InputSettings.Modifier3Sensitivity := FormSensitivity.UpDown3.Position;
    UpdateLabels;
  end;
  FormSensitivity.Free;
end;

{ $483220 }
procedure TFormSettings.ButtonAutoConfigureAdaptoidClick(Sender: TObject);
var
  I, Count: Integer;
  MenuItem: TMenuItem;
  P: TPoint;
begin
  I := 0;
  Count := 0;
  while PopupMenuAdaptoid.Items.Count > 0 do
    PopupMenuAdaptoid.Items.Delete(0);
  while I < ActiveDeviceCount do
  begin
    if DIDevices[ActiveDevices[I].DeviceIndex].ProductName = 'Adaptoid' then
    begin
      Inc(Count);
      MenuItem := TMenuItem.Create(PopupMenuAdaptoid);
      MenuItem.OnClick := MenuItemAdaptoidClick;
      MenuItem.Tag := I;
      MenuItem.Caption := 'Adaptoid ' + IntToStr(Count);
      PopupMenuAdaptoid.Items.Add(MenuItem);
    end;
    Inc(I);
  end;
  P := (Sender as TButton).ClientToScreen(
    Point(0, (Sender as TButton).ClientRect.Bottom));
  PopupMenuAdaptoid.Popup(P.X, P.Y);
end;

{ $4833BC; standalone helper between two published methods. }
procedure AutoConfigureKeyboard(Controller: Integer);
var
  I: Integer;
begin
  I := 0;
  while I < ActiveDeviceCount do
  begin
    if DIDevices[ActiveDevices[I].DeviceIndex].DeviceType and $FF = 3 then
      Break;
    Inc(I);
  end;
  if I = ActiveDeviceCount then Exit;
  InputSettings.Controllers[Controller].Enabled := True;
  InputSettings.Controllers[Controller].Buttons[0].Button := $1E;
  InputSettings.Controllers[Controller].Buttons[0].Device := I;
  InputSettings.Controllers[Controller].Buttons[1].Button := $1F;
  InputSettings.Controllers[Controller].Buttons[1].Device := I;
  InputSettings.Controllers[Controller].Buttons[2].Button := $2D;
  InputSettings.Controllers[Controller].Buttons[2].Device := I;
  InputSettings.Controllers[Controller].Buttons[3].Button := $02;
  InputSettings.Controllers[Controller].Buttons[3].Device := I;
  InputSettings.Controllers[Controller].Buttons[4].Button := $10;
  InputSettings.Controllers[Controller].Buttons[4].Device := I;
  InputSettings.Controllers[Controller].Buttons[5].Button := $11;
  InputSettings.Controllers[Controller].Buttons[5].Device := I;
  InputSettings.Controllers[Controller].Modifiers[0].Button := $2A;
  InputSettings.Controllers[Controller].Modifiers[0].Device := I;
  InputSettings.Controllers[Controller].Modifiers[1].Button := $1D;
  InputSettings.Controllers[Controller].Modifiers[1].Device := I;
  InputSettings.Controllers[Controller].Modifiers[2].Button := $9D;
  InputSettings.Controllers[Controller].Modifiers[2].Device := I;
  InputSettings.Controllers[Controller].Axes[0].Analog := False;
  InputSettings.Controllers[Controller].Axes[0].Negative.Button := $CB;
  InputSettings.Controllers[Controller].Axes[0].Negative.Device := I;
  InputSettings.Controllers[Controller].Axes[0].Positive.Button := $CD;
  InputSettings.Controllers[Controller].Axes[0].Positive.Device := I;
  InputSettings.Controllers[Controller].Axes[1].Analog := False;
  InputSettings.Controllers[Controller].Axes[1].Negative.Button := $C8;
  InputSettings.Controllers[Controller].Axes[1].Negative.Device := I;
  InputSettings.Controllers[Controller].Axes[1].Positive.Button := $D0;
  InputSettings.Controllers[Controller].Axes[1].Positive.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[0].Analog := False;
  InputSettings.Controllers[Controller].DigitalAxes[0].Negative.Button := $24;
  InputSettings.Controllers[Controller].DigitalAxes[0].Negative.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[0].Positive.Button := $26;
  InputSettings.Controllers[Controller].DigitalAxes[0].Positive.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[1].Analog := False;
  InputSettings.Controllers[Controller].DigitalAxes[1].Negative.Button := $17;
  InputSettings.Controllers[Controller].DigitalAxes[1].Negative.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[1].Positive.Button := $25;
  InputSettings.Controllers[Controller].DigitalAxes[1].Positive.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[2].Analog := False;
  InputSettings.Controllers[Controller].DigitalAxes[2].Negative.Button := $21;
  InputSettings.Controllers[Controller].DigitalAxes[2].Negative.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[2].Positive.Button := $23;
  InputSettings.Controllers[Controller].DigitalAxes[2].Positive.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[3].Analog := False;
  InputSettings.Controllers[Controller].DigitalAxes[3].Negative.Button := $14;
  InputSettings.Controllers[Controller].DigitalAxes[3].Negative.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[3].Positive.Button := $22;
  InputSettings.Controllers[Controller].DigitalAxes[3].Positive.Device := I;
end;

{ $483594 }
procedure TFormSettings.ButtonAutoconfigureKeyboardClick(Sender: TObject);
begin
  AutoConfigureKeyboard(TabControlController.TabIndex);
  UpdateController;
  MessageBox(Handle, PChar('Controller #' + IntToStr(TabControlController.TabIndex + 1) +
    ' configured to use keyboard'), 'Autoconfiguration complete', MB_ICONINFORMATION);
end;

{ $483690 }
procedure TFormSettings.MenuItemAdaptoidClick(Sender: TObject);
var
  Controller, I: Integer;
begin
  Controller := TabControlController.TabIndex;
  I := (Sender as TMenuItem).Tag;
  InputSettings.Controllers[Controller].Enabled := True;
  InputSettings.Controllers[Controller].Buttons[0].Button := 1;
  InputSettings.Controllers[Controller].Buttons[0].Device := I;
  InputSettings.Controllers[Controller].Buttons[1].Button := 4;
  InputSettings.Controllers[Controller].Buttons[1].Device := I;
  InputSettings.Controllers[Controller].Buttons[2].Button := 10;
  InputSettings.Controllers[Controller].Buttons[2].Device := I;
  InputSettings.Controllers[Controller].Buttons[3].Button := 9;
  InputSettings.Controllers[Controller].Buttons[3].Device := I;
  InputSettings.Controllers[Controller].Buttons[4].Button := 7;
  InputSettings.Controllers[Controller].Buttons[4].Device := I;
  InputSettings.Controllers[Controller].Buttons[5].Button := 8;
  InputSettings.Controllers[Controller].Buttons[5].Device := I;
  InputSettings.Controllers[Controller].Axes[0].Analog := True;
  InputSettings.Controllers[Controller].Axes[0].Axis := 1;
  InputSettings.Controllers[Controller].Axes[0].Device := I;
  InputSettings.Controllers[Controller].Axes[1].Analog := True;
  InputSettings.Controllers[Controller].Axes[1].Axis := 2;
  InputSettings.Controllers[Controller].Axes[1].Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[0].Analog := False;
  InputSettings.Controllers[Controller].DigitalAxes[0].Negative.Button := 13;
  InputSettings.Controllers[Controller].DigitalAxes[0].Positive.Button := 14;
  InputSettings.Controllers[Controller].DigitalAxes[0].Negative.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[0].Positive.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[1].Analog := False;
  InputSettings.Controllers[Controller].DigitalAxes[1].Negative.Button := 11;
  InputSettings.Controllers[Controller].DigitalAxes[1].Positive.Button := 12;
  InputSettings.Controllers[Controller].DigitalAxes[1].Negative.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[1].Positive.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[2].Analog := False;
  InputSettings.Controllers[Controller].DigitalAxes[2].Negative.Button := 5;
  InputSettings.Controllers[Controller].DigitalAxes[2].Positive.Button := 3;
  InputSettings.Controllers[Controller].DigitalAxes[2].Negative.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[2].Positive.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[3].Analog := False;
  InputSettings.Controllers[Controller].DigitalAxes[3].Negative.Button := 6;
  InputSettings.Controllers[Controller].DigitalAxes[3].Positive.Button := 2;
  InputSettings.Controllers[Controller].DigitalAxes[3].Negative.Device := I;
  InputSettings.Controllers[Controller].DigitalAxes[3].Positive.Device := I;
  UpdateController;
  MessageBox(Handle, PChar('Controller #' + IntToStr(Controller + 1) +
    ' configured to use Adaptoid'), 'Autoconfiguration complete', MB_ICONINFORMATION);
end;

{ $4838D4 }
procedure TFormSettings.TabControlControllerChange(Sender: TObject);
begin
  UpdateController;
end;

{ $4838DC }
procedure TFormSettings.ButtonWaitButtonClick(Sender: TObject);
begin
  FormWait := TFormWait.Create(FormSettings);
  FormWait.ParentWindow := FormSettings.Handle;
  if FormWait.ShowModal = mrOK then
  begin
    ComboBoxButton.ItemIndex := ComboBoxButton.Items.IndexOf(FormWait.PanelStatus.Caption);
    ComboBoxButtonChange(ComboBoxButton);
  end;
  FormWait.Free;
end;

{ $4839A8 }
procedure TFormSettings.ButtonWaitAxisButton2Click(Sender: TObject);
begin
  FormWait := TFormWait.Create(FormSettings);
  FormWait.ParentWindow := FormSettings.Handle;
  if FormWait.ShowModal = mrOK then
  begin
    ComboBoxAxisButton2.ItemIndex := ComboBoxAxisButton2.Items.IndexOf(FormWait.PanelStatus.Caption);
    ComboBoxAxisButton2Change(ComboBoxAxisButton2);
  end;
  FormWait.Free;
end;

{ $483A74 }
procedure TFormSettings.ButtonWaitAxisButton1Click(Sender: TObject);
begin
  FormWait := TFormWait.Create(FormSettings);
  FormWait.ParentWindow := FormSettings.Handle;
  if FormWait.ShowModal = mrOK then
  begin
    ComboBoxAxisButton1.ItemIndex := ComboBoxAxisButton1.Items.IndexOf(FormWait.PanelStatus.Caption);
    ComboBoxAxisButton1Change(ComboBoxAxisButton1);
  end;
  FormWait.Free;
end;

{ $483B40 }
procedure TFormSettings.ButtonWaitAxisModifier1Click(Sender: TObject);
begin
  FormWait := TFormWait.Create(FormSettings);
  FormWait.ParentWindow := FormSettings.Handle;
  if FormWait.ShowModal = mrOK then
  begin
    ComboBoxModifier1.ItemIndex := ComboBoxModifier1.Items.IndexOf(FormWait.PanelStatus.Caption);
    ComboBoxModifier1Change(ComboBoxModifier1);
  end;
  FormWait.Free;
end;

{ $483C0C }
procedure TFormSettings.ButtonWaitAxisModifier2Click(Sender: TObject);
begin
  FormWait := TFormWait.Create(FormSettings);
  FormWait.ParentWindow := FormSettings.Handle;
  if FormWait.ShowModal = mrOK then
  begin
    ComboBoxModifier2.ItemIndex := ComboBoxModifier2.Items.IndexOf(FormWait.PanelStatus.Caption);
    ComboBoxModifier2Change(ComboBoxModifier2);
  end;
  FormWait.Free;
end;

{ $483CD8 }
procedure TFormSettings.ButtonWaitAxisModifier3Click(Sender: TObject);
begin
  FormWait := TFormWait.Create(FormSettings);
  FormWait.ParentWindow := FormSettings.Handle;
  if FormWait.ShowModal = mrOK then
  begin
    ComboBoxModifier3.ItemIndex := ComboBoxModifier3.Items.IndexOf(FormWait.PanelStatus.Caption);
    ComboBoxModifier3Change(ComboBoxModifier3);
  end;
  FormWait.Free;
end;

procedure TFormSettings.ComboBoxDDeviceChange(Sender: TObject);
var
  Device: Integer;
  Count: Integer;
  I: Cardinal;
begin
  Device := -1;
  Count := 0;
  for I := 0 to ActiveDeviceCount - 1 do
    if ActiveDevices[I].ForceFeedback then
    begin
      Inc(Count);
      if Count = ComboBoxDDevice.ItemIndex then
      begin
        Device := I;
        Break;
      end;
    end;
  InputSettings.Controllers[TabControlController.TabIndex].ForceFeedbackDevice := Device;
end;


initialization
finalization
end.
