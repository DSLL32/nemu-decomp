unit UnitOpcode;

interface

const
  OpcodeNames: array[0..63] of string = (
    'SPECIAL', 'REGIMM', 'J', 'JAL',
    'BEQ', 'BNE', 'BLEZ', 'BGTZ',
    'ADDI', 'ADDIU', 'SLTI', 'SLTIU',
    'ANDI', 'ORI', 'XORI', 'LUI',
    'COP0', 'COP1', 'unused', 'unused',
    'BEQL', 'BNEL', 'BLEZL', 'BGTZL',
    'DADDI', 'DADDIU', 'LDL', 'LDR',
    'unused', 'unused', 'unused', 'unused',
    'LB', 'LH', 'LWL', 'LW',
    'LBU', 'LHU', 'LWR', 'LWU',
    'SB', 'SH', 'SWL', 'SW',
    'SDL', 'SDR', 'SWR', 'CACHE',
    'LL', 'LWC1', 'LWC2', 'unused',
    'LLD', 'LDC1', 'LDC2', 'LD',
    'SC', 'SWC1', 'SWC2', 'unused',
    'SCD', 'SDC1', 'SDC2', 'SD');

  SpecialNames: array[0..63] of string = (
    'SLL', 'unused', 'SRL', 'SRA',
    'SLLV', 'unused', 'SRLV', 'SRAV',
    'JR', 'JALR', 'unused', 'unused',
    'SYSCALL', 'BREAK', 'unused', 'SYNC',
    'MFHI', 'MTHI', 'MFLO', 'MTLO',
    'DSLLV', 'unused', 'DSRLV', 'DSRAV',
    'MULT', 'MULTU', 'DIV', 'DIVU',
    'DMULT', 'DMULTU', 'DDIV', 'DDIVU',
    'ADD', 'ADDU', 'SUB', 'SUBU',
    'AND', 'OR', 'XOR', 'NOR',
    'unused', 'unused', 'SLT', 'SLTU',
    'DADD', 'DADDU', 'DSUB', 'DSUBU',
    'TGE', 'TGEU', 'TLT', 'TLTU',
    'TEQ', 'unused', 'TNE', 'unused',
    'DSLL', 'unused', 'DSRL', 'DSRA',
    'DSLL32', 'unused', 'DSRL32', 'DSRA32');

  RegImmNames: array[0..19] of string = (
    'BLTZ', 'BGEZ', 'BLTZL', 'BGEZL',
    'unused', 'unused', 'unused', 'unused',
    'TGEI', 'TGEIU', 'TLTI', 'TLTIU',
    'TEQI', 'unused', 'TNEI', 'unused',
    'BLTZAL', 'BGEZAL', 'BLTZALL', 'BGEZALL');

  Cop0Names: array[0..31] of string = (
    'MFC0', 'unused', 'unused', 'unused',
    'MTC0', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'TLB', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused');

  TLBNames: array[0..63] of string = (
    'unused', 'TLBR', 'TLBWI', 'unused',
    'unused', 'unused', 'TLBWR', 'unused',
    'TLBP', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'ERET', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused');

  Cop1Names: array[0..31] of string = (
    'MFC1', 'DMFC1', 'CFC1', 'unused',
    'MTC1', 'DMTC1', 'CTC1', 'unused',
    'BC', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'S', 'D', 'unused', 'unused',
    'W', 'L', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused');

  Cop1BranchNames: array[0..3] of string = (
    'BC1F', 'BC1T', 'BC1FL', 'BC1TL');

  Cop1FunctionNames: array[0..63] of string = (
    'ADD', 'SUB', 'MUL', 'DIV',
    'SQRT', 'ABS', 'MOV', 'NEG',
    'ROUND.L', 'TRUNC.L', 'CEIL.L', 'FLOOR.L',
    'ROUND.W', 'TRUNC.W', 'CEIL.W', 'FLOOR.W',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'CVT.S', 'CVT.D', 'unused', 'unused',
    'CVT.W', 'CVT.L', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'unused', 'unused', 'unused', 'unused',
    'C.F', 'C.UN', 'C.EQ', 'C.UEQ',
    'C.OLT', 'C.ULT', 'C.OLE', 'C.ULE',
    'C.SF', 'C.NGLE', 'C.SEQ', 'C.NGL',
    'C.LT', 'C.NGE', 'C.LE', 'C.NGT');

implementation

end.
