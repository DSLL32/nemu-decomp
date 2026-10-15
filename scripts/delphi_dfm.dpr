program delphi_dfm;

{$APPTYPE CONSOLE}

uses Classes, SysUtils;

var
  Input, Output: TFileStream;
begin
  if (ParamCount <> 3) or
    ((ParamStr(1) <> 'text') and (ParamStr(1) <> 'binary') and
    (ParamStr(1) <> 'resource')) then
  begin
    WriteLn('Usage: delphi_dfm text|binary|resource input output');
    Halt(1);
  end;
  Input := TFileStream.Create(ParamStr(2), fmOpenRead);
  try
    Output := TFileStream.Create(ParamStr(3), fmCreate);
    try
      if ParamStr(1) = 'text' then ObjectBinaryToText(Input, Output)
      else if ParamStr(1) = 'resource' then ObjectTextToResource(Input, Output)
      else ObjectTextToBinary(Input, Output);
    finally
      Output.Free;
    end;
  finally
    Input.Free;
  end;
end.
