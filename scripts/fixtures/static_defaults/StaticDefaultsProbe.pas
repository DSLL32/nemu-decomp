unit StaticDefaultsProbe;
interface
uses StaticFiles, IniFiles;
function ReadText(Name: PChar; var Hash, Lines: Cardinal): Integer; cdecl;
function IniValue(Name, Section, Key: PChar): Integer; cdecl;
procedure IniWrite(Name, Section, Key: PChar; Value: Integer); cdecl;
implementation
function IniValue(Name, Section, Key: PChar): Integer; cdecl;
var Ini: TIniFile;
begin
  Ini := TIniFile.Create(string(Name));
  try Result := Ini.ReadInteger(string(Section), string(Key), -99);
  finally Ini.Free; end;
end;
procedure IniWrite(Name, Section, Key: PChar; Value: Integer); cdecl;
var Ini: TIniFile;
begin
  Ini := TIniFile.Create(string(Name));
  try Ini.WriteInteger(string(Section), string(Key), Value);
  finally Ini.Free; end;
end;
function ReadText(Name: PChar; var Hash, Lines: Cardinal): Integer; cdecl;
var
  F: Text;
  Line: string;
  I: Integer;
begin
  Hash := 2166136261;
  Lines := 0;
  AssignFile(F, string(Name));
  StaticReset(F, string(Name));
  while not Eof(F) do
  begin
    ReadLn(F, Line);
    for I := 1 to Length(Line) do Hash := (Hash xor Ord(Line[I])) * 16777619;
    Hash := (Hash xor 10) * 16777619;
    Inc(Lines);
  end;
  CloseFile(F);
  Result := 1;
end;
end.
