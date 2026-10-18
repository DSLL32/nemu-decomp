library LemCombineDebugger;

{$IMPLICITBUILD OFF}

uses
  SysUtils, Windows, Messages, Classes,
  UnitMain in 'UnitMain.pas',
  UnitDecs in 'UnitDecs.pas',
  UnitCombiners in 'UnitCombiners.pas',
  UnitCombineEditor in 'UnitCombineEditor.pas',
  RCP in 'RCP.pas',
  UnitCamera in 'UnitCamera.pas';

function ToolInit(Info: PNemu): Integer; cdecl;
begin
  Result := 5;
  Move(Info^, Nemu64_API, SizeOf(Nemu64_API));
  NemuMainWinHandle := THandle(Nemu64_API.RDRAM);
  FormCombiners := TFormCombiners.Create(nil);
  FormCombiners.Left := -1000;
  FormCombiners.Visible := True;
  FormCombiners.Visible := False;
  FormCombiners.Left := 192;
  FormEditor := TFormEditor.Create(nil);
  FormEditor.Left := -1000;
  FormEditor.Visible := True;
  FormEditor.Visible := False;
  FormEditor.Left := 192;
  FormCamera := TFormCamera.Create(nil);
  FormCamera.Left := -1000;
  FormCamera.Visible := True;
  FormCamera.Visible := False;
  FormCamera.Left := 192;
  GetDir(0, CombineFilename);
  if Length(CombineFilename) > 0 then
    if CombineFilename[Length(CombineFilename)] <> '\' then
      CombineFilename := CombineFilename + '\';
  NewCombineFilename := CombineFilename + 'LemD3DCombine.new';
  CombineFilename := CombineFilename + 'LemD3DCombine.dat';
  AssignFile(CombineFile, CombineFilename);
  AssignFile(NewFile, NewCombineFilename);
  if not FileExists(CombineFilename) then Rewrite(CombineFile);
  if not FileExists(NewCombineFilename) then Rewrite(NewFile);
end;

procedure ToolFree; cdecl;
begin
  FormCamera.Free;
  FormEditor.Free;
  FormCombiners.Free;
end;

procedure ToolFunc(Reason, Index: Integer); cdecl;
begin
  case Reason of
    1: case Index of
      0: begin
        PostMessage(CombinerFormHandle, WM_USER + 2, 0, 0);
        PostMessage(CombinerFormHandle, WM_USER + 1, 0, 0);
      end;
      1: PostMessage(CameraFormHandle, WM_USER + 2, 0, 0);
    end;
    4: PostMessage(CombinerFormHandle, WM_USER + 1, 0, 0);
  end;
end;

procedure ToolGetMenuText(Text: PChar); cdecl;
begin
  StrCopy(Text, 'Show Combine modes...'#13'Free camera movement...');
end;

exports
  ToolFunc index 1,
  ToolGetMenuText index 2,
  ToolFree index 3,
  ToolInit index 4;

begin
end.
