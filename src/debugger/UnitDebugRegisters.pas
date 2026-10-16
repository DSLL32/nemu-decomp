unit UnitDebugRegisters;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls, Menus, Buttons;

type
  TFormRegisters = class(TForm)
    PageControl: TPageControl;
    TabSheetGPR: TTabSheet;
    TabSheetPIRegs: TTabSheet;
    PanelGPRRegs: TPanel;
    LabelGRP0: TLabel;
    LabelGRP1: TLabel;
    LabelGRP2: TLabel;
    LabelGRP3: TLabel;
    LabelGRP4: TLabel;
    LabelGRP5: TLabel;
    LabelGRP6: TLabel;
    LabelGRP7: TLabel;
    LabelGRP8: TLabel;
    LabelGRP9: TLabel;
    LabelGRP10: TLabel;
    LabelGRP11: TLabel;
    LabelGRP12: TLabel;
    LabelGRP13: TLabel;
    LabelGRP14: TLabel;
    LabelGRP15: TLabel;
    LabelGRP16: TLabel;
    LabelGRP17: TLabel;
    LabelGRP18: TLabel;
    LabelGRP19: TLabel;
    LabelGRP20: TLabel;
    LabelGRP21: TLabel;
    LabelGRP22: TLabel;
    LabelGRP23: TLabel;
    LabelGRP24: TLabel;
    LabelGRP25: TLabel;
    LabelGRP26: TLabel;
    LabelGRP27: TLabel;
    LabelGRP28: TLabel;
    LabelGRP29: TLabel;
    LabelGRP30: TLabel;
    LabelGRP31: TLabel;
    EditGPR0: TEdit;
    EditGPR1: TEdit;
    EditGPR2: TEdit;
    EditGPR3: TEdit;
    EditGPR4: TEdit;
    EditGPR5: TEdit;
    EditGPR6: TEdit;
    EditGPR7: TEdit;
    EditGPR8: TEdit;
    EditGPR9: TEdit;
    EditGPR10: TEdit;
    EditGPR11: TEdit;
    EditGPR12: TEdit;
    EditGPR13: TEdit;
    EditGPR14: TEdit;
    EditGPR15: TEdit;
    EditGPR18: TEdit;
    EditGPR19: TEdit;
    EditGPR20: TEdit;
    EditGPR21: TEdit;
    EditGPR22: TEdit;
    EditGPR23: TEdit;
    EditGPR24: TEdit;
    EditGPR25: TEdit;
    EditGPR27: TEdit;
    EditGPR26: TEdit;
    EditGPR28: TEdit;
    EditGPR29: TEdit;
    EditGPR30: TEdit;
    EditGPR31: TEdit;
    EditGPR17: TEdit;
    EditGPR16: TEdit;
    TabSheetCOP1: TTabSheet;
    PanelFPR: TPanel;
    LabelFPR0: TLabel;
    LabelFPR1: TLabel;
    LabelFPR2: TLabel;
    LabelFPR3: TLabel;
    LabelFPR4: TLabel;
    LabelFPR5: TLabel;
    LabelFPR6: TLabel;
    LabelFPR7: TLabel;
    LabelFPR8: TLabel;
    LabelFPR9: TLabel;
    LabelFPR10: TLabel;
    LabelFPR11: TLabel;
    LabelFPR12: TLabel;
    LabelFPR13: TLabel;
    LabelFPR14: TLabel;
    LabelFPR15: TLabel;
    LabelFPR16: TLabel;
    LabelFPR17: TLabel;
    LabelFPR18: TLabel;
    LabelFPR19: TLabel;
    LabelFPR20: TLabel;
    LabelFPR21: TLabel;
    LabelFPR22: TLabel;
    LabelFPR23: TLabel;
    LabelFPR24: TLabel;
    LabelFPR25: TLabel;
    LabelFPR26: TLabel;
    LabelFPR27: TLabel;
    LabelFPR28: TLabel;
    LabelFPR29: TLabel;
    LabelFPR30: TLabel;
    LabelFPR31: TLabel;
    EditFPR3: TEdit;
    EditFPR4: TEdit;
    EditFPR5: TEdit;
    EditFPR6: TEdit;
    EditFPR7: TEdit;
    EditFPR8: TEdit;
    EditFPR9: TEdit;
    EditFPR10: TEdit;
    EditFPR11: TEdit;
    EditFPR12: TEdit;
    EditFPR13: TEdit;
    EditFPR14: TEdit;
    EditFPR15: TEdit;
    EditFPR19: TEdit;
    EditFPR20: TEdit;
    EditFPR21: TEdit;
    EditFPR22: TEdit;
    EditFPR23: TEdit;
    EditFPR24: TEdit;
    EditFPR25: TEdit;
    EditFPR26: TEdit;
    EditFPR27: TEdit;
    EditFPR28: TEdit;
    EditFPR29: TEdit;
    EditFPR30: TEdit;
    EditFPR31: TEdit;
    EditFPR0: TEdit;
    EditFPR1: TEdit;
    EditFPR2: TEdit;
    EditFPR16: TEdit;
    EditFPR17: TEdit;
    EditFPR18: TEdit;
    PanelCPURegs: TPanel;
    LabelPC: TLabel;
    EditPC: TEdit;
    LabelMultLO: TLabel;
    EditMultLO: TEdit;
    LabelMultHI: TLabel;
    EditMultHI: TEdit;
    TabSheetVIRegs: TTabSheet;
    PanelPIRegs: TPanel;
    LabelPI_BSD_DOM2_RLS_REG: TLabel;
    LabelPI_BSD_DOM2_PGS_REG: TLabel;
    LabelPI_BSD_DOM2_PWD_REG: TLabel;
    LabelPI_BSD_DOM2_LAT_REG: TLabel;
    LabelPI_BSD_DOM1_RLS_REG: TLabel;
    LabelPI_BSD_DOM1_PGS_REG: TLabel;
    LabelPI_BSD_DOM1_PWD_REG: TLabel;
    LabelPI_BSD_DOM1_LAT_REG: TLabel;
    LabelPI_STATUS_REG: TLabel;
    LabelPI_WR_LEN_REG: TLabel;
    LabelPI_RD_LEN_REG: TLabel;
    LabelPI_CART_ADDR_REG: TLabel;
    LabelPI_DRAM_ADDR_REG: TLabel;
    EditPI_DRAM_ADDR_REG: TEdit;
    EditPI_CART_ADDR_REG: TEdit;
    EditPI_RD_LEN_REG: TEdit;
    EditPI_WR_LEN_REG: TEdit;
    EditPI_STATUS_REG: TEdit;
    EditPI_BSD_DOM1_LAT_REG: TEdit;
    EditPI_BSD_DOM1_PWD_REG: TEdit;
    EditPI_BSD_DOM1_PGS_REG: TEdit;
    EditPI_BSD_DOM1_RLS_REG: TEdit;
    EditPI_BSD_DOM2_LAT_REG: TEdit;
    EditPI_BSD_DOM2_PWD_REG: TEdit;
    EditPI_BSD_DOM2_PGS_REG: TEdit;
    EditPI_BSD_DOM2_RLS_REG: TEdit;
    PanelVIRegs: TPanel;
    LabelVI_Y_SCALE_REG: TLabel;
    LabelVI_X_SCALE_REG: TLabel;
    LabelVI_V_BURST_REG: TLabel;
    LabelVI_V_VIDEO_REG: TLabel;
    LabelVI_H_VIDEO_REG: TLabel;
    LabelVI_H_SYNC_LEAP_REG: TLabel;
    LabelVI_H_SYNC_REG: TLabel;
    LabelVI_V_SYNC_REG: TLabel;
    LabelVI_TIMING_REG: TLabel;
    LabelVI_INTR_REG: TLabel;
    LabelVI_H_WIDTH_REG: TLabel;
    LabelVI_DRAM_ADDR_REG: TLabel;
    LabelVI_STATUS_REG: TLabel;
    EditVI_STATUS_REG: TEdit;
    EditVI_DRAM_ADDR_REG: TEdit;
    EditVI_H_WIDTH_REG: TEdit;
    EditVI_V_INTR_REG: TEdit;
    EditVI_TIMING_REG: TEdit;
    EditVI_V_SYNC_REG: TEdit;
    EditVI_H_SYNC_REG: TEdit;
    EditVI_H_SYNC_LEAP_REG: TEdit;
    EditVI_H_VIDEO_REG: TEdit;
    EditVI_V_VIDEO_REG: TEdit;
    EditVI_V_BURST_REG: TEdit;
    EditVI_X_SCALE_REG: TEdit;
    EditVI_Y_SCALE_REG: TEdit;
    EditVI_V_CURRENT_LINE_REG: TEdit;
    LabelVI_V_CURRENT_LINE_REG: TLabel;
    TabSheetCOP0: TTabSheet;
    PanelCOP0: TPanel;
    LabelIndex: TLabel;
    LabelEntryLo0: TLabel;
    LabelEntryLo1: TLabel;
    LabelContext: TLabel;
    LabelPageMask: TLabel;
    LabelWired: TLabel;
    LabelBadVAddr: TLabel;
    LabelCount: TLabel;
    LabelEntryHi: TLabel;
    LabelCompare: TLabel;
    LabelStatus: TLabel;
    LabelCause: TLabel;
    LabelExceptPC: TLabel;
    LabelPRevID: TLabel;
    LabelConfig: TLabel;
    LabelLLAddr: TLabel;
    LabelWatchLo: TLabel;
    LabelWatchHi: TLabel;
    LabelXContext: TLabel;
    LabelPErr: TLabel;
    LabelCacheErr: TLabel;
    LabelTagLo: TLabel;
    LabelTagHi: TLabel;
    LabelErrorEPC: TLabel;
    EditIndex: TEdit;
    EditEntryLo0: TEdit;
    EditEntryLo1: TEdit;
    EditContext: TEdit;
    EditPageMask: TEdit;
    EditWired: TEdit;
    EditBadVAddr: TEdit;
    EditCount: TEdit;
    EditEntryHi: TEdit;
    EditCompare: TEdit;
    EditStatus: TEdit;
    EditCause: TEdit;
    EditExceptPC: TEdit;
    EditPRevID: TEdit;
    EditConfig: TEdit;
    EditLLAddr: TEdit;
    EditWatchLo: TEdit;
    EditWatchHi: TEdit;
    EditXContext: TEdit;
    EditPErr: TEdit;
    EditCacheErr: TEdit;
    EditTagLo: TEdit;
    EditTagHi: TEdit;
    EditErrorEPC: TEdit;
    TabSheetRSPGP: TTabSheet;
    TabSheetMIRegisters: TTabSheet;
    PanelMIRegs: TPanel;
    LabelMIIntrReg: TLabel;
    LabelMIModeReg: TLabel;
    EditMI_MODE_REG: TEdit;
    EditMI_INTR_REG: TEdit;
    LabelMIIntrMaskReg: TLabel;
    EditMI_INTR_MASK_REG: TEdit;
    EditMI_VERSION_REG: TEdit;
    LabelMIVersionReg: TLabel;
    GroupBoxMI_INTR_REGInterpretation: TGroupBox;
    CheckBoxMI_INTR_SP: TCheckBox;
    CheckBoxMI_INTR_SI: TCheckBox;
    CheckBoxMI_INTR_AI: TCheckBox;
    CheckBoxMI_INTR_VI: TCheckBox;
    CheckBoxMI_INTR_PI: TCheckBox;
    CheckBoxMI_INTR_DP: TCheckBox;
    GroupBoxMI_INTR_MASK_REG: TGroupBox;
    CheckBoxMI_INTR_MASK_SP: TCheckBox;
    CheckBoxMI_INTR_MASK_SI: TCheckBox;
    CheckBoxMI_INTR_MASK_AI: TCheckBox;
    CheckBoxMI_INTR_MASK_VI: TCheckBox;
    CheckBoxMI_INTR_MASK_PI: TCheckBox;
    CheckBoxMI_INTR_MASK_DP: TCheckBox;
    TabSheetSIRegs: TTabSheet;
    PanelSIRegs: TPanel;
    LabelSI_DRAM_ADDR_REG: TLabel;
    LabelSI_STATUS_REG: TLabel;
    EditSI_DRAM_ADDR_REG: TEdit;
    EditSI_STATUS_REG: TEdit;
    TabSheetRSPVector: TTabSheet;
    TabSheetRSPHidden: TTabSheet;
    GroupBoxRSPAccum: TGroupBox;
    LabelAccum0: TLabel;
    EditAccum0: TEdit;
    LabelAccum1: TLabel;
    EditAccum1: TEdit;
    LabelAccum2: TLabel;
    EditAccum2: TEdit;
    LabelAccum3: TLabel;
    EditAccum3: TEdit;
    LabelAccum4: TLabel;
    EditAccum4: TEdit;
    LabelAccum5: TLabel;
    EditAccum5: TEdit;
    LabelAccum6: TLabel;
    EditAccum6: TEdit;
    LabelAccum7: TLabel;
    EditAccum7: TEdit;
    TabSheetSPRegs: TTabSheet;
    PanelSPRegs: TPanel;
    LabelSP_MEM_ADDR_REG: TLabel;
    LabelSP_DRAM_ADDR_REG: TLabel;
    LabelSP_RD_LEN_REG: TLabel;
    LabelSP_WR_LEN_REG: TLabel;
    EditSP_MEM_ADDR_REG: TEdit;
    EditSP_DRAM_ADDR_REG: TEdit;
    EditSP_RD_LEN_REG: TEdit;
    EditSP_WR_LEN_REG: TEdit;
    LabelSP_STATUS_REG: TLabel;
    LabelSP_DMA_FULL_REG: TLabel;
    LabelSP_DMA_BUSY_REG: TLabel;
    LabelSP_SEMAPHORE_REG: TLabel;
    EditSP_STATUS_REG: TEdit;
    EditSP_DMA_FULL_REG: TEdit;
    EditSP_DMA_BUSY_REG: TEdit;
    EditSP_SEMAPHORE_REG: TEdit;
    RadioGroupFPRShowAS: TRadioGroup;
    GroupBoxFCR31: TGroupBox;
    EditFCR31: TEdit;
    CheckBoxFCR31FS: TCheckBox;
    CheckBoxFCR31C: TCheckBox;
    LabelFCR31RoundingMode: TLabel;
    LabelLastHLETaskAddr: TLabel;
    EditHLESPTaskAddr: TEdit;
    GroupBoxRSPFlags: TGroupBox;
    LabelRSPFlagsVCE: TLabel;
    LabelRSPFlagsVCO: TLabel;
    LabelRSPFlagsVCC: TLabel;
    EditRSPFlagsVCE: TEdit;
    EditRSPFlagsVCO: TEdit;
    EditRSPFlagsVCC: TEdit;
    PanelRSPGP: TPanel;
    LabelRSPGP0: TLabel;
    LabelRSPGP1: TLabel;
    LabelRSPGP2: TLabel;
    LabelRSPGP3: TLabel;
    LabelRSPGP4: TLabel;
    LabelRSPGP5: TLabel;
    LabelRSPGP6: TLabel;
    LabelRSPGP7: TLabel;
    LabelRSPGP8: TLabel;
    LabelRSPGP9: TLabel;
    LabelRSPGP10: TLabel;
    LabelRSPGP11: TLabel;
    LabelRSPGP12: TLabel;
    LabelRSPGP13: TLabel;
    LabelRSPGP14: TLabel;
    LabelRSPGP15: TLabel;
    LabelRSPGP16: TLabel;
    LabelRSPGP17: TLabel;
    LabelRSPGP18: TLabel;
    LabelRSPGP19: TLabel;
    LabelRSPGP20: TLabel;
    LabelRSPGP21: TLabel;
    LabelRSPGP22: TLabel;
    LabelRSPGP23: TLabel;
    LabelRSPGP24: TLabel;
    LabelRSPGP25: TLabel;
    LabelRSPGP26: TLabel;
    LabelRSPGP27: TLabel;
    LabelRSPGP28: TLabel;
    LabelRSPGP29: TLabel;
    LabelRSPGP30: TLabel;
    LabelRSPGP31: TLabel;
    EditRSPGP16: TEdit;
    EditRSPGP17: TEdit;
    EditRSPGP0: TEdit;
    EditRSPGP1: TEdit;
    EditRSPGP2: TEdit;
    EditRSPGP3: TEdit;
    EditRSPGP4: TEdit;
    EditRSPGP5: TEdit;
    EditRSPGP6: TEdit;
    EditRSPGP7: TEdit;
    EditRSPGP8: TEdit;
    EditRSPGP9: TEdit;
    EditRSPGP10: TEdit;
    EditRSPGP11: TEdit;
    EditRSPGP12: TEdit;
    EditRSPGP13: TEdit;
    EditRSPGP14: TEdit;
    EditRSPGP15: TEdit;
    EditRSPGP18: TEdit;
    EditRSPGP19: TEdit;
    EditRSPGP20: TEdit;
    EditRSPGP21: TEdit;
    EditRSPGP22: TEdit;
    EditRSPGP23: TEdit;
    EditRSPGP24: TEdit;
    EditRSPGP25: TEdit;
    EditRSPGP26: TEdit;
    EditRSPGP27: TEdit;
    EditRSPGP28: TEdit;
    EditRSPGP29: TEdit;
    EditRSPGP30: TEdit;
    EditRSPGP31: TEdit;
    ScrollBoxVectorRegs: TScrollBox;
    EditRSPv0: TEdit;
    EditRSPv1: TEdit;
    EditRSPv2: TEdit;
    EditRSPv3: TEdit;
    EditRSPv4: TEdit;
    EditRSPv5: TEdit;
    EditRSPv6: TEdit;
    EditRSPv7: TEdit;
    EditRSPv8: TEdit;
    EditRSPv9: TEdit;
    EditRSPv10: TEdit;
    EditRSPv11: TEdit;
    EditRSPv12: TEdit;
    EditRSPv13: TEdit;
    EditRSPv14: TEdit;
    EditRSPv15: TEdit;
    LabelRSPv15: TLabel;
    LabelRSPv14: TLabel;
    LabelRSPv13: TLabel;
    LabelRSPv12: TLabel;
    LabelRSPv11: TLabel;
    LabelRSPv10: TLabel;
    LabelRSPv9: TLabel;
    LabelRSPv8: TLabel;
    LabelRSPv7: TLabel;
    LabelRSPv6: TLabel;
    LabelRSPv5: TLabel;
    LabelRSPv4: TLabel;
    LabelRSPv3: TLabel;
    LabelRSPv2: TLabel;
    LabelRSPv1: TLabel;
    LabelRSPv0: TLabel;
    EditRSPv16: TEdit;
    LabelRSPv16: TLabel;
    EditRSPv17: TEdit;
    EditRSPv18: TEdit;
    EditRSPv19: TEdit;
    EditRSPv20: TEdit;
    EditRSPv21: TEdit;
    EditRSPv22: TEdit;
    EditRSPv23: TEdit;
    EditRSPv24: TEdit;
    EditRSPv25: TEdit;
    EditRSPv26: TEdit;
    EditRSPv27: TEdit;
    EditRSPv28: TEdit;
    EditRSPv29: TEdit;
    EditRSPv30: TEdit;
    EditRSPv31: TEdit;
    LabelRSPv31: TLabel;
    LabelRSPv30: TLabel;
    LabelRSPv29: TLabel;
    LabelRSPv28: TLabel;
    LabelRSPv27: TLabel;
    LabelRSPv26: TLabel;
    LabelRSPv25: TLabel;
    LabelRSPv24: TLabel;
    LabelRSPv23: TLabel;
    LabelRSPv22: TLabel;
    LabelRSPv21: TLabel;
    LabelRSPv20: TLabel;
    LabelRSPv19: TLabel;
    LabelRSPv18: TLabel;
    LabelRSPv17: TLabel;
    TabSheetDP: TTabSheet;
    PanelDPRegs: TPanel;
    LabelDPC_START_REG: TLabel;
    LabelDPC_END_REG: TLabel;
    EditDPC_START_REG: TEdit;
    EditDPC_END_REG: TEdit;
    EditDPC_CURRENT_REG: TEdit;
    EditDPC_STATUS_REG: TEdit;
    EditDPC_CLOCK_REG: TEdit;
    EditDPC_BUFBUSY_REG: TEdit;
    EditDPC_PIPEBUSY_REG: TEdit;
    EditDPC_TMEM_REG: TEdit;
    LabelDPC_CURRENT_REG: TLabel;
    LabelDPC_STATUS_REG: TLabel;
    LabelDPC_CLOCK_REG: TLabel;
    LabelDPC_BUFBUSY_REG: TLabel;
    LabelDPC_PIPEBUSY_REG: TLabel;
    LabelDPC_TMEM_REG: TLabel;
    procedure FormShow(Sender: TObject);
    procedure EditGPRChange(Sender: TObject);
    procedure RadioGroupFPRShowASClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure EditRSPGPChange(Sender: TObject);
    procedure TabSheetRSPGPResize(Sender: TObject);
    procedure FormKeyUp(Sender: TObject; var Key: Word; Shift: TShiftState);
  public
    function GetRegisterLabel(Index: Integer): TLabel;
    procedure RefreshVectors;
    procedure ShowFPRPair(EvenReg, OddReg: Cardinal; EvenEdit, OddEdit: TEdit);
    procedure RefreshRegisters;
  end;

var
  FormRegisters: TFormRegisters;

implementation

uses UnitDecs, r4300;

{$R *.dfm}

type
  TFPURegister64 = record
    case Byte of
      0: (AsInt64: Int64);
      1: (AsSingle: Single);
      2: (AsDouble: Double);
      3: (AsInteger: Integer);
  end;
  TFPURegisters64 = array[0..31] of TFPURegister64;
  PFPURegisters64 = ^TFPURegisters64;
  TSingleRegisters = array[0..31] of Single;
  PSingleRegisters = ^TSingleRegisters;
  TDoubleRegisters = array[0..15] of Double;
  PDoubleRegisters = ^TDoubleRegisters;

function TFormRegisters.GetRegisterLabel(Index: Integer): TLabel;
begin
  case Index of
    0: Result := LabelGRP0;
    1: Result := LabelGRP1;
    2: Result := LabelGRP2;
    3: Result := LabelGRP3;
    4: Result := LabelGRP4;
    5: Result := LabelGRP5;
    6: Result := LabelGRP6;
    7: Result := LabelGRP7;
    8: Result := LabelGRP8;
    9: Result := LabelGRP9;
    10: Result := LabelGRP10;
    11: Result := LabelGRP11;
    12: Result := LabelGRP12;
    13: Result := LabelGRP13;
    14: Result := LabelGRP14;
    15: Result := LabelGRP15;
    16: Result := LabelGRP16;
    17: Result := LabelGRP17;
    18: Result := LabelGRP18;
    19: Result := LabelGRP19;
    20: Result := LabelGRP20;
    21: Result := LabelGRP21;
    22: Result := LabelGRP22;
    23: Result := LabelGRP23;
    24: Result := LabelGRP24;
    25: Result := LabelGRP25;
    26: Result := LabelGRP26;
    27: Result := LabelGRP27;
    28: Result := LabelGRP28;
    29: Result := LabelGRP29;
    30: Result := LabelGRP30;
    31: Result := LabelGRP31;
    32: Result := LabelRSPGP0;
    33: Result := LabelRSPGP1;
    34: Result := LabelRSPGP2;
    35: Result := LabelRSPGP3;
    36: Result := LabelRSPGP4;
    37: Result := LabelRSPGP5;
    38: Result := LabelRSPGP6;
    39: Result := LabelRSPGP7;
    40: Result := LabelRSPGP8;
    41: Result := LabelRSPGP9;
    42: Result := LabelRSPGP10;
    43: Result := LabelRSPGP11;
    44: Result := LabelRSPGP12;
    45: Result := LabelRSPGP13;
    46: Result := LabelRSPGP14;
    47: Result := LabelRSPGP15;
    48: Result := LabelRSPGP16;
    49: Result := LabelRSPGP17;
    50: Result := LabelRSPGP18;
    51: Result := LabelRSPGP19;
    52: Result := LabelRSPGP20;
    53: Result := LabelRSPGP21;
    54: Result := LabelRSPGP22;
    55: Result := LabelRSPGP23;
    56: Result := LabelRSPGP24;
    57: Result := LabelRSPGP25;
    58: Result := LabelRSPGP26;
    59: Result := LabelRSPGP27;
    60: Result := LabelRSPGP28;
    61: Result := LabelRSPGP29;
    62: Result := LabelRSPGP30;
    63: Result := LabelRSPGP31;
    64: Result := LabelRSPv0;
    65: Result := LabelRSPv1;
    66: Result := LabelRSPv2;
    67: Result := LabelRSPv3;
    68: Result := LabelRSPv4;
    69: Result := LabelRSPv5;
    70: Result := LabelRSPv6;
    71: Result := LabelRSPv7;
    72: Result := LabelRSPv8;
    73: Result := LabelRSPv9;
    74: Result := LabelRSPv10;
    75: Result := LabelRSPv11;
    76: Result := LabelRSPv12;
    77: Result := LabelRSPv13;
    78: Result := LabelRSPv14;
    79: Result := LabelRSPv15;
    80: Result := LabelRSPv16;
    81: Result := LabelRSPv17;
    82: Result := LabelRSPv18;
    83: Result := LabelRSPv19;
    84: Result := LabelRSPv20;
    85: Result := LabelRSPv21;
    86: Result := LabelRSPv22;
    87: Result := LabelRSPv23;
    88: Result := LabelRSPv24;
    89: Result := LabelRSPv25;
    90: Result := LabelRSPv26;
    91: Result := LabelRSPv27;
    92: Result := LabelRSPv28;
    93: Result := LabelRSPv29;
    94: Result := LabelRSPv30;
    95: Result := LabelRSPv31;
  else
    Result := LabelGRP0;
  end;
end;

{ Names for the private helpers are inferred from their uses. }
function FormatRegister(Value: Int64): string;
begin
  Result := IntToHex((Value shr 32) and $FFFFFFFF, 8) + ' - ' +
    IntToHex(Cardinal(Value), 8);
end;

function FormatAccumulator(Value: Int64): string;
begin
  Result := IntToHex(Cardinal(Value shr 32) and $FFFF, 4) + ' - ' +
    IntToHex(Cardinal(Value shr 16) and $FFFF, 4) + ' - ' +
    IntToHex(Cardinal(Value) and $FFFF, 4);
end;

procedure ShowHardwareRegister(Address: Cardinal; Edit: TEdit);
begin
  Edit.Text := IntToHex(ReadWord(Address), 8);
end;

procedure TFormRegisters.RefreshVectors;
var
  I: Integer;
begin
  I := 0;
  EditRSPv0.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 1;
  EditRSPv1.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 2;
  EditRSPv2.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 3;
  EditRSPv3.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 4;
  EditRSPv4.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 5;
  EditRSPv5.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 6;
  EditRSPv6.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 7;
  EditRSPv7.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 8;
  EditRSPv8.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 9;
  EditRSPv9.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 10;
  EditRSPv10.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 11;
  EditRSPv11.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 12;
  EditRSPv12.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 13;
  EditRSPv13.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 14;
  EditRSPv14.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 15;
  EditRSPv15.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 16;
  EditRSPv16.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 17;
  EditRSPv17.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 18;
  EditRSPv18.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 19;
  EditRSPv19.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 20;
  EditRSPv20.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 21;
  EditRSPv21.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 22;
  EditRSPv22.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 23;
  EditRSPv23.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 24;
  EditRSPv24.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 25;
  EditRSPv25.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 26;
  EditRSPv26.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 27;
  EditRSPv27.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 28;
  EditRSPv28.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 29;
  EditRSPv29.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 30;
  EditRSPv30.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
  I := 31;
  EditRSPv31.Text :=
    IntToHex(Nemu64_API.RSPVectors^[I][0], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][1], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][2], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][3], 4) + ' - ' +
    IntToHex(Nemu64_API.RSPVectors^[I][4], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][5], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][6], 4) + ' ' +
    IntToHex(Nemu64_API.RSPVectors^[I][7], 4);
end;

procedure SetRegisterText(Edit: TEdit; Text: string);
var
  OnChange: TNotifyEvent;
begin
  OnChange := Edit.OnChange;
  Edit.OnChange := nil;
  Edit.Text := Text;
  Edit.OnChange := OnChange;
end;

procedure TFormRegisters.ShowFPRPair(EvenReg, OddReg: Cardinal; EvenEdit, OddEdit: TEdit);
var
  Value: Int64;
begin
  case RadioGroupFPRShowAS.ItemIndex of
    0:
      if Nemu64_API.COP0Registers^[12] and $04000000 <> 0 then
      begin
        EvenEdit.Text := FormatRegister(Nemu64_API.FPURegisters64^[EvenReg]);
        OddEdit.Text := FormatRegister(Nemu64_API.FPURegisters64^[OddReg]);
      end
      else
      begin
        EvenEdit.Text := IntToHex(Nemu64_API.FPURegisters32^[EvenReg], 8);
        OddEdit.Text := IntToHex(Nemu64_API.FPURegisters32^[OddReg], 8);
      end;
    1:
      if Nemu64_API.COP0Registers^[12] and $04000000 <> 0 then
      begin
        EvenEdit.Text := FloatToStrF(PFPURegisters64(Nemu64_API.FPURegisters64)^[EvenReg].AsSingle, ffFixed, 7, 18);
        OddEdit.Text := FloatToStrF(PFPURegisters64(Nemu64_API.FPURegisters64)^[OddReg].AsSingle, ffFixed, 7, 18);
      end
      else
      begin
        EvenEdit.Text := FloatToStrF(PSingleRegisters(Nemu64_API.FPURegisters32)^[EvenReg], ffFixed, 7, 18);
        OddEdit.Text := FloatToStrF(PSingleRegisters(Nemu64_API.FPURegisters32)^[OddReg], ffFixed, 7, 18);
      end;
    2:
      if Nemu64_API.COP0Registers^[12] and $04000000 <> 0 then
      begin
        EvenEdit.Text := FloatToStrF(PFPURegisters64(Nemu64_API.FPURegisters64)^[EvenReg].AsDouble, ffFixed, 7, 18);
        OddEdit.Text := FloatToStrF(PFPURegisters64(Nemu64_API.FPURegisters64)^[OddReg].AsDouble, ffFixed, 7, 18);
      end
      else
        EvenEdit.Text := FloatToStrF(PDoubleRegisters(Nemu64_API.FPURegisters32)^[EvenReg shr 1], ffFixed, 15, 18);
    3:
      if Nemu64_API.COP0Registers^[12] and $04000000 <> 0 then
      begin
        EvenEdit.Text := IntToStr(Integer(Nemu64_API.FPURegisters64^[EvenReg]));
        OddEdit.Text := IntToStr(Integer(Nemu64_API.FPURegisters64^[OddReg]));
      end
      else
      begin
        EvenEdit.Text := IntToStr(Nemu64_API.FPURegisters32^[EvenReg]);
        OddEdit.Text := IntToStr(Nemu64_API.FPURegisters32^[OddReg]);
      end;
    4:
      if Nemu64_API.COP0Registers^[12] and $04000000 <> 0 then
      begin
        EvenEdit.Text := IntToStr(Nemu64_API.FPURegisters64^[EvenReg]);
        OddEdit.Text := IntToStr(Nemu64_API.FPURegisters64^[OddReg]);
      end
      else
      begin
        Value := Nemu64_API.FPURegisters32^[OddReg] or (Nemu64_API.FPURegisters32^[EvenReg] shl 32);
        EvenEdit.Text := IntToStr(Value);
      end;
  end;
end;

procedure TFormRegisters.RefreshRegisters;
begin
  if not Visible then Exit;
  SetRegisterText(EditGPR0, FormatRegister(Nemu64_API.CPURegisters^[0]));
  SetRegisterText(EditGPR1, FormatRegister(Nemu64_API.CPURegisters^[1]));
  SetRegisterText(EditGPR2, FormatRegister(Nemu64_API.CPURegisters^[2]));
  SetRegisterText(EditGPR3, FormatRegister(Nemu64_API.CPURegisters^[3]));
  SetRegisterText(EditGPR4, FormatRegister(Nemu64_API.CPURegisters^[4]));
  SetRegisterText(EditGPR5, FormatRegister(Nemu64_API.CPURegisters^[5]));
  SetRegisterText(EditGPR6, FormatRegister(Nemu64_API.CPURegisters^[6]));
  SetRegisterText(EditGPR7, FormatRegister(Nemu64_API.CPURegisters^[7]));
  SetRegisterText(EditGPR8, FormatRegister(Nemu64_API.CPURegisters^[8]));
  SetRegisterText(EditGPR9, FormatRegister(Nemu64_API.CPURegisters^[9]));
  SetRegisterText(EditGPR10, FormatRegister(Nemu64_API.CPURegisters^[10]));
  SetRegisterText(EditGPR11, FormatRegister(Nemu64_API.CPURegisters^[11]));
  SetRegisterText(EditGPR12, FormatRegister(Nemu64_API.CPURegisters^[12]));
  SetRegisterText(EditGPR13, FormatRegister(Nemu64_API.CPURegisters^[13]));
  SetRegisterText(EditGPR14, FormatRegister(Nemu64_API.CPURegisters^[14]));
  SetRegisterText(EditGPR15, FormatRegister(Nemu64_API.CPURegisters^[15]));
  SetRegisterText(EditGPR16, FormatRegister(Nemu64_API.CPURegisters^[16]));
  SetRegisterText(EditGPR17, FormatRegister(Nemu64_API.CPURegisters^[17]));
  SetRegisterText(EditGPR18, FormatRegister(Nemu64_API.CPURegisters^[18]));
  SetRegisterText(EditGPR19, FormatRegister(Nemu64_API.CPURegisters^[19]));
  SetRegisterText(EditGPR20, FormatRegister(Nemu64_API.CPURegisters^[20]));
  SetRegisterText(EditGPR21, FormatRegister(Nemu64_API.CPURegisters^[21]));
  SetRegisterText(EditGPR22, FormatRegister(Nemu64_API.CPURegisters^[22]));
  SetRegisterText(EditGPR23, FormatRegister(Nemu64_API.CPURegisters^[23]));
  SetRegisterText(EditGPR24, FormatRegister(Nemu64_API.CPURegisters^[24]));
  SetRegisterText(EditGPR25, FormatRegister(Nemu64_API.CPURegisters^[25]));
  SetRegisterText(EditGPR26, FormatRegister(Nemu64_API.CPURegisters^[26]));
  SetRegisterText(EditGPR27, FormatRegister(Nemu64_API.CPURegisters^[27]));
  SetRegisterText(EditGPR28, FormatRegister(Nemu64_API.CPURegisters^[28]));
  SetRegisterText(EditGPR29, FormatRegister(Nemu64_API.CPURegisters^[29]));
  SetRegisterText(EditGPR30, FormatRegister(Nemu64_API.CPURegisters^[30]));
  SetRegisterText(EditGPR31, FormatRegister(Nemu64_API.CPURegisters^[31]));
  EditPC.Text := IntToHex(Nemu64_API.CPUProgramCounter^, 8);
  EditMultLO.Text := FormatRegister(Nemu64_API.MultLO^);
  EditMultHI.Text := FormatRegister(Nemu64_API.MultHI^);
  ShowFPRPair(0, 1, EditFPR0, EditFPR1);
  ShowFPRPair(2, 3, EditFPR2, EditFPR3);
  ShowFPRPair(4, 5, EditFPR4, EditFPR5);
  ShowFPRPair(6, 7, EditFPR6, EditFPR7);
  ShowFPRPair(8, 9, EditFPR8, EditFPR9);
  ShowFPRPair(10, 11, EditFPR10, EditFPR11);
  ShowFPRPair(12, 13, EditFPR12, EditFPR13);
  ShowFPRPair(14, 15, EditFPR14, EditFPR15);
  ShowFPRPair(16, 17, EditFPR16, EditFPR17);
  ShowFPRPair(18, 19, EditFPR18, EditFPR19);
  ShowFPRPair(20, 21, EditFPR20, EditFPR21);
  ShowFPRPair(22, 23, EditFPR22, EditFPR23);
  ShowFPRPair(24, 25, EditFPR24, EditFPR25);
  ShowFPRPair(26, 27, EditFPR26, EditFPR27);
  ShowFPRPair(28, 29, EditFPR28, EditFPR29);
  ShowFPRPair(30, 31, EditFPR30, EditFPR31);
  EditFCR31.Text := IntToHex(Nemu64_API.FCR31^, 8);
  CheckBoxFCR31FS.Checked := Nemu64_API.FCR31^ and $01000000 <> 0;
  CheckBoxFCR31C.Checked := Nemu64_API.FCR31^ and $00800000 <> 0;
  case Nemu64_API.FCR31^ and 3 of
    0: LabelFCR31RoundingMode.Caption := 'Round';
    1: LabelFCR31RoundingMode.Caption := 'Trunc';
    2: LabelFCR31RoundingMode.Caption := 'Positive Infinity';
    3: LabelFCR31RoundingMode.Caption := 'Negative Infinity';
  end;
  EditIndex.Text := IntToHex(Nemu64_API.COP0Registers^[0], 8);
  EditEntryLo0.Text := IntToHex(Nemu64_API.COP0Registers^[2], 8);
  EditEntryLo1.Text := IntToHex(Nemu64_API.COP0Registers^[3], 8);
  EditContext.Text := IntToHex(Nemu64_API.COP0Registers^[4], 8);
  EditPageMask.Text := IntToHex(Nemu64_API.COP0Registers^[5], 8);
  EditWired.Text := IntToHex(Nemu64_API.COP0Registers^[6], 8);
  EditBadVAddr.Text := IntToHex(Nemu64_API.COP0Registers^[8], 8);
  EditCount.Text := IntToHex(Nemu64_API.COP0Registers^[9], 8);
  EditEntryHi.Text := IntToHex(Nemu64_API.COP0Registers^[10], 8);
  EditCompare.Text := IntToHex(Nemu64_API.COP0Registers^[11], 8);
  EditStatus.Text := IntToHex(Nemu64_API.COP0Registers^[12], 8);
  EditCause.Text := IntToHex(Nemu64_API.COP0Registers^[13], 8);
  EditExceptPC.Text := IntToHex(Nemu64_API.COP0Registers^[14], 8);
  EditPRevID.Text := IntToHex(Nemu64_API.COP0Registers^[15], 8);
  EditConfig.Text := IntToHex(Nemu64_API.COP0Registers^[16], 8);
  EditLLAddr.Text := IntToHex(Nemu64_API.COP0Registers^[17], 8);
  EditWatchLo.Text := IntToHex(Nemu64_API.COP0Registers^[18], 8);
  EditWatchHi.Text := IntToHex(Nemu64_API.COP0Registers^[19], 8);
  EditXContext.Text := IntToHex(Nemu64_API.COP0Registers^[20], 8);
  EditPErr.Text := IntToHex(Nemu64_API.COP0Registers^[26], 8);
  EditCacheErr.Text := IntToHex(Nemu64_API.COP0Registers^[27], 8);
  EditTagLo.Text := IntToHex(Nemu64_API.COP0Registers^[28], 8);
  EditTagHi.Text := IntToHex(Nemu64_API.COP0Registers^[29], 8);
  EditErrorEPC.Text := IntToHex(Nemu64_API.COP0Registers^[30], 8);
  EditRSPGP0.Text := IntToHex(Nemu64_API.RSPRegisters^[0], 8);
  EditRSPGP1.Text := IntToHex(Nemu64_API.RSPRegisters^[1], 8);
  EditRSPGP2.Text := IntToHex(Nemu64_API.RSPRegisters^[2], 8);
  EditRSPGP3.Text := IntToHex(Nemu64_API.RSPRegisters^[3], 8);
  EditRSPGP4.Text := IntToHex(Nemu64_API.RSPRegisters^[4], 8);
  EditRSPGP5.Text := IntToHex(Nemu64_API.RSPRegisters^[5], 8);
  EditRSPGP6.Text := IntToHex(Nemu64_API.RSPRegisters^[6], 8);
  EditRSPGP7.Text := IntToHex(Nemu64_API.RSPRegisters^[7], 8);
  EditRSPGP8.Text := IntToHex(Nemu64_API.RSPRegisters^[8], 8);
  EditRSPGP9.Text := IntToHex(Nemu64_API.RSPRegisters^[9], 8);
  EditRSPGP10.Text := IntToHex(Nemu64_API.RSPRegisters^[10], 8);
  EditRSPGP11.Text := IntToHex(Nemu64_API.RSPRegisters^[11], 8);
  EditRSPGP12.Text := IntToHex(Nemu64_API.RSPRegisters^[12], 8);
  EditRSPGP13.Text := IntToHex(Nemu64_API.RSPRegisters^[13], 8);
  EditRSPGP14.Text := IntToHex(Nemu64_API.RSPRegisters^[14], 8);
  EditRSPGP15.Text := IntToHex(Nemu64_API.RSPRegisters^[15], 8);
  EditRSPGP16.Text := IntToHex(Nemu64_API.RSPRegisters^[16], 8);
  EditRSPGP17.Text := IntToHex(Nemu64_API.RSPRegisters^[17], 8);
  EditRSPGP18.Text := IntToHex(Nemu64_API.RSPRegisters^[18], 8);
  EditRSPGP19.Text := IntToHex(Nemu64_API.RSPRegisters^[19], 8);
  EditRSPGP20.Text := IntToHex(Nemu64_API.RSPRegisters^[20], 8);
  EditRSPGP21.Text := IntToHex(Nemu64_API.RSPRegisters^[21], 8);
  EditRSPGP22.Text := IntToHex(Nemu64_API.RSPRegisters^[22], 8);
  EditRSPGP23.Text := IntToHex(Nemu64_API.RSPRegisters^[23], 8);
  EditRSPGP24.Text := IntToHex(Nemu64_API.RSPRegisters^[24], 8);
  EditRSPGP25.Text := IntToHex(Nemu64_API.RSPRegisters^[25], 8);
  EditRSPGP26.Text := IntToHex(Nemu64_API.RSPRegisters^[26], 8);
  EditRSPGP27.Text := IntToHex(Nemu64_API.RSPRegisters^[27], 8);
  EditRSPGP28.Text := IntToHex(Nemu64_API.RSPRegisters^[28], 8);
  EditRSPGP29.Text := IntToHex(Nemu64_API.RSPRegisters^[29], 8);
  EditRSPGP30.Text := IntToHex(Nemu64_API.RSPRegisters^[30], 8);
  EditRSPGP31.Text := IntToHex(Nemu64_API.RSPRegisters^[31], 8);
  ShowHardwareRegister($04300000, EditMI_MODE_REG);
  ShowHardwareRegister($04300004, EditMI_VERSION_REG);
  ShowHardwareRegister($04300008, EditMI_INTR_REG);
  ShowHardwareRegister($0430000C, EditMI_INTR_MASK_REG);
  ShowHardwareRegister($04040000, EditSP_MEM_ADDR_REG);
  ShowHardwareRegister($04040004, EditSP_DRAM_ADDR_REG);
  ShowHardwareRegister($04040008, EditSP_RD_LEN_REG);
  ShowHardwareRegister($0404000C, EditSP_WR_LEN_REG);
  ShowHardwareRegister($04040010, EditSP_STATUS_REG);
  ShowHardwareRegister($04040014, EditSP_DMA_FULL_REG);
  ShowHardwareRegister($04040018, EditSP_DMA_BUSY_REG);
  EditSP_SEMAPHORE_REG.Text := IntToHex(Nemu64_API.RSPControlRegisters^.Semaphore, 8);
  EditHLESPTaskAddr.Text := IntToHex(Nemu64_API.RSPTask^, 8);
  ShowHardwareRegister($04100000, EditDPC_START_REG);
  ShowHardwareRegister($04100004, EditDPC_END_REG);
  ShowHardwareRegister($04100008, EditDPC_CURRENT_REG);
  ShowHardwareRegister($0410000C, EditDPC_STATUS_REG);
  ShowHardwareRegister($04100010, EditDPC_CLOCK_REG);
  ShowHardwareRegister($04100014, EditDPC_PIPEBUSY_REG);
  ShowHardwareRegister($04100018, EditDPC_BUFBUSY_REG);
  ShowHardwareRegister($0410001C, EditDPC_TMEM_REG);
  ShowHardwareRegister($04400000, EditVI_STATUS_REG);
  ShowHardwareRegister($04400004, EditVI_DRAM_ADDR_REG);
  ShowHardwareRegister($04400008, EditVI_H_WIDTH_REG);
  ShowHardwareRegister($0440000C, EditVI_V_INTR_REG);
  ShowHardwareRegister($04400010, EditVI_V_CURRENT_LINE_REG);
  ShowHardwareRegister($04400014, EditVI_TIMING_REG);
  ShowHardwareRegister($04400018, EditVI_V_SYNC_REG);
  ShowHardwareRegister($0440001C, EditVI_H_SYNC_REG);
  ShowHardwareRegister($04400020, EditVI_H_SYNC_LEAP_REG);
  ShowHardwareRegister($04400024, EditVI_H_VIDEO_REG);
  ShowHardwareRegister($04400028, EditVI_V_VIDEO_REG);
  ShowHardwareRegister($0440002C, EditVI_V_BURST_REG);
  ShowHardwareRegister($04400030, EditVI_X_SCALE_REG);
  ShowHardwareRegister($04400034, EditVI_Y_SCALE_REG);
  ShowHardwareRegister($04800000, EditSI_DRAM_ADDR_REG);
  ShowHardwareRegister($04800018, EditSI_STATUS_REG);
  ShowHardwareRegister($04600000, EditPI_DRAM_ADDR_REG);
  ShowHardwareRegister($04600004, EditPI_CART_ADDR_REG);
  ShowHardwareRegister($04600008, EditPI_RD_LEN_REG);
  ShowHardwareRegister($0460000C, EditPI_WR_LEN_REG);
  ShowHardwareRegister($04600010, EditPI_STATUS_REG);
  ShowHardwareRegister($04600014, EditPI_BSD_DOM1_LAT_REG);
  ShowHardwareRegister($04600018, EditPI_BSD_DOM1_PWD_REG);
  ShowHardwareRegister($0460001C, EditPI_BSD_DOM1_PGS_REG);
  ShowHardwareRegister($04600020, EditPI_BSD_DOM1_RLS_REG);
  ShowHardwareRegister($04600024, EditPI_BSD_DOM2_LAT_REG);
  ShowHardwareRegister($04600028, EditPI_BSD_DOM2_PWD_REG);
  ShowHardwareRegister($0460002C, EditPI_BSD_DOM2_PGS_REG);
  ShowHardwareRegister($04600030, EditPI_BSD_DOM2_RLS_REG);
  CheckBoxMI_INTR_SP.Checked := Boolean(Nemu64_API.MIRegisters^.Interrupt and $1);
  CheckBoxMI_INTR_SI.Checked := Boolean(Nemu64_API.MIRegisters^.Interrupt and $2);
  CheckBoxMI_INTR_AI.Checked := Boolean(Nemu64_API.MIRegisters^.Interrupt and $4);
  CheckBoxMI_INTR_VI.Checked := Boolean(Nemu64_API.MIRegisters^.Interrupt and $8);
  CheckBoxMI_INTR_PI.Checked := Boolean(Nemu64_API.MIRegisters^.Interrupt and $10);
  CheckBoxMI_INTR_DP.Checked := Boolean(Nemu64_API.MIRegisters^.Interrupt and $20);
  CheckBoxMI_INTR_MASK_SP.Checked := Boolean(Nemu64_API.MIRegisters^.InterruptMask and $1);
  CheckBoxMI_INTR_MASK_SI.Checked := Boolean(Nemu64_API.MIRegisters^.InterruptMask and $2);
  CheckBoxMI_INTR_MASK_AI.Checked := Boolean(Nemu64_API.MIRegisters^.InterruptMask and $4);
  CheckBoxMI_INTR_MASK_VI.Checked := Boolean(Nemu64_API.MIRegisters^.InterruptMask and $8);
  CheckBoxMI_INTR_MASK_PI.Checked := Boolean(Nemu64_API.MIRegisters^.InterruptMask and $10);
  CheckBoxMI_INTR_MASK_DP.Checked := Boolean(Nemu64_API.MIRegisters^.InterruptMask and $20);
  RefreshVectors;
  EditAccum0.Text := FormatAccumulator(Nemu64_API.RSPAccumulator^[0]);
  EditAccum1.Text := FormatAccumulator(Nemu64_API.RSPAccumulator^[1]);
  EditAccum2.Text := FormatAccumulator(Nemu64_API.RSPAccumulator^[2]);
  EditAccum3.Text := FormatAccumulator(Nemu64_API.RSPAccumulator^[3]);
  EditAccum4.Text := FormatAccumulator(Nemu64_API.RSPAccumulator^[4]);
  EditAccum5.Text := FormatAccumulator(Nemu64_API.RSPAccumulator^[5]);
  EditAccum6.Text := FormatAccumulator(Nemu64_API.RSPAccumulator^[6]);
  EditAccum7.Text := FormatAccumulator(Nemu64_API.RSPAccumulator^[7]);
  EditRSPFlagsVCO.Text := IntToHex(Nemu64_API.RSPFlags^[0], 4);
  EditRSPFlagsVCC.Text := IntToHex(Nemu64_API.RSPFlags^[1], 4);
  EditRSPFlagsVCE.Text := IntToHex(Nemu64_API.RSPFlags^[2] and $FF, 2);
end;

procedure TFormRegisters.FormShow(Sender: TObject);
begin
  RefreshRegisters;
end;

procedure TFormRegisters.EditGPRChange(Sender: TObject);
var
  Reg: Integer;
  Text: string;
begin
  Reg := 0;
  if Sender = EditGPR1 then Reg := 1;
  if Sender = EditGPR2 then Reg := 2;
  if Sender = EditGPR3 then Reg := 3;
  if Sender = EditGPR4 then Reg := 4;
  if Sender = EditGPR5 then Reg := 5;
  if Sender = EditGPR6 then Reg := 6;
  if Sender = EditGPR7 then Reg := 7;
  if Sender = EditGPR8 then Reg := 8;
  if Sender = EditGPR9 then Reg := 9;
  if Sender = EditGPR10 then Reg := 10;
  if Sender = EditGPR11 then Reg := 11;
  if Sender = EditGPR12 then Reg := 12;
  if Sender = EditGPR13 then Reg := 13;
  if Sender = EditGPR14 then Reg := 14;
  if Sender = EditGPR15 then Reg := 15;
  if Sender = EditGPR16 then Reg := 16;
  if Sender = EditGPR17 then Reg := 17;
  if Sender = EditGPR18 then Reg := 18;
  if Sender = EditGPR19 then Reg := 19;
  if Sender = EditGPR20 then Reg := 20;
  if Sender = EditGPR21 then Reg := 21;
  if Sender = EditGPR22 then Reg := 22;
  if Sender = EditGPR23 then Reg := 23;
  if Sender = EditGPR24 then Reg := 24;
  if Sender = EditGPR25 then Reg := 25;
  if Sender = EditGPR26 then Reg := 26;
  if Sender = EditGPR27 then Reg := 27;
  if Sender = EditGPR28 then Reg := 28;
  if Sender = EditGPR29 then Reg := 29;
  if Sender = EditGPR30 then Reg := 30;
  if Sender = EditGPR31 then Reg := 31;
  Text := (Sender as TEdit).Text;
  while Pos('-', Text) <> 0 do Delete(Text, Pos('-', Text), 1);
  while Pos(' ', Text) <> 0 do Delete(Text, Pos(' ', Text), 1);
  Nemu64_API.CPURegisters^[Reg] := StrToInt64Def('$' + Text, Nemu64_API.CPURegisters^[Reg]);
end;

procedure TFormRegisters.RadioGroupFPRShowASClick(Sender: TObject);
var
  ShowOdd: Boolean;
begin
  if (RadioGroupFPRShowAS.ItemIndex = 2) or (RadioGroupFPRShowAS.ItemIndex = 4) then
    ShowOdd := False
  else
    ShowOdd := True;
  if Nemu64_API.COP0Registers^[12] and $04000000 <> 0 then ShowOdd := True;
  EditFPR1.Visible := ShowOdd;
  EditFPR3.Visible := ShowOdd;
  EditFPR5.Visible := ShowOdd;
  EditFPR7.Visible := ShowOdd;
  EditFPR9.Visible := ShowOdd;
  EditFPR11.Visible := ShowOdd;
  EditFPR13.Visible := ShowOdd;
  EditFPR15.Visible := ShowOdd;
  EditFPR17.Visible := ShowOdd;
  EditFPR19.Visible := ShowOdd;
  EditFPR21.Visible := ShowOdd;
  EditFPR23.Visible := ShowOdd;
  EditFPR25.Visible := ShowOdd;
  EditFPR27.Visible := ShowOdd;
  EditFPR29.Visible := ShowOdd;
  EditFPR31.Visible := ShowOdd;
  LabelFPR1.Visible := ShowOdd;
  LabelFPR3.Visible := ShowOdd;
  LabelFPR5.Visible := ShowOdd;
  LabelFPR7.Visible := ShowOdd;
  LabelFPR9.Visible := ShowOdd;
  LabelFPR11.Visible := ShowOdd;
  LabelFPR13.Visible := ShowOdd;
  LabelFPR15.Visible := ShowOdd;
  LabelFPR17.Visible := ShowOdd;
  LabelFPR19.Visible := ShowOdd;
  LabelFPR21.Visible := ShowOdd;
  LabelFPR23.Visible := ShowOdd;
  LabelFPR25.Visible := ShowOdd;
  LabelFPR27.Visible := ShowOdd;
  LabelFPR29.Visible := ShowOdd;
  LabelFPR31.Visible := ShowOdd;
  RefreshRegisters;
end;

procedure TFormRegisters.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Action := caHide;
end;

procedure TFormRegisters.EditRSPGPChange(Sender: TObject);
var
  Reg: Integer;
  Text: string;
begin
  Reg := 0;
  if Sender = EditRSPGP1 then Reg := 1;
  if Sender = EditRSPGP2 then Reg := 2;
  if Sender = EditRSPGP3 then Reg := 3;
  if Sender = EditRSPGP4 then Reg := 4;
  if Sender = EditRSPGP5 then Reg := 5;
  if Sender = EditRSPGP6 then Reg := 6;
  if Sender = EditRSPGP7 then Reg := 7;
  if Sender = EditRSPGP8 then Reg := 8;
  if Sender = EditRSPGP9 then Reg := 9;
  if Sender = EditRSPGP10 then Reg := 10;
  if Sender = EditRSPGP11 then Reg := 11;
  if Sender = EditRSPGP12 then Reg := 12;
  if Sender = EditRSPGP13 then Reg := 13;
  if Sender = EditRSPGP14 then Reg := 14;
  if Sender = EditRSPGP15 then Reg := 15;
  if Sender = EditRSPGP16 then Reg := 16;
  if Sender = EditRSPGP17 then Reg := 17;
  if Sender = EditRSPGP18 then Reg := 18;
  if Sender = EditRSPGP19 then Reg := 19;
  if Sender = EditRSPGP20 then Reg := 20;
  if Sender = EditRSPGP21 then Reg := 21;
  if Sender = EditRSPGP22 then Reg := 22;
  if Sender = EditRSPGP23 then Reg := 23;
  if Sender = EditRSPGP24 then Reg := 24;
  if Sender = EditRSPGP25 then Reg := 25;
  if Sender = EditRSPGP26 then Reg := 26;
  if Sender = EditRSPGP27 then Reg := 27;
  if Sender = EditRSPGP28 then Reg := 28;
  if Sender = EditRSPGP29 then Reg := 29;
  if Sender = EditRSPGP30 then Reg := 30;
  if Sender = EditRSPGP31 then Reg := 31;
  Text := (Sender as TEdit).Text;
  while Pos('-', Text) <> 0 do Delete(Text, Pos('-', Text), 1);
  while Pos(' ', Text) <> 0 do Delete(Text, Pos(' ', Text), 1);
  Nemu64_API.RSPRegisters^[Reg] := StrToIntDef('$' + Text, Nemu64_API.RSPRegisters^[Reg]);
end;

procedure TFormRegisters.TabSheetRSPGPResize(Sender: TObject);
begin
  PanelRSPGP.Left := TabSheetRSPGP.Width div 2 - PanelRSPGP.Width div 2;
  PanelRSPGP.Top := TabSheetRSPGP.Height div 2 - PanelRSPGP.Height div 2;
end;

procedure TFormRegisters.FormKeyUp(Sender: TObject; var Key: Word; Shift: TShiftState);
begin
  if Key = VK_F5 then RefreshRegisters;
end;

end.
