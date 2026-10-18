unit UnitCamera;

interface

uses Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, Buttons, ComCtrls, UnitDecs;

type
  TFormCamera = class(TForm)
    GroupBoxPan: TGroupBox;
    SpeedButtonPanUp: TSpeedButton;
    SpeedButtonPanDown: TSpeedButton;
    SpeedButtonPanRight: TSpeedButton;
    SpeedButtonPanLeft: TSpeedButton;
    GroupBox1: TGroupBox;
    SpeedButtonZUp: TSpeedButton;
    SpeedButtonZDown: TSpeedButton;
    GroupBox2: TGroupBox;
    SpeedButtonRotateUp: TSpeedButton;
    SpeedButtonRotateDown: TSpeedButton;
    SpeedButtonRotateRight: TSpeedButton;
    SpeedButtonRotateLeft: TSpeedButton;
    GroupBox3: TGroupBox;
    SpeedButtonRotateZLeft: TSpeedButton;
    SpeedButtonRotateZRight: TSpeedButton;
    ButtonCancel: TButton;
    UpDownUnits: TUpDown;
    EditUnits: TEdit;
    ButtonReset: TButton;
    procedure SpeedButtonPanDownClick(Sender: TObject);
    procedure SpeedButtonPanUpClick(Sender: TObject);
    procedure SpeedButtonPanLeftClick(Sender: TObject);
    procedure SpeedButtonPanRightClick(Sender: TObject);
    procedure SpeedButtonZUpClick(Sender: TObject);
    procedure SpeedButtonZDownClick(Sender: TObject);
    procedure SpeedButtonRotateUpClick(Sender: TObject);
    procedure SpeedButtonRotateDownClick(Sender: TObject);
    procedure SpeedButtonRotateRightClick(Sender: TObject);
    procedure SpeedButtonRotateLeftClick(Sender: TObject);
    procedure SpeedButtonRotateZLeftClick(Sender: TObject);
    procedure SpeedButtonRotateZRightClick(Sender: TObject);
    procedure ButtonCancelClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FormKeyPress(Sender: TObject; var Key: Char);
    procedure ButtonResetClick(Sender: TObject);
  private
    procedure WMShow(var Message: TMessage); message WM_USER + 2;
  end;

var
  FormCamera: TFormCamera;
  CameraFormHandle: THandle;

implementation

uses UnitMain;

{$R *.dfm}

procedure TFormCamera.WMShow(var Message: TMessage);
begin
  Visible := True;
end;

procedure TFormCamera.SpeedButtonPanDownClick(Sender: TObject);
begin
  Nemu64_API.CombineModes^^.AddY := Nemu64_API.CombineModes^^.AddY - UpDownUnits.Position;
end;

procedure TFormCamera.SpeedButtonPanUpClick(Sender: TObject);
begin
  Nemu64_API.CombineModes^^.AddY := Nemu64_API.CombineModes^^.AddY + UpDownUnits.Position;
end;

procedure TFormCamera.SpeedButtonPanLeftClick(Sender: TObject);
begin
  Nemu64_API.CombineModes^^.AddX := Nemu64_API.CombineModes^^.AddX - UpDownUnits.Position;
end;

procedure TFormCamera.SpeedButtonPanRightClick(Sender: TObject);
begin
  Nemu64_API.CombineModes^^.AddX := Nemu64_API.CombineModes^^.AddX + UpDownUnits.Position;
end;

procedure TFormCamera.SpeedButtonZUpClick(Sender: TObject);
begin
  Nemu64_API.CombineModes^^.AddZ := Nemu64_API.CombineModes^^.AddZ + UpDownUnits.Position;
end;

procedure TFormCamera.SpeedButtonZDownClick(Sender: TObject);
begin
  Nemu64_API.CombineModes^^.AddZ := Nemu64_API.CombineModes^^.AddZ - UpDownUnits.Position;
end;

procedure TFormCamera.SpeedButtonRotateUpClick(Sender: TObject);
begin
  Nemu64_API.CombineModes^^.RotX := Nemu64_API.CombineModes^^.RotX + Pi / 9;
end;

procedure TFormCamera.SpeedButtonRotateDownClick(Sender: TObject);
begin
  Nemu64_API.CombineModes^^.RotX := Nemu64_API.CombineModes^^.RotX - Pi / 9;
end;

procedure TFormCamera.SpeedButtonRotateRightClick(Sender: TObject);
begin
  Nemu64_API.CombineModes^^.RotY := Nemu64_API.CombineModes^^.RotY + Pi / 9;
end;

procedure TFormCamera.SpeedButtonRotateLeftClick(Sender: TObject);
begin
  Nemu64_API.CombineModes^^.RotY := Nemu64_API.CombineModes^^.RotY - Pi / 9;
end;

procedure TFormCamera.SpeedButtonRotateZLeftClick(Sender: TObject);
begin
  Nemu64_API.CombineModes^^.RotZ := Nemu64_API.CombineModes^^.RotZ - Pi / 9;
end;

procedure TFormCamera.SpeedButtonRotateZRightClick(Sender: TObject);
begin
  Nemu64_API.CombineModes^^.RotZ := Nemu64_API.CombineModes^^.RotZ + Pi / 9;
end;

procedure TFormCamera.ButtonCancelClick(Sender: TObject);
begin
  Close;
end;

procedure TFormCamera.FormCreate(Sender: TObject);
begin
  CameraFormHandle := Handle;
end;

procedure TFormCamera.FormKeyPress(Sender: TObject; var Key: Char);
begin
  if Key = '+' then SpeedButtonZUp.Click;
  if Key = '-' then SpeedButtonZDown.Click;
end;

procedure TFormCamera.ButtonResetClick(Sender: TObject);
begin
  Nemu64_API.CombineModes^^.AddX := 0;
  Nemu64_API.CombineModes^^.AddY := 0;
  Nemu64_API.CombineModes^^.AddZ := 0;
  Nemu64_API.CombineModes^^.RotX := 0;
  Nemu64_API.CombineModes^^.RotY := 0;
  Nemu64_API.CombineModes^^.RotZ := 0;
end;

end.
