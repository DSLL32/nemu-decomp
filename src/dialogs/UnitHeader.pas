unit UnitHeader;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls;

type
  TFormHeader = class(TForm)
    ButtonClose: TButton;
    GroupBoxRomInfo: TGroupBox;
    LabelEntryPointTitel: TLabel;
    LabelCRC1Titel: TLabel;
    LabelCRC2Titel: TLabel;
    LabelCartnameTitel: TLabel;
    EditCartname: TEdit;
    LabelManufacturerIDTitel: TLabel;
    LabelCartIDTitel: TLabel;
    LabelCartID: TLabel;
    LabelCountryCodeTitel: TLabel;
    LabelFilenameTitel: TLabel;
    EditFilename: TEdit;
    LabelManufacturerID: TLabel;
    LabelCountryCode: TLabel;
    EditEntrypoint: TEdit;
    EditCRC1: TEdit;
    EditCRC2: TEdit;
    GroupBox1: TGroupBox;
    LabelTitle: TLabel;
    LabelMsg: TLabel;
    Image1: TImage;
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure FormCreate(Sender: TObject);
  public
    procedure UpdateInfo;
  end;

var
  FormHeader: TFormHeader;

implementation

uses UnitMain, UnitDecs, UnitNemuSettings, IniFiles;

{$R *.dfm}

procedure TFormHeader.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Action := caFree;
end;

function ReadHeaderByte(Address: Cardinal): Byte;
var
  Value: Cardinal;
begin
  Value := ReadWord((Address and $FFFFFFFC) + $10000000);
  case Address and 3 of
    0: Result := Value shr 24;
    1: Result := (Value shr 16) and $FF;
    2: Result := (Value shr 8) and $FF;
    3: Result := Value and $FF;
  else
    Result := 0;
  end;
end;

procedure TFormHeader.UpdateInfo;
var
  Manufacturer: Cardinal;
begin
  if Nemu.RomSize^ = 0 then
  begin
    EditFilename.Text := '(no rom loaded)';
    EditEntrypoint.Text := '-';
    EditCRC1.Text := '-';
    EditCRC2.Text := '-';
    EditCartname.Text := '';
    LabelManufacturerID.Caption := '';
    LabelCartID.Caption := '-';
    LabelCountryCode.Caption := '';
  end
  else
  begin
    EditFilename.Text := Nemu.RomFilename;
    EditEntrypoint.Text := '0x' + IntToHex(Nemu.ReadWord($10000008), 8);
    EditCRC1.Text := '0x' + IntToHex(Nemu.ReadWord($10000010), 8);
    EditCRC2.Text := '0x' + IntToHex(Nemu.ReadWord($10000014), 8);
    EditCartname.Text := Nemu.RomName;
    Manufacturer := Nemu.ReadWord($10000038);
    if Manufacturer = 0 then
      LabelManufacturerID.Caption := '(not specified)'
    else
    begin
      case Char(Manufacturer) of
        'N': LabelManufacturerID.Caption := 'Nintendo';
      else
        LabelManufacturerID.Caption := Char(Manufacturer) + ' (0x' +
          IntToHex(Manufacturer, 2) + ')';
      end;
    end;
    LabelCartID.Caption := Char(ReadHeaderByte($3C)) + Char(ReadHeaderByte($3D)) +
      ' (0x' + IntToHex(ReadHeaderByte($3C), 2) + IntToHex(ReadHeaderByte($3D), 2) + ')';
    if ReadHeaderByte($3E) = 0 then
      LabelCountryCode.Caption := '(not specified)'
    else
      case ReadHeaderByte($3E) of
        Ord('D'): LabelCountryCode.Caption := 'D (Germany)';
        Ord('E'): LabelCountryCode.Caption := 'E (USA)';
        Ord('J'): LabelCountryCode.Caption := 'J (Japan)';
        Ord('P'): LabelCountryCode.Caption := 'P (Europe)';
        Ord('U'): LabelCountryCode.Caption := 'U (Australia)';
      else
        LabelCountryCode.Caption := Char(ReadHeaderByte($3E));
      end;
  end;
end;

procedure TFormHeader.FormCreate(Sender: TObject);
begin
  UpdateInfo;
end;

end.
