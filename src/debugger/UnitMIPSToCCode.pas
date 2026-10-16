unit UnitMIPSToCCode;

interface

function MIPSToCCode(PC, Opcode: Cardinal; var DelaySlot: Boolean): string;

implementation

uses SysUtils, UnitDecs;

function MIPSToCCode(PC, Opcode: Cardinal; var DelaySlot: Boolean): string;
var
  DelayCode: string;

  function RD: string;
  begin
    Result := IntToStr((Opcode shr 11) and $1F);
  end;

  function RS: string;
  begin
    Result := IntToStr((Opcode shr 21) and $1F);
  end;

  function RT: string;
  begin
    Result := IntToStr((Opcode shr 16) and $1F);
  end;

  function SA: string;
  begin
    Result := IntToStr((Opcode shr 6) and $1F);
  end;

  function Imm32: string;
  begin
    Result := '0x' + IntToHex(Cardinal(SmallInt(Opcode and $FFFF)), 8);
  end;

  function Imm64: string;
  begin
    Result := '0x' + IntToHex(Int64(SmallInt(Opcode and $FFFF)), 16);
  end;

  function JumpTarget: string;
  begin
    Result := 'pc' + IntToHex((PC and $F0000000) or ((Opcode shl 2) and $0FFFFFFF), 8);
  end;

  function BranchTarget: string;
  begin
    Result := 'pc' + IntToHex(PC + 4 + Cardinal(SmallInt(Opcode) shl 2), 8);
  end;

  procedure ConvertDelaySlot;
  var
    IgnoreDelaySlot: Boolean;
    NextPC: Cardinal;
  begin
    NextPC := PC + 4;
    DelayCode := MIPSToCCode(NextPC, ReadWord(TranslateAddress(PC + 4)), IgnoreDelaySlot);
  end;

begin
  DelaySlot := False;
  case Opcode shr 26 of
    $00:
      case Opcode and $3F of
        $00:
          begin
            if Opcode <> 0 then
            begin
              Result := 'Registers[' +
                RD +
                '] = (s64)(u64)(s32)((u32)Registers[' +
                RT +
                '] << ' +
                SA +
                ');';
            end
            else
              Result := '';
          end;
        $20:
          begin
            Result := 'Registers[' +
              RD +
              '] = (s64)(u64)(s32)((u32)Registers[' +
              RS +
              '] + (u32)Registers[' +
              RT +
              ']);';
          end;
        $21:
          begin
            Result := 'Registers[' +
              RD +
              '] = (s64)(u64)(s32)((u32)Registers[' +
              RS +
              '] + (u32)Registers[' +
              RT +
              ']);';
          end;
        $24:
          begin
            Result := 'Registers[' +
              RD +
              '] = (s64)(u64)(s32)((u32)Registers[' +
              RS +
              '] & (u32)Registers[' +
              RT +
              ']);';
          end;
        $0D:
          begin
            Result := ' do{}until (0);';
          end;
        $2C:
          begin
            Result := 'Registers[' +
              RD +
              '] = Registers[' +
              RS +
              '] + Registers[' +
              RT +
              '];';
          end;
        $2D:
          begin
            Result := 'Registers[' +
              RD +
              '] = Registers[' +
              RS +
              '] + Registers[' +
              RT +
              '];';
          end;
        $1E:
          begin
            Result := 'MultLo = (s64)Registers[' +
              RS +
              '] / (s64)Registers[' +
              RT +
              ']);'#13#10#9'MultHi = (s64)Registers[' +
              RS +
              '] % (s64)Registers[' +
              RT +
              '];';
          end;
        $1F:
          begin
            Result := 'MultLo = (u64)Registers[' +
              RS +
              '] / (u64)Registers[' +
              RT +
              ']);'#13#10#9'MultHi = (u64)Registers[' +
              RS +
              '] % (u64)Registers[' +
              RT +
              '];';
          end;
        $1A:
          begin
            Result := 'MultLo = (s64)(s32)((s32)Registers[' +
              RS +
              '] / (s32)Registers[' +
              RT +
              ']);'#13#10#9'MultHi = (s64)(s32)((s32)Registers[' +
              RS +
              '] % (s32)Registers[' +
              RT +
              ']);';
          end;
        $1B:
          begin
            Result := 'MultLo = (s64)(s32)((u32)Registers[' +
              RS +
              '] / (u32)Registers[' +
              RT +
              ']);'#13#10#9'MultHi = (s64)(s32)((u32)Registers[' +
              RS +
              '] % (u32)Registers[' +
              RT +
              ']);';
          end;
        $1C:
          begin
            Result := 'MultLo = (s64)Registers[' +
              RS +
              '] * (s64)Registers[' +
              RT +
              ']);'#13#10#9'MultHi = 0';
          end;
        $1D:
          begin
            Result := 'MultLo = (u64)Registers[' +
              RS +
              '] * (u64)Registers[' +
              RT +
              ']);'#13#10#9'MultHi = 0';
          end;
        $38:
          begin
            Result := 'Registers[' +
              RD +
              '] = Registers[' +
              RT +
              '] << ' +
              SA +
              ';';
          end;
        $3C:
          begin
            Result := 'Registers[' +
              RD +
              '] = Registers[' +
              RT +
              '] << (' +
              SA +
              '+32);';
          end;
        $14:
          begin
            Result := 'Registers[' +
              RD +
              '] = Registers[' +
              RT +
              '] << (u8)Registers[' +
              RS +
              '];';
          end;
        $3B:
          begin
            Result := 'Registers[' +
              RD +
              '] = (s64)Registers[' +
              RT +
              '] >> ' +
              SA +
              ';';
          end;
        $3F:
          begin
            Result := 'Registers[' +
              RD +
              '] = (s64)Registers[' +
              RT +
              '] >> (' +
              SA +
              '+32);';
          end;
        $17:
          begin
            Result := 'Registers[' +
              RD +
              '] = (s64)Registers[' +
              RT +
              '] >> (u8)Registers[' +
              RS +
              '];';
          end;
        $3A:
          begin
            Result := 'Registers[' +
              RD +
              '] = (u64)Registers[' +
              RT +
              '] >> ' +
              SA +
              ';';
          end;
        $3E:
          begin
            Result := 'Registers[' +
              RD +
              '] = (u64)Registers[' +
              RT +
              '] >> (' +
              SA +
              '+32);';
          end;
        $16:
          begin
            Result := 'Registers[' +
              RD +
              '] = (u64)Registers[' +
              RT +
              '] >> (u8)Registers[' +
              RS +
              '];';
          end;
        $2E:
          begin
            Result := 'Registers[' +
              RD +
              '] = Registers[' +
              RS +
              '] - Registers[' +
              RT +
              '];';
          end;
        $2F:
          begin
            Result := 'Registers[' +
              RD +
              '] = Registers[' +
              RS +
              '] - Registers[' +
              RT +
              '];';
          end;
        $09:
          begin
            ConvertDelaySlot;
            Result := #13#10#9'Registers[' +
              RD +
              '] = 0x' +
              IntToHex(Int64(Integer(PC + 8)), 16) +
              ';'#13#10#9 +
              DelayCode +
              #13#10#9'goto //' +
              JumpTarget;
            DelaySlot := True;
          end;
        $08:
          begin
            ConvertDelaySlot;
            Result := DelayCode +
              #13#10#9'PC = Registers[31];//goto ' +
              JumpTarget;
            DelaySlot := True;
          end;
        $10:
          begin
            Result := 'MultHi = Registers[' +
              RS +
              ']';
          end;
        $12:
          begin
            Result := 'MultLo = Registers[' +
              RS +
              ']';
          end;
        $11:
          begin
            Result := 'Registers[' +
              RD +
              '] = MultHi';
          end;
        $13:
          begin
            Result := 'Registers[' +
              RD +
              '] = MultLo';
          end;
        $18:
          begin
            Result := 'MultLo = (s64)Registers[' +
              RS +
              '] * (s64)Registers[' +
              RT +
              ']);'#13#10#9'MultHi = 0';
          end;
        $19:
          begin
            Result := 'MultLo = (s64)Registers[' +
              RS +
              '] * (s64)Registers[' +
              RT +
              ']);'#13#10#9'MultHi = 0';
          end;
        $27:
          begin
            Result := 'Registers[' +
              RD +
              '] = ~(Registers[' +
              RS +
              '] | Registers[' +
              RT +
              ']);';
          end;
        $25:
          begin
            Result := 'Registers[' +
              RD +
              '] = Registers[' +
              RS +
              '] | Registers[' +
              RT +
              '];';
          end;
        $04:
          begin
            Result := 'Registers[' +
              RD +
              '] = (u64)(s64)(s32)((u32)Registers[' +
              RT +
              '] << (u8)Registers[' +
              RS +
              ']);';
          end;
        $2A:
          begin
            Result := 'Registers[' +
              RD +
              '] = (s64)Registers[' +
              RS +
              '] < (s64)Registers[' +
              RT +
              '];';
          end;
        $2B:
          begin
            Result := 'Registers[' +
              RD +
              '] = (u64)Registers[' +
              RS +
              '] < (u64)Registers[' +
              RT +
              '];';
          end;
        $03:
          begin
            Result := 'Registers[' +
              RD +
              '] = (s32)((s32)Registers[' +
              RT +
              '] >> ' +
              SA +
              ');';
          end;
        $07:
          begin
            Result := 'Registers[' +
              RD +
              '] = (s32)((s32)Registers[' +
              RT +
              '] >> Registers[' +
              RS +
              ']);';
          end;
        $02:
          begin
            Result := 'Registers[' +
              RD +
              '] = (s32)((u32)Registers[' +
              RT +
              '] >> ' +
              SA +
              ');';
          end;
        $06:
          begin
            Result := 'Registers[' +
              RD +
              '] = (s32)((u32)Registers[' +
              RT +
              '] >> Registers[' +
              RS +
              ']);';
          end;
        $22:
          begin
            Result := 'Registers[' +
              RD +
              '] = (s32)((u32)Registers[' +
              RS +
              '] - (U32)Registers[' +
              RT +
              ']);';
          end;
        $23:
          begin
            Result := 'Registers[' +
              RD +
              '] = (s32)((u32)Registers[' +
              RS +
              '] - (U32)Registers[' +
              RT +
              ']);';
          end;
        $0F:
          begin
            Result := '// SYNC';
          end;
        $0C:
          begin
            Result := '// SYSCALL';
          end;
        $34:
          begin
            Result := '// TEQ ' +
              RS +
              RT;
          end;
        $30:
          begin
            Result := '// TGE ' +
              RS +
              RT;
          end;
        $31:
          begin
            Result := '// TGEU ' +
              RS +
              RT;
          end;
        $32:
          begin
            Result := '// TLT ' +
              RS +
              RT;
          end;
        $33:
          begin
            Result := '// TLTU ' +
              RS +
              RT;
          end;
        $36:
          begin
            Result := '// TNE ' +
              RS +
              RT;
          end;
        $26:
          begin
            Result := 'Registers[' +
              RD +
              '] = Registers[' +
              RS +
              '] ^ Registers[' +
              RT +
              '];';
          end;
      else
        Result := '// ? (Special)';
      end;
    $01:
      case (Opcode shr 16) and $1F of
        $00:
          begin
            DelaySlot := True;
            ConvertDelaySlot;
            Result := 'if (Registers[' +
              RS +
              '] < 0){' +
              #13#10#9#9 +
              DelayCode +
              '; goto pc' +
              BranchTarget +
              #13#10#9'} else {' +
              #13#10#9#9 +
              DelayCode +
              #13#10#9'}';
          end;
        $01:
          begin
            DelaySlot := True;
            ConvertDelaySlot;
            Result := 'if (Registers[' +
              RS +
              '] >= 0){' +
              #13#10#9#9 +
              DelayCode +
              '; goto pc' +
              BranchTarget +
              #13#10#9'} else {' +
              #13#10#9#9 +
              DelayCode +
              #13#10#9'}';
          end;
        $02:
          begin
            DelaySlot := True;
            ConvertDelaySlot;
            Result := 'if (Registers[' +
              RS +
              '] < 0){' +
              #13#10#9#9 +
              DelayCode +
              '; goto pc' +
              BranchTarget +
              #13#10#9'}';
          end;
        $03:
          begin
            DelaySlot := True;
            ConvertDelaySlot;
            Result := 'if (Registers[' +
              RS +
              '] >= 0){' +
              #13#10#9#9 +
              DelayCode +
              '; goto pc' +
              BranchTarget +
              #13#10#9'}';
          end;
        $10:
          begin
            DelaySlot := True;
            ConvertDelaySlot;
            Result := 'Registers[31] = 0x' +
              IntToHex(Int64(Integer(PC + 8)), 16) +
              ';' +
              #13#10#9'if (Registers[' +
              RS +
              '] < 0){' +
              #13#10#9#9 +
              DelayCode +
              '; goto pc' +
              BranchTarget +
              #13#10#9'} else {' +
              #13#10#9#9 +
              DelayCode +
              #13#10#9'}';
          end;
        $11:
          begin
            DelaySlot := True;
            ConvertDelaySlot;
            Result := 'Registers[31] = 0x' +
              IntToHex(Int64(Integer(PC + 8)), 16) +
              ';' +
              #13#10#9'if (Registers[' +
              RS +
              '] >= 0){' +
              #13#10#9#9 +
              DelayCode +
              '; goto pc' +
              BranchTarget +
              #13#10#9'} else {' +
              #13#10#9#9 +
              DelayCode +
              #13#10#9'}';
          end;
        $12:
          begin
            DelaySlot := True;
            ConvertDelaySlot;
            Result := 'Registers[31] = 0x' +
              IntToHex(Int64(Integer(PC + 8)), 16) +
              ';' +
              #13#10#9'if (Registers[' +
              RS +
              '] < 0){' +
              #13#10#9#9 +
              DelayCode +
              '; goto pc' +
              BranchTarget +
              #13#10#9'}';
          end;
        $13:
          begin
            DelaySlot := True;
            ConvertDelaySlot;
            Result := 'Registers[31] = 0x' +
              IntToHex(Int64(Integer(PC + 8)), 16) +
              ';' +
              #13#10#9'if (Registers[' +
              RS +
              '] >= 0){' +
              #13#10#9#9 +
              DelayCode +
              '; goto pc' +
              BranchTarget +
              #13#10#9'}';
          end;
        $08:
          begin
            Result := '// TGEI ' +
              RS +
              Imm32;
          end;
        $09:
          begin
            Result := '// TGEIU ' +
              RS +
              Imm32;
          end;
        $0A:
          begin
            Result := '// TLTI ' +
              RS +
              Imm32;
          end;
        $0B:
          begin
            Result := '// TLTIU ' +
              RS +
              Imm32;
          end;
        $0C:
          begin
            Result := '// TEQI ' +
              RS +
              Imm32;
          end;
        $0E:
          begin
            Result := '// TNEI ' +
              RS +
              Imm32;
          end;
      else
        Result := '// ? (Regimm)';
      end;
    $0F:
      begin
        Result := 'Registers[' +
          RT +
          '] = 0x' +
          IntToHex(Int64(Integer((Opcode and $FFFF) shl 16)), 16) +
          ';';
      end;
    $08, $09:
      begin
        Result := 'Registers[' +
          RT +
          '] = (u64)((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          ');';
      end;
    $18, $19:
      begin
        Result := 'Registers[' +
          RT +
          '] = Registers[' +
          RS +
          '] + ' +
          Imm64 +
          ';';
      end;
    $0C:
      begin
        Result := 'Registers[' +
          RT +
          '] = (u64)((u32)Registers[' +
          RS +
          '] & ' +
          Imm32 +
          ');';
      end;
    $0D:
      begin
        Result := 'Registers[' +
          RT +
          '] = (u64)((u32)Registers[' +
          RS +
          '] | ' +
          Imm32 +
          ');';
      end;
    $26:
      begin
        Result := 'Registers[' +
          RT +
          '] = (u64)((u32)Registers[' +
          RS +
          '] ^ ' +
          Imm32 +
          ');';
      end;
    $0A:
      begin
        Result := 'Registers[' +
          RT +
          '] = (u64)((s64)Registers[' +
          RS +
          '] < (s64)' +
          Imm64 +
          ');';
      end;
    $0B:
      begin
        Result := 'Registers[' +
          RT +
          '] = (u64)((u64)Registers[' +
          RS +
          '] < (u64)' +
          Imm64 +
          ');';
      end;
    $03:
      begin
        ConvertDelaySlot;
        Result := 'Registers[31] = 0x' +
          IntToHex(Int64(Integer(PC + 8)), 16) +
          ';'#13#10#9 +
          DelayCode +
          #13#10#9'goto ' +
          JumpTarget +
          ';';
        DelaySlot := True;
      end;
    $02:
      begin
        ConvertDelaySlot;
        Result := DelayCode +
          #13#10#9'goto ' +
          JumpTarget;
        DelaySlot := True;
      end;
    $04:
      begin
        ConvertDelaySlot;
        Result := DelayCode +
          #13#10#9'if (Registers[' +
          RS +
          '] == Registers[' +
          RT +
          ']){'#13#10#9#9'goto ' +
          BranchTarget +
          ';'#13#10#9'}';
        Inc(PC, 4);
        DelaySlot := True;
      end;
    $05:
      begin
        ConvertDelaySlot;
        Result := DelayCode +
          #13#10#9'if (Registers[' +
          RS +
          '] != Registers[' +
          RT +
          ']){'#13#10#9#9'goto ' +
          BranchTarget +
          ';'#13#10#9'}';
        Inc(PC, 4);
        DelaySlot := True;
      end;
    $14:
      begin
        ConvertDelaySlot;
        Result := 'if (Registers[' +
          RS +
          '] == Registers[' +
          RT +
          ']){'#13#10#9#9 +
          DelayCode +
          'goto ' +
          BranchTarget +
          ';'#13#10#9'}';
        Inc(PC, 4);
        DelaySlot := True;
      end;
    $15:
      begin
        ConvertDelaySlot;
        Result := 'if (Registers[' +
          RS +
          '] != Registers[' +
          RT +
          ']){'#13#10#9#9 +
          DelayCode +
          'goto ' +
          BranchTarget +
          ';'#13#10#9'}';
        Inc(PC, 4);
        DelaySlot := True;
      end;
    $06:
      begin
        ConvertDelaySlot;
        Result := DelayCode +
          #13#10#9'if (Registers[' +
          RS +
          '] <= 0){'#13#10#9#9'goto ' +
          BranchTarget +
          ';'#13#10#9'}';
        Inc(PC, 4);
        DelaySlot := True;
      end;
    $07:
      begin
        ConvertDelaySlot;
        Result := DelayCode +
          #13#10#9'if (Registers[' +
          RS +
          '] > 0){'#13#10#9#9'goto ' +
          BranchTarget +
          ';'#13#10#9'}';
        Inc(PC, 4);
        DelaySlot := True;
      end;
    $16:
      begin
        ConvertDelaySlot;
        Result := 'if (Registers[' +
          RS +
          '] <= 0){'#13#10#9#9 +
          DelayCode +
          'goto ' +
          BranchTarget +
          ';'#13#10#9'}';
        Inc(PC, 4);
        DelaySlot := True;
      end;
    $17:
      begin
        ConvertDelaySlot;
        Result := 'if (Registers[' +
          RS +
          '] > 0){'#13#10#9#9 +
          DelayCode +
          'goto ' +
          BranchTarget +
          ';'#13#10#9'}';
        Inc(PC, 4);
        DelaySlot := True;
      end;
    $20:
      begin
        Result := 'Registers[' +
          RT +
          '] = (u64)(u8)Read8(Map((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '));';
      end;
    $24:
      begin
        Result := 'Registers[' +
          RT +
          '] = (u64)(s64)(s8)Read8(Map((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '));';
      end;
    $21:
      begin
        Result := 'Registers[' +
          RT +
          '] = (u64)(u16)Read16(Map((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '));';
      end;
    $25:
      begin
        Result := 'Registers[' +
          RT +
          '] = (u64)(s64)(s16)Read16(Map((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '));';
      end;
    $23:
      begin
        Result := 'Registers[' +
          RT +
          '] = (u64)(u32)Read32(Map((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '));';
      end;
    $27:
      begin
        Result := 'Registers[' +
          RT +
          '] = (u64)(s64)(s32)Read32(Map((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '));';
      end;
    $37:
      begin
        Result := 'Registers[' +
          RT +
          '] = Read64(Map((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '));';
      end;
    $31:
      begin
        Result := 'COP1Registers.R32[' +
          RT +
          '] = Read32(Map((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '));';
      end;
    $35:
      begin
        Result := 'COP1Registers.R64[' +
          RT +
          '>>1] = Read64(Map((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '));';
      end;
    $39:
      begin
        Result := 'Write32(MapWrite((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '), (u32)COP1Registers.U64[' +
          RT +
          ']);';
      end;
    $3D:
      begin
        Result := 'Write64(MapWrite((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '), COP1Registers.U32[' +
          RT +
          ']);';
      end;
    $28:
      begin
        Result := 'Write8(MapWrite((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '), (u8)Registers[' +
          RT +
          ']);';
      end;
    $29:
      begin
        Result := 'Write16(MapWrite((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '), (u16)Registers[' +
          RT +
          ']);';
      end;
    $2B:
      begin
        Result := 'Write32(MapWrite((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '), (u32)Registers[' +
          RT +
          ']);';
      end;
    $3F:
      begin
        Result := 'Write64(MapWrite((u32)Registers[' +
          RS +
          '] + ' +
          Imm32 +
          '), Registers[' +
          RT +
          ']);';
      end;
  else
    Result := ' // unhandled opcode';
  end;
end;

end.
