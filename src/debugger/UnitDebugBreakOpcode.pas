unit UnitDebugBreakOpcode;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, CheckLst;

type
  TFormDebugBreakOpcode = class(TForm)
    CheckListBox: TCheckListBox;
    procedure CheckListBoxClickCheck(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
  private
    procedure UpdateOpcodeBreakpoints;
  end;

var
  FormDebugBreakOpcode: TFormDebugBreakOpcode;

implementation

uses UnitDecs;

{$R *.dfm}

{ The helper name and local names are inferred. The ordered comparisons and
  masks preserve the compiler-observed decision tree at original VA $004B794C. }
procedure TFormDebugBreakOpcode.UpdateOpcodeBreakpoints;
var
  I: Integer;
  Name: string;
  Invalid: Boolean;
  Opcode, Mask: Cardinal;
begin
  Nemu64_API.OpcodeBreakpointCount^ := 0;
  for I := 0 to CheckListBox.Items.Count - 1 do
    if CheckListBox.Checked[I] then
    begin
      Name := CheckListBox.Items[I];
      Invalid := False;
      Opcode := 0;
      Mask := $FFE0003F;
      if Name = 'ABS.D' then
        Opcode := $46200005
      else if Name = 'ABS.S' then
        Opcode := $46000005
      else if Name = 'ADD.D' then
        Opcode := $46200000
      else if Name = 'ADD.S' then
        Opcode := $46000000
      else if Name = 'C.EQ.D' then
        Opcode := $46200032
      else if Name = 'C.EQ.S' then
        Opcode := $46000032
      else if Name = 'C.UEQ.D' then
        Opcode := $46200033
      else if Name = 'C.UEQ.S' then
        Opcode := $46000033
      else if Name = 'C.UN.D' then
        Opcode := $46200031
      else if Name = 'C.UN.S' then
        Opcode := $46000031
      else if Name = 'C.F.D' then
        Opcode := $46200030
      else if Name = 'C.F.S' then
        Opcode := $46000030
      else if Name = 'C.OLT.D' then
        Opcode := $46200034
      else if Name = 'C.OLT.S' then
        Opcode := $46000034
      else if Name = 'C.ULT.D' then
        Opcode := $46200035
      else if Name = 'C.ULT.S' then
        Opcode := $46000035
      else if Name = 'C.OLE.D' then
        Opcode := $46200036
      else if Name = 'C.OLE.S' then
        Opcode := $46000036
      else if Name = 'C.ULE.D' then
        Opcode := $46200037
      else if Name = 'C.ULE.S' then
        Opcode := $46000037
      else if Name = 'C.SF.D' then
        Opcode := $46200038
      else if Name = 'C.SF.S' then
        Opcode := $46000038
      else if Name = 'C.NGLE.D' then
        Opcode := $46200039
      else if Name = 'C.NGLE.S' then
        Opcode := $46000039
      else if Name = 'C.SEQ.D' then
        Opcode := $4620003A
      else if Name = 'C.SEQ.S' then
        Opcode := $4600003A
      else if Name = 'C.SF.D' then
        Opcode := $46200038
      else if Name = 'C.SF.S' then
        Opcode := $46000038
      else if Name = 'C.NGL.D' then
        Opcode := $4620003B
      else if Name = 'C.NGL.S' then
        Opcode := $4600003B
      else if Name = 'C.LT.D' then
        Opcode := $4620003C
      else if Name = 'C.LT.S' then
        Opcode := $4600003C
      else if Name = 'C.NGE.D' then
        Opcode := $4620003D
      else if Name = 'C.NGE.S' then
        Opcode := $4600003D
      else if Name = 'C.LE.D' then
        Opcode := $4620003E
      else if Name = 'C.LE.S' then
        Opcode := $4600003E
      else if Name = 'C.NGT.D' then
        Opcode := $4620003F
      else if Name = 'C.NGT.S' then
        Opcode := $4600003F
      else if Name = 'CEIL.L.D' then
        Opcode := $4620000A
      else if Name = 'CEIL.L.S' then
        Opcode := $4600000A
      else if Name = 'CEIL.W.D' then
        Opcode := $4620000E
      else if Name = 'CEIL.W.S' then
        Opcode := $4600000E
      else if Name = 'CVT.D.S' then
        Opcode := $46000021
      else if Name = 'CVT.D.W' then
        Opcode := $46800021
      else if Name = 'CVT.D.L' then
        Opcode := $46A00021
      else if Name = 'CVT.L.S' then
        Opcode := $46000025
      else if Name = 'CVT.L.D' then
        Opcode := $46200025
      else if Name = 'CVT.S.D' then
        Opcode := $46200020
      else if Name = 'CVT.S.W' then
        Opcode := $46800020
      else if Name = 'CVT.S.L' then
        Opcode := $46A00020
      else if Name = 'CVT.W.S' then
        Opcode := $46000024
      else if Name = 'CVT.W.D' then
        Opcode := $46200024
      else if Name = 'DIV.S' then
        Opcode := $46000003
      else if Name = 'DIV.D' then
        Opcode := $46200003
      else if Name = 'FLOOR.L.S' then
        Opcode := $4600000B
      else if Name = 'FLOOR.L.D' then
        Opcode := $4620000B
      else if Name = 'FLOOR.W.S' then
        Opcode := $4600000F
      else if Name = 'FLOOR.W.D' then
        Opcode := $4620000F
      else if Name = 'MOV.S' then
        Opcode := $46000006
      else if Name = 'MOV.D' then
        Opcode := $46200006
      else if Name = 'MUL.S' then
        Opcode := $46000002
      else if Name = 'MUL.D' then
        Opcode := $46200002
      else if Name = 'NEG.S' then
        Opcode := $46000007
      else if Name = 'NEG.D' then
        Opcode := $46200007
      else if Name = 'ROUND.L.S' then
        Opcode := $46000008
      else if Name = 'ROUND.L.D' then
        Opcode := $46200008
      else if Name = 'ROUND.W.S' then
        Opcode := $4600000C
      else if Name = 'ROUND.W.D' then
        Opcode := $4620000C
      else if Name = 'SQRT.S' then
        Opcode := $46000004
      else if Name = 'SQRT.D' then
        Opcode := $46200004
      else if Name = 'SUB.S' then
        Opcode := $46000001
      else if Name = 'SUB.D' then
        Opcode := $46200001
      else if Name = 'TRUNC.L.S' then
        Opcode := $46000009
      else if Name = 'TRUNC.L.D' then
        Opcode := $46200009
      else if Name = 'TRUNC.W.S' then
        Opcode := $4600000D
      else if Name = 'TRUNC.W.D' then
        Opcode := $4620000D
      else if Name = 'TRUNC.W.S' then
        Opcode := $4600000D
      else if Name = 'TRUNC.W.D' then
        Opcode := $4620000D
      else
      begin
        Mask := $FC000000;
        if Name = 'ADDI' then
          Opcode := $20000000
        else if Name = 'ADDIU' then
          Opcode := $24000000
        else if Name = 'ANDI' then
          Opcode := $30000000
        else if Name = 'BEQ' then
          Opcode := $10000000
        else if Name = 'BEQL' then
          Opcode := $50000000
        else if Name = 'BGTZ' then
          Opcode := $1C000000
        else if Name = 'BGTZL' then
          Opcode := $5C000000
        else if Name = 'BLEZ' then
          Opcode := $18000000
        else if Name = 'BLEZL' then
          Opcode := $58000000
        else if Name = 'BNE' then
          Opcode := $14000000
        else if Name = 'BNEL' then
          Opcode := $54000000
        else if Name = 'CACHE' then
          Opcode := $BC000000
        else if Name = 'DADDI' then
          Opcode := $60000000
        else if Name = 'DADDIU' then
          Opcode := $64000000
        else if Name = 'J' then
          Opcode := $08000000
        else if Name = 'JAL' then
          Opcode := $0C000000
        else if Name = 'LB' then
          Opcode := $80000000
        else if Name = 'LBU' then
          Opcode := $90000000
        else if Name = 'LB' then
          Opcode := $80000000
        else if Name = 'LBU' then
          Opcode := $90000000
        else if Name = 'LD' then
          Opcode := $DC000000
        else if Name = 'LDC1' then
          Opcode := $D4000000
        else if Name = 'LDL' then
          Opcode := $68000000
        else if Name = 'LDR' then
          Opcode := $6C000000
        else if Name = 'LH' then
          Opcode := $84000000
        else if Name = 'LHU' then
          Opcode := $94000000
        else if Name = 'LL' then
          Opcode := $C0000000
        else if Name = 'LLD' then
          Opcode := $D0000000
        else if Name = 'LUI' then
          Opcode := $3C000000
        else if Name = 'LW' then
          Opcode := $8C000000
        else if Name = 'LWC1' then
          Opcode := $C4000000
        else if Name = 'LWL' then
          Opcode := $88000000
        else if Name = 'LWR' then
          Opcode := $98000000
        else if Name = 'LWU' then
          Opcode := $9C000000
        else if Name = 'ORI' then
          Opcode := $34000000
        else if Name = 'SB' then
          Opcode := $A0000000
        else if Name = 'SC' then
          Opcode := $E0000000
        else if Name = 'SCD' then
          Opcode := $F0000000
        else if Name = 'SD' then
          Opcode := $FC000000
        else if Name = 'SDC1' then
          Opcode := $F4000000
        else if Name = 'SDL' then
          Opcode := $B0000000
        else if Name = 'SDR' then
          Opcode := $B4000000
        else if Name = 'SH' then
          Opcode := $A4000000
        else if Name = 'SLTI' then
          Opcode := $28000000
        else if Name = 'SLTIU' then
          Opcode := $2C000000
        else if Name = 'SW' then
          Opcode := $AC000000
        else if Name = 'SWC1' then
          Opcode := $E4000000
        else if Name = 'SWL' then
          Opcode := $A8000000
        else if Name = 'SWR' then
          Opcode := $B8000000
        else if Name = 'XORI' then
          Opcode := $38000000
        else
        begin
          Mask := $FC00003F;
          if Name = 'ADD' then
            Opcode := $00000020
          else if Name = 'ADDU' then
            Opcode := $00000021
          else if Name = 'AND' then
            Opcode := $00000024
          else if Name = 'BREAK' then
            Opcode := $0000000D
          else if Name = 'DADD' then
            Opcode := $0000002C
          else if Name = 'DADDU' then
            Opcode := $0000002D
          else if Name = 'DDIV' then
            Opcode := $0000001E
          else if Name = 'DDIVU' then
            Opcode := $0000001F
          else if Name = 'DIV' then
            Opcode := $0000001A
          else if Name = 'DIVU' then
            Opcode := $0000001B
          else if Name = 'DMULT' then
            Opcode := $0000001C
          else if Name = 'DMULTU' then
            Opcode := $0000001D
          else if Name = 'DSLL' then
            Opcode := $00000038
          else if Name = 'DSLL32' then
            Opcode := $0000003C
          else if Name = 'DSLLV' then
            Opcode := $00000014
          else if Name = 'DSRA' then
            Opcode := $0000003B
          else if Name = 'DSRA32' then
            Opcode := $0000003F
          else if Name = 'DSRAV' then
            Opcode := $00000017
          else if Name = 'DSRL' then
            Opcode := $0000003A
          else if Name = 'DSRL32' then
            Opcode := $0000003E
          else if Name = 'DSRLV' then
            Opcode := $00000016
          else if Name = 'DSUB' then
            Opcode := $0000002E
          else if Name = 'DSUBU' then
            Opcode := $0000002F
          else if Name = 'JALR' then
            Opcode := $00000009
          else if Name = 'JR' then
            Opcode := $00000008
          else if Name = 'MFHI' then
            Opcode := $00000010
          else if Name = 'MFLO' then
            Opcode := $00000012
          else if Name = 'MTHI' then
            Opcode := $00000011
          else if Name = 'MTLO' then
            Opcode := $00000013
          else if Name = 'MULT' then
            Opcode := $00000018
          else if Name = 'MULTU' then
            Opcode := $00000019
          else if Name = 'NOR' then
            Opcode := $00000027
          else if Name = 'OR' then
            Opcode := $00000025
          else if Name = 'SLL' then
            Opcode := $00000000
          else if Name = 'SLLV' then
            Opcode := $00000004
          else if Name = 'SLT' then
            Opcode := $0000002A
          else if Name = 'SLTU' then
            Opcode := $0000002B
          else if Name = 'SRA' then
            Opcode := $00000003
          else if Name = 'SRAV' then
            Opcode := $00000007
          else if Name = 'SRL' then
            Opcode := $00000002
          else if Name = 'SRLV' then
            Opcode := $00000006
          else if Name = 'SUB' then
            Opcode := $00000022
          else if Name = 'SUBU' then
            Opcode := $00000023
          else if Name = 'SYNC' then
            Opcode := $0000000F
          else if Name = 'SYSCALL' then
            Opcode := $0000000C
          else if Name = 'TEQ' then
            Opcode := $00000034
          else if Name = 'TGE' then
            Opcode := $00000030
          else if Name = 'TGEU' then
            Opcode := $00000031
          else if Name = 'TLT' then
            Opcode := $00000032
          else if Name = 'TLTU' then
            Opcode := $00000033
          else if Name = 'TNE' then
            Opcode := $00000036
          else if Name = 'XOR' then
            Opcode := $00000026
          else
          begin
            Mask := $FC1F0000;
            if Name = 'BGEZ' then
              Opcode := $04010000
            else if Name = 'BGEZAL' then
              Opcode := $04110000
            else if Name = 'BGEZALL' then
              Opcode := $04130000
            else if Name = 'BGEZL' then
              Opcode := $04030000
            else if Name = 'BLTZ' then
              Opcode := $04000000
            else if Name = 'BLTZAL' then
              Opcode := $04100000
            else if Name = 'BLTZALL' then
              Opcode := $04120000
            else if Name = 'BLTZL' then
              Opcode := $04020000
            else if Name = 'TEQI' then
              Opcode := $040C0000
            else if Name = 'TGEI' then
              Opcode := $04080000
            else if Name = 'TGEIU' then
              Opcode := $04090000
            else if Name = 'TLTI' then
              Opcode := $040A0000
            else if Name = 'TLTIU' then
              Opcode := $040B0000
            else if Name = 'TNEI' then
              Opcode := $040E0000
            else
            begin
              Mask := $FFE00000;
              if Name = 'CFC1' then
                Opcode := $44400000
              else if Name = 'CTC1' then
                Opcode := $44C00000
              else if Name = 'DMFC1' then
                Opcode := $44200000
              else if Name = 'DMTC1' then
                Opcode := $44A00000
              else if Name = 'MFC1' then
                Opcode := $44000000
              else if Name = 'MTC1' then
                Opcode := $44800000
              else
              begin
                Mask := $FFFF0000;
                if Name = 'BC1F' then
                  Opcode := $45000000
                else if Name = 'BC1FL' then
                  Opcode := $45020000
                else if Name = 'BC1T' then
                  Opcode := $45010000
                else if Name = 'BC1TL' then
                  Opcode := $45030000
                else
                begin
                  Mask := $FFE00000;
                  if Name = 'MFC0' then
                    Opcode := $40000000
                  else if Name = 'MTC0' then
                    Opcode := $40800000
                  else
                  begin
                    Mask := $FFE0003F;
                    if Name = 'TLBR' then
                      Opcode := $42000001
                    else if Name = 'TLBWI' then
                      Opcode := $42000002
                    else if Name = 'TLBWR' then
                      Opcode := $42000006
                    else if Name = 'TLBP' then
                      Opcode := $42000008
                    else if Name = 'ERET' then
                      Opcode := $42000018
                    else
                    begin
                      Application.MessageBox(PChar('Bug: ' + Name), 'Bug', MB_ICONERROR);
                      Invalid := True;
                    end;
                  end;
                end;
              end;
            end;
          end;
        end;
      end;
      if not Invalid then
      begin
        Nemu64_API.OpcodeBreakpoints^[Nemu64_API.OpcodeBreakpointCount^].Opcode := Opcode;
        Nemu64_API.OpcodeBreakpoints^[Nemu64_API.OpcodeBreakpointCount^].Mask := Mask;
        Inc(Nemu64_API.OpcodeBreakpointCount^);
      end;
    end;
end;

procedure TFormDebugBreakOpcode.CheckListBoxClickCheck(Sender: TObject);
begin
  UpdateOpcodeBreakpoints;
  Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $400;
end;

procedure TFormDebugBreakOpcode.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Action := caHide;
end;

end.
