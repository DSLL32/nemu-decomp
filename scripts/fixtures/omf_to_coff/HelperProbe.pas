unit HelperProbe;
interface
var HelperValue: Integer = 5;
function HelperAdd(A, B: Integer): Integer;
implementation
var HelperReady: Integer;
    Zeros: array[0..16] of Integer;
function HelperAdd(A, B: Integer): Integer;
var I: Integer;
begin
  Result := A + B + HelperReady;
  for I := Low(Zeros) to High(Zeros) do
    if Zeros[I] <> 0 then Result := -1000;
end;
initialization HelperReady := 10;
finalization HelperReady := -10;
end.
