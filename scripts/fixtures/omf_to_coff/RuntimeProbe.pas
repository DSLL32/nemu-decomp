unit RuntimeProbe;
interface
uses HelperProbe;
function Values(A: Integer): Integer; cdecl;
function Strings: Integer; cdecl;
function CrossUnit(A: Integer): Integer; cdecl;
function Callback(F: Pointer; A: Integer): Integer; cdecl;
function FinallyTest(A: Integer): Integer; cdecl;
function ExceptionTest(A: Integer): Integer; cdecl;
function ThreadValue(A: Integer): Integer; cdecl;
implementation
type TCallback = function(A: Integer): Integer; cdecl;
var V: Integer = 11; Ready: Integer; Cleanups: Integer;
threadvar LocalValue: Integer;
function Values(A: Integer): Integer; cdecl;
begin Result := A + V + Ready end;
function CrossUnit(A: Integer): Integer; cdecl;
begin
 Result := HelperValue + HelperAdd(A, 1);
 HelperValue := HelperValue + 2;
end;
function Strings: Integer; cdecl;
var S, T: AnsiString;
begin
 S := 'Delphi'; T := S + ' linked with MSVC';
 UniqueString(S); S[1] := 'd';
 if (S = 'delphi') and (T = 'Delphi linked with MSVC') then Result := Length(T) else Result := -1;
end;
function Callback(F: Pointer; A: Integer): Integer; cdecl;
begin Result := TCallback(F)(A) + V end;
function FinallyTest(A: Integer): Integer; cdecl;
var B: Integer;
begin B := 0; try B := A * 3; finally Inc(B, 5) end; Result := B end;
function ExceptionTest(A: Integer): Integer; cdecl;
begin
 try
  try
   if A <> 0 then raise TObject.Create;
   Result := 17;
  finally
   Inc(Cleanups);
  end;
 except
  Result := -777;
 end;
 Result := Result + Cleanups;
end;
function ThreadValue(A: Integer): Integer; cdecl;
begin Result := LocalValue; LocalValue := A end;
initialization Ready := HelperAdd(10, 11);
finalization Ready := -99;
end.
