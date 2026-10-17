library Input;

{$IMPLICITBUILD OFF}

uses
  Windows,
  SysUtils,
  Classes,
  Registry,
  MMSystem,
  Forms,
  DInput in 'DInput.pas',
  UnitDecs in 'UnitDecs.pas',
  RCP in 'RCP.pas',
  ToolWin,
  ComCtrls,
  CheckLst,
  UnitSettings in 'UnitSettings.pas',
  UnitWaitKey in 'UnitWaitKey.pas',
  UnitDIStuff in 'UnitDIStuff.pas',
  UnitSelectInputDevices in 'UnitSelectInputDevices.pas',
  UnitAdaptoid in 'UnitAdaptoid.pas',
  UnitSensitivity in 'UnitSensitivity.pas';

type
  PControllerSettings = ^TControllerSettings;

procedure CIInit(Window: HWND; Data: PControllerDataArray); cdecl;
begin
  Controllers := Data;
  DIWindow := Window;
  InitDirectInput;
  InitAdaptoid;
  FormSettings := TFormSettings.Create(nil);
end;

procedure CIFree; cdecl;
begin
  FormSettings.Free;
  FreeAdaptoid;
  FreeDirectInput;
end;

procedure CIRumble(Controller: Integer); cdecl;
var
  Device: Integer;
begin
  while CurrentlyConfiguring do
    Windows.Sleep(1);
  Device := ControllerSettings.Controllers[Controller].ForceFeedbackDevice;
  if Device = -1 then
    Exit;
  if not OpenDevices[Device].ForceFeedback then
    Exit;
  if Controllers^[Controller].Rumble then
    DIError := OpenDevices[Device].Effect.Start(1, 1)
  else
    DIError := OpenDevices[Device].Effect.Stop;
  if DIError <> 0 then
    MessageBox(DIWindow, PChar('There was an error activating force feedback: ' +
      DIErrorString(DIError) + #13 + 'This message will only show once'), '', 0);
end;

procedure UpdateButton(ButtonIndex: Integer; Mask: Cardinal);
var
  I: Integer;
  Button: PButtonMapping;
begin
  for I := 0 to 3 do
    if ControllerSettings.Controllers[I].Enabled then
    begin
      Button := nil;
      if ButtonIndex = 0 then Button := @ControllerSettings.Controllers[I].Buttons[0];
      if ButtonIndex = 1 then Button := @ControllerSettings.Controllers[I].Buttons[1];
      if ButtonIndex = 2 then Button := @ControllerSettings.Controllers[I].Buttons[2];
      if ButtonIndex = 3 then Button := @ControllerSettings.Controllers[I].Buttons[3];
      if ButtonIndex = 4 then Button := @ControllerSettings.Controllers[I].Buttons[4];
      if ButtonIndex = 5 then Button := @ControllerSettings.Controllers[I].Buttons[5];
      if Button^.Device = -1 then Continue;
      if Button^.Turbo then TurboOn[ButtonIndex] := not TurboOn[ButtonIndex]
      else TurboOn[ButtonIndex] := True;
case DIDevices[OpenDevices[Button^.Device].DeviceIndex].DeviceType and $FF of
  3: if (OpenDevices[Button^.Device].KeyboardState[Button^.Button] and $80 <> 0) and TurboOn[ButtonIndex] then
    Controllers^[I].Buttons := Controllers^[I].Buttons or Mask
  else
    Controllers^[I].Buttons := Controllers^[I].Buttons and not Mask;
  4: if (OpenDevices[Button^.Device].JoyState.rgbButtons[Button^.Button - 1] and $80 <> 0) and TurboOn[ButtonIndex] then
    Controllers^[I].Buttons := Controllers^[I].Buttons or Mask
  else
    Controllers^[I].Buttons := Controllers^[I].Buttons and not Mask;
  2: if (OpenDevices[Button^.Device].MouseState.rgbButtons[Button^.Button - 1] and $80 <> 0) and TurboOn[ButtonIndex] then
    Controllers^[I].Buttons := Controllers^[I].Buttons or Mask
  else
    Controllers^[I].Buttons := Controllers^[I].Buttons and not Mask;
end;
    end;
end;

procedure UpdateAnalogAxis(AxisIndex: Integer; Mask: Cardinal; Shift: Byte; Invert: Boolean);
var
  Axis: PAxisMapping;
  Speed1, Speed2, Speed3: PButtonMapping;
  I, Value, Speed, Delta: Integer;
  Pressed: Boolean;
  PackedValue: Cardinal;
begin
  for I := 0 to 3 do
    begin
      if not ControllerSettings.Controllers[I].Enabled then Continue;
      Axis := nil;
      if AxisIndex = 0 then Axis := @ControllerSettings.Controllers[I].Axes[0];
      if AxisIndex = 1 then Axis := @ControllerSettings.Controllers[I].Axes[1];
      Speed1 := @ControllerSettings.Controllers[I].Modifiers[0];
      Speed2 := @ControllerSettings.Controllers[I].Modifiers[1];
      Speed3 := @ControllerSettings.Controllers[I].Modifiers[2];
      if Axis^.Analog then
      begin
        Value := 0;
        case DIDevices[OpenDevices[Axis^.Device].DeviceIndex].DeviceType and $FF of
          4:
            begin
              if Axis^.Device = -1 then Continue;
              if Axis^.Axis = 1 then Value := OpenDevices[Axis^.Device].JoyState.lX;
              if Axis^.Axis = 2 then Value := OpenDevices[Axis^.Device].JoyState.lY;
              if Axis^.Axis = 3 then Value := OpenDevices[Axis^.Device].JoyState.lZ;
              if Invert then Value := -Value;
            end;
          2:
            begin
              if Axis^.Device = -1 then Continue;
              if Axis^.Axis = 1 then Value := OpenDevices[Axis^.Device].MouseState.lX;
              if Axis^.Axis = 2 then Value := OpenDevices[Axis^.Device].MouseState.lY;
              if Axis^.Axis = 3 then Value := OpenDevices[Axis^.Device].MouseState.lZ;
              if Invert then Value := -Value * 7 else Value := Value * 7;
            end;
        end;
        if Value > 126 then Value := 126;
        if Value < -126 then Value := -126;
        PackedValue := (Value and $FF) shl Shift;
        Controllers^[I].Buttons := PackedValue or
          (Controllers^[I].Buttons and not Mask);
      end
      else
      begin
        Value := ShortInt((Controllers^[I].Buttons and Mask) shr Shift);
        Pressed := False;
        Speed := ControllerSettings.DefaultSensitivity;
        if Speed1^.Device <> -1 then
case DIDevices[OpenDevices[Speed1^.Device].DeviceIndex].DeviceType and $FF of
  3: if (OpenDevices[Speed1^.Device].KeyboardState[Speed1^.Button] and $80 <> 0) then
    Speed := ControllerSettings.Modifier1Sensitivity;
  4: if (OpenDevices[Speed1^.Device].JoyState.rgbButtons[Speed1^.Button - 1] and $80 <> 0) then
    Speed := ControllerSettings.Modifier1Sensitivity;
  2: if (OpenDevices[Speed1^.Device].MouseState.rgbButtons[Speed1^.Button - 1] and $80 <> 0) then
    Speed := ControllerSettings.Modifier1Sensitivity;
end;
        if Speed2^.Device <> -1 then
case DIDevices[OpenDevices[Speed2^.Device].DeviceIndex].DeviceType and $FF of
  3: if (OpenDevices[Speed2^.Device].KeyboardState[Speed2^.Button] and $80 <> 0) then
    Speed := ControllerSettings.Modifier2Sensitivity;
  4: if (OpenDevices[Speed2^.Device].JoyState.rgbButtons[Speed2^.Button - 1] and $80 <> 0) then
    Speed := ControllerSettings.Modifier2Sensitivity;
  2: if (OpenDevices[Speed2^.Device].MouseState.rgbButtons[Speed2^.Button - 1] and $80 <> 0) then
    Speed := ControllerSettings.Modifier2Sensitivity;
end;
        if Speed3^.Device <> -1 then
case DIDevices[OpenDevices[Speed3^.Device].DeviceIndex].DeviceType and $FF of
  3: if (OpenDevices[Speed3^.Device].KeyboardState[Speed3^.Button] and $80 <> 0) then
    Speed := ControllerSettings.Modifier3Sensitivity;
  4: if (OpenDevices[Speed3^.Device].JoyState.rgbButtons[Speed3^.Button - 1] and $80 <> 0) then
    Speed := ControllerSettings.Modifier3Sensitivity;
  2: if (OpenDevices[Speed3^.Device].MouseState.rgbButtons[Speed3^.Button - 1] and $80 <> 0) then
    Speed := ControllerSettings.Modifier3Sensitivity;
end;
        if Invert then Delta := -20 * Speed div 100
        else Delta := 20 * Speed div 100;
        if Axis^.Negative.Device <> -1 then
case DIDevices[OpenDevices[Axis^.Negative.Device].DeviceIndex].DeviceType and $FF of
  3: if (OpenDevices[Axis^.Negative.Device].KeyboardState[Axis^.Negative.Button] and $80 <> 0) then
    begin
      Dec(Value, Delta);
      Pressed := True;
    end;
  4: if (OpenDevices[Axis^.Negative.Device].JoyState.rgbButtons[Axis^.Negative.Button - 1] and $80 <> 0) then
    begin
      Dec(Value, Delta);
      Pressed := True;
    end;
  2: if (OpenDevices[Axis^.Negative.Device].MouseState.rgbButtons[Axis^.Negative.Button - 1] and $80 <> 0) then
    begin
      Dec(Value, Delta);
      Pressed := True;
    end;
end;
        if Axis^.Positive.Device <> -1 then
case DIDevices[OpenDevices[Axis^.Positive.Device].DeviceIndex].DeviceType and $FF of
  3: if (OpenDevices[Axis^.Positive.Device].KeyboardState[Axis^.Positive.Button] and $80 <> 0) then
    begin
      Inc(Value, Delta);
      Pressed := True;
    end;
  4: if (OpenDevices[Axis^.Positive.Device].JoyState.rgbButtons[Axis^.Positive.Button - 1] and $80 <> 0) then
    begin
      Inc(Value, Delta);
      Pressed := True;
    end;
  2: if (OpenDevices[Axis^.Positive.Device].MouseState.rgbButtons[Axis^.Positive.Button - 1] and $80 <> 0) then
    begin
      Inc(Value, Delta);
      Pressed := True;
    end;
end;
        if not Pressed then
          if Abs(Value) < 20 * Speed div 100 then Value := 0
          else if Value > 0 then Dec(Value, 20 * Speed div 100)
          else if Value < 0 then Inc(Value, 20 * Speed div 100);
        if Value > 126 * Speed div 100 then Value := 126 * Speed div 100;
        if Value < -126 * Speed div 100 then Value := -126 * Speed div 100;
        PackedValue := (Value and $FF) shl Shift;
        Controllers^[I].Buttons := PackedValue or
          (Controllers^[I].Buttons and not Mask);
      end;
    end;
end;

procedure UpdateDigitalAxis(AxisIndex: Integer; NegativeMask, PositiveMask: Cardinal);
var
  I: Integer;
  Axis: PAxisMapping;
  Value: Integer;
begin
  for I := 0 to 3 do
    if ControllerSettings.Controllers[I].Enabled then
    begin
      Axis := nil;
      if AxisIndex = 0 then Axis := @ControllerSettings.Controllers[I].DigitalAxes[0];
      if AxisIndex = 1 then Axis := @ControllerSettings.Controllers[I].DigitalAxes[1];
      if AxisIndex = 2 then Axis := @ControllerSettings.Controllers[I].DigitalAxes[2];
      if AxisIndex = 3 then Axis := @ControllerSettings.Controllers[I].DigitalAxes[3];
      Value := 0;
      if Axis^.Analog then
      begin
        if Axis^.Device = -1 then Continue;
        if Axis^.Axis = 1 then Value := OpenDevices[Axis^.Device].JoyState.lX;
        if Axis^.Axis = 2 then Value := OpenDevices[Axis^.Device].JoyState.lY;
        if Axis^.Axis = 3 then Value := OpenDevices[Axis^.Device].JoyState.lZ;
        if Value > 126 then Value := 126;
        if Value < -126 then Value := -126;
      end
      else
      begin
        Value := 0;
        if Axis^.Negative.Device <> -1 then
case DIDevices[OpenDevices[Axis^.Negative.Device].DeviceIndex].DeviceType and $FF of
  3: if (OpenDevices[Axis^.Negative.Device].KeyboardState[Axis^.Negative.Button] and $80 <> 0) then
    Value := -126;
  4: if (OpenDevices[Axis^.Negative.Device].JoyState.rgbButtons[Axis^.Negative.Button - 1] and $80 <> 0) then
    Value := -126;
end;
        if Axis^.Positive.Device <> -1 then
case DIDevices[OpenDevices[Axis^.Positive.Device].DeviceIndex].DeviceType and $FF of
  3: if (OpenDevices[Axis^.Positive.Device].KeyboardState[Axis^.Positive.Button] and $80 <> 0) then
    Value := 126;
  4: if (OpenDevices[Axis^.Positive.Device].JoyState.rgbButtons[Axis^.Positive.Button - 1] and $80 <> 0) then
    Value := 126;
end;
      end;
      if Value = 0 then
        Controllers^[I].Buttons := Controllers^[I].Buttons and not (NegativeMask or PositiveMask)
      else if Value > 0 then
        Controllers^[I].Buttons := Controllers^[I].Buttons or (not NegativeMask and PositiveMask)
      else
        Controllers^[I].Buttons := Controllers^[I].Buttons or (not PositiveMask and NegativeMask);
    end;
end;

procedure CIGetJoyData; cdecl;
var
  I: Integer;
  HR: HRESULT;
begin
  while CurrentlyConfiguring do
    Windows.Sleep(1);
  for I := 0 to OpenDevicesCount - 1 do
    case DIDevices[OpenDevices[I].DeviceIndex].DeviceType and $FF of
      DIDEVTYPE_KEYBOARD:
        begin
          FillChar(OpenDevices[I].KeyboardState, 256, 0);
          HR := OpenDevices[I].Device.GetDeviceState(256, OpenDevices[I].KeyboardState);
          if HR <> 0 then
          begin
            FillChar(OpenDevices[I].KeyboardState, 256, 0);
            if (HR = DIERR_NOTACQUIRED) or (HR = DIERR_INPUTLOST) then
              OpenDevices[I].Device.Acquire
            else
              MessageBox(0, PChar(DIErrorString(HR)), '', 0);
          end;
        end;
      DIDEVTYPE_JOYSTICK:
        begin
          DIErrorCheck(OpenDevices[I].Device.Poll);
          FillChar(OpenDevices[I].JoyState, SizeOf(TDJoyState), 0);
          DIErrorCheck(OpenDevices[I].Device.GetDeviceState(SizeOf(TDJoyState), OpenDevices[I].JoyState));
        end;
      DIDEVTYPE_MOUSE:
        begin
          FillChar(OpenDevices[I].MouseState, SizeOf(TDIMouseState), 0);
          HR := OpenDevices[I].Device.GetDeviceState(SizeOf(TDIMouseState), OpenDevices[I].MouseState);
          if HR <> 0 then
          begin
            FillChar(OpenDevices[I].MouseState, SizeOf(TDIMouseState), 0);
            if (HR = DIERR_NOTACQUIRED) or (HR = DIERR_INPUTLOST) then
              OpenDevices[I].Device.Acquire
            else
              MessageBox(0, PChar(DIErrorString(HR)), '', 0);
          end;
        end;
    end;
  UpdateButton(0, $80000000);
  UpdateButton(1, $40000000);
  UpdateButton(2, $20000000);
  UpdateButton(3, $10000000);
  UpdateButton(4, $00200000);
  UpdateButton(5, $00100000);
  UpdateAnalogAxis(0, $0000FF00, 8, False);
  UpdateAnalogAxis(1, $000000FF, 0, True);
  UpdateDigitalAxis(0, $02000000, $01000000);
  UpdateDigitalAxis(1, $08000000, $04000000);
  UpdateDigitalAxis(2, $00020000, $00010000);
  UpdateDigitalAxis(3, $00080000, $00040000);
end;

procedure CIConfigure(Window: HWND); cdecl;
var
  I: Integer;
begin
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
  FormSettings.ShowModal;
  for I := 0 to 3 do
  begin
    Controllers^[I].PluggedIn := ControllerSettings.Controllers[I].Enabled;
    Controllers^[I].MemPack := ControllerSettings.Controllers[I].MemPack;
    Controllers^[I].RumblePack := ControllerSettings.Controllers[I].RumblePack;
  end;
end;

exports
  CIInit,
  CIFree,
  CIGetJoyData,
  CIRumble,
  CIConfigure;

begin
end.
