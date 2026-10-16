unit UnitSymbolViewer;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls, Menus, Buttons;

type
  TFormSymbols = class(TForm)
    ListBoxSymbols: TListBox;
    PanelBottom: TPanel;
    ButtonClose: TButton;
    ButtonDebug: TButton;
    procedure ButtonCloseClick(Sender: TObject);
    procedure ListBoxSymbolsDblClick(Sender: TObject);
  end;

var
  FormSymbols: TFormSymbols;

implementation

uses UnitDebugCommands;

{$R *.dfm}

procedure TFormSymbols.ButtonCloseClick(Sender: TObject);
begin
  Close;
end;

procedure TFormSymbols.ListBoxSymbolsDblClick(Sender: TObject);
var
  S: string;
begin
  if ListBoxSymbols.ItemIndex <> -1 then
  begin
    S := ListBoxSymbols.Items[ListBoxSymbols.ItemIndex];
    FormDebug.EditCPUPC.Text := Copy(S, Length(S) - 7, 8);
    FormDebug.Show;
  end;
end;

end.
