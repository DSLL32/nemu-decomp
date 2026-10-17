unit UnitWaitKey;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls, Buttons, CheckLst;

type
  TFormWait = class(TForm)
    SpeedButtonCancel: TSpeedButton;
    Timer: TTimer;
    PanelStatus: TPanel;
    Label1: TLabel;
    procedure SpeedButtonCancelClick(Sender: TObject);
    procedure TimerTimer(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
  private
    Key: Integer;
  end;

var
  FormWait: TFormWait;

implementation

uses UnitDIStuff, DInput;

{$R *.dfm}

procedure TFormWait.SpeedButtonCancelClick(Sender: TObject);
begin
  ModalResult := mrCancel;
end;

procedure TFormWait.TimerTimer(Sender: TObject);
var
  KeyName: string;
  C: Char;
  I, J: Cardinal;
begin
  if Timer.Tag = 1 then
  begin
    ModalResult := mrOk;
    Exit;
  end;
  Timer.Enabled := False;
  for I := 0 to ActiveDeviceCount - 1 do
    case DIDevices[ActiveDevices[I].DeviceIndex].DeviceType and $FF of
      DIDEVTYPE_KEYBOARD:
        begin
          KeyName := DIDevices[ActiveDevices[I].DeviceIndex].Name + '.';
          if (GetKeyState(Ord('1')) and $80) <> 0 then
            KeyName := KeyName + '1'
          else if (GetKeyState(Ord('2')) and $80) <> 0 then
            KeyName := KeyName + '2'
          else if (GetKeyState(Ord('3')) and $80) <> 0 then
            KeyName := KeyName + '3'
          else if (GetKeyState(Ord('4')) and $80) <> 0 then
            KeyName := KeyName + '4'
          else if (GetKeyState(Ord('5')) and $80) <> 0 then
            KeyName := KeyName + '5'
          else if (GetKeyState(Ord('6')) and $80) <> 0 then
            KeyName := KeyName + '6'
          else if (GetKeyState(Ord('7')) and $80) <> 0 then
            KeyName := KeyName + '7'
          else if (GetKeyState(Ord('8')) and $80) <> 0 then
            KeyName := KeyName + '8'
          else if (GetKeyState(Ord('9')) and $80) <> 0 then
            KeyName := KeyName + '9'
          else if (GetKeyState(Ord('0')) and $80) <> 0 then
            KeyName := KeyName + '0'
          else if (GetKeyState(VK_NUMLOCK) and $80) <> 0 then
            KeyName := KeyName + 'NUMLOCK'
          else if (GetKeyState(VK_TAB) and $80) <> 0 then
            KeyName := KeyName + 'TAB'
          else if (GetKeyState(VK_RETURN) and $80) <> 0 then
            KeyName := KeyName + 'RETURN'
          else if (GetKeyState(VK_LSHIFT) and $80) <> 0 then
            KeyName := KeyName + 'LEFTSHIFT'
          else if (GetKeyState(VK_RSHIFT) and $80) <> 0 then
            KeyName := KeyName + 'RIGHTSHIFT'
          else if (GetKeyState(VK_LCONTROL) and $80) <> 0 then
            KeyName := KeyName + 'LEFTCONTROL'
          else if (GetKeyState(VK_RCONTROL) and $80) <> 0 then
            KeyName := KeyName + 'RIGHTCONTROL'
          else if (GetKeyState(VK_CAPITAL) and $80) <> 0 then
            KeyName := KeyName + 'CAPSLOCK'
          else if (GetKeyState(VK_SPACE) and $80) <> 0 then
            KeyName := KeyName + 'SPACE'
          else if (GetKeyState(VK_LEFT) and $80) <> 0 then
            KeyName := KeyName + 'LEFT (Arrow)'
          else if (GetKeyState(VK_RIGHT) and $80) <> 0 then
            KeyName := KeyName + 'RIGHT (Arrow)'
          else if (GetKeyState(VK_UP) and $80) <> 0 then
            KeyName := KeyName + 'UP (Arrow)'
          else if (GetKeyState(VK_DOWN) and $80) <> 0 then
            KeyName := KeyName + 'DOWN (Arrow)'
          else if (GetKeyState(VK_INSERT) and $80) <> 0 then
            KeyName := KeyName + 'INSERT (Arrow)'
          else if (GetKeyState(VK_DELETE) and $80) <> 0 then
            KeyName := KeyName + 'DELETE (Arrow)'
          else if (GetKeyState(VK_HOME) and $80) <> 0 then
            KeyName := KeyName + 'HOME (Arrow)'
          else if (GetKeyState(VK_END) and $80) <> 0 then
            KeyName := KeyName + 'END (Arrow)'
          else if (GetKeyState(VK_PRIOR) and $80) <> 0 then
            KeyName := KeyName + 'PAGEUP (Arrow)'
          else if (GetKeyState(VK_NEXT) and $80) <> 0 then
            KeyName := KeyName + 'PAGEDOWN (Arrow)'
          else if (GetKeyState(VK_DIVIDE) and $80) <> 0 then
            KeyName := KeyName + '/ (Numpad)'
          else if (GetKeyState(VK_MULTIPLY) and $80) <> 0 then
            KeyName := KeyName + '* (Numpad)'
          else if (GetKeyState(VK_ADD) and $80) <> 0 then
            KeyName := KeyName + '+ (Numpad)'
          else if (GetKeyState(VK_SUBTRACT) and $80) <> 0 then
            KeyName := KeyName + '- (Numpad)'
          else
            for C := 'A' to 'Z' do
              if (GetKeyState(Ord(C)) and $80) <> 0 then
              begin
                KeyName := KeyName + C;
                Break;
              end;
          if KeyName <> DIDevices[ActiveDevices[I].DeviceIndex].Name + '.' then
          begin
            PanelStatus.Caption := KeyName;
            FadeOutForm(Self, 500);
            ModalResult := mrOk;
            Exit;
          end;
        end;
      DIDEVTYPE_JOYSTICK:
        begin
          DIErrorCheck(ActiveDevices[I].Device.Poll);
          FillChar(ActiveDevices[I].JoyState, SizeOf(TDJoyState), 0);
          DIErrorCheck(ActiveDevices[I].Device.GetDeviceState(SizeOf(TDJoyState), ActiveDevices[I].JoyState));
          for J := 0 to ActiveDevices[I].ButtonCount - 1 do
            if (ActiveDevices[I].JoyState.rgbButtons[J] and $80) <> 0 then
            begin
              PanelStatus.Caption := DIDevices[ActiveDevices[I].DeviceIndex].Name + '.' + ActiveDevices[I].ButtonNames[J];
              FadeOutForm(Self, 500);
              ModalResult := mrOk;
              Exit;
            end;
        end;
    end;
  Timer.Enabled := True;
end;

procedure TFormWait.FormCreate(Sender: TObject);
begin
end;

procedure TFormWait.FormClose(Sender: TObject; var Action: TCloseAction);
begin
end;

initialization
finalization
end.
