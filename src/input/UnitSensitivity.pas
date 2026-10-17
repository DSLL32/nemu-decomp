unit UnitSensitivity;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls;

type
  TFormSensitivity = class(TForm)
    LabelDefault: TLabel;
    EditDefault: TEdit;
    UpDownDefault: TUpDown;
    Label2: TLabel;
    Edit1: TEdit;
    UpDown1: TUpDown;
    Label3: TLabel;
    Edit2: TEdit;
    UpDown2: TUpDown;
    Label4: TLabel;
    Edit3: TEdit;
    UpDown3: TUpDown;
    ButtonOK: TButton;
    ButtonCancel: TButton;
    GroupBox1: TGroupBox;
    LabelTitle: TLabel;
    LabelMsg: TLabel;
    BevelBottom: TBevel;
    ImageStick: TImage;
    LabelName: TLabel;
  end;

var
  FormSensitivity: TFormSensitivity;

implementation

{$R *.dfm}

initialization
finalization
end.
