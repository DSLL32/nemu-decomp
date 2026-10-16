unit UnitLibOptions;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls, Menus, Buttons;

type
  TFormLibOptions = class(TForm)
    CheckBoxAutoloadCodFiles: TCheckBox;
    PanelLoad: TPanel;
    EditCodFolder: TEdit;
    LabelCodFolder: TLabel;
    GroupBoxLFE: TGroupBox;
    ButtonSave: TButton;
    EditLFE: TEdit;
    LabelLFE: TLabel;
    LabelLFECommandline: TLabel;
    EditLFECommandline: TEdit;
    procedure ButtonSaveClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
  end;

var
  FormLibOptions: TFormLibOptions;

implementation

uses Registry;

{$R *.dfm}

procedure TFormLibOptions.ButtonSaveClick(Sender: TObject);
var
  Reg: TRegistry;
begin
  Reg := TRegistry.Create;
  Reg.RootKey := HKEY_CURRENT_USER;
  Reg.OpenKey('\Software\Nemu\0.8\Debugger\', True);
  Reg.WriteBool('AutoLoad', CheckBoxAutoloadCodFiles.Checked);
  Reg.WriteString('CodFolder', EditCodFolder.Text);
  Reg.WriteString('LFE', EditLFE.Text);
  Reg.WriteString('LFECommandLine', EditLFECommandline.Text);
  Reg.Free;
  Close;
end;

procedure TFormLibOptions.FormCreate(Sender: TObject);
var
  Reg: TRegistry;
begin
  Reg := TRegistry.Create;
  Reg.RootKey := HKEY_CURRENT_USER;
  Reg.OpenKey('\Software\Nemu\0.8\Debugger\', True);
  if Reg.ValueExists('AutoLoad') then
  begin
    CheckBoxAutoloadCodFiles.Checked := Reg.ReadBool('AutoLoad');
    EditCodFolder.Text := Reg.ReadString('CodFolder');
    EditLFE.Text := Reg.ReadString('LFE');
    EditLFECommandline.Text := Reg.ReadString('LFECommandLine');
  end;
  Reg.Free;
end;

end.
