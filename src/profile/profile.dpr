library profile;

{$IMPLICITBUILD OFF}

uses
  SysUtils, Windows, Classes,
  RCP in 'RCP.pas',
  UnitDecs in 'UnitDecs.pas',
  UnitMain in 'UnitMain.pas',
  UnitProfiling in 'UnitProfiling.pas';

function ToolInit(Info: PNemu): Integer; cdecl;
begin
  Result := $27D;
  Move(Info^, Nemu64_API, SizeOf(Nemu64_API));
  NemuMainWinHandle := Nemu64_API.Instance;
  FormProfiler := TFormProfiler.Create(nil);
  ProfilerFormHandle := FormProfiler.Handle;
end;

procedure ToolFree; cdecl;
begin
  FormProfiler.Free;
end;

procedure ToolFunc(Reason, Index: Integer); cdecl;
begin
  if not DoProfiling and (Reason <> 1) then Exit;
  case Reason of
    1: case Index of 0: FormProfiler.Show; end;
    4: if Index = 0 then SendMessage(ProfilerFormHandle, $401, 0, 0)
       else SendMessage(ProfilerFormHandle, $401, 1, 0);
    16: if Index = 0 then SendMessage(ProfilerFormHandle, $401, 0, 0)
        else SendMessage(ProfilerFormHandle, $401, 2, 0);
    8: if Index = 0 then SendMessage(ProfilerFormHandle, $401, 0, 0)
       else SendMessage(ProfilerFormHandle, $401, 3, 0);
    32: if Index = 0 then SendMessage(ProfilerFormHandle, $401, 0, 0)
        else SendMessage(ProfilerFormHandle, $401, 4, 0);
    64: if Index = 0 then SendMessage(ProfilerFormHandle, $401, 0, 0)
        else SendMessage(ProfilerFormHandle, $401, 5, 0);
    512: if Index = 0 then SendMessage(ProfilerFormHandle, $401, 0, 0)
         else SendMessage(ProfilerFormHandle, $401, 6, 0);
  end;
end;

procedure ToolGetMenuText(Text: PChar); cdecl;
begin
  StrCopy(Text, 'Show Profiler...');
end;

exports
  ToolFunc index 1,
  ToolGetMenuText index 2,
  ToolFree index 3,
  ToolInit index 4;

begin
end.
