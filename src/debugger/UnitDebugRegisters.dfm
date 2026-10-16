object FormRegisters: TFormRegisters
  Left = 196
  Top = 108
  Width = 452
  Height = 541
  Caption = 'Registers'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = True
  OnClose = FormClose
  OnKeyUp = FormKeyUp
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object PageControl: TPageControl
    Left = 0
    Top = 0
    Width = 444
    Height = 507
    ActivePage = TabSheetGPR
    Align = alClient
    MultiLine = True
    Style = tsButtons
    TabIndex = 0
    TabOrder = 0
    object TabSheetGPR: TTabSheet
      Caption = 'General Purpose'
      object PanelGPRRegs: TPanel
        Left = 8
        Top = 4
        Width = 417
        Height = 358
        BevelInner = bvLowered
        BorderWidth = 3
        TabOrder = 0
        object LabelGRP0: TLabel
          Left = 12
          Top = 20
          Width = 55
          Height = 13
          Caption = 'Reg 0 - R0:'
        end
        object LabelGRP1: TLabel
          Left = 12
          Top = 40
          Width = 55
          Height = 13
          Caption = 'Reg 1 - AT:'
        end
        object LabelGRP2: TLabel
          Left = 12
          Top = 60
          Width = 54
          Height = 13
          Caption = 'Reg 2 - V0:'
        end
        object LabelGRP3: TLabel
          Left = 12
          Top = 80
          Width = 54
          Height = 13
          Caption = 'Reg 3 - V1:'
        end
        object LabelGRP4: TLabel
          Left = 12
          Top = 100
          Width = 54
          Height = 13
          Caption = 'Reg 4 - A0:'
        end
        object LabelGRP5: TLabel
          Left = 12
          Top = 120
          Width = 54
          Height = 13
          Caption = 'Reg 5 - A1:'
        end
        object LabelGRP6: TLabel
          Left = 12
          Top = 140
          Width = 54
          Height = 13
          Caption = 'Reg 6 - A2:'
        end
        object LabelGRP7: TLabel
          Left = 12
          Top = 160
          Width = 54
          Height = 13
          Caption = 'Reg 7 - A3:'
        end
        object LabelGRP8: TLabel
          Left = 12
          Top = 180
          Width = 54
          Height = 13
          Caption = 'Reg 8 - T0:'
        end
        object LabelGRP9: TLabel
          Left = 12
          Top = 200
          Width = 54
          Height = 13
          Caption = 'Reg 9 - T1:'
        end
        object LabelGRP10: TLabel
          Left = 12
          Top = 220
          Width = 60
          Height = 13
          Caption = 'Reg 10 - T2:'
        end
        object LabelGRP11: TLabel
          Left = 12
          Top = 240
          Width = 60
          Height = 13
          Caption = 'Reg 11 - T3:'
        end
        object LabelGRP12: TLabel
          Left = 12
          Top = 260
          Width = 60
          Height = 13
          Caption = 'Reg 12 - T4:'
        end
        object LabelGRP13: TLabel
          Left = 12
          Top = 280
          Width = 60
          Height = 13
          Caption = 'Reg 13 - T5:'
        end
        object LabelGRP14: TLabel
          Left = 12
          Top = 300
          Width = 60
          Height = 13
          Caption = 'Reg 14 - T6:'
        end
        object LabelGRP15: TLabel
          Left = 12
          Top = 320
          Width = 60
          Height = 13
          Caption = 'Reg 15 - T7:'
        end
        object LabelGRP16: TLabel
          Left = 208
          Top = 20
          Width = 60
          Height = 13
          Caption = 'Reg 16 - S0:'
        end
        object LabelGRP17: TLabel
          Left = 208
          Top = 40
          Width = 60
          Height = 13
          Caption = 'Reg 17 - S1:'
        end
        object LabelGRP18: TLabel
          Left = 208
          Top = 60
          Width = 60
          Height = 13
          Caption = 'Reg 18 - S2:'
        end
        object LabelGRP19: TLabel
          Left = 208
          Top = 80
          Width = 60
          Height = 13
          Caption = 'Reg 19 - S3:'
        end
        object LabelGRP20: TLabel
          Left = 208
          Top = 100
          Width = 60
          Height = 13
          Caption = 'Reg 20 - S4:'
        end
        object LabelGRP21: TLabel
          Left = 208
          Top = 120
          Width = 60
          Height = 13
          Caption = 'Reg 21 - S5:'
        end
        object LabelGRP22: TLabel
          Left = 208
          Top = 140
          Width = 60
          Height = 13
          Caption = 'Reg 22 - S6:'
        end
        object LabelGRP23: TLabel
          Left = 208
          Top = 160
          Width = 60
          Height = 13
          Caption = 'Reg 23 - S7:'
        end
        object LabelGRP24: TLabel
          Left = 208
          Top = 180
          Width = 60
          Height = 13
          Caption = 'Reg 24 - T8:'
        end
        object LabelGRP25: TLabel
          Left = 208
          Top = 200
          Width = 60
          Height = 13
          Caption = 'Reg 25 - T9:'
        end
        object LabelGRP26: TLabel
          Left = 208
          Top = 220
          Width = 60
          Height = 13
          Caption = 'Reg 26 - K0:'
        end
        object LabelGRP27: TLabel
          Left = 208
          Top = 240
          Width = 60
          Height = 13
          Caption = 'Reg 27 - K1:'
        end
        object LabelGRP28: TLabel
          Left = 208
          Top = 260
          Width = 62
          Height = 13
          Caption = 'Reg 28 - GP:'
        end
        object LabelGRP29: TLabel
          Left = 208
          Top = 280
          Width = 61
          Height = 13
          Caption = 'Reg 29 - SP:'
        end
        object LabelGRP30: TLabel
          Left = 208
          Top = 300
          Width = 60
          Height = 13
          Caption = 'Reg 30 - S8:'
        end
        object LabelGRP31: TLabel
          Left = 208
          Top = 320
          Width = 62
          Height = 13
          Caption = 'Reg 31 - RA:'
        end
        object EditGPR16: TEdit
          Left = 276
          Top = 16
          Width = 121
          Height = 21
          TabOrder = 16
          OnChange = EditGPRChange
        end
        object EditGPR17: TEdit
          Left = 276
          Top = 36
          Width = 121
          Height = 21
          TabOrder = 17
          OnChange = EditGPRChange
        end
        object EditGPR0: TEdit
          Left = 80
          Top = 16
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 0
        end
        object EditGPR1: TEdit
          Left = 80
          Top = 36
          Width = 121
          Height = 21
          TabOrder = 1
          OnChange = EditGPRChange
        end
        object EditGPR2: TEdit
          Left = 80
          Top = 56
          Width = 121
          Height = 21
          TabOrder = 2
          OnChange = EditGPRChange
        end
        object EditGPR3: TEdit
          Left = 80
          Top = 76
          Width = 121
          Height = 21
          TabOrder = 3
          OnChange = EditGPRChange
        end
        object EditGPR4: TEdit
          Left = 80
          Top = 96
          Width = 121
          Height = 21
          TabOrder = 4
          OnChange = EditGPRChange
        end
        object EditGPR5: TEdit
          Left = 80
          Top = 116
          Width = 121
          Height = 21
          TabOrder = 5
          OnChange = EditGPRChange
        end
        object EditGPR6: TEdit
          Left = 80
          Top = 136
          Width = 121
          Height = 21
          TabOrder = 6
          OnChange = EditGPRChange
        end
        object EditGPR7: TEdit
          Left = 80
          Top = 156
          Width = 121
          Height = 21
          TabOrder = 7
          OnChange = EditGPRChange
        end
        object EditGPR8: TEdit
          Left = 80
          Top = 176
          Width = 121
          Height = 21
          TabOrder = 8
          OnChange = EditGPRChange
        end
        object EditGPR9: TEdit
          Left = 80
          Top = 196
          Width = 121
          Height = 21
          TabOrder = 9
          OnChange = EditGPRChange
        end
        object EditGPR10: TEdit
          Left = 80
          Top = 216
          Width = 121
          Height = 21
          TabOrder = 10
          OnChange = EditGPRChange
        end
        object EditGPR11: TEdit
          Left = 80
          Top = 236
          Width = 121
          Height = 21
          TabOrder = 11
          OnChange = EditGPRChange
        end
        object EditGPR12: TEdit
          Left = 80
          Top = 256
          Width = 121
          Height = 21
          TabOrder = 12
          OnChange = EditGPRChange
        end
        object EditGPR13: TEdit
          Left = 80
          Top = 276
          Width = 121
          Height = 21
          TabOrder = 13
          OnChange = EditGPRChange
        end
        object EditGPR14: TEdit
          Left = 80
          Top = 296
          Width = 121
          Height = 21
          TabOrder = 14
          OnChange = EditGPRChange
        end
        object EditGPR15: TEdit
          Left = 80
          Top = 316
          Width = 121
          Height = 21
          TabOrder = 15
          OnChange = EditGPRChange
        end
        object EditGPR18: TEdit
          Left = 276
          Top = 56
          Width = 121
          Height = 21
          TabOrder = 18
          OnChange = EditGPRChange
        end
        object EditGPR19: TEdit
          Left = 276
          Top = 76
          Width = 121
          Height = 21
          TabOrder = 19
          OnChange = EditGPRChange
        end
        object EditGPR20: TEdit
          Left = 276
          Top = 96
          Width = 121
          Height = 21
          TabOrder = 20
          OnChange = EditGPRChange
        end
        object EditGPR21: TEdit
          Left = 276
          Top = 116
          Width = 121
          Height = 21
          TabOrder = 21
          OnChange = EditGPRChange
        end
        object EditGPR22: TEdit
          Left = 276
          Top = 136
          Width = 121
          Height = 21
          TabOrder = 22
          OnChange = EditGPRChange
        end
        object EditGPR23: TEdit
          Left = 276
          Top = 156
          Width = 121
          Height = 21
          TabOrder = 23
          OnChange = EditGPRChange
        end
        object EditGPR24: TEdit
          Left = 276
          Top = 176
          Width = 121
          Height = 21
          TabOrder = 24
          OnChange = EditGPRChange
        end
        object EditGPR25: TEdit
          Left = 276
          Top = 196
          Width = 121
          Height = 21
          TabOrder = 25
          OnChange = EditGPRChange
        end
        object EditGPR26: TEdit
          Left = 276
          Top = 216
          Width = 121
          Height = 21
          TabOrder = 26
          OnChange = EditGPRChange
        end
        object EditGPR27: TEdit
          Left = 276
          Top = 236
          Width = 121
          Height = 21
          TabOrder = 27
          OnChange = EditGPRChange
        end
        object EditGPR28: TEdit
          Left = 276
          Top = 256
          Width = 121
          Height = 21
          TabOrder = 28
          OnChange = EditGPRChange
        end
        object EditGPR29: TEdit
          Left = 276
          Top = 276
          Width = 121
          Height = 21
          TabOrder = 29
          OnChange = EditGPRChange
        end
        object EditGPR30: TEdit
          Left = 276
          Top = 296
          Width = 121
          Height = 21
          TabOrder = 30
          OnChange = EditGPRChange
        end
        object EditGPR31: TEdit
          Left = 276
          Top = 316
          Width = 121
          Height = 21
          TabOrder = 31
          OnChange = EditGPRChange
        end
      end
      object PanelCPURegs: TPanel
        Left = 8
        Top = 368
        Width = 417
        Height = 77
        BevelInner = bvLowered
        BorderWidth = 3
        TabOrder = 1
        object LabelPC: TLabel
          Left = 12
          Top = 20
          Width = 17
          Height = 13
          Caption = 'PC:'
        end
        object LabelMultLO: TLabel
          Left = 12
          Top = 40
          Width = 40
          Height = 13
          Caption = 'Mult LO:'
        end
        object LabelMultHI: TLabel
          Left = 208
          Top = 40
          Width = 34
          Height = 13
          Caption = 'MultHI:'
        end
        object EditPC: TEdit
          Left = 80
          Top = 16
          Width = 65
          Height = 21
          Color = clBtnFace
          TabOrder = 0
        end
        object EditMultLO: TEdit
          Left = 80
          Top = 36
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 1
        end
        object EditMultHI: TEdit
          Left = 276
          Top = 36
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 2
        end
      end
    end
    object TabSheetCOP0: TTabSheet
      Caption = 'COP 0'
      ImageIndex = 4
      object PanelCOP0: TPanel
        Left = 8
        Top = 4
        Width = 417
        Height = 358
        BevelInner = bvLowered
        BorderWidth = 3
        TabOrder = 0
        object LabelIndex: TLabel
          Left = 12
          Top = 20
          Width = 29
          Height = 13
          BiDiMode = bdLeftToRight
          Caption = 'Index:'
          ParentBiDiMode = False
        end
        object LabelEntryLo0: TLabel
          Left = 12
          Top = 40
          Width = 45
          Height = 13
          Caption = 'EntryLo0:'
        end
        object LabelEntryLo1: TLabel
          Left = 12
          Top = 60
          Width = 45
          Height = 13
          Caption = 'EntryLo1:'
        end
        object LabelContext: TLabel
          Left = 12
          Top = 80
          Width = 39
          Height = 13
          Caption = 'Context:'
        end
        object LabelPageMask: TLabel
          Left = 12
          Top = 100
          Width = 54
          Height = 13
          Caption = 'PageMask:'
        end
        object LabelWired: TLabel
          Left = 12
          Top = 120
          Width = 31
          Height = 13
          Caption = 'Wired:'
        end
        object LabelBadVAddr: TLabel
          Left = 12
          Top = 140
          Width = 51
          Height = 13
          Caption = 'BadVAddr:'
        end
        object LabelCount: TLabel
          Left = 12
          Top = 160
          Width = 31
          Height = 13
          Caption = 'Count:'
        end
        object LabelEntryHi: TLabel
          Left = 12
          Top = 180
          Width = 37
          Height = 13
          Caption = 'EntryHi:'
        end
        object LabelCompare: TLabel
          Left = 12
          Top = 200
          Width = 45
          Height = 13
          Caption = 'Compare:'
        end
        object LabelStatus: TLabel
          Left = 12
          Top = 220
          Width = 30
          Height = 13
          Caption = 'Status'
        end
        object LabelCause: TLabel
          Left = 12
          Top = 240
          Width = 33
          Height = 13
          Caption = 'Cause:'
        end
        object LabelExceptPC: TLabel
          Left = 12
          Top = 260
          Width = 50
          Height = 13
          Caption = 'ExcectPC:'
        end
        object LabelPRevID: TLabel
          Left = 12
          Top = 280
          Width = 41
          Height = 13
          Caption = 'PRevID:'
        end
        object LabelConfig: TLabel
          Left = 12
          Top = 300
          Width = 33
          Height = 13
          Caption = 'Config:'
        end
        object LabelLLAddr: TLabel
          Left = 12
          Top = 320
          Width = 37
          Height = 13
          Caption = 'LLAddr:'
        end
        object LabelWatchLo: TLabel
          Left = 208
          Top = 20
          Width = 47
          Height = 13
          Caption = 'WatchLo:'
        end
        object LabelWatchHi: TLabel
          Left = 208
          Top = 40
          Width = 45
          Height = 13
          Caption = 'WatchHi:'
        end
        object LabelXContext: TLabel
          Left = 208
          Top = 60
          Width = 46
          Height = 13
          Caption = 'XContext:'
        end
        object LabelPErr: TLabel
          Left = 208
          Top = 80
          Width = 23
          Height = 13
          Caption = 'PErr:'
        end
        object LabelCacheErr: TLabel
          Left = 208
          Top = 100
          Width = 47
          Height = 13
          Caption = 'CacheErr:'
        end
        object LabelTagLo: TLabel
          Left = 208
          Top = 120
          Width = 34
          Height = 13
          Caption = 'TagLo:'
        end
        object LabelTagHi: TLabel
          Left = 208
          Top = 140
          Width = 32
          Height = 13
          Caption = 'TagHi:'
        end
        object LabelErrorEPC: TLabel
          Left = 208
          Top = 160
          Width = 46
          Height = 13
          Caption = 'ErrorEPC:'
        end
        object EditIndex: TEdit
          Left = 80
          Top = 16
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 0
        end
        object EditEntryLo0: TEdit
          Left = 80
          Top = 36
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 1
        end
        object EditEntryLo1: TEdit
          Left = 80
          Top = 56
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 2
        end
        object EditContext: TEdit
          Left = 80
          Top = 76
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 3
        end
        object EditPageMask: TEdit
          Left = 80
          Top = 96
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 4
        end
        object EditWired: TEdit
          Left = 80
          Top = 116
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 5
        end
        object EditBadVAddr: TEdit
          Left = 80
          Top = 136
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 6
        end
        object EditCount: TEdit
          Left = 80
          Top = 156
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 7
        end
        object EditEntryHi: TEdit
          Left = 80
          Top = 176
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 8
        end
        object EditCompare: TEdit
          Left = 80
          Top = 196
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 9
        end
        object EditStatus: TEdit
          Left = 80
          Top = 216
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 10
        end
        object EditCause: TEdit
          Left = 80
          Top = 236
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 11
        end
        object EditExceptPC: TEdit
          Left = 80
          Top = 256
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 12
        end
        object EditPRevID: TEdit
          Left = 80
          Top = 276
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 13
        end
        object EditConfig: TEdit
          Left = 80
          Top = 296
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 14
        end
        object EditLLAddr: TEdit
          Left = 80
          Top = 316
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 15
        end
        object EditWatchLo: TEdit
          Left = 276
          Top = 16
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 16
        end
        object EditWatchHi: TEdit
          Left = 276
          Top = 36
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 17
        end
        object EditXContext: TEdit
          Left = 276
          Top = 56
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 18
        end
        object EditPErr: TEdit
          Left = 276
          Top = 76
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 19
        end
        object EditCacheErr: TEdit
          Left = 276
          Top = 96
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 20
        end
        object EditTagLo: TEdit
          Left = 276
          Top = 116
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 21
        end
        object EditTagHi: TEdit
          Left = 276
          Top = 136
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 22
        end
        object EditErrorEPC: TEdit
          Left = 276
          Top = 156
          Width = 121
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 23
        end
      end
    end
    object TabSheetCOP1: TTabSheet
      Caption = 'COP 1'
      ImageIndex = 1
      object PanelFPR: TPanel
        Left = 120
        Top = 5
        Width = 307
        Height = 352
        BevelInner = bvLowered
        BorderWidth = 3
        TabOrder = 0
        object LabelFPR0: TLabel
          Left = 12
          Top = 20
          Width = 33
          Height = 13
          BiDiMode = bdLeftToRight
          Caption = 'FPR 0:'
          ParentBiDiMode = False
        end
        object LabelFPR1: TLabel
          Left = 12
          Top = 40
          Width = 33
          Height = 13
          Caption = 'FPR 1:'
        end
        object LabelFPR2: TLabel
          Left = 12
          Top = 60
          Width = 33
          Height = 13
          Caption = 'FPR 2:'
        end
        object LabelFPR3: TLabel
          Left = 12
          Top = 80
          Width = 33
          Height = 13
          Caption = 'FPR 3:'
        end
        object LabelFPR4: TLabel
          Left = 12
          Top = 100
          Width = 33
          Height = 13
          Caption = 'FPR 4:'
        end
        object LabelFPR5: TLabel
          Left = 12
          Top = 120
          Width = 33
          Height = 13
          Caption = 'FPR 5:'
        end
        object LabelFPR6: TLabel
          Left = 12
          Top = 140
          Width = 33
          Height = 13
          Caption = 'FPR 6:'
        end
        object LabelFPR7: TLabel
          Left = 12
          Top = 160
          Width = 33
          Height = 13
          Caption = 'FPR 7:'
        end
        object LabelFPR8: TLabel
          Left = 12
          Top = 180
          Width = 33
          Height = 13
          Caption = 'FPR 8:'
        end
        object LabelFPR9: TLabel
          Left = 12
          Top = 200
          Width = 33
          Height = 13
          Caption = 'FPR 9:'
        end
        object LabelFPR10: TLabel
          Left = 12
          Top = 220
          Width = 39
          Height = 13
          Caption = 'FPR 10:'
        end
        object LabelFPR11: TLabel
          Left = 12
          Top = 240
          Width = 39
          Height = 13
          Caption = 'FPR 11:'
        end
        object LabelFPR12: TLabel
          Left = 12
          Top = 260
          Width = 39
          Height = 13
          Caption = 'FPR 12:'
        end
        object LabelFPR13: TLabel
          Left = 12
          Top = 280
          Width = 39
          Height = 13
          Caption = 'FPR 13:'
        end
        object LabelFPR14: TLabel
          Left = 12
          Top = 300
          Width = 39
          Height = 13
          Caption = 'FPR 14:'
        end
        object LabelFPR15: TLabel
          Left = 12
          Top = 320
          Width = 39
          Height = 13
          Caption = 'FPR 15:'
        end
        object LabelFPR16: TLabel
          Left = 156
          Top = 20
          Width = 39
          Height = 13
          Caption = 'FPR 16:'
        end
        object LabelFPR17: TLabel
          Left = 156
          Top = 40
          Width = 39
          Height = 13
          Caption = 'FPR 17:'
        end
        object LabelFPR18: TLabel
          Left = 156
          Top = 60
          Width = 39
          Height = 13
          Caption = 'FPR 18:'
        end
        object LabelFPR19: TLabel
          Left = 156
          Top = 80
          Width = 39
          Height = 13
          Caption = 'FPR 19:'
        end
        object LabelFPR20: TLabel
          Left = 156
          Top = 100
          Width = 39
          Height = 13
          Caption = 'FPR 20:'
        end
        object LabelFPR21: TLabel
          Left = 156
          Top = 120
          Width = 39
          Height = 13
          Caption = 'FPR 21:'
        end
        object LabelFPR22: TLabel
          Left = 156
          Top = 140
          Width = 39
          Height = 13
          Caption = 'FPR 22:'
        end
        object LabelFPR23: TLabel
          Left = 156
          Top = 160
          Width = 39
          Height = 13
          Caption = 'FPR 23:'
        end
        object LabelFPR24: TLabel
          Left = 156
          Top = 180
          Width = 39
          Height = 13
          Caption = 'FPR 24:'
        end
        object LabelFPR25: TLabel
          Left = 156
          Top = 200
          Width = 39
          Height = 13
          Caption = 'FPR 25:'
        end
        object LabelFPR26: TLabel
          Left = 156
          Top = 220
          Width = 39
          Height = 13
          Caption = 'FPR 26:'
        end
        object LabelFPR27: TLabel
          Left = 156
          Top = 240
          Width = 39
          Height = 13
          Caption = 'FPR 27:'
        end
        object LabelFPR28: TLabel
          Left = 156
          Top = 260
          Width = 39
          Height = 13
          Caption = 'FPR 28:'
        end
        object LabelFPR29: TLabel
          Left = 156
          Top = 280
          Width = 39
          Height = 13
          Caption = 'FPR 29:'
        end
        object LabelFPR30: TLabel
          Left = 156
          Top = 300
          Width = 39
          Height = 13
          Caption = 'FPR 30:'
        end
        object LabelFPR31: TLabel
          Left = 156
          Top = 320
          Width = 39
          Height = 13
          Caption = 'FPR 31:'
        end
        object EditFPR16: TEdit
          Left = 208
          Top = 16
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 29
        end
        object EditFPR17: TEdit
          Left = 208
          Top = 36
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 30
        end
        object EditFPR18: TEdit
          Left = 208
          Top = 56
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 31
        end
        object EditFPR0: TEdit
          Left = 64
          Top = 16
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 0
        end
        object EditFPR1: TEdit
          Left = 64
          Top = 36
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 1
        end
        object EditFPR2: TEdit
          Left = 64
          Top = 56
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 2
        end
        object EditFPR3: TEdit
          Left = 64
          Top = 76
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 3
        end
        object EditFPR4: TEdit
          Left = 64
          Top = 96
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 4
        end
        object EditFPR5: TEdit
          Left = 64
          Top = 116
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 5
        end
        object EditFPR6: TEdit
          Left = 64
          Top = 136
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 6
        end
        object EditFPR7: TEdit
          Left = 64
          Top = 156
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 7
        end
        object EditFPR8: TEdit
          Left = 64
          Top = 176
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 8
        end
        object EditFPR9: TEdit
          Left = 64
          Top = 196
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 9
        end
        object EditFPR10: TEdit
          Left = 64
          Top = 216
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 10
        end
        object EditFPR11: TEdit
          Left = 64
          Top = 236
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 11
        end
        object EditFPR12: TEdit
          Left = 64
          Top = 256
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 12
        end
        object EditFPR13: TEdit
          Left = 64
          Top = 276
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 13
        end
        object EditFPR14: TEdit
          Left = 64
          Top = 296
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 14
        end
        object EditFPR15: TEdit
          Left = 64
          Top = 316
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 15
        end
        object EditFPR19: TEdit
          Left = 208
          Top = 76
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 16
        end
        object EditFPR20: TEdit
          Left = 208
          Top = 96
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 17
        end
        object EditFPR21: TEdit
          Left = 208
          Top = 116
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 18
        end
        object EditFPR22: TEdit
          Left = 208
          Top = 136
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 19
        end
        object EditFPR23: TEdit
          Left = 208
          Top = 156
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 20
        end
        object EditFPR24: TEdit
          Left = 208
          Top = 176
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 21
        end
        object EditFPR25: TEdit
          Left = 208
          Top = 196
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 22
        end
        object EditFPR26: TEdit
          Left = 208
          Top = 216
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 23
        end
        object EditFPR27: TEdit
          Left = 208
          Top = 236
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 24
        end
        object EditFPR28: TEdit
          Left = 208
          Top = 256
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 25
        end
        object EditFPR29: TEdit
          Left = 208
          Top = 276
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 26
        end
        object EditFPR30: TEdit
          Left = 208
          Top = 296
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 27
        end
        object EditFPR31: TEdit
          Left = 208
          Top = 316
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 28
        end
      end
      object RadioGroupFPRShowAS: TRadioGroup
        Left = 8
        Top = 4
        Width = 105
        Height = 121
        Caption = 'Show as'
        ItemIndex = 0
        Items.Strings = (
          'raw hex'
          'single'
          'double'
          'word'
          'long')
        TabOrder = 1
        OnClick = RadioGroupFPRShowASClick
      end
      object GroupBoxFCR31: TGroupBox
        Left = 8
        Top = 248
        Width = 105
        Height = 109
        Caption = 'FCR Status Reg 31'
        Enabled = False
        TabOrder = 2
        object LabelFCR31RoundingMode: TLabel
          Left = 12
          Top = 88
          Width = 76
          Height = 13
          Caption = 'Rounding Mode'
        end
        object EditFCR31: TEdit
          Left = 12
          Top = 20
          Width = 81
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 0
          Text = 'EditFCR31'
        end
        object CheckBoxFCR31FS: TCheckBox
          Left = 12
          Top = 48
          Width = 90
          Height = 17
          Caption = 'FS'
          TabOrder = 1
        end
        object CheckBoxFCR31C: TCheckBox
          Left = 12
          Top = 68
          Width = 85
          Height = 17
          Caption = 'Condition'
          TabOrder = 2
        end
      end
    end
    object TabSheetMIRegisters: TTabSheet
      Caption = 'MI Registers'
      ImageIndex = 6
      object PanelMIRegs: TPanel
        Left = 8
        Top = 4
        Width = 277
        Height = 113
        BevelInner = bvLowered
        BorderWidth = 3
        TabOrder = 0
        object LabelMIModeReg: TLabel
          Left = 12
          Top = 20
          Width = 147
          Height = 13
          Caption = '0x04300000: MI_MODE_REG:'
        end
        object LabelMIVersionReg: TLabel
          Left = 12
          Top = 40
          Width = 163
          Height = 13
          Caption = '0x04300004: MI_VERSION_REG:'
        end
        object LabelMIIntrReg: TLabel
          Left = 12
          Top = 60
          Width = 141
          Height = 13
          Caption = '0x04300008: MI_INTR_REG:'
        end
        object LabelMIIntrMaskReg: TLabel
          Left = 12
          Top = 80
          Width = 178
          Height = 13
          Caption = '0x0430000C: MI_INTR_MASK_REG:'
        end
        object EditMI_MODE_REG: TEdit
          Left = 200
          Top = 16
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 0
        end
        object EditMI_VERSION_REG: TEdit
          Left = 200
          Top = 36
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 3
        end
        object EditMI_INTR_REG: TEdit
          Left = 200
          Top = 56
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 1
        end
        object EditMI_INTR_MASK_REG: TEdit
          Left = 200
          Top = 76
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 2
        end
      end
      object GroupBoxMI_INTR_REGInterpretation: TGroupBox
        Left = 8
        Top = 124
        Width = 157
        Height = 149
        Caption = 'MI_INTR_REG'
        Enabled = False
        TabOrder = 1
        object CheckBoxMI_INTR_SP: TCheckBox
          Left = 12
          Top = 20
          Width = 97
          Height = 17
          Caption = 'MI_INTR_SP'
          TabOrder = 0
        end
        object CheckBoxMI_INTR_SI: TCheckBox
          Left = 12
          Top = 40
          Width = 97
          Height = 17
          Caption = 'MI_INTR_SI'
          TabOrder = 1
        end
        object CheckBoxMI_INTR_AI: TCheckBox
          Left = 12
          Top = 60
          Width = 97
          Height = 17
          Caption = 'MI_INTR_AI'
          TabOrder = 2
        end
        object CheckBoxMI_INTR_VI: TCheckBox
          Left = 12
          Top = 80
          Width = 97
          Height = 17
          Caption = 'MI_INTR_VI'
          TabOrder = 3
        end
        object CheckBoxMI_INTR_PI: TCheckBox
          Left = 12
          Top = 100
          Width = 97
          Height = 17
          Caption = 'MI_INTR_PI'
          TabOrder = 4
        end
        object CheckBoxMI_INTR_DP: TCheckBox
          Left = 12
          Top = 120
          Width = 97
          Height = 17
          Caption = 'MI_INTR_DP'
          TabOrder = 5
        end
      end
      object GroupBoxMI_INTR_MASK_REG: TGroupBox
        Left = 172
        Top = 124
        Width = 157
        Height = 149
        Caption = 'MI_INTR_MASK_REG'
        Enabled = False
        TabOrder = 2
        object CheckBoxMI_INTR_MASK_SP: TCheckBox
          Left = 12
          Top = 20
          Width = 130
          Height = 17
          Caption = 'MI_INTR_MASK_SP'
          TabOrder = 0
        end
        object CheckBoxMI_INTR_MASK_SI: TCheckBox
          Left = 12
          Top = 40
          Width = 130
          Height = 17
          Caption = 'MI_INTR_MASK_SI'
          TabOrder = 1
        end
        object CheckBoxMI_INTR_MASK_AI: TCheckBox
          Left = 12
          Top = 60
          Width = 130
          Height = 17
          Caption = 'MI_INTR_MASK_AI'
          TabOrder = 2
        end
        object CheckBoxMI_INTR_MASK_VI: TCheckBox
          Left = 12
          Top = 80
          Width = 130
          Height = 17
          Caption = 'MI_INTR_MASK_VI'
          TabOrder = 3
        end
        object CheckBoxMI_INTR_MASK_PI: TCheckBox
          Left = 12
          Top = 100
          Width = 130
          Height = 17
          Caption = 'MI_INTR_MASK_PI'
          TabOrder = 4
        end
        object CheckBoxMI_INTR_MASK_DP: TCheckBox
          Left = 12
          Top = 120
          Width = 130
          Height = 17
          Caption = 'MI_INTR_MASK_DP'
          TabOrder = 5
        end
      end
    end
    object TabSheetSPRegs: TTabSheet
      Caption = 'SP Registers'
      ImageIndex = 10
      object LabelLastHLETaskAddr: TLabel
        Left = 84
        Top = 332
        Width = 129
        Height = 13
        Caption = 'Last HLE SPTask Address:'
      end
      object PanelSPRegs: TPanel
        Left = 75
        Top = 132
        Width = 283
        Height = 191
        BevelInner = bvLowered
        BorderWidth = 3
        TabOrder = 0
        object LabelSP_MEM_ADDR_REG: TLabel
          Left = 12
          Top = 20
          Width = 179
          Height = 13
          Caption = '0x04040000: SP_MEM_ADDR_REG:'
        end
        object LabelSP_DRAM_ADDR_REG: TLabel
          Left = 12
          Top = 40
          Width = 186
          Height = 13
          Caption = '0x04040004: SP_DRAM_ADDR_REG:'
        end
        object LabelSP_RD_LEN_REG: TLabel
          Left = 12
          Top = 60
          Width = 160
          Height = 13
          Caption = '0x04040008: SP_RD_LEN_REG:'
        end
        object LabelSP_WR_LEN_REG: TLabel
          Left = 12
          Top = 80
          Width = 164
          Height = 13
          Caption = '0x0404000C: SP_WR_LEN_REG:'
        end
        object LabelSP_STATUS_REG: TLabel
          Left = 12
          Top = 100
          Width = 160
          Height = 13
          Caption = '0x04040010: SP_STATUS_REG:'
        end
        object LabelSP_DMA_FULL_REG: TLabel
          Left = 12
          Top = 120
          Width = 173
          Height = 13
          Caption = '0x04040014: SP_DMA_FULL_REG:'
        end
        object LabelSP_DMA_BUSY_REG: TLabel
          Left = 12
          Top = 140
          Width = 176
          Height = 13
          Caption = '0x04040018: SP_DMA_BUSY_REG:'
        end
        object LabelSP_SEMAPHORE_REG: TLabel
          Left = 12
          Top = 160
          Width = 186
          Height = 13
          Caption = '0x0404001C: SP_SEMAPHORE_REG:'
        end
        object EditSP_MEM_ADDR_REG: TEdit
          Left = 204
          Top = 16
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 0
        end
        object EditSP_DRAM_ADDR_REG: TEdit
          Left = 204
          Top = 36
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 3
        end
        object EditSP_RD_LEN_REG: TEdit
          Left = 204
          Top = 56
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 1
        end
        object EditSP_WR_LEN_REG: TEdit
          Left = 204
          Top = 76
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 2
        end
        object EditSP_STATUS_REG: TEdit
          Left = 204
          Top = 96
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 4
        end
        object EditSP_DMA_FULL_REG: TEdit
          Left = 204
          Top = 116
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 5
        end
        object EditSP_DMA_BUSY_REG: TEdit
          Left = 204
          Top = 136
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 6
        end
        object EditSP_SEMAPHORE_REG: TEdit
          Left = 204
          Top = 156
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 7
        end
      end
      object EditHLESPTaskAddr: TEdit
        Left = 272
        Top = 328
        Width = 65
        Height = 21
        Color = clBtnFace
        ReadOnly = True
        TabOrder = 1
      end
    end
    object TabSheetDP: TTabSheet
      Caption = 'DP Registers'
      ImageIndex = 11
      object PanelDPRegs: TPanel
        Left = 75
        Top = 132
        Width = 283
        Height = 191
        BevelInner = bvLowered
        BorderWidth = 3
        TabOrder = 0
        object LabelDPC_START_REG: TLabel
          Left = 12
          Top = 20
          Width = 161
          Height = 13
          Caption = '0x04100000: DPC_START_REG:'
        end
        object LabelDPC_END_REG: TLabel
          Left = 12
          Top = 40
          Width = 148
          Height = 13
          Caption = '0x04100004: DPC_END_REG:'
        end
        object LabelDPC_CURRENT_REG: TLabel
          Left = 12
          Top = 60
          Width = 178
          Height = 13
          Caption = '0x04100008: DPC_CURRENT_REG:'
        end
        object LabelDPC_STATUS_REG: TLabel
          Left = 12
          Top = 80
          Width = 169
          Height = 13
          Caption = '0x0410000C: DPC_STATUS_REG:'
        end
        object LabelDPC_CLOCK_REG: TLabel
          Left = 12
          Top = 100
          Width = 160
          Height = 13
          Caption = '0x04100010: DPC_CLOCK_REG:'
        end
        object LabelDPC_BUFBUSY_REG: TLabel
          Left = 12
          Top = 120
          Width = 175
          Height = 13
          Caption = '0x04100014: DPC_BUFBUSY_REG:'
        end
        object LabelDPC_PIPEBUSY_REG: TLabel
          Left = 12
          Top = 140
          Width = 178
          Height = 13
          Caption = '0x04100018: DPC_PIPEBUSY_REG:'
        end
        object LabelDPC_TMEM_REG: TLabel
          Left = 12
          Top = 160
          Width = 158
          Height = 13
          Caption = '0x0410001C: DPC_TMEM_REG:'
        end
        object EditDPC_START_REG: TEdit
          Left = 204
          Top = 16
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 0
        end
        object EditDPC_END_REG: TEdit
          Left = 204
          Top = 36
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 3
        end
        object EditDPC_CURRENT_REG: TEdit
          Left = 204
          Top = 56
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 1
        end
        object EditDPC_STATUS_REG: TEdit
          Left = 204
          Top = 76
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 2
        end
        object EditDPC_CLOCK_REG: TEdit
          Left = 204
          Top = 96
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 4
        end
        object EditDPC_BUFBUSY_REG: TEdit
          Left = 204
          Top = 116
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 5
        end
        object EditDPC_PIPEBUSY_REG: TEdit
          Left = 204
          Top = 136
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 6
        end
        object EditDPC_TMEM_REG: TEdit
          Left = 204
          Top = 156
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 7
        end
      end
    end
    object TabSheetVIRegs: TTabSheet
      Caption = 'VI Registers'
      ImageIndex = 2
      object PanelVIRegs: TPanel
        Left = 67
        Top = 71
        Width = 299
        Height = 313
        BevelInner = bvLowered
        BorderWidth = 3
        TabOrder = 0
        object LabelVI_INTR_REG: TLabel
          Left = 12
          Top = 80
          Width = 153
          Height = 13
          Caption = '0x0440000C: VI_V_INTR_REG:'
        end
        object LabelVI_H_WIDTH_REG: TLabel
          Left = 12
          Top = 60
          Width = 164
          Height = 13
          Caption = '0x04400008: VI_H_WIDTH_REG:'
        end
        object LabelVI_DRAM_ADDR_REG: TLabel
          Left = 12
          Top = 40
          Width = 182
          Height = 13
          Caption = '0x04400004: VI_DRAM_ADDR_REG:'
        end
        object LabelVI_STATUS_REG: TLabel
          Left = 12
          Top = 20
          Width = 156
          Height = 13
          Caption = '0x04400000: VI_STATUS_REG:'
        end
        object LabelVI_V_CURRENT_LINE_REG: TLabel
          Left = 12
          Top = 100
          Width = 209
          Height = 13
          Caption = '0x04400010: VI_V_CURRENT_LINE_REG:'
        end
        object LabelVI_TIMING_REG: TLabel
          Left = 12
          Top = 120
          Width = 151
          Height = 13
          Caption = '0x04400014: VI_TIMING_REG:'
        end
        object LabelVI_V_SYNC_REG: TLabel
          Left = 12
          Top = 140
          Width = 155
          Height = 13
          Caption = '0x04400018: VI_V_SYNC_REG:'
        end
        object LabelVI_H_SYNC_REG: TLabel
          Left = 12
          Top = 160
          Width = 157
          Height = 13
          Caption = '0x0440001C: VI_H_SYNC_REG:'
        end
        object LabelVI_H_SYNC_LEAP_REG: TLabel
          Left = 12
          Top = 180
          Width = 186
          Height = 13
          Caption = '0x04400020: VI_H_SYNC_LEAP_REG'
        end
        object LabelVI_H_VIDEO_REG: TLabel
          Left = 12
          Top = 200
          Width = 160
          Height = 13
          Caption = '0x04400024: VI_H_VIDEO_REG:'
        end
        object LabelVI_V_VIDEO_REG: TLabel
          Left = 12
          Top = 220
          Width = 159
          Height = 13
          Caption = '0x04400028: VI_V_VIDEO_REG:'
        end
        object LabelVI_V_BURST_REG: TLabel
          Left = 12
          Top = 240
          Width = 164
          Height = 13
          Caption = '0x0440002C: VI_V_BURST_REG:'
        end
        object LabelVI_X_SCALE_REG: TLabel
          Left = 12
          Top = 260
          Width = 160
          Height = 13
          Caption = '0x04400030: VI_X_SCALE_REG:'
        end
        object LabelVI_Y_SCALE_REG: TLabel
          Left = 12
          Top = 280
          Width = 160
          Height = 13
          Caption = '0x04400034: VI_Y_SCALE_REG:'
        end
        object EditVI_STATUS_REG: TEdit
          Left = 220
          Top = 16
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 0
        end
        object EditVI_DRAM_ADDR_REG: TEdit
          Left = 220
          Top = 36
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 1
        end
        object EditVI_H_WIDTH_REG: TEdit
          Left = 220
          Top = 56
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 2
        end
        object EditVI_V_INTR_REG: TEdit
          Left = 220
          Top = 76
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 3
        end
        object EditVI_V_CURRENT_LINE_REG: TEdit
          Left = 220
          Top = 96
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 4
        end
        object EditVI_TIMING_REG: TEdit
          Left = 220
          Top = 116
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 5
        end
        object EditVI_V_SYNC_REG: TEdit
          Left = 220
          Top = 136
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 6
        end
        object EditVI_H_SYNC_REG: TEdit
          Left = 220
          Top = 156
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 7
        end
        object EditVI_H_SYNC_LEAP_REG: TEdit
          Left = 220
          Top = 176
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 8
        end
        object EditVI_H_VIDEO_REG: TEdit
          Left = 220
          Top = 196
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 9
        end
        object EditVI_V_VIDEO_REG: TEdit
          Left = 220
          Top = 216
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 10
        end
        object EditVI_V_BURST_REG: TEdit
          Left = 220
          Top = 236
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 11
        end
        object EditVI_X_SCALE_REG: TEdit
          Left = 220
          Top = 256
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 12
        end
        object EditVI_Y_SCALE_REG: TEdit
          Left = 220
          Top = 276
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 13
        end
      end
    end
    object TabSheetSIRegs: TTabSheet
      Caption = 'SI Registers'
      ImageIndex = 7
      object PanelSIRegs: TPanel
        Left = 64
        Top = 171
        Width = 306
        Height = 70
        BevelInner = bvLowered
        BorderWidth = 3
        TabOrder = 0
        object LabelSI_DRAM_ADDR_REG: TLabel
          Left = 12
          Top = 20
          Width = 182
          Height = 13
          Caption = '0x04800000: SI_DRAM_ADDR_REG:'
        end
        object LabelSI_STATUS_REG: TLabel
          Left = 12
          Top = 40
          Width = 156
          Height = 13
          Caption = '0x04800018: SI_STATUS_REG:'
        end
        object EditSI_DRAM_ADDR_REG: TEdit
          Left = 224
          Top = 16
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 0
        end
        object EditSI_STATUS_REG: TEdit
          Left = 224
          Top = 36
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 1
        end
      end
    end
    object TabSheetPIRegs: TTabSheet
      Caption = 'PI Registers'
      ImageIndex = 1
      object PanelPIRegs: TPanel
        Left = 83
        Top = 73
        Width = 302
        Height = 309
        BevelInner = bvLowered
        BorderWidth = 3
        TabOrder = 0
        object LabelPI_BSD_DOM2_RLS_REG: TLabel
          Left = 12
          Top = 276
          Width = 199
          Height = 13
          Caption = '0x04600030: PI_BSD_DOM2_RLS_REG:'
        end
        object LabelPI_BSD_DOM2_PGS_REG: TLabel
          Left = 12
          Top = 256
          Width = 201
          Height = 13
          Caption = '0x0460002C: PI_BSD_DOM2_PGS_REG:'
        end
        object LabelPI_BSD_DOM2_PWD_REG: TLabel
          Left = 12
          Top = 236
          Width = 204
          Height = 13
          Caption = '0x04600028: PI_BSD_DOM2_PWD_REG:'
        end
        object LabelPI_BSD_DOM2_LAT_REG: TLabel
          Left = 12
          Top = 216
          Width = 198
          Height = 13
          Caption = '0x04600024: PI_BSD_DOM2_LAT_REG:'
        end
        object LabelPI_BSD_DOM1_RLS_REG: TLabel
          Left = 12
          Top = 188
          Width = 199
          Height = 13
          Caption = '0x04600020: PI_BSD_DOM1_RLS_REG:'
        end
        object LabelPI_BSD_DOM1_PGS_REG: TLabel
          Left = 12
          Top = 168
          Width = 201
          Height = 13
          Caption = '0x0460001C: PI_BSD_DOM1_PGS_REG:'
        end
        object LabelPI_BSD_DOM1_PWD_REG: TLabel
          Left = 12
          Top = 148
          Width = 204
          Height = 13
          Caption = '0x04600018: PI_BSD_DOM1_PWD_REG:'
        end
        object LabelPI_BSD_DOM1_LAT_REG: TLabel
          Left = 12
          Top = 128
          Width = 198
          Height = 13
          Caption = '0x04600014: PI_BSD_DOM1_LAT_REG:'
        end
        object LabelPI_STATUS_REG: TLabel
          Left = 12
          Top = 100
          Width = 156
          Height = 13
          Caption = '0x04600010: PI_STATUS_REG:'
        end
        object LabelPI_WR_LEN_REG: TLabel
          Left = 12
          Top = 80
          Width = 160
          Height = 13
          Caption = '0x0460000C: PI_WR_LEN_REG:'
        end
        object LabelPI_RD_LEN_REG: TLabel
          Left = 12
          Top = 60
          Width = 156
          Height = 13
          Caption = '0x04600008: PI_RD_LEN_REG:'
        end
        object LabelPI_CART_ADDR_REG: TLabel
          Left = 12
          Top = 40
          Width = 179
          Height = 13
          Caption = '0x04600004: PI_CART_ADDR_REG:'
        end
        object LabelPI_DRAM_ADDR_REG: TLabel
          Left = 12
          Top = 20
          Width = 182
          Height = 13
          Caption = '0x04600000: PI_DRAM_ADDR_REG:'
        end
        object EditPI_DRAM_ADDR_REG: TEdit
          Left = 224
          Top = 16
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 0
        end
        object EditPI_CART_ADDR_REG: TEdit
          Left = 224
          Top = 36
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 1
        end
        object EditPI_RD_LEN_REG: TEdit
          Left = 224
          Top = 56
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 2
        end
        object EditPI_WR_LEN_REG: TEdit
          Left = 224
          Top = 76
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 3
        end
        object EditPI_STATUS_REG: TEdit
          Left = 224
          Top = 96
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 4
        end
        object EditPI_BSD_DOM1_LAT_REG: TEdit
          Left = 224
          Top = 124
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 5
        end
        object EditPI_BSD_DOM1_PWD_REG: TEdit
          Left = 224
          Top = 144
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 6
        end
        object EditPI_BSD_DOM1_PGS_REG: TEdit
          Left = 224
          Top = 164
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 7
        end
        object EditPI_BSD_DOM1_RLS_REG: TEdit
          Left = 224
          Top = 184
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 8
        end
        object EditPI_BSD_DOM2_LAT_REG: TEdit
          Left = 224
          Top = 212
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 9
        end
        object EditPI_BSD_DOM2_PWD_REG: TEdit
          Left = 224
          Top = 232
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 10
        end
        object EditPI_BSD_DOM2_PGS_REG: TEdit
          Left = 224
          Top = 252
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 11
        end
        object EditPI_BSD_DOM2_RLS_REG: TEdit
          Left = 224
          Top = 272
          Width = 65
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 12
        end
      end
    end
    object TabSheetRSPGP: TTabSheet
      Caption = 'RSP GP'
      ImageIndex = 5
      OnResize = TabSheetRSPGPResize
      object PanelRSPGP: TPanel
        Left = 62
        Top = 49
        Width = 309
        Height = 358
        BevelInner = bvLowered
        BorderWidth = 3
        TabOrder = 0
        object LabelRSPGP0: TLabel
          Left = 12
          Top = 20
          Width = 55
          Height = 13
          Caption = 'Reg 0 - R0:'
        end
        object LabelRSPGP1: TLabel
          Left = 12
          Top = 40
          Width = 55
          Height = 13
          Caption = 'Reg 1 - AT:'
        end
        object LabelRSPGP2: TLabel
          Left = 12
          Top = 60
          Width = 54
          Height = 13
          Caption = 'Reg 2 - V0:'
        end
        object LabelRSPGP3: TLabel
          Left = 12
          Top = 80
          Width = 54
          Height = 13
          Caption = 'Reg 3 - V1:'
        end
        object LabelRSPGP4: TLabel
          Left = 12
          Top = 100
          Width = 54
          Height = 13
          Caption = 'Reg 4 - A0:'
        end
        object LabelRSPGP5: TLabel
          Left = 12
          Top = 120
          Width = 54
          Height = 13
          Caption = 'Reg 5 - A1:'
        end
        object LabelRSPGP6: TLabel
          Left = 12
          Top = 140
          Width = 54
          Height = 13
          Caption = 'Reg 6 - A2:'
        end
        object LabelRSPGP7: TLabel
          Left = 12
          Top = 160
          Width = 54
          Height = 13
          Caption = 'Reg 7 - A3:'
        end
        object LabelRSPGP8: TLabel
          Left = 12
          Top = 180
          Width = 54
          Height = 13
          Caption = 'Reg 8 - T0:'
        end
        object LabelRSPGP9: TLabel
          Left = 12
          Top = 200
          Width = 54
          Height = 13
          Caption = 'Reg 9 - T1:'
        end
        object LabelRSPGP10: TLabel
          Left = 12
          Top = 220
          Width = 60
          Height = 13
          Caption = 'Reg 10 - T2:'
        end
        object LabelRSPGP11: TLabel
          Left = 12
          Top = 240
          Width = 60
          Height = 13
          Caption = 'Reg 11 - T3:'
        end
        object LabelRSPGP12: TLabel
          Left = 12
          Top = 260
          Width = 60
          Height = 13
          Caption = 'Reg 12 - T4:'
        end
        object LabelRSPGP13: TLabel
          Left = 12
          Top = 280
          Width = 60
          Height = 13
          Caption = 'Reg 13 - T5:'
        end
        object LabelRSPGP14: TLabel
          Left = 12
          Top = 300
          Width = 60
          Height = 13
          Caption = 'Reg 14 - T6:'
        end
        object LabelRSPGP15: TLabel
          Left = 12
          Top = 320
          Width = 60
          Height = 13
          Caption = 'Reg 15 - T7:'
        end
        object LabelRSPGP16: TLabel
          Left = 160
          Top = 20
          Width = 60
          Height = 13
          Caption = 'Reg 16 - S0:'
        end
        object LabelRSPGP17: TLabel
          Left = 160
          Top = 40
          Width = 60
          Height = 13
          Caption = 'Reg 17 - S1:'
        end
        object LabelRSPGP18: TLabel
          Left = 160
          Top = 60
          Width = 60
          Height = 13
          Caption = 'Reg 18 - S2:'
        end
        object LabelRSPGP19: TLabel
          Left = 160
          Top = 80
          Width = 60
          Height = 13
          Caption = 'Reg 19 - S3:'
        end
        object LabelRSPGP20: TLabel
          Left = 160
          Top = 100
          Width = 60
          Height = 13
          Caption = 'Reg 20 - S4:'
        end
        object LabelRSPGP21: TLabel
          Left = 160
          Top = 120
          Width = 60
          Height = 13
          Caption = 'Reg 21 - S5:'
        end
        object LabelRSPGP22: TLabel
          Left = 160
          Top = 140
          Width = 60
          Height = 13
          Caption = 'Reg 22 - S6:'
        end
        object LabelRSPGP23: TLabel
          Left = 160
          Top = 160
          Width = 60
          Height = 13
          Caption = 'Reg 23 - S7:'
        end
        object LabelRSPGP24: TLabel
          Left = 160
          Top = 180
          Width = 60
          Height = 13
          Caption = 'Reg 24 - T8:'
        end
        object LabelRSPGP25: TLabel
          Left = 160
          Top = 200
          Width = 60
          Height = 13
          Caption = 'Reg 25 - T9:'
        end
        object LabelRSPGP26: TLabel
          Left = 160
          Top = 220
          Width = 60
          Height = 13
          Caption = 'Reg 26 - K0:'
        end
        object LabelRSPGP27: TLabel
          Left = 160
          Top = 240
          Width = 60
          Height = 13
          Caption = 'Reg 27 - K1:'
        end
        object LabelRSPGP28: TLabel
          Left = 160
          Top = 260
          Width = 62
          Height = 13
          Caption = 'Reg 28 - GP:'
        end
        object LabelRSPGP29: TLabel
          Left = 160
          Top = 280
          Width = 61
          Height = 13
          Caption = 'Reg 29 - SP:'
        end
        object LabelRSPGP30: TLabel
          Left = 160
          Top = 300
          Width = 60
          Height = 13
          Caption = 'Reg 30 - S8:'
        end
        object LabelRSPGP31: TLabel
          Left = 160
          Top = 320
          Width = 62
          Height = 13
          Caption = 'Reg 31 - RA:'
        end
        object EditRSPGP16: TEdit
          Left = 228
          Top = 16
          Width = 66
          Height = 21
          TabOrder = 16
          OnChange = EditRSPGPChange
        end
        object EditRSPGP17: TEdit
          Left = 228
          Top = 36
          Width = 66
          Height = 21
          TabOrder = 17
          OnChange = EditRSPGPChange
        end
        object EditRSPGP0: TEdit
          Left = 80
          Top = 16
          Width = 66
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 0
        end
        object EditRSPGP1: TEdit
          Left = 80
          Top = 36
          Width = 66
          Height = 21
          TabOrder = 1
          OnChange = EditRSPGPChange
        end
        object EditRSPGP2: TEdit
          Left = 80
          Top = 56
          Width = 66
          Height = 21
          TabOrder = 2
          OnChange = EditRSPGPChange
        end
        object EditRSPGP3: TEdit
          Left = 80
          Top = 76
          Width = 66
          Height = 21
          TabOrder = 3
          OnChange = EditRSPGPChange
        end
        object EditRSPGP4: TEdit
          Left = 80
          Top = 96
          Width = 66
          Height = 21
          TabOrder = 4
          OnChange = EditRSPGPChange
        end
        object EditRSPGP5: TEdit
          Left = 80
          Top = 116
          Width = 66
          Height = 21
          TabOrder = 5
          OnChange = EditRSPGPChange
        end
        object EditRSPGP6: TEdit
          Left = 80
          Top = 136
          Width = 66
          Height = 21
          TabOrder = 6
          OnChange = EditRSPGPChange
        end
        object EditRSPGP7: TEdit
          Left = 80
          Top = 156
          Width = 66
          Height = 21
          TabOrder = 7
          OnChange = EditRSPGPChange
        end
        object EditRSPGP8: TEdit
          Left = 80
          Top = 176
          Width = 66
          Height = 21
          TabOrder = 8
          OnChange = EditRSPGPChange
        end
        object EditRSPGP9: TEdit
          Left = 80
          Top = 196
          Width = 66
          Height = 21
          TabOrder = 9
          OnChange = EditRSPGPChange
        end
        object EditRSPGP10: TEdit
          Left = 80
          Top = 216
          Width = 66
          Height = 21
          TabOrder = 10
          OnChange = EditRSPGPChange
        end
        object EditRSPGP11: TEdit
          Left = 80
          Top = 236
          Width = 66
          Height = 21
          TabOrder = 11
          OnChange = EditRSPGPChange
        end
        object EditRSPGP12: TEdit
          Left = 80
          Top = 256
          Width = 66
          Height = 21
          TabOrder = 12
          OnChange = EditRSPGPChange
        end
        object EditRSPGP13: TEdit
          Left = 80
          Top = 276
          Width = 66
          Height = 21
          TabOrder = 13
          OnChange = EditRSPGPChange
        end
        object EditRSPGP14: TEdit
          Left = 80
          Top = 296
          Width = 66
          Height = 21
          TabOrder = 14
          OnChange = EditRSPGPChange
        end
        object EditRSPGP15: TEdit
          Left = 80
          Top = 316
          Width = 66
          Height = 21
          TabOrder = 15
          OnChange = EditRSPGPChange
        end
        object EditRSPGP18: TEdit
          Left = 228
          Top = 56
          Width = 66
          Height = 21
          TabOrder = 18
          OnChange = EditRSPGPChange
        end
        object EditRSPGP19: TEdit
          Left = 228
          Top = 76
          Width = 66
          Height = 21
          TabOrder = 19
          OnChange = EditRSPGPChange
        end
        object EditRSPGP20: TEdit
          Left = 228
          Top = 96
          Width = 66
          Height = 21
          TabOrder = 20
          OnChange = EditRSPGPChange
        end
        object EditRSPGP21: TEdit
          Left = 228
          Top = 116
          Width = 66
          Height = 21
          TabOrder = 21
          OnChange = EditRSPGPChange
        end
        object EditRSPGP22: TEdit
          Left = 228
          Top = 136
          Width = 66
          Height = 21
          TabOrder = 22
          OnChange = EditRSPGPChange
        end
        object EditRSPGP23: TEdit
          Left = 228
          Top = 156
          Width = 66
          Height = 21
          TabOrder = 23
          OnChange = EditRSPGPChange
        end
        object EditRSPGP24: TEdit
          Left = 228
          Top = 176
          Width = 66
          Height = 21
          TabOrder = 24
          OnChange = EditRSPGPChange
        end
        object EditRSPGP25: TEdit
          Left = 228
          Top = 196
          Width = 66
          Height = 21
          TabOrder = 25
          OnChange = EditRSPGPChange
        end
        object EditRSPGP26: TEdit
          Left = 228
          Top = 216
          Width = 66
          Height = 21
          TabOrder = 26
          OnChange = EditRSPGPChange
        end
        object EditRSPGP27: TEdit
          Left = 228
          Top = 236
          Width = 66
          Height = 21
          TabOrder = 27
          OnChange = EditRSPGPChange
        end
        object EditRSPGP28: TEdit
          Left = 228
          Top = 256
          Width = 66
          Height = 21
          TabOrder = 28
          OnChange = EditRSPGPChange
        end
        object EditRSPGP29: TEdit
          Left = 228
          Top = 276
          Width = 66
          Height = 21
          TabOrder = 29
          OnChange = EditRSPGPChange
        end
        object EditRSPGP30: TEdit
          Left = 228
          Top = 296
          Width = 66
          Height = 21
          TabOrder = 30
          OnChange = EditRSPGPChange
        end
        object EditRSPGP31: TEdit
          Left = 228
          Top = 316
          Width = 66
          Height = 21
          TabOrder = 31
          OnChange = EditRSPGPChange
        end
      end
    end
    object TabSheetRSPVector: TTabSheet
      Caption = 'RSP Vector'
      ImageIndex = 8
      object ScrollBoxVectorRegs: TScrollBox
        Left = 0
        Top = 0
        Width = 329
        Height = 452
        VertScrollBar.Tracking = True
        Align = alLeft
        TabOrder = 0
        object LabelRSPv15: TLabel
          Left = 4
          Top = 308
          Width = 27
          Height = 13
          BiDiMode = bdLeftToRight
          Caption = '$v15:'
          ParentBiDiMode = False
        end
        object LabelRSPv14: TLabel
          Left = 4
          Top = 288
          Width = 27
          Height = 13
          Caption = '$v14:'
        end
        object LabelRSPv13: TLabel
          Left = 4
          Top = 268
          Width = 27
          Height = 13
          Caption = '$v13:'
        end
        object LabelRSPv12: TLabel
          Left = 4
          Top = 248
          Width = 27
          Height = 13
          Caption = '$v12:'
        end
        object LabelRSPv11: TLabel
          Left = 4
          Top = 228
          Width = 27
          Height = 13
          Caption = '$v11:'
        end
        object LabelRSPv10: TLabel
          Left = 4
          Top = 208
          Width = 27
          Height = 13
          Caption = '$v10:'
        end
        object LabelRSPv9: TLabel
          Left = 4
          Top = 188
          Width = 21
          Height = 13
          Caption = '$v9:'
        end
        object LabelRSPv8: TLabel
          Left = 4
          Top = 168
          Width = 21
          Height = 13
          Caption = '$v8:'
        end
        object LabelRSPv7: TLabel
          Left = 4
          Top = 148
          Width = 21
          Height = 13
          Caption = '$v7:'
        end
        object LabelRSPv6: TLabel
          Left = 4
          Top = 128
          Width = 21
          Height = 13
          Caption = '$v6:'
        end
        object LabelRSPv5: TLabel
          Left = 4
          Top = 108
          Width = 21
          Height = 13
          Caption = '$v5:'
        end
        object LabelRSPv4: TLabel
          Left = 4
          Top = 88
          Width = 21
          Height = 13
          Caption = '$v4:'
        end
        object LabelRSPv3: TLabel
          Left = 4
          Top = 68
          Width = 21
          Height = 13
          Caption = '$v3:'
        end
        object LabelRSPv2: TLabel
          Left = 4
          Top = 48
          Width = 21
          Height = 13
          Caption = '$v2:'
        end
        object LabelRSPv1: TLabel
          Left = 4
          Top = 28
          Width = 21
          Height = 13
          Caption = '$v1:'
        end
        object LabelRSPv0: TLabel
          Left = 4
          Top = 8
          Width = 21
          Height = 13
          Caption = '$v0:'
        end
        object LabelRSPv16: TLabel
          Left = 4
          Top = 328
          Width = 27
          Height = 13
          Caption = '$v16:'
        end
        object LabelRSPv31: TLabel
          Left = 4
          Top = 628
          Width = 27
          Height = 13
          Caption = '$v31:'
        end
        object LabelRSPv30: TLabel
          Left = 4
          Top = 608
          Width = 27
          Height = 13
          Caption = '$v30:'
        end
        object LabelRSPv29: TLabel
          Left = 4
          Top = 588
          Width = 27
          Height = 13
          Caption = '$v29:'
        end
        object LabelRSPv28: TLabel
          Left = 4
          Top = 568
          Width = 27
          Height = 13
          Caption = '$v28:'
        end
        object LabelRSPv27: TLabel
          Left = 4
          Top = 548
          Width = 27
          Height = 13
          Caption = '$v27:'
        end
        object LabelRSPv26: TLabel
          Left = 4
          Top = 528
          Width = 27
          Height = 13
          Caption = '$v26:'
        end
        object LabelRSPv25: TLabel
          Left = 4
          Top = 508
          Width = 27
          Height = 13
          Caption = '$v25:'
        end
        object LabelRSPv24: TLabel
          Left = 4
          Top = 488
          Width = 27
          Height = 13
          Caption = '$v24:'
        end
        object LabelRSPv23: TLabel
          Left = 4
          Top = 468
          Width = 27
          Height = 13
          Caption = '$v23:'
        end
        object LabelRSPv22: TLabel
          Left = 4
          Top = 448
          Width = 27
          Height = 13
          Caption = '$v22:'
        end
        object LabelRSPv21: TLabel
          Left = 4
          Top = 428
          Width = 27
          Height = 13
          Caption = '$v21:'
        end
        object LabelRSPv20: TLabel
          Left = 4
          Top = 408
          Width = 27
          Height = 13
          Caption = '$v20:'
        end
        object LabelRSPv19: TLabel
          Left = 4
          Top = 388
          Width = 27
          Height = 13
          Caption = '$v19:'
        end
        object LabelRSPv18: TLabel
          Left = 4
          Top = 368
          Width = 27
          Height = 13
          Caption = '$v18:'
        end
        object LabelRSPv17: TLabel
          Left = 4
          Top = 348
          Width = 27
          Height = 13
          Caption = '$v17:'
        end
        object EditRSPv0: TEdit
          Left = 44
          Top = 4
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 0
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv1: TEdit
          Left = 44
          Top = 24
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 1
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv2: TEdit
          Left = 44
          Top = 44
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 2
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv3: TEdit
          Left = 44
          Top = 64
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 3
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv4: TEdit
          Left = 44
          Top = 84
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 4
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv5: TEdit
          Left = 44
          Top = 104
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 5
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv6: TEdit
          Left = 44
          Top = 124
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 6
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv7: TEdit
          Left = 44
          Top = 144
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 7
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv8: TEdit
          Left = 44
          Top = 164
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 8
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv9: TEdit
          Left = 44
          Top = 184
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 9
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv10: TEdit
          Left = 44
          Top = 204
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 10
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv11: TEdit
          Left = 44
          Top = 224
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 11
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv12: TEdit
          Left = 44
          Top = 244
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 12
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv13: TEdit
          Left = 44
          Top = 264
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 13
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv14: TEdit
          Left = 44
          Top = 284
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 14
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv15: TEdit
          Left = 44
          Top = 304
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 15
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv16: TEdit
          Left = 44
          Top = 324
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 16
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv17: TEdit
          Left = 44
          Top = 344
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 17
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv18: TEdit
          Left = 44
          Top = 364
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 18
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv19: TEdit
          Left = 44
          Top = 384
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 19
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv20: TEdit
          Left = 44
          Top = 404
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 20
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv21: TEdit
          Left = 44
          Top = 424
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 21
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv22: TEdit
          Left = 44
          Top = 444
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 22
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv23: TEdit
          Left = 44
          Top = 464
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 23
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv24: TEdit
          Left = 44
          Top = 484
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 24
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv25: TEdit
          Left = 44
          Top = 504
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 25
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv26: TEdit
          Left = 44
          Top = 524
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 26
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv27: TEdit
          Left = 44
          Top = 544
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 27
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv28: TEdit
          Left = 44
          Top = 564
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 28
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv29: TEdit
          Left = 44
          Top = 584
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 29
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv30: TEdit
          Left = 44
          Top = 604
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 30
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
        object EditRSPv31: TEdit
          Left = 44
          Top = 624
          Width = 257
          Height = 21
          Color = clBtnFace
          ReadOnly = True
          TabOrder = 31
          Text = 'EEEE EEEE EEEE EEEE - EEEE EEEE EEEE EEEE'
        end
      end
    end
    object TabSheetRSPHidden: TTabSheet
      Caption = 'RSP Hidden'
      ImageIndex = 9
      object GroupBoxRSPAccum: TGroupBox
        Left = 109
        Top = 37
        Width = 215
        Height = 221
        Caption = 'RSP Accumulators'
        TabOrder = 0
        object LabelAccum0: TLabel
          Left = 16
          Top = 24
          Width = 45
          Height = 13
          Caption = 'Accum 0:'
        end
        object LabelAccum1: TLabel
          Left = 16
          Top = 48
          Width = 45
          Height = 13
          Caption = 'Accum 1:'
        end
        object LabelAccum2: TLabel
          Left = 16
          Top = 72
          Width = 45
          Height = 13
          Caption = 'Accum 2:'
        end
        object LabelAccum3: TLabel
          Left = 16
          Top = 96
          Width = 45
          Height = 13
          Caption = 'Accum 3:'
        end
        object LabelAccum4: TLabel
          Left = 16
          Top = 120
          Width = 45
          Height = 13
          Caption = 'Accum 4:'
        end
        object LabelAccum5: TLabel
          Left = 16
          Top = 144
          Width = 45
          Height = 13
          Caption = 'Accum 5:'
        end
        object LabelAccum6: TLabel
          Left = 16
          Top = 168
          Width = 45
          Height = 13
          Caption = 'Accum 6:'
        end
        object LabelAccum7: TLabel
          Left = 16
          Top = 192
          Width = 45
          Height = 13
          Caption = 'Accum 7:'
        end
        object EditAccum0: TEdit
          Left = 68
          Top = 20
          Width = 137
          Height = 21
          Color = clBtnFace
          TabOrder = 0
        end
        object EditAccum1: TEdit
          Left = 68
          Top = 44
          Width = 137
          Height = 21
          Color = clBtnFace
          TabOrder = 1
        end
        object EditAccum2: TEdit
          Left = 68
          Top = 68
          Width = 137
          Height = 21
          Color = clBtnFace
          TabOrder = 2
        end
        object EditAccum3: TEdit
          Left = 68
          Top = 92
          Width = 137
          Height = 21
          Color = clBtnFace
          TabOrder = 3
        end
        object EditAccum4: TEdit
          Left = 68
          Top = 116
          Width = 137
          Height = 21
          Color = clBtnFace
          TabOrder = 4
        end
        object EditAccum5: TEdit
          Left = 68
          Top = 140
          Width = 137
          Height = 21
          Color = clBtnFace
          TabOrder = 5
        end
        object EditAccum6: TEdit
          Left = 68
          Top = 164
          Width = 137
          Height = 21
          Color = clBtnFace
          TabOrder = 6
        end
        object EditAccum7: TEdit
          Left = 68
          Top = 188
          Width = 137
          Height = 21
          Color = clBtnFace
          TabOrder = 7
        end
      end
      object GroupBoxRSPFlags: TGroupBox
        Left = 109
        Top = 265
        Width = 215
        Height = 100
        Caption = 'RSP Flags'
        TabOrder = 1
        object LabelRSPFlagsVCE: TLabel
          Left = 16
          Top = 72
          Width = 62
          Height = 13
          Caption = 'Reg 2 - VCE:'
        end
        object LabelRSPFlagsVCO: TLabel
          Left = 16
          Top = 24
          Width = 63
          Height = 13
          Caption = 'Reg 0 - VCO:'
        end
        object LabelRSPFlagsVCC: TLabel
          Left = 16
          Top = 48
          Width = 62
          Height = 13
          Caption = 'Reg 1 - VCC:'
        end
        object EditRSPFlagsVCE: TEdit
          Left = 140
          Top = 68
          Width = 61
          Height = 21
          Color = clBtnFace
          TabOrder = 0
        end
        object EditRSPFlagsVCO: TEdit
          Left = 140
          Top = 20
          Width = 61
          Height = 21
          Color = clBtnFace
          TabOrder = 1
        end
        object EditRSPFlagsVCC: TEdit
          Left = 140
          Top = 44
          Width = 61
          Height = 21
          Color = clBtnFace
          TabOrder = 2
        end
      end
    end
  end
end
