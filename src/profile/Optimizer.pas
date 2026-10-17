unit Optimizer;

interface

uses Windows, Messages, SysUtils, Classes, Graphics, Controls;

type
  TOptimizer = class(TGraphicControl)
  private
    { Private field names are inferred from the original layout and uses. }
    FProcesses: TStringList;
    FProcess: Integer;
    FFrequency, FStart, FStop: Int64;
    FRunning: Boolean;
    FTimes: array[0..100] of Int64;
    FTotal: Int64;
    FTextWidth: Integer;
    procedure SetProcesses(Value: TStringList);
  protected
    procedure Paint; override;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
  published
    procedure Start(Process: Integer);
    procedure Stop;
    procedure Reset;
    property Processes: TStringList read FProcesses write SetProcesses;
  end;

implementation

procedure TOptimizer.Stop;
begin
  if FRunning then
  begin
    QueryPerformanceCounter(FStop);
    FTimes[FProcess] := FTimes[FProcess] + (FStop - FStart);
    FTotal := FTotal + (FStop - FStart);
    FRunning := False;
  end;
end;

procedure TOptimizer.Start(Process: Integer);
begin
  Stop;
  FProcess := Process;
  QueryPerformanceCounter(FStart);
  FRunning := True;
end;

procedure TOptimizer.Paint;
var
  I, H: Integer;
begin
  FTextWidth := 0;
  for I := 0 to FProcesses.Count - 1 do
    if Canvas.TextWidth(FProcesses[I]) > FTextWidth then
      FTextWidth := Canvas.TextWidth(FProcesses[I]);
  Inc(FTextWidth, 4);
  H := Canvas.TextHeight('O');
  for I := 0 to FProcesses.Count - 1 do
  begin
    Canvas.Brush.Style := bsClear;
    Canvas.TextOut(0, I * H, FProcesses[I]);
    Canvas.Brush.Color := clWhite;
    Canvas.Brush.Style := bsSolid;
    Canvas.Rectangle(FTextWidth, I * H + 1, Width, (I + 1) * H - 1);
    Canvas.Brush.Color := $FF8080;
    if FTotal <> 0 then
      Canvas.FillRect(Rect(FTextWidth + 1, I * H + 2,
        Round(FTextWidth + 1 + (Width - FTextWidth - 2) * FTimes[I] / FTotal),
        (I + 1) * H - 2));
  end;
end;

procedure TOptimizer.Reset;
var
  I: Integer;
begin
  Stop;
  for I := 0 to 100 do FTimes[I] := 0;
  FTotal := 0;
  FRunning := False;
end;

destructor TOptimizer.Destroy;
begin
  FProcesses.Free;
  inherited Destroy;
end;

constructor TOptimizer.Create(AOwner: TComponent);
begin
  inherited Create(AOwner);
  FProcesses := TStringList.Create;
  Width := 100;
  Height := 40;
  QueryPerformanceFrequency(FFrequency);
  Reset;
end;

procedure TOptimizer.SetProcesses(Value: TStringList);
begin
  FTextWidth := 0;
  FProcesses.Assign(Value);
  Invalidate;
end;

end.
