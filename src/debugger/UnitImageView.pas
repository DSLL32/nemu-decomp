unit UnitImageView;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls, Menus, Buttons;

type
  TFormImageView = class(TForm)
    ImageSizer: TImage;
    PaintBox: TPaintBox;
    PanelTop: TPanel;
    LabelAddress: TLabel;
    EditMemAddress: TEdit;
    ScrollBar: TScrollBar;
    chkRefreshEveryFrame: TCheckBox;
    ComboBoxFormat: TComboBox;
    Button1: TButton;
    procedure ScrollBarChange(Sender: TObject);
    procedure PaintBoxPaint(Sender: TObject);
    procedure ImageSizerMouseDown(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
    procedure ImageSizerMouseMove(Sender: TObject; Shift: TShiftState; X, Y: Integer);
    procedure ImageSizerMouseUp(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
    procedure FormResize(Sender: TObject);
    procedure chkRefreshEveryFrameClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure FormCreate(Sender: TObject);
    procedure EditMemAddressChange(Sender: TObject);
    procedure ComboBoxFormatChange(Sender: TObject);
  private
    Address: Cardinal;
    Resizing: Boolean;
    ResizePoint: TPoint;
  end;

var
  FormImageView: TFormImageView;
  RefreshAfterEveryFrame: Boolean;

implementation

uses UnitMain, UnitDecs, UnitBookmarks;

{$R *.dfm}

procedure TFormImageView.ScrollBarChange(Sender: TObject);
begin
  Address := ScrollBar.Position shl 2;
  EditMemAddress.OnChange := nil;
  EditMemAddress.Text := IntToHex(Int64(Address), 8);
  EditMemAddress.OnChange := EditMemAddressChange;
  PaintBoxPaint(PaintBox);
end;

procedure TFormImageView.PaintBoxPaint(Sender: TObject);
var
  W, H, X, Y, Value: Cardinal;
  Bitmap: TBitmap;
  Line: PCardinalArray;
begin
  if PaintBox.Width > 0 then
  begin
    W := PaintBox.Width div 2;
    H := PaintBox.Height div 2;
    Bitmap := TBitmap.Create;
    Bitmap.PixelFormat := pf32bit;
    Bitmap.Width := PaintBox.Width;
    Bitmap.Height := PaintBox.Height;
    case ComboBoxFormat.ItemIndex of
      0:
        if Address + H * W shr 1 >= Nemu64_API.RamSize^ then
          Bitmap.Canvas.TextOut(0, 0, 'RDRAM is not that big!')
        else
          for Y := 0 to H - 1 do
          begin
            Line := Bitmap.ScanLine[Y];
            for X := 0 to W shr 1 - 1 do
            begin
              Value := Nemu64_API.RDRAM^[(Address + (Y * W + X) shr 1) xor 3] shr 4;
              Line^[X * 2 + 1] := RGB(Value shl 4, Value shl 4, Value shl 4);
              Value := Nemu64_API.RDRAM^[(Address + (Y * W + X) shr 1) xor 3] and $F;
              Line^[X * 2] := RGB(Value shl 4, Value shl 4, Value shl 4);
            end;
          end;
      1:
        if Address + H * W >= Nemu64_API.RamSize^ then
          Bitmap.Canvas.TextOut(0, 0, 'RDRAM is not that big!')
        else
          for Y := 0 to H - 1 do
          begin
            Line := Bitmap.ScanLine[Y];
            for X := 0 to W - 1 do
            begin
              Value := Nemu64_API.RDRAM^[(Address + (Y * W + X)) xor 2];
              Line^[X] := RGB(Value, Value, Value);
            end;
          end;
      2:
        if Address + H * W * 2 >= Nemu64_API.RamSize^ then
          Bitmap.Canvas.TextOut(0, 0, 'RDRAM is not that big!')
        else
          for Y := 0 to H - 1 do
          begin
            Line := Bitmap.ScanLine[Y];
            for X := 0 to W - 1 do
            begin
              Value := Nemu64_API.RDRAM^[(Address + (Y * W + X) * 2) xor 2] or
                (Nemu64_API.RDRAM^[(Address + (Y * W + X) * 2 + 1) xor 2] shl 8);
              Value := 255 - (Value and $FF);
              Line^[X] := RGB(Value, Value, Value);
            end;
          end;
    end;
    StretchBlt(PaintBox.Canvas.Handle, 0, 0, Bitmap.Width, Bitmap.Height,
      Bitmap.Canvas.Handle, 0, 0, Bitmap.Width div 2, Bitmap.Height div 2,
      SRCCOPY);
    Bitmap.Free;
  end;
end;

procedure TFormImageView.ImageSizerMouseDown(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  Resizing := True;
  GetCursorPos(ResizePoint);
  OnResize := nil;
end;

procedure TFormImageView.ImageSizerMouseMove(Sender: TObject; Shift: TShiftState; X, Y: Integer);
var
  P: TPoint;
  DX, DY: Integer;
begin
  if Resizing then
  begin
    GetCursorPos(P);
    DX := P.X - ResizePoint.X;
    DY := P.Y - ResizePoint.Y;
    ImageSizer.Left := ImageSizer.Left + DX;
    if ImageSizer.Left < 2 then
      ImageSizer.Left := 2;
    ImageSizer.Top := ImageSizer.Top + DY;
    if ImageSizer.Top < 2 then
      ImageSizer.Top := 2;
    ResizePoint := P;
    PaintBox.Width := ImageSizer.Left and not 1;
    PaintBox.Height := (ImageSizer.Top - PanelTop.Height) and not 1;
  end;
end;

procedure TFormImageView.ImageSizerMouseUp(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
begin
  Resizing := False;
  OnResize := FormResize;
end;

procedure TFormImageView.FormResize(Sender: TObject);
begin
end;

procedure TFormImageView.chkRefreshEveryFrameClick(Sender: TObject);
begin
  RefreshAfterEveryFrame := chkRefreshEveryFrame.Checked;
end;

procedure TFormImageView.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Action := caHide;
  chkRefreshEveryFrame.Checked := False;
end;

procedure TFormImageView.FormCreate(Sender: TObject);
begin
  RefreshAfterEveryFrame := False;
end;

procedure TFormImageView.EditMemAddressChange(Sender: TObject);
var
  NewAddress: Integer;
begin
  NewAddress := StrToIntDef('$' + EditMemAddress.Text, -1);
  if (NewAddress <> -1) and (Nemu64_API.RamSize^ > Cardinal(NewAddress)) then
  begin
    Address := NewAddress;
    ScrollBar.OnChange := nil;
    ScrollBar.Position := Address shr 2;
    ScrollBar.OnChange := ScrollBarChange;
    PaintBoxPaint(PaintBox);
  end;
end;

procedure TFormImageView.ComboBoxFormatChange(Sender: TObject);
begin
  PaintBoxPaint(PaintBox);
end;

initialization
finalization
end.
