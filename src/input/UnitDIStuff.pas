unit UnitDIStuff;

interface

uses Windows, Forms, DInput, UnitSettings, UnitDecs;

var
  DI: IDirectInputA;
  DIError: HRESULT;
  DeviceCount: Integer;
  DIWindow: HWND;
  KeyboardEnabled: Boolean;
  AdaptoidEnabled: Boolean;
  DIDevices: array[0..19] of record
    GUID: TGUID;
    Name: string;
    ProductName: string;
    DeviceType: DWORD;
    Caps: TDIDevCaps;
  end;
  OpenDevicesCount: Integer;
  OpenDevices: array[0..19] of record
    Device: IDirectInputDevice2A;
    Effect: IDirectInputEffect;
    JoyState: TDJoyState;
    KeyboardState: array[0..255] of Byte;
    MouseState: TDIMouseState;
    DeviceIndex: Integer;
    ButtonNames: array[0..49] of array[1..40] of Char;
    ButtonCount: Integer;
    ForceFeedback: Boolean;
    DeadZone: Cardinal;
  end;
  TurboOn: array[0..5] of Boolean;
  CurrentlyConfiguring: Boolean = False;

type
  TKeyDef = record
    Name: string;
    Code: Integer;
  end;

const
  Keys: array[0..71] of TKeyDef = (
    (Name: 'ESC'; Code: $01),
    (Name: '1'; Code: $02),
    (Name: '2'; Code: $03),
    (Name: '3'; Code: $04),
    (Name: '4'; Code: $05),
    (Name: '5'; Code: $06),
    (Name: '6'; Code: $07),
    (Name: '7'; Code: $08),
    (Name: '8'; Code: $09),
    (Name: '9'; Code: $0A),
    (Name: '0'; Code: $0B),
    (Name: 'TAB'; Code: $0F),
    (Name: 'RETURN'; Code: $1C),
    (Name: 'LEFTSHIFT'; Code: $2A),
    (Name: 'RIGHTSHIFT'; Code: $36),
    (Name: 'LEFTCONTROL'; Code: $1D),
    (Name: 'RIGHTCONTROL'; Code: $9D),
    (Name: 'CAPSLOCK'; Code: $3A),
    (Name: 'SPACE'; Code: $39),
    (Name: 'LEFT (Arrow)'; Code: $CB),
    (Name: 'RIGHT (Arrow)'; Code: $CD),
    (Name: 'UP (Arrow)'; Code: $C8),
    (Name: 'DOWN (Arrow)'; Code: $D0),
    (Name: 'INSERT (Arrow)'; Code: $D2),
    (Name: 'DELETE (Arrow)'; Code: $D3),
    (Name: 'HOME (Arrow)'; Code: $C7),
    (Name: 'END (Arrow)'; Code: $CF),
    (Name: 'PAGEUP (Arrow)'; Code: $C9),
    (Name: 'PAGEDOWN (Arrow)'; Code: $D1),
    (Name: 'NUMPAD0'; Code: $52),
    (Name: 'NUMPAD1'; Code: $4F),
    (Name: 'NUMPAD2'; Code: $50),
    (Name: 'NUMPAD3'; Code: $51),
    (Name: 'NUMPAD4'; Code: $4B),
    (Name: 'NUMPAD5'; Code: $4C),
    (Name: 'NUMPAD6'; Code: $4D),
    (Name: 'NUMPAD7'; Code: $47),
    (Name: 'NUMPAD8'; Code: $48),
    (Name: 'NUMPAD9'; Code: $49),
    (Name: 'NUMLOCK'; Code: $45),
    (Name: '/ (Numpad)'; Code: $B5),
    (Name: '* (Numpad)'; Code: $37),
    (Name: '- (Numpad)'; Code: $4A),
    (Name: '+ (Numpad)'; Code: $4E),
    (Name: 'ENTER (Numpad)'; Code: $9C),
    (Name: '. (Numpad)'; Code: $53),
    (Name: 'A'; Code: $1E),
    (Name: 'B'; Code: $30),
    (Name: 'C'; Code: $2E),
    (Name: 'D'; Code: $20),
    (Name: 'E'; Code: $12),
    (Name: 'F'; Code: $21),
    (Name: 'G'; Code: $22),
    (Name: 'H'; Code: $23),
    (Name: 'I'; Code: $17),
    (Name: 'J'; Code: $24),
    (Name: 'K'; Code: $25),
    (Name: 'L'; Code: $26),
    (Name: 'M'; Code: $32),
    (Name: 'N'; Code: $31),
    (Name: 'O'; Code: $18),
    (Name: 'P'; Code: $19),
    (Name: 'Q'; Code: $10),
    (Name: 'R'; Code: $13),
    (Name: 'S'; Code: $1F),
    (Name: 'T'; Code: $14),
    (Name: 'U'; Code: $16),
    (Name: 'V'; Code: $2F),
    (Name: 'W'; Code: $11),
    (Name: 'X'; Code: $2D),
    (Name: 'Y'; Code: $15),
    (Name: 'Z'; Code: $2C));

procedure DIErrorCheck(HR: HRESULT);
procedure SetAxisRange(Device: IDirectInputDevice2A; Minimum, Maximum: Integer);
procedure SetRelativeAxes(Device: IDirectInputDevice2A);
procedure FadeOutForm(Form: TForm; Milliseconds: Integer);
procedure InitDirectInput;
procedure FreeDirectInput;

implementation

uses SysUtils, Classes, Registry;

procedure DIErrorCheck(HR: HRESULT);
begin
  if HR <> 0 then
    MessageBox(0, PChar(DIErrorString(HR)), 'Error in input code', 0);
end;

procedure SetAxisRange(Device: IDirectInputDevice2A; Minimum, Maximum: Integer);
var
  Range: TDIPROPRange;
begin
  FillChar(Range, SizeOf(Range), 0);
  Range.diph.dwSize := SizeOf(Range);
  Range.diph.dwHeaderSize := SizeOf(Range.diph);
  Range.diph.dwObj := 0;
  Range.diph.dwHow := DIPH_DEVICE;
  Range.lMin := -126;
  Range.lMax := 126;
  DIErrorCheck(Device.SetProperty(DIPROP_RANGE, Range.diph));
end;

procedure SetRelativeAxes(Device: IDirectInputDevice2A);
var
  Mode: TDIPROPDWord;
begin
  FillChar(Mode, SizeOf(Mode), 0);
  Mode.diph.dwSize := SizeOf(Mode);
  Mode.diph.dwHeaderSize := SizeOf(Mode.diph);
  Mode.diph.dwObj := 0;
  Mode.diph.dwHow := DIPH_DEVICE;
  Mode.dwData := DIPROPAXISMODE_REL;
  DIErrorCheck(Device.SetProperty(DIPROP_AXISMODE, Mode.diph));
end;

function EnumDevicesCallback(const Instance: TDIDeviceInstanceA;
  Context: Pointer): Integer; stdcall;
var
  Number: Integer;
  I: Integer;
begin
  Result := 1;
  if (Instance.dwDevType and $FF = DIDEVTYPE_KEYBOARD) or
    (Instance.dwDevType and $FF = DIDEVTYPE_JOYSTICK) or
    (Instance.dwDevType and $FF = DIDEVTYPE_MOUSE) then
  begin
    DIDevices[DeviceCount].GUID := Instance.guidInstance;
    DIDevices[DeviceCount].Name := Instance.tszInstanceName;
    Number := 1;
    for I := 0 to DeviceCount - 1 do
      if DIDevices[DeviceCount].Name + ' ' + IntToStr(Number) =
        DIDevices[I].Name then Inc(Number);
    if DIDevices[DeviceCount].Name <> 'Keyboard' then
      DIDevices[DeviceCount].Name := DIDevices[DeviceCount].Name + ' ' +
        IntToStr(Number);
    DIDevices[DeviceCount].ProductName := Instance.tszProductName;
    DIDevices[DeviceCount].DeviceType := Instance.dwDevType;
    Inc(DeviceCount);
  end;
end;

procedure FadeOutForm(Form: TForm; Milliseconds: Integer);
var
  Started: TDateTime;
begin
  Started := Now;
  Form.AlphaBlend := True;
  Form.AlphaBlendValue := 255;
  Form.Repaint;
  repeat
    Form.AlphaBlendValue := 255 - Round((Now - Started) * 24 * 60 * 60 *
      1000 * 256 / Milliseconds);
  until (Now - Started) * 24 * 60 * 60 * 1000 >= Milliseconds;
end;

function EnumButtonsCallback(const Instance: TDIDeviceObjectInstanceA;
  Context: Pointer): Integer; stdcall;
begin
  StrCopy(@OpenDevices[Integer(Context^)].ButtonNames[
    OpenDevices[Integer(Context^)].ButtonCount], Instance.tszName);
  Inc(OpenDevices[Integer(Context^)].ButtonCount);
  Result := 1;
end;

procedure InitDirectInput;
const
  EffectAxes: array[0..1] of DWORD = (DIJOFS_X, DIJOFS_Y);
  EffectDirections: array[0..1] of Integer = (18000, 0);

var
  Reg: TRegistry;
  Names: TStringList;
  I, J: Integer;
  Name: string;
  Found: Boolean;
  EnumContext: Integer;
  Device: IDirectInputDeviceA;
  Force: TDIConstantForce;
  DeadZone: TDIPROPDWord;
  Effect: TDIEffect;
  UseDefaults: Boolean;
begin
  Reg := TRegistry.Create;
  Reg.RootKey := HKEY_CURRENT_USER;
  FillChar(DI, SizeOf(DI), 0);
  DIError := DirectInputCreate(HInstance, $0500, DI, nil);
  DIErrorCheck(DIError);
  DeviceCount := 0;
{$IFDEF NEMU_FIX_HID_ENUMERATION}
  { Filter before DirectInput creates devices. The callback ignores other
    classes too, but some modern HID devices corrupt the legacy DirectInput
    heap during initialization, before that callback can reject them. }
  DIError := DI.EnumDevices(DIDEVTYPE_MOUSE, @EnumDevicesCallback, @EnumContext, 0);
  DIErrorCheck(DIError);
  DIError := DI.EnumDevices(DIDEVTYPE_KEYBOARD, @EnumDevicesCallback, @EnumContext, 0);
  DIErrorCheck(DIError);
  DIError := DI.EnumDevices(DIDEVTYPE_JOYSTICK, @EnumDevicesCallback, @EnumContext, 0);
{$ELSE}
  DIError := DI.EnumDevices(0, @EnumDevicesCallback, @EnumContext, 0);
{$ENDIF}
  DIErrorCheck(DIError);
  Reg.OpenKey('\Software\Nemu\0.8\Input.5\Use', True);
  Names := TStringList.Create;
  Reg.GetValueNames(Names);
  KeyboardEnabled := False;
  AdaptoidEnabled := False;
  I := 0;
  if Names.Count = 0 then UseDefaults := True else UseDefaults := False;
  OpenDevicesCount := 0;
  Found := True;
  repeat
    if not UseDefaults then
    begin
      Found := False;
      if Names.Count <= I then Break;
      Name := Names[I];
      if Name = '' then Continue;
    end;
    for J := 0 to DeviceCount - 1 do
      if UseDefaults or (DIDevices[J].Name = Name) then
      begin
        if UseDefaults then Reg.WriteString(DIDevices[J].Name, '');
        Found := True;
        DIErrorCheck(DI.CreateDevice(DIDevices[J].GUID, Device, nil));
        DIErrorCheck(Device.QueryInterface(IID_IDirectInputDevice2,
          OpenDevices[OpenDevicesCount].Device));
        Device := nil;
        OpenDevices[OpenDevicesCount].DeviceIndex := J;
        case DIDevices[J].DeviceType and $FF of
          DIDEVTYPE_KEYBOARD:
            begin
              FillChar(DIDevices[J].Caps, SizeOf(TDIDevCaps), 0);
              DIDevices[J].Caps.dwSize := SizeOf(TDIDevCaps);
              DIErrorCheck(OpenDevices[OpenDevicesCount].Device.GetCapabilities(
                DIDevices[J].Caps));
              DIErrorCheck(OpenDevices[OpenDevicesCount].Device.SetDataFormat(
                c_dfDIKeyboard));
              DIErrorCheck(OpenDevices[OpenDevicesCount].Device.SetCooperativeLevel(
                DIWindow, DISCL_NONEXCLUSIVE or DISCL_FOREGROUND));
              KeyboardEnabled := True;
            end;
          DIDEVTYPE_JOYSTICK:
            begin
              if not UseDefaults then Name := Reg.ReadString(Name);
              FillChar(DIDevices[J].Caps, SizeOf(TDIDevCaps), 0);
              DIDevices[J].Caps.dwSize := SizeOf(TDIDevCaps);
              DIErrorCheck(OpenDevices[OpenDevicesCount].Device.GetCapabilities(
                DIDevices[J].Caps));
              DIErrorCheck(OpenDevices[OpenDevicesCount].Device.SetDataFormat(
                c_dfDIJoystick));
              DIErrorCheck(OpenDevices[OpenDevicesCount].Device.SetCooperativeLevel(
                DIWindow, DISCL_EXCLUSIVE or DISCL_BACKGROUND));
              SetAxisRange(OpenDevices[OpenDevicesCount].Device, -126, 126);
              DIErrorCheck(OpenDevices[OpenDevicesCount].Device.Acquire);
              if DIDevices[J].ProductName = 'Adaptoid' then AdaptoidEnabled := True;
              OpenDevices[OpenDevicesCount].ButtonCount := 0;
              OpenDevices[OpenDevicesCount].Device.EnumObjects(
                EnumButtonsCallback, @OpenDevicesCount, DIDFT_BUTTON);
              OpenDevices[OpenDevicesCount].ForceFeedback :=
                (Pos('f', Name) <> 0) and
                (DIDevices[J].Caps.dwFlags and DIDC_FORCEFEEDBACK <> 0);
              OpenDevices[OpenDevicesCount].DeadZone :=
                StrToIntDef(Copy(Name, 2, 10), 1);
              if OpenDevices[OpenDevicesCount].ForceFeedback then
              begin
                DeadZone.diph.dwSize := SizeOf(DeadZone);
                DeadZone.diph.dwHeaderSize := SizeOf(DeadZone.diph);
                DeadZone.diph.dwObj := 0;
                DeadZone.diph.dwHow := DIPH_DEVICE;
                DeadZone.dwData := OpenDevices[OpenDevicesCount].DeadZone * 100;
                DIError := OpenDevices[OpenDevicesCount].Device.SetProperty(
                  DIPROP_DEADZONE, DeadZone.diph);
                if DIError <> 0 then
                  MessageBox(0, PChar(DIErrorString(DIError)), '', 0);
                Force.lMagnitude := DI_FFNOMINALMAX;
                Effect.dwSize := SizeOf(Effect);
                Effect.dwDuration := 100000;
                Effect.dwFlags := DIEFF_POLAR or DIEFF_OBJECTOFFSETS;
                Effect.dwDuration := INFINITE;
                Effect.dwSamplePeriod := 0;
                Effect.dwGain := DI_FFNOMINALMAX;
                Effect.dwTriggerButton := DIEB_NOTRIGGER;
                Effect.dwTriggerRepeatInterval := 0;
                Effect.cAxes := 2;
                Effect.rgdwAxes := @EffectAxes[0];
                Effect.rglDirection := @EffectDirections[0];
                Effect.lpEnvelope := nil;
                Effect.cbTypeSpecificParams := SizeOf(Force);
                Effect.lpvTypeSpecificParams := @Force;
                DIError := OpenDevices[OpenDevicesCount].Device.CreateEffect(
                  GUID_ConstantForce, Effect, OpenDevices[OpenDevicesCount].Effect, nil);
                if DIError <> 0 then
                  MessageBox(0, PChar('Create Effect Error: ' +
                    DIErrorString(DIError)), '', 0);
              end;
            end;
          DIDEVTYPE_MOUSE:
            begin
              FillChar(DIDevices[J].Caps, SizeOf(TDIDevCaps), 0);
              DIDevices[J].Caps.dwSize := SizeOf(TDIDevCaps);
              DIErrorCheck(OpenDevices[OpenDevicesCount].Device.GetCapabilities(
                DIDevices[J].Caps));
              DIErrorCheck(OpenDevices[OpenDevicesCount].Device.SetDataFormat(
                c_dfDIMouse));
              DIErrorCheck(OpenDevices[OpenDevicesCount].Device.SetCooperativeLevel(
                DIWindow, DISCL_NONEXCLUSIVE or DISCL_FOREGROUND));
              SetRelativeAxes(OpenDevices[OpenDevicesCount].Device);
              OpenDevices[OpenDevicesCount].ButtonCount := 0;
              OpenDevices[OpenDevicesCount].Device.EnumObjects(
                EnumButtonsCallback, @OpenDevicesCount, DIDFT_BUTTON);
            end;
        end;
        Inc(OpenDevicesCount);
      end;
    if not Found and not UseDefaults then Names.Delete(I);
    Inc(I);
  until (Names.Count <= I) or (UseDefaults <> False);
  Names.Free;
  Reg.OpenKey('\Software\Nemu\0.8\Input.5', True);
  if Reg.ValueExists('Data') then
    Reg.ReadBinaryData('Data', ControllerSettings, SizeOf(ControllerSettings))
  else
  begin
    ZeroMemory(@ControllerSettings, SizeOf(ControllerSettings));
    AutoConfigureKeyboard(0);
    ControllerSettings.Controllers[0].MemPack := False;
    ControllerSettings.Controllers[0].RumblePack := False;
    ControllerSettings.DefaultSensitivity := 55;
    ControllerSettings.Modifier1Sensitivity := 20;
    ControllerSettings.Modifier2Sensitivity := 30;
    ControllerSettings.Modifier3Sensitivity := 100;
  end;
  for I := 0 to 3 do
  begin
    Controllers^[I].PluggedIn := ControllerSettings.Controllers[I].Enabled;
    Controllers^[I].MemPack := ControllerSettings.Controllers[I].MemPack;
    Controllers^[I].RumblePack := ControllerSettings.Controllers[I].RumblePack;
  end;
  Reg.Free;
end;

procedure FreeDirectInput;
var
  I: Integer;
begin
  for I := 0 to OpenDevicesCount - 1 do
    if OpenDevices[I].Device <> nil then
    begin
      if OpenDevices[I].ForceFeedback then
        OpenDevices[I].Effect.Unload;
      OpenDevices[I].Effect := nil;
      OpenDevices[I].Device.Unacquire;
      OpenDevices[I].Device := nil;
    end;
  DI := nil;
end;

end.
