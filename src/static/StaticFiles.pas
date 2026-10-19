unit StaticFiles;

interface

procedure StaticReset(var F: Text; const Filename: string);

implementation

{ In -J object mode this is a direct C external, resolved by native LINK. }
function NemuStaticDefaultData(Filename: PChar; var Size: Cardinal): PChar; cdecl;
  external 'nemu64-static.exe' name 'NemuStaticDefaultData';

type
  PDefaultText = ^TDefaultText;
  TDefaultText = packed record
    Data: PChar;
    Size, Position: Cardinal;
  end;

function ReadDefault(var F: TTextRec): Integer;
var
  State: PDefaultText;
  Count: Cardinal;
begin
  State := @F.UserData;
  Count := State^.Size - State^.Position;
  if Count > F.BufSize then Count := F.BufSize;
  if Count <> 0 then Move(State^.Data[State^.Position], F.BufPtr^, Count);
  Inc(State^.Position, Count);
  F.BufPos := 0;
  F.BufEnd := Count;
  Result := 0;
end;

function CloseDefault(var F: TTextRec): Integer;
begin
  Result := 0;
end;

procedure StaticReset(var F: Text; const Filename: string);
var
  Data: PChar;
  Size: Cardinal;
  State: PDefaultText;
begin
  Data := NemuStaticDefaultData(PChar(Filename), Size);
  if Data = nil then
  begin
    Reset(F);
    Exit;
  end;
  if TTextRec(F).Mode <> fmClosed then CloseFile(F);
  AssignFile(F, Filename);
  with TTextRec(F) do
  begin
    Handle := -1;
    Mode := fmInput;
    Flags := tfCRLF;
    BufSize := SizeOf(Buffer);
    BufPos := 0;
    BufEnd := 0;
    BufPtr := @Buffer;
    InOutFunc := @ReadDefault;
    FlushFunc := @ReadDefault;
    CloseFunc := @CloseDefault;
    State := @UserData;
    State^.Data := Data;
    State^.Size := Size;
    State^.Position := 0;
  end;
end;

end.
