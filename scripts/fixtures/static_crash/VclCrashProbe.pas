unit VclCrashProbe;
interface
uses Classes;
type
  TCrashClick = class
    procedure CrashClick(Sender: TObject);
  end;
procedure RunVclCrash; cdecl;
implementation
uses Windows, Messages, Forms, StdCtrls;
procedure TCrashClick.CrashClick(Sender: TObject);
begin
  PInteger(nil)^ := 1;
end;
procedure RunVclCrash;
var Form: TForm; Button: TButton; Handler: TCrashClick; Msg: TMsg;
begin
  Form := TForm.Create(nil);
  Form.Caption := 'VCL crash probe';
  Button := TButton.Create(Form);
  Button.Parent := Form;
  Handler := TCrashClick.Create;
  Button.OnClick := Handler.CrashClick;
  Form.Show;
  PostMessage(Button.Handle, BM_CLICK, 0, 0);
  while GetMessage(Msg, 0, 0, 0) do
  begin
    TranslateMessage(Msg);
    DispatchMessage(Msg);
  end;
end;
end.
