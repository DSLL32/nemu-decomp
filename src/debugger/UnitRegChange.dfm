object FormDebugRegisterChange: TFormDebugRegisterChange
  Left = 394
  Top = 109
  Width = 365
  Height = 277
  Caption = 'Break on register access...'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object PageControlWhichAccess: TPageControl
    Left = 0
    Top = 0
    Width = 357
    Height = 250
    ActivePage = TabSheetRead
    Align = alClient
    TabOrder = 0
    object TabSheetRead: TTabSheet
      Caption = 'Read'
      object CheckListBoxRead: TCheckListBox
        Left = 0
        Top = 0
        Width = 349
        Height = 222
        OnClickCheck = CheckListBoxClickCheck
        Align = alClient
        Columns = 3
        ItemHeight = 13
        Items.Strings = (
          'Reg 0 - R0'
          'Reg 1 - AT'
          'Reg 2 - V0'
          'Reg 3 - V1'
          'Reg 4 - A0'
          'Reg 5 - A1'
          'Reg 6 - A2'
          'Reg 7 - A3'
          'Reg 8 - T0'
          'Reg 9 - T1'
          'Reg 10 - T2'
          'Reg 11 - T3'
          'Reg 12 - T4'
          'Reg 13 - T5'
          'Reg 14 - T6'
          'Reg 15 - T7'
          'Reg 16 - S0'
          'Reg 17 - S1'
          'Reg 18 - S2'
          'Reg 19 - S3'
          'Reg 20 - S4'
          'Reg 21 - S5'
          'Reg 22 - S6'
          'Reg 23 - S7'
          'Reg 24 - T8'
          'Reg 25 - T9'
          'Reg 26 - K0'
          'Reg 27 - K1'
          'Reg 28 - GP'
          'Reg 29 - SP'
          'Reg 30 - S8'
          'Reg 31 - RA')
        TabOrder = 0
      end
    end
    object TabSheet1: TTabSheet
      Caption = 'Write'
      ImageIndex = 1
      object CheckListBoxWrite: TCheckListBox
        Left = 0
        Top = 0
        Width = 349
        Height = 222
        OnClickCheck = CheckListBoxClickCheck
        Align = alClient
        Columns = 3
        ItemHeight = 13
        Items.Strings = (
          'Reg 0 - R0'
          'Reg 1 - AT'
          'Reg 2 - V0'
          'Reg 3 - V1'
          'Reg 4 - A0'
          'Reg 5 - A1'
          'Reg 6 - A2'
          'Reg 7 - A3'
          'Reg 8 - T0'
          'Reg 9 - T1'
          'Reg 10 - T2'
          'Reg 11 - T3'
          'Reg 12 - T4'
          'Reg 13 - T5'
          'Reg 14 - T6'
          'Reg 15 - T7'
          'Reg 16 - S0'
          'Reg 17 - S1'
          'Reg 18 - S2'
          'Reg 19 - S3'
          'Reg 20 - S4'
          'Reg 21 - S5'
          'Reg 22 - S6'
          'Reg 23 - S7'
          'Reg 24 - T8'
          'Reg 25 - T9'
          'Reg 26 - K0'
          'Reg 27 - K1'
          'Reg 28 - GP'
          'Reg 29 - SP'
          'Reg 30 - S8'
          'Reg 31 - RA')
        TabOrder = 0
      end
    end
  end
end
