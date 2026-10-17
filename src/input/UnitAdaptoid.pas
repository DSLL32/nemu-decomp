unit UnitAdaptoid;

interface

uses Windows;

type
  { The Adaptoid driver exchanges one 64-byte PIF command block at a time. }
  TAdaptoidPacket = array[0..15] of Cardinal;

var
  AdaptoidHandle: THandle;
  AdaptoidCount: Integer;
  AdaptoidCommand, AdaptoidResponse: TAdaptoidPacket;
  AdaptoidRumbleOn1, AdaptoidRumbleOff1: TAdaptoidPacket;
  AdaptoidRumbleOn2, AdaptoidRumbleOff2: TAdaptoidPacket;
  AdaptoidRumbleOn3, AdaptoidRumbleOff3: TAdaptoidPacket;
  AdaptoidRumbleOn4, AdaptoidRumbleOff4: TAdaptoidPacket;

function InitAdaptoid: Boolean;
procedure FreeAdaptoid;

implementation

uses SysUtils;

function InitAdaptoid: Boolean;
begin
  AdaptoidCount := 0;
  AdaptoidHandle := CreateFile('\\.\Wish_NA1', GENERIC_READ or GENERIC_WRITE,
    FILE_SHARE_READ or FILE_SHARE_WRITE, nil, OPEN_EXISTING, 0, 0);
  if AdaptoidHandle = INVALID_HANDLE_VALUE then
  begin
    Result := False;
    Exit;
  end;
  { The driver reports the number of connected adapters in a single byte. }
  FileRead(AdaptoidHandle, AdaptoidCount, 1);
  if AdaptoidCount > 0 then
  begin
    AdaptoidCommand[0] := $FF010300;
    AdaptoidCommand[1] := $FFFFFFFF;
    AdaptoidCommand[2] := $FF010300;
    AdaptoidCommand[3] := $FFFFFFFF;
    AdaptoidCommand[4] := $FF010300;
    AdaptoidCommand[5] := $FFFFFFFF;
    AdaptoidCommand[6] := $FF010300;
    AdaptoidCommand[7] := $FFFFFFFF;
    AdaptoidCommand[8] := $FE000000;
    AdaptoidCommand[9] := 0;
    AdaptoidCommand[10] := 0;
    AdaptoidCommand[11] := 0;
    AdaptoidCommand[12] := 0;
    AdaptoidCommand[13] := 0;
    AdaptoidCommand[14] := 0;
    AdaptoidCommand[15] := 1;
    FileWrite(AdaptoidHandle, AdaptoidCommand, 64);
    FileRead(AdaptoidHandle, AdaptoidResponse, 64);

    AdaptoidCommand[0] := $23010380;
    AdaptoidCommand[1] := $01808080;
    AdaptoidCommand[2] := $80808080;
    AdaptoidCommand[3] := $80808080;
    AdaptoidCommand[4] := $80808080;
    AdaptoidCommand[5] := $80808080;
    AdaptoidCommand[6] := $80808080;
    AdaptoidCommand[7] := $80808080;
    AdaptoidCommand[8] := $80808080;
    AdaptoidCommand[9] := $80FFFE00;
    AdaptoidCommand[10] := 0;
    AdaptoidCommand[11] := 0;
    AdaptoidCommand[12] := 0;
    AdaptoidCommand[13] := 0;
    AdaptoidCommand[14] := 0;
    AdaptoidCommand[15] := 1;
    FileWrite(AdaptoidHandle, AdaptoidCommand, 64);
    FileRead(AdaptoidHandle, AdaptoidResponse, 64);
  end;

  if AdaptoidCount > 1 then
  begin
    AdaptoidCommand[0] := $00230103;
    AdaptoidCommand[1] := $80018080;
    AdaptoidCommand[2] := $80808080;
    AdaptoidCommand[3] := $80808080;
    AdaptoidCommand[4] := $80808080;
    AdaptoidCommand[5] := $80808080;
    AdaptoidCommand[6] := $80808080;
    AdaptoidCommand[7] := $80808080;
    AdaptoidCommand[8] := $80808080;
    AdaptoidCommand[9] := $8080FFFE;
    AdaptoidCommand[10] := 0;
    AdaptoidCommand[11] := 0;
    AdaptoidCommand[12] := 0;
    AdaptoidCommand[13] := 0;
    AdaptoidCommand[14] := 0;
    AdaptoidCommand[15] := 1;
    FileWrite(AdaptoidHandle, AdaptoidCommand, 64);
    FileRead(AdaptoidHandle, AdaptoidResponse, 64);
  end;

  if AdaptoidCount > 2 then
  begin
    AdaptoidCommand[0] := $00002301;
    AdaptoidCommand[1] := $03800180;
    AdaptoidCommand[2] := $80808080;
    AdaptoidCommand[3] := $80808080;
    AdaptoidCommand[4] := $80808080;
    AdaptoidCommand[5] := $80808080;
    AdaptoidCommand[6] := $80808080;
    AdaptoidCommand[7] := $80808080;
    AdaptoidCommand[8] := $80808080;
    AdaptoidCommand[9] := $808080FF;
    AdaptoidCommand[10] := $FE000000;
    AdaptoidCommand[11] := 0;
    AdaptoidCommand[12] := 0;
    AdaptoidCommand[13] := 0;
    AdaptoidCommand[14] := 0;
    AdaptoidCommand[15] := 1;
    FileWrite(AdaptoidHandle, AdaptoidCommand, 64);
    FileRead(AdaptoidHandle, AdaptoidResponse, 64);
  end;

  if AdaptoidCount > 3 then
  begin
    AdaptoidCommand[0] := $00000023;
    AdaptoidCommand[1] := $01038001;
    AdaptoidCommand[2] := $80808080;
    AdaptoidCommand[3] := $80808080;
    AdaptoidCommand[4] := $80808080;
    AdaptoidCommand[5] := $80808080;
    AdaptoidCommand[6] := $80808080;
    AdaptoidCommand[7] := $80808080;
    AdaptoidCommand[8] := $80808080;
    AdaptoidCommand[9] := $80808080;
    AdaptoidCommand[10] := $FFFE0000;
    AdaptoidCommand[11] := 0;
    AdaptoidCommand[12] := 0;
    AdaptoidCommand[13] := 0;
    AdaptoidCommand[14] := 0;
    AdaptoidCommand[15] := 1;
    FileWrite(AdaptoidHandle, AdaptoidCommand, 64);
    FileRead(AdaptoidHandle, AdaptoidResponse, 64);
  end;

  { Prepare separate Rumble Pak write commands for each controller channel. }
  AdaptoidRumbleOn1[0] := $230103C0;
  AdaptoidRumbleOn1[1] := $1B010101;
  AdaptoidRumbleOn1[2] := $01010101;
  AdaptoidRumbleOn1[3] := $01010101;
  AdaptoidRumbleOn1[4] := $01010101;
  AdaptoidRumbleOn1[5] := $01010101;
  AdaptoidRumbleOn1[6] := $01010101;
  AdaptoidRumbleOn1[7] := $01010101;
  AdaptoidRumbleOn1[8] := $01010101;
  AdaptoidRumbleOn1[9] := $01FFFE00;
  AdaptoidRumbleOn1[10] := 0;
  AdaptoidRumbleOn1[11] := 0;
  AdaptoidRumbleOn1[12] := 0;
  AdaptoidRumbleOn1[13] := 0;
  AdaptoidRumbleOn1[14] := 0;
  AdaptoidRumbleOn1[15] := 1;

  AdaptoidRumbleOff1[0] := $230103C0;
  AdaptoidRumbleOff1[1] := $1B000000;
  AdaptoidRumbleOff1[2] := 0;
  AdaptoidRumbleOff1[3] := 0;
  AdaptoidRumbleOff1[4] := 0;
  AdaptoidRumbleOff1[5] := 0;
  AdaptoidRumbleOff1[6] := 0;
  AdaptoidRumbleOff1[7] := 0;
  AdaptoidRumbleOff1[8] := 0;
  AdaptoidRumbleOff1[9] := $00FFFE00;
  AdaptoidRumbleOff1[10] := 0;
  AdaptoidRumbleOff1[11] := 0;
  AdaptoidRumbleOff1[12] := 0;
  AdaptoidRumbleOff1[13] := 0;
  AdaptoidRumbleOff1[14] := 0;
  AdaptoidRumbleOff1[15] := 1;

  AdaptoidRumbleOn2[0] := $00230103;
  AdaptoidRumbleOn2[1] := $C01B0101;
  AdaptoidRumbleOn2[2] := $01010101;
  AdaptoidRumbleOn2[3] := $01010101;
  AdaptoidRumbleOn2[4] := $01010101;
  AdaptoidRumbleOn2[5] := $01010101;
  AdaptoidRumbleOn2[6] := $01010101;
  AdaptoidRumbleOn2[7] := $01010101;
  AdaptoidRumbleOn2[8] := $01010101;
  AdaptoidRumbleOn2[9] := $0101FFFE;
  AdaptoidRumbleOn2[10] := 0;
  AdaptoidRumbleOn2[11] := 0;
  AdaptoidRumbleOn2[12] := 0;
  AdaptoidRumbleOn2[13] := 0;
  AdaptoidRumbleOn2[14] := 0;
  AdaptoidRumbleOn2[15] := 1;

  AdaptoidRumbleOff2[0] := $00230103;
  AdaptoidRumbleOff2[1] := $C01B0000;
  AdaptoidRumbleOff2[2] := 0;
  AdaptoidRumbleOff2[3] := 0;
  AdaptoidRumbleOff2[4] := 0;
  AdaptoidRumbleOff2[5] := 0;
  AdaptoidRumbleOff2[6] := 0;
  AdaptoidRumbleOff2[7] := 0;
  AdaptoidRumbleOff2[8] := 0;
  AdaptoidRumbleOff2[9] := $0000FFFE;
  AdaptoidRumbleOff2[10] := 0;
  AdaptoidRumbleOff2[11] := 0;
  AdaptoidRumbleOff2[12] := 0;
  AdaptoidRumbleOff2[13] := 0;
  AdaptoidRumbleOff2[14] := 0;
  AdaptoidRumbleOff2[15] := 1;

  AdaptoidRumbleOn3[0] := $00002301;
  AdaptoidRumbleOn3[1] := $03C01B01;
  AdaptoidRumbleOn3[2] := $01010101;
  AdaptoidRumbleOn3[3] := $01010101;
  AdaptoidRumbleOn3[4] := $01010101;
  AdaptoidRumbleOn3[5] := $01010101;
  AdaptoidRumbleOn3[6] := $01010101;
  AdaptoidRumbleOn3[7] := $01010101;
  AdaptoidRumbleOn3[8] := $01010101;
  AdaptoidRumbleOn3[9] := $0100FF00;
  AdaptoidRumbleOn3[10] := $FE000000;
  AdaptoidRumbleOn3[11] := 0;
  AdaptoidRumbleOn3[12] := 0;
  AdaptoidRumbleOn3[13] := 0;
  AdaptoidRumbleOn3[14] := 0;
  AdaptoidRumbleOn3[15] := 1;

  AdaptoidRumbleOff3[0] := $00002301;
  AdaptoidRumbleOff3[1] := $03C01B01;
  AdaptoidRumbleOff3[2] := 0;
  AdaptoidRumbleOff3[3] := 0;
  AdaptoidRumbleOff3[4] := 0;
  AdaptoidRumbleOff3[5] := 0;
  AdaptoidRumbleOff3[6] := 0;
  AdaptoidRumbleOff3[7] := 0;
  AdaptoidRumbleOff3[8] := 0;
  AdaptoidRumbleOff3[9] := $0000FF00;
  AdaptoidRumbleOff3[10] := $FE000000;
  AdaptoidRumbleOff3[11] := 0;
  AdaptoidRumbleOff3[12] := 0;
  AdaptoidRumbleOff3[13] := 0;
  AdaptoidRumbleOff3[14] := 0;
  AdaptoidRumbleOff3[15] := 1;

  AdaptoidRumbleOn4[0] := $00000023;
  AdaptoidRumbleOn4[1] := $0103C01B;
  AdaptoidRumbleOn4[2] := $01010101;
  AdaptoidRumbleOn4[3] := $01010101;
  AdaptoidRumbleOn4[4] := $01010101;
  AdaptoidRumbleOn4[5] := $01010101;
  AdaptoidRumbleOn4[6] := $01010101;
  AdaptoidRumbleOn4[7] := $01010101;
  AdaptoidRumbleOn4[8] := $01010101;
  AdaptoidRumbleOn4[9] := $010100FF;
  AdaptoidRumbleOn4[10] := $FE000000;
  AdaptoidRumbleOn4[11] := 0;
  AdaptoidRumbleOn4[12] := 0;
  AdaptoidRumbleOn4[13] := 0;
  AdaptoidRumbleOn4[14] := 0;
  AdaptoidRumbleOn4[15] := 1;

  AdaptoidRumbleOff4[0] := $00000023;
  AdaptoidRumbleOff4[1] := $0103C01B;
  AdaptoidRumbleOff4[2] := 0;
  AdaptoidRumbleOff4[3] := 0;
  AdaptoidRumbleOff4[4] := 0;
  AdaptoidRumbleOff4[5] := 0;
  AdaptoidRumbleOff4[6] := 0;
  AdaptoidRumbleOff4[7] := 0;
  AdaptoidRumbleOff4[8] := 0;
  AdaptoidRumbleOff4[9] := $000000FF;
  AdaptoidRumbleOff4[10] := $FE000000;
  AdaptoidRumbleOff4[11] := 0;
  AdaptoidRumbleOff4[12] := 0;
  AdaptoidRumbleOff4[13] := 0;
  AdaptoidRumbleOff4[14] := 0;
  AdaptoidRumbleOff4[15] := 1;

  Result := True;
end;

procedure FreeAdaptoid;
begin
  if AdaptoidHandle <> INVALID_HANDLE_VALUE then
    CloseHandle(AdaptoidHandle);
end;

end.
