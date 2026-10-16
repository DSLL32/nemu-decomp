unit UnitDisassembly;

interface

var
  Reg1, Reg2, Reg3: Integer;
  DebuggerAddressReg: Cardinal;
  MemAccess: Boolean;
  HasDisplacement: Boolean;
  ImmValue: Integer;
  PC, CurrentOpcode: Cardinal;
  UseRegisterNames: Boolean;

function DisassembleCPU(PC, Opcode: Cardinal; RegisterNames: Boolean): string;
function DisassembleRSP(PC, Opcode: Cardinal; RegisterNames: Boolean): string;
function DisassembleRDP(Position: Cardinal; Word0, Word1: Int64): string;

implementation

uses SysUtils, RCP, UnitOpcode;

procedure AddRegister(Value: Integer);
begin
  if Reg1 = -1 then Reg1 := Value
  else if Reg2 = -1 then Reg2 := Value
  else Reg3 := Value;
end;

function Op: Cardinal;
begin
  Result := CurrentOpcode shr 26;
end;

function Func: Cardinal;
begin
  Result := CurrentOpcode and $3F;
end;

function RtIndex: Cardinal;
begin
  Result := (CurrentOpcode shr 16) and $1F;
end;

function Branch: string;
begin
  Result := '0x' + IntToHex(Int64(PC + 4 + Cardinal(SmallInt(CurrentOpcode) shl 2)), 8) + ', ';
end;

function Imm: string;
begin
  Result := '0x' + IntToHex(Int64(CurrentOpcode and $FFFF), 4) + ', ';
  HasDisplacement := True;
  ImmValue := SmallInt(CurrentOpcode);
end;

function RD: string;
begin
  if UseRegisterNames then Result := GPRNames[(CurrentOpcode shr 11) and $1F] + ', '
  else Result := 'R' + IntToStr(Int64((CurrentOpcode shr 11) and $1F)) + ', ';
  AddRegister((CurrentOpcode shr 11) and $1F);
end;

function RS: string;
begin
  if UseRegisterNames then Result := GPRNames[(CurrentOpcode shr 21) and $1F] + ', '
  else Result := 'R' + IntToStr(Int64((CurrentOpcode shr 21) and $1F)) + ', ';
  AddRegister((CurrentOpcode shr 21) and $1F);
end;

function RT: string;
begin
  if UseRegisterNames then Result := GPRNames[(CurrentOpcode shr 16) and $1F] + ', '
  else Result := 'R' + IntToStr(Int64((CurrentOpcode shr 16) and $1F)) + ', ';
  AddRegister((CurrentOpcode shr 16) and $1F);
end;

function SA: string;
begin
  Result := '0x' + IntToHex(Int64((CurrentOpcode shr 6) and $1F), 0) + ', ';
end;

function CP0: string;
begin
  Result := COP0RegNames[(CurrentOpcode shr 11) and $1F] + ', ';
end;

function Fmt: Cardinal;
begin
  Result := (CurrentOpcode shr 21) and $1F;
end;

function FD: string;
begin
  Result := 'F' + IntToStr(Int64((CurrentOpcode shr 6) and $1F)) + ', ';
end;

function FS: string;
begin
  Result := 'F' + IntToStr(Int64((CurrentOpcode shr 11) and $1F)) + ', ';
end;

function FT: string;
begin
  Result := 'F' + IntToStr(Int64((CurrentOpcode shr 16) and $1F)) + ', ';
end;

function Jump: string;
begin
  Result := '0x' + IntToHex(Int64((PC and $F0000000) or ((CurrentOpcode shl 2) and $0FFFFFFF)), 8) + ', ';
end;

function Base: string;
begin
  if UseRegisterNames then Result := '(' + GPRNames[(CurrentOpcode shr 21) and $1F] + ')'
  else Result := '(R' + IntToStr(Int64((CurrentOpcode shr 21) and $1F)) + ')';
  AddRegister((CurrentOpcode shr 21) and $1F);
  DebuggerAddressReg := (CurrentOpcode shr 21) and $1F;
  MemAccess := True;
end;

function DisassembleCPU(PC, Opcode: Cardinal; RegisterNames: Boolean): string;
begin
  Reg1 := -1;
  Reg2 := -1;
  Reg3 := -1;
  MemAccess := False;
  HasDisplacement := False;
  UnitDisassembly.PC := PC;
  CurrentOpcode := Opcode;
  UseRegisterNames := RegisterNames;
  case Op of
    0:     case Func of
      32: Result := 'ADD ' + RD + RS + RT;
      33: Result := 'ADDU ' + RD + RS + RT;
      36: Result := 'AND ' + RD + RS + RT;
      13: Result := 'BREAK';
      44: Result := 'DADD ' + RD + RS + RT;
      45: Result := 'DADDU ' + RD + RS + RT;
      30: Result := 'DDIV ' + RS + RT;
      31: Result := 'DDIVU ' + RS + RT;
      26: Result := 'DIV ' + RS + RT;
      27: Result := 'DIVU ' + RS + RT;
      28: Result := 'DMULT ' + RS + RT;
      29: Result := 'DMULTU ' + RS + RT;
      56: Result := 'DSLL ' + RD + RT + SA;
      60: Result := 'DSLL32 ' + RD + RT + SA;
      20: Result := 'DSLLV ' + RD + RT + RS;
      59: Result := 'DSRA ' + RD + RT + SA;
      63: Result := 'DSRA32 ' + RD + RT + SA;
      23: Result := 'DSRAV ' + RD + RT + RS;
      58: Result := 'DSRL ' + RD + RT + SA;
      62: Result := 'DSRL32 ' + RD + RT + SA;
      22: Result := 'DSRLV ' + RD + RT + RS;
      46: Result := 'DSUB ' + RD + RS + RT;
      47: Result := 'DSUBU ' + RD + RS + RT;
      9: Result := 'JALR ' + RD + RS;
      8: Result := 'JR ' + RS;
      16: Result := 'MFHI ' + RD;
      18: Result := 'MFLO ' + RD;
      17: Result := 'MTHI ' + RS;
      19: Result := 'MTLO ' + RS;
      24: Result := 'MULT ' + RS + RT;
      25: Result := 'MULTU ' + RS + RT;
      39: Result := 'NOR ' + RD + RS + RT;
      37: Result := 'OR ' + RD + RS + RT;
      0: if CurrentOpcode = 0 then Result := 'NOP' else Result := 'SLL ' + RD + RT + SA;
      4: Result := 'SLLV ' + RD + RT + RS;
      42: Result := 'SLT ' + RD + RS + RT;
      43: Result := 'SLTU ' + RD + RS + RT;
      3: Result := 'SRA ' + RD + RT + SA;
      7: Result := 'SRAV ' + RD + RT + RS;
      2: Result := 'SRL ' + RD + RT + SA;
      6: Result := 'SRLV ' + RD + RT + RS;
      34: Result := 'SUB ' + RD + RS + RT;
      35: Result := 'SUBU ' + RD + RS + RT;
      15: Result := 'SYNC';
      12: Result := 'SYSCALL';
      52: Result := 'TEQ ' + RS + RT;
      48: Result := 'TGE ' + RS + RT;
      49: Result := 'TGEU ' + RS + RT;
      50: Result := 'TLT ' + RS + RT;
      51: Result := 'TLTU ' + RS + RT;
      54: Result := 'TNE ' + RS + RT;
      38: Result := 'XOR ' + RD + RS + RT;
    else Result := '? (Special)';
    end;
    1:     case RtIndex of
      0: Result := 'BLTZ ' + RS + Branch;
      1: Result := 'BGEZ ' + RS + Branch;
      2: Result := 'BLTZL ' + RS + Branch;
      3: Result := 'BGEZL ' + RS + Branch;
      8: Result := 'TGEI ' + RS + Imm;
      9: Result := 'TGEIU ' + RS + Imm;
      10: Result := 'TLTI ' + RS + Imm;
      11: Result := 'TLTIU ' + RS + Imm;
      12: Result := 'TEQI ' + RS + Imm;
      14: Result := 'TNEI ' + RS + Imm;
      16: Result := 'BLTZAL ' + RS + Branch;
      17: Result := 'BGEZAL ' + RS + Branch;
      18: Result := 'BLTZALL ' + RS + Branch;
      19: Result := 'BGEZALL ' + RS + Branch;
    else Result := '? (Regimm)';
    end;
    16: case Fmt of
      0: Result := 'MFC0 ' + RT + CP0;
      4: Result := 'MTC0 ' + RT + CP0;
      16: case CurrentOpcode and $3F of
        1: Result := 'TLBR';
        2: Result := 'TLBWI';
        6: Result := 'TLBWR';
        8: Result := 'TLBP';
        24: Result := 'ERET';
      else Result := '? (COP 0 - TLB)';
      end;
    else Result := '? (COP 0)';
    end;
    17:     case Fmt of
      0: Result := 'MFC1 ' + RT + FS;
      1: Result := 'DMFC1 ' + RT + FS;
      2: Result := 'CFC1 ' + RT + FS;
      4: Result := 'MTC1 ' + RT + FS;
      5: Result := 'DMTC1 ' + RT + FS;
      6: Result := 'CTC1 ' + RT + FS;
      8: case (CurrentOpcode shr 16) and $1F of
      0: Result := 'BC1F ' + Branch;
      2: Result := 'BC1FL ' + Branch;
      1: Result := 'BC1T ' + Branch;
      3: Result := 'BC1TL ' + Branch;
    else Result := '? (BC1)';
    end;
      16:     case CurrentOpcode and $3F of
      0: Result := 'ADD.S ' + FD + FS + FT;
      1: Result := 'SUB.S ' + FD + FS + FT;
      2: Result := 'MUL.S ' + FD + FS + FT;
      3: Result := 'DIV.S ' + FD + FS + FT;
      4: Result := 'SQRT.S ' + FD + FS;
      5: Result := 'ABS.S ' + FD + FS;
      6: Result := 'MOV.S ' + FD + FS;
      7: Result := 'NEG.S ' + FD + FS;
      8: Result := 'ROUND.L.S ' + FD + FS;
      9: Result := 'TRUNC.L.S ' + FD + FS;
      10: Result := 'CEIL.L.S ' + FD + FS;
      11: Result := 'FLOOR.L.S ' + FD + FS;
      12: Result := 'ROUND.W.S ' + FD + FS;
      13: Result := 'TRUNC.W.S ' + FD + FS;
      14: Result := 'CEIL.W.S ' + FD + FS;
      15: Result := 'FLOOR.W.S ' + FD + FS;
      33: Result := 'CVT.D.S ' + FD + FS;
      36: Result := 'CVT.W.S ' + FD + FS;
      37: Result := 'CVT.L.S ' + FD + FS;
      48: Result := 'C.F.S ' + FS + FT;
      49: Result := 'C.UN.S ' + FS + FT;
      50: Result := 'C.EQ.S ' + FS + FT;
      51: Result := 'C.UEQ.S ' + FS + FT;
      52: Result := 'C.OLT.S ' + FS + FT;
      53: Result := 'C.ULT.S ' + FS + FT;
      54: Result := 'C.OLE.S ' + FS + FT;
      55: Result := 'C.ULE.S ' + FS + FT;
      56: Result := 'C.SF.S ' + FS + FT;
      57: Result := 'C.NGLE.S ' + FS + FT;
      58: Result := 'C.SEQ.S ' + FS + FT;
      59: Result := 'C.NGL.S ' + FS + FT;
      60: Result := 'C.LT.S ' + FS + FT;
      61: Result := 'C.NGE.S ' + FS + FT;
      62: Result := 'C.LE.S ' + FS + FT;
      63: Result := 'C.NGT.S ' + FS + FT;
    else Result := '?  COP1 - S';
    end;
      17:     case CurrentOpcode and $3F of
      0: Result := 'ADD.D ' + FD + FS + FT;
      1: Result := 'SUB.D ' + FD + FS + FT;
      2: Result := 'MUL.D ' + FD + FS + FT;
      3: Result := 'DIV.D ' + FD + FS + FT;
      4: Result := 'SQRT.D ' + FD + FS;
      5: Result := 'ABS.D ' + FD + FS;
      6: Result := 'MOV.D ' + FD + FS;
      7: Result := 'NEG.D ' + FD + FS;
      8: Result := 'ROUND.L.D ' + FD + FS;
      9: Result := 'TRUNC.L.D ' + FD + FS;
      10: Result := 'CEIL.L.D ' + FD + FS;
      11: Result := 'FLOOR.L.D ' + FD + FS;
      12: Result := 'ROUND.W.D ' + FD + FS;
      13: Result := 'TRUNC.W.D ' + FD + FS;
      14: Result := 'CEIL.W.D ' + FD + FS;
      15: Result := 'FLOOR.W.D ' + FD + FS;
      32: Result := 'CVT.S.D ' + FD + FS;
      36: Result := 'CVT.W.D ' + FD + FS;
      37: Result := 'CVT.L.D ' + FD + FS;
      48: Result := 'C.F.D ' + FS + FT;
      49: Result := 'C.UN.D ' + FS + FT;
      50: Result := 'C.EQ.D ' + FS + FT;
      51: Result := 'C.UEQ.D ' + FS + FT;
      52: Result := 'C.OLT.D ' + FS + FT;
      53: Result := 'C.ULT.D ' + FS + FT;
      54: Result := 'C.OLE.D ' + FS + FT;
      55: Result := 'C.ULE.D ' + FS + FT;
      56: Result := 'C.SF.D ' + FS + FT;
      57: Result := 'C.NGLE.D ' + FS + FT;
      58: Result := 'C.SEQ.D ' + FS + FT;
      59: Result := 'C.NGL.D ' + FS + FT;
      60: Result := 'C.LT.D ' + FS + FT;
      61: Result := 'C.NGE.D ' + FS + FT;
      62: Result := 'C.LE.D ' + FS + FT;
      63: Result := 'C.NGT.D ' + FS + FT;
    else Result := '?  COP1 - D';
    end;
      20: case CurrentOpcode and $3F of
      32: Result := 'CVT.S.W ' + FD + FS;
      33: Result := 'CVT.D.W ' + FD + FS;
    else Result := '? COP1 - W';
    end;
      21: case CurrentOpcode and $3F of
      32: Result := 'CVT.S.L ' + FD + FS;
      33: Result := 'CVT.D.L ' + FD + FS;
    else Result := '?  COP1 - L';
    end;
    else Result := '? (COP 1)';
    end;
    8: Result := 'ADDI ' + RT + RS + Imm;
    9: Result := 'ADDIU ' + RT + RS + Imm;
    12: Result := 'ANDI ' + RT + RS + Imm;
    4: Result := 'BEQ ' + RS + RT + Branch;
    20: Result := 'BEQL ' + RS + RT + Branch;
    7: Result := 'BGTZ ' + RS + Branch;
    23: Result := 'BGTZL ' + RS + Branch;
    6: Result := 'BLEZ ' + RS + Branch;
    22: Result := 'BLEZL ' + RS + Branch;
    5: if RT = 'R0, ' then Result := 'BNEZ ' + RS + Branch else Result := 'BNE ' + RS + RT + Branch;
    21: Result := 'BNEL ' + RS + RT + Branch;
    47: Result := 'CACHE';
    24: Result := 'DADDI ' + RT + RS + Imm;
    25: Result := 'DADDIU ' + RT + RS + Imm;
    2: Result := 'J ' + Jump;
    3: Result := 'JAL ' + Jump;
    32: Result := 'LB ' + RT + Imm + Base;
    36: Result := 'LBU ' + RT + Imm + Base;
    53: Result := 'LDC1 ' + FT + Imm + Base;
    55: Result := 'LD ' + RT + Imm + Base;
    26: Result := 'LDL ' + RT + Imm + Base;
    27: Result := 'LDR ' + RT + Imm + Base;
    33: Result := 'LH ' + RT + Imm + Base;
    37: Result := 'LHU ' + RT + Imm + Base;
    15: Result := 'LUI ' + RT + Imm;
    35: Result := 'LW ' + RT + Imm + Base;
    49: Result := 'LWC1 ' + FT + Imm + Base;
    34: Result := 'LWL ' + RT + Imm + Base;
    38: Result := 'LWR ' + RT + Imm + Base;
    39: Result := 'LWU ' + RT + Imm + Base;
    13: Result := 'ORI ' + RT + RS + Imm;
    40: Result := 'SB ' + RT + Imm + Base;
    61: Result := 'SDC1 ' + FT + Imm + Base;
    63: Result := 'SD ' + RT + Imm + Base;
    44: Result := 'SDL ' + RT + Imm + Base;
    45: Result := 'SDR ' + RT + Imm + Base;
    41: Result := 'SH ' + RT + Imm + Base;
    10: Result := 'SLTI ' + RT + RS + Imm;
    11: Result := 'SLTIU ' + RT + RS + Imm;
    43: Result := 'SW ' + RT + Imm + Base;
    57: Result := 'SWC1 ' + FT + Imm + Base;
    46: Result := 'SWR ' + RT + Imm + Base;
    42: Result := 'SWL ' + RT + Imm + Base;
    14: Result := 'XORI ' + RT + RS + Imm;
  else Result := '? (' + IntToStr(Int64(Op)) + ')';
  end;
  if Result[Length(Result) - 1] = ',' then SetLength(Result, Length(Result) - 2);
  while Pos(', (', Result) > 0 do Delete(Result, Pos(', (', Result), 1);
end;

const
  VectorElements: array[0..15] of string = (
    ' ', ' [Inv]', ' [0q]', ' [1q]', ' [0h]', ' [1h]', ' [2h]', ' [3h]',
    ' [0]', ' [1]', ' [2]', ' [3]', ' [4]', ' [5]', ' [6]', ' [7]');


function DisassembleRSP(PC, Opcode: Cardinal; RegisterNames: Boolean): string;
  function Op: Cardinal;
  begin
    Result := Opcode shr 26;
  end;

  function Func: Cardinal;
  begin
    Result := Opcode and $3F;
  end;

  function RtIndex: Cardinal;
  begin
    Result := (Opcode shr 16) and $1F;
  end;

  function Branch: string;
  begin
    Result := '0x' + IntToHex(Int64(PC + 4 + Cardinal(SmallInt(Opcode) shl 2)), 8) + ', ';
  end;

  function Imm: string;
  begin
    Result := '0x' + IntToHex(Int64(Opcode and $FFFF), 4) + ', ';
    HasDisplacement := True;
    ImmValue := SmallInt(Opcode);
  end;

  function RD: string;
  begin
    if RegisterNames then Result := GPRNames[(Opcode shr 11) and $1F] + ', '
    else Result := 'R' + IntToStr(Int64((Opcode shr 11) and $1F)) + ', ';
    AddRegister((Opcode shr 11) and $1F);
  end;

  function RS: string;
  begin
    if RegisterNames then Result := GPRNames[(Opcode shr 21) and $1F] + ', '
    else Result := 'R' + IntToStr(Int64((Opcode shr 21) and $1F)) + ', ';
    AddRegister((Opcode shr 21) and $1F);
  end;

  function RT: string;
  begin
    if RegisterNames then Result := GPRNames[(Opcode shr 16) and $1F] + ', '
    else Result := 'R' + IntToStr(Int64((Opcode shr 16) and $1F)) + ', ';
    AddRegister((Opcode shr 16) and $1F);
  end;

  function SA: string;
  begin
    Result := '0x' + IntToHex(Int64((Opcode shr 6) and $1F), 0) + ', ';
  end;

  function Fmt: Cardinal;
  begin
    Result := (Opcode shr 21) and $1F;
  end;

  function CP0: string;
  begin
    Result := RSPCOP0RegNames[(Opcode shr 11) and $1F] + ', ';
  end;

  function Jump: string;
  begin
    Result := '0x' + IntToHex(Int64((Opcode shl 2) and $FFF), 3);
  end;

  function Base: string;
  begin
    if RegisterNames then Result := '(' + GPRNames[(Opcode shr 21) and $1F] + ')'
    else Result := '(R' + IntToStr(Int64((Opcode shr 21) and $1F)) + ')';
    DebuggerAddressReg := (Opcode shr 21) and $1F;
    MemAccess := True;
    AddRegister((Opcode shr 21) and $1F);
  end;

  function T: Cardinal;
  begin
    Result := (Opcode shr 16) and $1F;
  end;

  function D: Cardinal;
  begin
    Result := (Opcode shr 11) and $1F;
  end;

  function S: Cardinal;
  begin
    Result := (Opcode shr 21) and $1F;
  end;

begin
  Reg1 := -1;
  Reg2 := -1;
  Reg3 := -1;
  MemAccess := False;
  HasDisplacement := False;
  case Op of
    0:     case Func of
      32: Result := 'ADD '#1'' + RD + RS + RT;
      33: Result := 'ADDU '#1'' + RD + RS + RT;
      36: Result := 'AND '#1'' + RD + RS + RT;
      13: Result := 'BREAK';
      9: Result := 'JALR #1' + RD + RS;
      8: Result := 'JR '#1'' + RS;
      37: Result := 'OR '#1'' + RD + RS + RT;
      39: Result := 'NOR '#1'' + RD + RS + RT;
      0: if Opcode = 0 then Result := 'NOP' else Result := 'SLL '#1 + RD + RT + SA;
      4: Result := 'SLLV '#1'' + RD + RT + RS;
      42: Result := 'SLT '#1'' + RD + RS + RT;
      43: Result := 'SLTU '#1'' + RD + RS + RT;
      3: Result := 'SRA '#1'' + RD + RT + SA;
      7: Result := 'SRAV '#1'' + RD + RT + RS;
      2: Result := 'SRL '#1'' + RD + RT + SA;
      6: Result := 'SRLV '#1'' + RD + RT + RS;
      34: Result := 'SUB '#1'' + RD + RS + RT;
      35: Result := 'SUBU '#1'' + RD + RS + RT;
      38: Result := 'XOR '#1'' + RD + RS + RT;
    else Result := '? (Special 0x' + IntToHex(Int64(Opcode), 8) + ')';
    end;
    1:     case RtIndex of
      0: Result := 'BLTZ '#1'' + RS + Branch;
      1: Result := 'BGEZ '#1'' + RS + Branch;
      16: Result := 'BLTZAL '#1'' + RS + Branch;
      17: Result := 'BGEZAL '#1'' + RS + Branch;
    else Result := '? (Regimm)';
    end;
    16:     case Fmt of
      0: Result := 'MFC0 '#1'' + RT + CP0;
      4: Result := 'MTC0 '#1'' + RT + CP0;
    else Result := '? (COP0)';
    end;
    18: case Fmt of
      0: Result := 'MFC2 '#1 + RT + 'v' + IntToStr(Int64((Opcode shr 11) and $1F)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) + ']';
      4: Result := 'MTC2 '#1 + RT + 'v' + IntToStr(Int64((Opcode shr 11) and $1F)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) + ']';
      2: Result := 'CFC2 '#1 + RT + RSPFlagNames[(Opcode shr 11) and $1F];
      6: Result := 'CTC2 '#1 + RT + RSPFlagNames[(Opcode shr 11) and $1F];
    else
      if Fmt and $10 <> 0 then
      begin
        AddRegister(((Opcode shr 6) and $1F) + 32);
        AddRegister(((Opcode shr 16) and $1F) + 32);
        AddRegister(((Opcode shr 21) and $1F) + 32);
        case Opcode and $3F of
          0: Result := 'VMULF '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          1: Result := 'VMULU '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          2: Result := 'VRNDP '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          3: Result := 'VMULQ '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          4: Result := 'VMUDL '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          5: Result := 'VMUDM '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          6: Result := 'VMUDN '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          7: Result := 'VMUDH '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          8: Result := 'VMACF '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          9: Result := 'VMACU '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          10: Result := 'VRNDN '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          11: Result := 'VMACQ '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          12: Result := 'VMADL '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          13: Result := 'VMADM '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          14: Result := 'VMADN '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          15: Result := 'VMADH '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          16: Result := 'VADD '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          17: Result := 'VSUB '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          18: Result := 'VSUT '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          19: Result := 'VABS '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          20: Result := 'VADDC '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          21: Result := 'VSUBC '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          22: Result := 'VADDB '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          23: Result := 'VSUBB '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          24: Result := 'VACCB '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          25: Result := 'VSUCB '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          26: Result := 'VSAD '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          27: Result := 'VSAC '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          28: Result := 'VSUM '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          29: Result := 'VSAW '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          32: Result := 'VLT '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          33: Result := 'VEQ '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          34: Result := 'VNE '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          35: Result := 'VGE '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          36: Result := 'VCL '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          37: Result := 'VCH '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          38: Result := 'VCR '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          39: Result := 'VMRG '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          40: Result := 'VAND '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          41: Result := 'VNAND '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          42: Result := 'VOR '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          43: Result := 'VNOR '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          44: Result := 'VXOR '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          45: Result := 'VNXOR '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          48: Result := 'VRCP  '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          49: Result := 'VRCPL '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          50: Result := 'VRCPH '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          51: Result := 'VMOV '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          52: Result := 'VRSQ '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          53: Result := 'VRSQL '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          54: Result := 'VRSQH '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          55: Result := 'VNOOP '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          56: Result := 'VEXTT '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          57: Result := 'VEXTQ '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          58: Result := 'VEXTN '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          60: Result := 'VINST '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          61: Result := 'VINSQ '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
          62: Result := 'VINSN '#1'v' + IntToStr(Int64((Opcode shr 6) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 11) and $1F)) +
    ', v' + IntToStr(Int64((Opcode shr 16) and $1F)) + VectorElements[(Opcode shr 21) and $F];
        end;
      end;
    end;
    8: Result := 'ADDI '#1'' + RT + RS + Imm;
    9: Result := 'ADDIU '#1'' + RT + RS + Imm;
    12: Result := 'ANDI '#1'' + RT + RS + Imm;
    4: Result := 'BEQ '#1'' + RS + RT + Branch;
    7: Result := 'BGTZ '#1'' + RS + Branch;
    6: Result := 'BLEZ '#1'' + RS + Branch;
    5: if RT = 'R0, ' then Result := 'BNEZ '#1 + RS + Branch else Result := 'BNE '#1 + RS + RT + Branch;
    2: Result := 'J '#1'' + Jump;
    3: Result := 'JAL '#1'' + Jump;
    32: Result := 'LB '#1'' + RT + Imm + Base;
    36: Result := 'LBU '#1'' + RT + Imm + Base;
    33: Result := 'LH '#1'' + RT + Imm + Base;
    37: Result := 'LHU '#1'' + RT + Imm + Base;
    15: Result := 'LUI '#1'' + RT + Imm;
    35: Result := 'LW '#1'' + RT + Imm + Base;
    50: begin
  AddRegister(T + 32);
  AddRegister(S);
  DebuggerAddressReg := S;
  ImmValue := Opcode and $7F;
  MemAccess := True;
  case D of
    0: begin
      ImmValue := (ImmValue shl 0) and $FFF;
      Result := 'LBV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    1: begin
      ImmValue := (ImmValue shl 1) and $FFF;
      Result := 'LSV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    2: begin
      ImmValue := (ImmValue shl 2) and $FFF;
      Result := 'LLV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    3: begin
      ImmValue := (ImmValue shl 3) and $FFF;
      Result := 'LDV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    4: begin
      ImmValue := (ImmValue shl 4) and $FFF;
      Result := 'LQV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    5: begin
      ImmValue := (ImmValue shl 4) and $FFF;
      Result := 'LRV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    6: begin
      ImmValue := (ImmValue shl 3) and $FFF;
      Result := 'LPV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    7: begin
      ImmValue := (ImmValue shl 3) and $FFF;
      Result := 'LUV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    8: begin
      ImmValue := (ImmValue shl 4) and $FFF;
      Result := 'LHV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    9: begin
      ImmValue := (ImmValue shl 4) and $FFF;
      Result := 'LFV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    10: begin
      ImmValue := (ImmValue shl 4) and $FFF;
      Result := 'LWV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    11: begin
      ImmValue := (ImmValue shl 4) and $FFF;
      Result := 'LTV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
  end;
  if ImmValue and $800 <> 0 then ImmValue := ImmValue or $FFFF000;
end;
    13: Result := 'ORI '#1'' + RT + RS + Imm;
    40: Result := 'SB '#1'' + RT + Imm + Base;
    41: Result := 'SH '#1'' + RT + Imm + Base;
    10: Result := 'SLTI '#1'' + RT + RS + Imm;
    11: Result := 'SLTIU '#1'' + RT + RS + Imm;
    43: Result := 'SW '#1'' + RT + Imm + Base;
    58: begin
  AddRegister(T + 32);
  AddRegister(S);
  MemAccess := True;
  ImmValue := Opcode and $7F;
  DebuggerAddressReg := S;
  case D of
    0: begin
      ImmValue := (ImmValue shl 0) and $FFF;
      Result := 'SBV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    1: begin
      ImmValue := (ImmValue shl 1) and $FFF;
      Result := 'SSV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    2: begin
      ImmValue := (ImmValue shl 2) and $FFF;
      Result := 'SLV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    3: begin
      ImmValue := (ImmValue shl 3) and $FFF;
      Result := 'SDV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    4: begin
      ImmValue := (ImmValue shl 4) and $FFF;
      Result := 'SQV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    5: begin
      ImmValue := (ImmValue shl 4) and $FFF;
      Result := 'SRV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    6: begin
      ImmValue := (ImmValue shl 3) and $FFF;
      Result := 'SPV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    7: begin
      ImmValue := (ImmValue shl 3) and $FFF;
      Result := 'SUV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    8: begin
      ImmValue := (ImmValue shl 4) and $FFF;
      Result := 'SHV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    9: begin
      ImmValue := (ImmValue shl 4) and $FFF;
      Result := 'SFV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    10: begin
      ImmValue := (ImmValue shl 4) and $FFF;
      Result := 'SWV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
    11: begin
      ImmValue := (ImmValue shl 4) and $FFF;
      Result := 'STV '#1'v' + IntToStr(Int64(T)) + '[' + IntToStr(Int64((Opcode shr 7) and $F)) +
        '], 0x' + IntToHex(ImmValue, 3) + ' (' + RS + ')';
    end;
  end;
  if ImmValue and $800 <> 0 then ImmValue := ImmValue or $FFFF000;
end;
    14: Result := 'XORI '#1'' + RT + RS + Imm;
  else Result := '? (' + IntToStr(Int64(Op)) + ')';
  end;
  if Result[Length(Result) - 1] = ',' then SetLength(Result, Length(Result) - 2);
  while Pos(', (', Result) > 0 do Delete(Result, Pos(', (', Result), 1);
  while Pos(', )', Result) > 0 do Delete(Result, Pos(', )', Result), 2);
  while Pos(',)', Result) > 0 do Delete(Result, Pos(',)', Result), 1);
end;
function DisassembleRDP(Position: Cardinal; Word0, Word1: Int64): string;
begin
  case (Word0 shr 56) or $C0 of
    $FF: Result := 'G_SETCIMG';
    $FE: Result := 'G_SETZIMG';
    $FD: Result := 'G_SETTIMG';
    $FC: Result := 'G_SETCOMBINE';
    $FB: Result := 'G_SETENVCOLOR';
    $FA: Result := 'G_SETPRIMCOLOR';
    $F9: Result := 'G_SETBLENDCOLOR';
    $F8: Result := 'G_SETFOGCOLOR';
    $F7: Result := 'G_SETFILLCOLOR';
    $F6: Result := 'G_FILLRECT';
    $F5: Result := 'G_SETTILE';
    $F4: Result := 'G_LOADTILE';
    $F3: Result := 'G_LOADBLOCK';
    $F2: Result := 'G_SETTILESIZE';
    $F0: Result := 'G_LOADTLUT';
    $EF: Result := 'G_RDPSETOTHERMODE';
    $EE: Result := 'G_SETPRIMDEPTH';
    $ED: Result := 'G_SETSCISSOR';
    $EC: Result := 'G_SETCONVERT';
    $EB: Result := 'G_SETKEYR';
    $EA: Result := 'G_SETKEYGB';
    $E9: Result := 'G_RDPFULLSYNC';
    $E8: Result := 'G_RDPTILESYNC';
    $E7: Result := 'G_RDPPIPESYNC';
    $E6: Result := 'G_RDPLOADSYNC';
    $E5: Result := 'G_TEXRECTFLIP';
    $E4: Result := 'G_TEXRECT x1:' +
      FloatToStrF(((Word0 shr 44) and $FFF) / 4, ffFixed, 7, 2) + ' y1:' +
      FloatToStrF(((Word0 shr 32) and $FFF) / 4, ffFixed, 7, 2) + ' x2:' +
      FloatToStrF(((Word0 shr 12) and $FFF) / 4, ffFixed, 7, 2) + ' y2:' +
      FloatToStrF((Word0 and $FFF) / 4, ffFixed, 7, 2);
    $C8: Result := 'TRI_FILL';
    $CC: Result := 'TRI_SHADE';
    $CA: Result := 'TRI_TXTR';
    $CE: Result := 'TRI_SHADE_TXTR';
    $C9: Result := 'TRI_FILL_ZBUFF';
    $CD: Result := 'TRI_SHADE_ZBUFF';
    $CB: Result := 'TRI_TXTR_ZBUFF';
    $CF: Result := 'TRI_SHADE_TXTR_ZBUFF';
  else
    Result := 'Unknown rdp command: ' + IntToHex(Word0, 8);
  end;
end;

end.
