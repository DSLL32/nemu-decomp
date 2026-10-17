unit UnitProfiling;

interface

uses
  Profiler, Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, Optimizer, ExtCtrls;

type
  TFormProfiler = class(TForm)
    ButtonReset: TButton;
    CheckBoxEnabled: TCheckBox;
    Profiler: TOptimizer;
    Timer: TTimer;
    CheckBoxAccumulate: TCheckBox;
    procedure ButtonResetClick(Sender: TObject);
    procedure CheckBoxEnabledClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure TimerTimer(Sender: TObject);
  private
    procedure WMProfile(var Message: TMessage); message WM_USER + 1;
  end;

var
  FormProfiler: TFormProfiler;
  ProfilerFormHandle: HWND;
  DoProfiling: Boolean;

implementation

uses RCP, UnitDecs, UnitMain;

{$R *.dfm}

procedure TFormProfiler.WMProfile(var Message: TMessage);
begin
  Profiler.Start(Message.WParam);
end;

procedure TFormProfiler.ButtonResetClick(Sender: TObject);
begin
  Profiler.Reset;
end;

procedure TFormProfiler.CheckBoxEnabledClick(Sender: TObject);
begin
  DoProfiling := CheckBoxEnabled.Checked;
  Timer.Enabled := DoProfiling;
end;

procedure TFormProfiler.FormCreate(Sender: TObject);
begin
  DoProfiling := False;
end;

procedure TFormProfiler.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Action := caHide;
end;

procedure TFormProfiler.TimerTimer(Sender: TObject);
begin
  Profiler.Repaint;
  if not CheckBoxAccumulate.Checked then Profiler.Reset;
end;

end.
