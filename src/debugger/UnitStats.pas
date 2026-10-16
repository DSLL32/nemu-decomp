unit UnitStats;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls, Menus, Buttons, ThemeMgr;

type
  TFormStats = class(TForm)
    PageControl: TPageControl;
    TabSheetRegStats: TTabSheet;
    Memo: TMemo;
    Panel: TPanel;
    CheckBoxRegisterStat: TCheckBox;
    ButtonReset: TButton;
    ButtonRead: TButton;
    TabSheetOpcode: TTabSheet;
    Panel1: TPanel;
    CheckBoxToggleOpcodeStats: TCheckBox;
    ButtonResetOpcodeStats: TButton;
    ButtonReadOpcodeStats: TButton;
    MemoOpcodeStats: TMemo;
    CheckBoxDivideVI: TCheckBox;
    TabSheetBlockStats: TTabSheet;
    Panel2: TPanel;
    CheckBox1: TCheckBox;
    Button1: TButton;
    ButtonBProfRead: TButton;
    ListView: TListView;
    ThemeManager: TThemeManager;
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure CheckBoxRegisterStatClick(Sender: TObject);
    procedure ButtonResetClick(Sender: TObject);
    procedure ButtonReadClick(Sender: TObject);
    procedure CheckBoxToggleOpcodeStatsClick(Sender: TObject);
    procedure ButtonResetOpcodeStatsClick(Sender: TObject);
    procedure ButtonReadOpcodeStatsClick(Sender: TObject);
    procedure ButtonBProfReadClick(Sender: TObject);
    procedure ListViewCompare(Sender: TObject; Item1, Item2: TListItem; Data: Integer; var Compare: Integer);
    procedure ListViewColumnClick(Sender: TObject; Column: TListColumn);
    procedure ListViewDblClick(Sender: TObject);
    procedure CheckBox1Click(Sender: TObject);
    procedure Button1Click(Sender: TObject);
  private
    SortColumn: Integer;
    procedure AddOpcodeStat(Name: string; Value: Integer);
  end;

var
  FormStats: TFormStats;

implementation

uses UnitDecs, UnitDebugCommands;

{$R *.dfm}

procedure TFormStats.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Action := caHide;
end;

procedure TFormStats.CheckBoxRegisterStatClick(Sender: TObject);
begin
  Nemu64_API.RegisterStatsEnabled^ := (Sender as TCheckBox).Checked;
  Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $400;
end;

procedure TFormStats.ButtonResetClick(Sender: TObject);
begin
  FillChar(Nemu64_API.RegisterStats^, 128, 0);
end;

procedure TFormStats.ButtonReadClick(Sender: TObject);
var
  I: Cardinal;
begin
  Memo.Lines.Clear;
  for I := 0 to 31 do
    Memo.Lines.Add('Register #' + IntToStr(I) + ': ' + IntToStr(Nemu64_API.RegisterStats^[I]));
end;

procedure TFormStats.CheckBoxToggleOpcodeStatsClick(Sender: TObject);
begin
  Nemu64_API.OpcodeStatsEnabled^ := (Sender as TCheckBox).Checked;
  Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $400;
end;

procedure TFormStats.ButtonResetOpcodeStatsClick(Sender: TObject);
begin
  FillChar(Nemu64_API.OpcodeStats^, $880, 0);
  Nemu64_API.VICount^ := 0;
end;

procedure TFormStats.AddOpcodeStat(Name: string; Value: Integer);
begin
  if Value <> 0 then
    if CheckBoxDivideVI.Checked then
      MemoOpcodeStats.Lines.Add(Name + IntToStr(Cardinal(Value) div Nemu64_API.VICount^))
    else
      MemoOpcodeStats.Lines.Add(Name + IntToStr(Value));
end;

procedure TFormStats.ButtonReadOpcodeStatsClick(Sender: TObject);
begin
  MemoOpcodeStats.Lines.Clear;
  AddOpcodeStat('ADDI: ', Nemu64_API.OpcodeStats^[$8]);
  AddOpcodeStat('ADDIU: ', Nemu64_API.OpcodeStats^[$9]);
  AddOpcodeStat('ANDI: ', Nemu64_API.OpcodeStats^[$C]);
  AddOpcodeStat('BEQ: ', Nemu64_API.OpcodeStats^[$4]);
  AddOpcodeStat('BEQL: ', Nemu64_API.OpcodeStats^[$14]);
  AddOpcodeStat('BGTZ: ', Nemu64_API.OpcodeStats^[$7]);
  AddOpcodeStat('BGTZL: ', Nemu64_API.OpcodeStats^[$17]);
  AddOpcodeStat('BLEZ: ', Nemu64_API.OpcodeStats^[$6]);
  AddOpcodeStat('BLEZL: ', Nemu64_API.OpcodeStats^[$16]);
  AddOpcodeStat('BNE: ', Nemu64_API.OpcodeStats^[$5]);
  AddOpcodeStat('BNEL: ', Nemu64_API.OpcodeStats^[$15]);
  AddOpcodeStat('CACHE: ', Nemu64_API.OpcodeStats^[$2F]);
  AddOpcodeStat('DADDI: ', Nemu64_API.OpcodeStats^[$18]);
  AddOpcodeStat('DADDIU: ', Nemu64_API.OpcodeStats^[$19]);
  AddOpcodeStat('J: ', Nemu64_API.OpcodeStats^[$2]);
  AddOpcodeStat('JAL: ', Nemu64_API.OpcodeStats^[$3]);
  AddOpcodeStat('LB: ', Nemu64_API.OpcodeStats^[$20]);
  AddOpcodeStat('LBU: ', Nemu64_API.OpcodeStats^[$24]);
  AddOpcodeStat('LD: ', Nemu64_API.OpcodeStats^[$37]);
  AddOpcodeStat('LDC1: ', Nemu64_API.OpcodeStats^[$35]);
  AddOpcodeStat('LDL: ', Nemu64_API.OpcodeStats^[$1A]);
  AddOpcodeStat('LDR: ', Nemu64_API.OpcodeStats^[$1B]);
  AddOpcodeStat('LH: ', Nemu64_API.OpcodeStats^[$21]);
  AddOpcodeStat('LHU: ', Nemu64_API.OpcodeStats^[$25]);
  AddOpcodeStat('LL: ', Nemu64_API.OpcodeStats^[$30]);
  AddOpcodeStat('LLD: ', Nemu64_API.OpcodeStats^[$34]);
  AddOpcodeStat('LUI: ', Nemu64_API.OpcodeStats^[$F]);
  AddOpcodeStat('LW: ', Nemu64_API.OpcodeStats^[$23]);
  AddOpcodeStat('LWC1: ', Nemu64_API.OpcodeStats^[$31]);
  AddOpcodeStat('LWL: ', Nemu64_API.OpcodeStats^[$22]);
  AddOpcodeStat('LWR: ', Nemu64_API.OpcodeStats^[$26]);
  AddOpcodeStat('LWU: ', Nemu64_API.OpcodeStats^[$27]);
  AddOpcodeStat('ORI: ', Nemu64_API.OpcodeStats^[$D]);
  AddOpcodeStat('SB: ', Nemu64_API.OpcodeStats^[$28]);
  AddOpcodeStat('SC: ', Nemu64_API.OpcodeStats^[$38]);
  AddOpcodeStat('SCD: ', Nemu64_API.OpcodeStats^[$3C]);
  AddOpcodeStat('SD: ', Nemu64_API.OpcodeStats^[$3F]);
  AddOpcodeStat('SDC1: ', Nemu64_API.OpcodeStats^[$3D]);
  AddOpcodeStat('SDL: ', Nemu64_API.OpcodeStats^[$2C]);
  AddOpcodeStat('SDR: ', Nemu64_API.OpcodeStats^[$2D]);
  AddOpcodeStat('SH: ', Nemu64_API.OpcodeStats^[$29]);
  AddOpcodeStat('SLTI: ', Nemu64_API.OpcodeStats^[$A]);
  AddOpcodeStat('SLTIU: ', Nemu64_API.OpcodeStats^[$B]);
  AddOpcodeStat('SW: ', Nemu64_API.OpcodeStats^[$2B]);
  AddOpcodeStat('SWC1: ', Nemu64_API.OpcodeStats^[$39]);
  AddOpcodeStat('SWL: ', Nemu64_API.OpcodeStats^[$2A]);
  AddOpcodeStat('SWR: ', Nemu64_API.OpcodeStats^[$2E]);
  AddOpcodeStat('XORI: ', Nemu64_API.OpcodeStats^[$E]);
  AddOpcodeStat('SLL: ', Nemu64_API.OpcodeStats^[$40]);
  AddOpcodeStat('SRL: ', Nemu64_API.OpcodeStats^[$42]);
  AddOpcodeStat('SRA: ', Nemu64_API.OpcodeStats^[$43]);
  AddOpcodeStat('SLLV: ', Nemu64_API.OpcodeStats^[$44]);
  AddOpcodeStat('SRLV: ', Nemu64_API.OpcodeStats^[$46]);
  AddOpcodeStat('SRAV: ', Nemu64_API.OpcodeStats^[$47]);
  AddOpcodeStat('JR: ', Nemu64_API.OpcodeStats^[$48]);
  AddOpcodeStat('JALR: ', Nemu64_API.OpcodeStats^[$49]);
  AddOpcodeStat('SYSCALL: ', Nemu64_API.OpcodeStats^[$4C]);
  AddOpcodeStat('BREAK: ', Nemu64_API.OpcodeStats^[$4D]);
  AddOpcodeStat('SYNC: ', Nemu64_API.OpcodeStats^[$4F]);
  AddOpcodeStat('MFHI: ', Nemu64_API.OpcodeStats^[$50]);
  AddOpcodeStat('MTHI: ', Nemu64_API.OpcodeStats^[$51]);
  AddOpcodeStat('MFLO: ', Nemu64_API.OpcodeStats^[$52]);
  AddOpcodeStat('MTLO: ', Nemu64_API.OpcodeStats^[$53]);
  AddOpcodeStat('DSLLV: ', Nemu64_API.OpcodeStats^[$54]);
  AddOpcodeStat('DSRLV: ', Nemu64_API.OpcodeStats^[$56]);
  AddOpcodeStat('DSRAV: ', Nemu64_API.OpcodeStats^[$57]);
  AddOpcodeStat('MULT: ', Nemu64_API.OpcodeStats^[$58]);
  AddOpcodeStat('MULTU: ', Nemu64_API.OpcodeStats^[$59]);
  AddOpcodeStat('DIV: ', Nemu64_API.OpcodeStats^[$5A]);
  AddOpcodeStat('DIVU: ', Nemu64_API.OpcodeStats^[$5B]);
  AddOpcodeStat('DMULT: ', Nemu64_API.OpcodeStats^[$5C]);
  AddOpcodeStat('DMULTU: ', Nemu64_API.OpcodeStats^[$5D]);
  AddOpcodeStat('DDIV: ', Nemu64_API.OpcodeStats^[$5E]);
  AddOpcodeStat('DDIVU: ', Nemu64_API.OpcodeStats^[$5F]);
  AddOpcodeStat('ADD: ', Nemu64_API.OpcodeStats^[$60]);
  AddOpcodeStat('ADDU: ', Nemu64_API.OpcodeStats^[$61]);
  AddOpcodeStat('SUB: ', Nemu64_API.OpcodeStats^[$62]);
  AddOpcodeStat('SUBU: ', Nemu64_API.OpcodeStats^[$63]);
  AddOpcodeStat('AND: ', Nemu64_API.OpcodeStats^[$64]);
  AddOpcodeStat('OR: ', Nemu64_API.OpcodeStats^[$65]);
  AddOpcodeStat('XOR: ', Nemu64_API.OpcodeStats^[$66]);
  AddOpcodeStat('NOR: ', Nemu64_API.OpcodeStats^[$67]);
  AddOpcodeStat('SLT: ', Nemu64_API.OpcodeStats^[$6A]);
  AddOpcodeStat('SLTU: ', Nemu64_API.OpcodeStats^[$6B]);
  AddOpcodeStat('DADD: ', Nemu64_API.OpcodeStats^[$6C]);
  AddOpcodeStat('DADDU: ', Nemu64_API.OpcodeStats^[$6D]);
  AddOpcodeStat('DSUB: ', Nemu64_API.OpcodeStats^[$6E]);
  AddOpcodeStat('DSUBU: ', Nemu64_API.OpcodeStats^[$6F]);
  AddOpcodeStat('TGE: ', Nemu64_API.OpcodeStats^[$70]);
  AddOpcodeStat('TGEU: ', Nemu64_API.OpcodeStats^[$71]);
  AddOpcodeStat('TLT: ', Nemu64_API.OpcodeStats^[$72]);
  AddOpcodeStat('TLTU: ', Nemu64_API.OpcodeStats^[$73]);
  AddOpcodeStat('TEQ: ', Nemu64_API.OpcodeStats^[$74]);
  AddOpcodeStat('TNE: ', Nemu64_API.OpcodeStats^[$76]);
  AddOpcodeStat('DSLL: ', Nemu64_API.OpcodeStats^[$78]);
  AddOpcodeStat('DSRL: ', Nemu64_API.OpcodeStats^[$7A]);
  AddOpcodeStat('DSRA: ', Nemu64_API.OpcodeStats^[$7B]);
  AddOpcodeStat('DSLL32: ', Nemu64_API.OpcodeStats^[$7C]);
  AddOpcodeStat('DSRL32: ', Nemu64_API.OpcodeStats^[$7E]);
  AddOpcodeStat('DSRA32: ', Nemu64_API.OpcodeStats^[$7F]);
  AddOpcodeStat('BLTZ: ', Nemu64_API.OpcodeStats^[$80]);
  AddOpcodeStat('BGEZ: ', Nemu64_API.OpcodeStats^[$81]);
  AddOpcodeStat('BLTZL: ', Nemu64_API.OpcodeStats^[$82]);
  AddOpcodeStat('BGEZL: ', Nemu64_API.OpcodeStats^[$83]);
  AddOpcodeStat('TGEI: ', Nemu64_API.OpcodeStats^[$88]);
  AddOpcodeStat('TGEIU: ', Nemu64_API.OpcodeStats^[$89]);
  AddOpcodeStat('TLTI: ', Nemu64_API.OpcodeStats^[$8A]);
  AddOpcodeStat('TLTIU: ', Nemu64_API.OpcodeStats^[$8B]);
  AddOpcodeStat('TEQI: ', Nemu64_API.OpcodeStats^[$8C]);
  AddOpcodeStat('TNEI: ', Nemu64_API.OpcodeStats^[$8E]);
  AddOpcodeStat('BLTZAL: ', Nemu64_API.OpcodeStats^[$90]);
  AddOpcodeStat('BGEZAL: ', Nemu64_API.OpcodeStats^[$91]);
  AddOpcodeStat('BLTZALL: ', Nemu64_API.OpcodeStats^[$92]);
  AddOpcodeStat('BGEZALL: ', Nemu64_API.OpcodeStats^[$93]);
  AddOpcodeStat('MFC0: ', Nemu64_API.OpcodeStats^[$A0]);
  AddOpcodeStat('MTC0: ', Nemu64_API.OpcodeStats^[$A4]);
  AddOpcodeStat('MFC1: ', Nemu64_API.OpcodeStats^[$C0]);
  AddOpcodeStat('DMFC1: ', Nemu64_API.OpcodeStats^[$C1]);
  AddOpcodeStat('CFC1: ', Nemu64_API.OpcodeStats^[$C2]);
  AddOpcodeStat('MTC1: ', Nemu64_API.OpcodeStats^[$C4]);
  AddOpcodeStat('DMTC1: ', Nemu64_API.OpcodeStats^[$C5]);
  AddOpcodeStat('CTC1: ', Nemu64_API.OpcodeStats^[$C6]);
  AddOpcodeStat('BC1: ', Nemu64_API.OpcodeStats^[$C8]);
  AddOpcodeStat('TLBR: ', Nemu64_API.OpcodeStats^[$E1]);
  AddOpcodeStat('TLBWI: ', Nemu64_API.OpcodeStats^[$E2]);
  AddOpcodeStat('TLBWR: ', Nemu64_API.OpcodeStats^[$E6]);
  AddOpcodeStat('TLBP: ', Nemu64_API.OpcodeStats^[$E8]);
  AddOpcodeStat('ERET: ', Nemu64_API.OpcodeStats^[$F8]);
  AddOpcodeStat('S.ADD: ', Nemu64_API.OpcodeStats^[$120]);
  AddOpcodeStat('S.SUB: ', Nemu64_API.OpcodeStats^[$121]);
  AddOpcodeStat('S.MUL: ', Nemu64_API.OpcodeStats^[$122]);
  AddOpcodeStat('S.DIV: ', Nemu64_API.OpcodeStats^[$123]);
  AddOpcodeStat('S.SQRT: ', Nemu64_API.OpcodeStats^[$124]);
  AddOpcodeStat('S.ABS: ', Nemu64_API.OpcodeStats^[$125]);
  AddOpcodeStat('S.MOV: ', Nemu64_API.OpcodeStats^[$126]);
  AddOpcodeStat('S.NEG: ', Nemu64_API.OpcodeStats^[$127]);
  AddOpcodeStat('S.ROUND.L: ', Nemu64_API.OpcodeStats^[$128]);
  AddOpcodeStat('S.TRUNC.L: ', Nemu64_API.OpcodeStats^[$129]);
  AddOpcodeStat('S.CEIL.L: ', Nemu64_API.OpcodeStats^[$12A]);
  AddOpcodeStat('S.FLOOR.L: ', Nemu64_API.OpcodeStats^[$12B]);
  AddOpcodeStat('S.ROUND.W: ', Nemu64_API.OpcodeStats^[$12C]);
  AddOpcodeStat('S.TRUNC.W: ', Nemu64_API.OpcodeStats^[$12D]);
  AddOpcodeStat('S.CEIL.W: ', Nemu64_API.OpcodeStats^[$12E]);
  AddOpcodeStat('S.FLOOR.W: ', Nemu64_API.OpcodeStats^[$12F]);
  AddOpcodeStat('S.CVT.D: ', Nemu64_API.OpcodeStats^[$141]);
  AddOpcodeStat('S.CVT.W: ', Nemu64_API.OpcodeStats^[$144]);
  AddOpcodeStat('S.CVT.L: ', Nemu64_API.OpcodeStats^[$145]);
  AddOpcodeStat('S.C.F: ', Nemu64_API.OpcodeStats^[$150]);
  AddOpcodeStat('S.C.UN: ', Nemu64_API.OpcodeStats^[$151]);
  AddOpcodeStat('S.C.EQ: ', Nemu64_API.OpcodeStats^[$152]);
  AddOpcodeStat('S.C.UEQ: ', Nemu64_API.OpcodeStats^[$153]);
  AddOpcodeStat('S.C.OLT: ', Nemu64_API.OpcodeStats^[$154]);
  AddOpcodeStat('S.C.ULT: ', Nemu64_API.OpcodeStats^[$155]);
  AddOpcodeStat('S.C.OLE: ', Nemu64_API.OpcodeStats^[$156]);
  AddOpcodeStat('S.C.ULE: ', Nemu64_API.OpcodeStats^[$157]);
  AddOpcodeStat('S.C.SF: ', Nemu64_API.OpcodeStats^[$158]);
  AddOpcodeStat('S.C.NGLE: ', Nemu64_API.OpcodeStats^[$159]);
  AddOpcodeStat('S.C.SEQ: ', Nemu64_API.OpcodeStats^[$15A]);
  AddOpcodeStat('S.C.NGL: ', Nemu64_API.OpcodeStats^[$15B]);
  AddOpcodeStat('S.C.LT: ', Nemu64_API.OpcodeStats^[$15C]);
  AddOpcodeStat('S.C.NGE: ', Nemu64_API.OpcodeStats^[$15D]);
  AddOpcodeStat('S.C.LE: ', Nemu64_API.OpcodeStats^[$15E]);
  AddOpcodeStat('S.C.NGT: ', Nemu64_API.OpcodeStats^[$15F]);
  AddOpcodeStat('D.ADD: ', Nemu64_API.OpcodeStats^[$160]);
  AddOpcodeStat('D.SUB: ', Nemu64_API.OpcodeStats^[$161]);
  AddOpcodeStat('D.MUL: ', Nemu64_API.OpcodeStats^[$162]);
  AddOpcodeStat('D.DIV: ', Nemu64_API.OpcodeStats^[$163]);
  AddOpcodeStat('D.SQRT: ', Nemu64_API.OpcodeStats^[$164]);
  AddOpcodeStat('D.ABS: ', Nemu64_API.OpcodeStats^[$165]);
  AddOpcodeStat('D.MOV: ', Nemu64_API.OpcodeStats^[$166]);
  AddOpcodeStat('D.NEG: ', Nemu64_API.OpcodeStats^[$167]);
  AddOpcodeStat('D.ROUND.L: ', Nemu64_API.OpcodeStats^[$168]);
  AddOpcodeStat('D.TRUNC.L: ', Nemu64_API.OpcodeStats^[$169]);
  AddOpcodeStat('D.CEIL.L: ', Nemu64_API.OpcodeStats^[$16A]);
  AddOpcodeStat('D.FLOOR.L: ', Nemu64_API.OpcodeStats^[$16B]);
  AddOpcodeStat('D.ROUND.W: ', Nemu64_API.OpcodeStats^[$16C]);
  AddOpcodeStat('D.TRUNC.W: ', Nemu64_API.OpcodeStats^[$16D]);
  AddOpcodeStat('D.CEIL.W: ', Nemu64_API.OpcodeStats^[$16E]);
  AddOpcodeStat('D.FLOOR.W: ', Nemu64_API.OpcodeStats^[$16F]);
  AddOpcodeStat('D.CVT.S: ', Nemu64_API.OpcodeStats^[$180]);
  AddOpcodeStat('D.CVT.W: ', Nemu64_API.OpcodeStats^[$184]);
  AddOpcodeStat('D.CVT.L: ', Nemu64_API.OpcodeStats^[$185]);
  AddOpcodeStat('D.C.F: ', Nemu64_API.OpcodeStats^[$190]);
  AddOpcodeStat('D.C.UN: ', Nemu64_API.OpcodeStats^[$191]);
  AddOpcodeStat('D.C.EQ: ', Nemu64_API.OpcodeStats^[$192]);
  AddOpcodeStat('D.C.UEQ: ', Nemu64_API.OpcodeStats^[$193]);
  AddOpcodeStat('D.C.OLT: ', Nemu64_API.OpcodeStats^[$194]);
  AddOpcodeStat('D.C.ULT: ', Nemu64_API.OpcodeStats^[$195]);
  AddOpcodeStat('D.C.OLE: ', Nemu64_API.OpcodeStats^[$196]);
  AddOpcodeStat('D.C.ULE: ', Nemu64_API.OpcodeStats^[$197]);
  AddOpcodeStat('D.C.SF: ', Nemu64_API.OpcodeStats^[$198]);
  AddOpcodeStat('D.C.NGLE: ', Nemu64_API.OpcodeStats^[$199]);
  AddOpcodeStat('D.C.SEQ: ', Nemu64_API.OpcodeStats^[$19A]);
  AddOpcodeStat('D.C.NGL: ', Nemu64_API.OpcodeStats^[$19B]);
  AddOpcodeStat('D.C.LT: ', Nemu64_API.OpcodeStats^[$19C]);
  AddOpcodeStat('D.C.NGE: ', Nemu64_API.OpcodeStats^[$19D]);
  AddOpcodeStat('D.C.LE: ', Nemu64_API.OpcodeStats^[$19E]);
  AddOpcodeStat('D.C.NGT: ', Nemu64_API.OpcodeStats^[$19F]);
  AddOpcodeStat('W.CVT.S: ', Nemu64_API.OpcodeStats^[$1C0]);
  AddOpcodeStat('W.CVT.L: ', Nemu64_API.OpcodeStats^[$1C5]);
  AddOpcodeStat('D.CVT.S: ', Nemu64_API.OpcodeStats^[$200]);
  AddOpcodeStat('D.CVT.L: ', Nemu64_API.OpcodeStats^[$205]);
end;

procedure TFormStats.ButtonBProfReadClick(Sender: TObject);
var
  I: Integer;
  Item: TListItem;
  Total: Int64;
begin
  Total := 0;
  ListView.Items.Clear;
  for I := 0 to Integer(Nemu64_API.RamSize^ shr 2) - 1 do
    if Nemu64_API.BlockExecutionCounts^[I] <> 0 then
    begin
      Item := ListView.Items.Add;
      Item.Caption := '8' + IntToHex(I shl 2, 7);
      Item.SubItems.Add(IntToStr(Nemu64_API.BlockExecutionCounts^[I]));
      Item.SubItems.Add(IntToStr(Nemu64_API.BlockCycleCounts^[I]));
      Inc(Total, Nemu64_API.BlockCycleCounts^[I]);
    end;
  for I := 0 to ListView.Items.Count - 1 do
    ListView.Items[I].SubItems.Add(IntToStr(Round(StrToIntDef(ListView.Items[I].SubItems[1], 0) / Total * 100)));
  ListView.AlphaSort;
end;

procedure TFormStats.ListViewCompare(Sender: TObject; Item1, Item2: TListItem; Data: Integer; var Compare: Integer);

  function CompareValues(Value1, Value2: Integer): Integer;
  begin
    if Value1 < Value2 then
      Result := -1
    else if Value1 > Value2 then
      Result := 1
    else
      Result := 0;
    if SortColumn < 0 then
      Result := -Result;
  end;

begin
  case SortColumn of
    -1, 1: Compare := CompareText(Item1.Caption, Item2.Caption) * SortColumn;
    -2, 2: Compare := CompareValues(StrToIntDef(Item1.SubItems[0], 0), StrToIntDef(Item2.SubItems[0], 0));
    -3, 3: Compare := CompareValues(StrToIntDef(Item1.SubItems[1], 0), StrToIntDef(Item2.SubItems[1], 0));
  end;
end;

procedure TFormStats.ListViewColumnClick(Sender: TObject; Column: TListColumn);
var
  NewColumn: Integer;
begin
  NewColumn := Column.ID + 1;
  if NewColumn = 4 then NewColumn := 3;
  if NewColumn <> Abs(SortColumn) then
    SortColumn := NewColumn
  else
    SortColumn := -SortColumn;
  ListView.AlphaSort;
end;

procedure TFormStats.ListViewDblClick(Sender: TObject);
begin
  if ListView.Selected <> nil then
  begin
    FormDebug.EditCPUPC.Text := ListView.Selected.Caption;
    FormDebug.Show;
  end;
end;

procedure TFormStats.CheckBox1Click(Sender: TObject);
begin
  Nemu64_API.BlockStatsEnabled^ := (Sender as TCheckBox).Checked;
  Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $400;
end;

procedure TFormStats.Button1Click(Sender: TObject);
begin
  FillChar(Nemu64_API.BlockExecutionCounts^, $800000, 0);
  FillChar(Nemu64_API.BlockCycleCounts^, $800000, 0);
end;

end.
