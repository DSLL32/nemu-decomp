object FormStats: TFormStats
  Left = 194
  Top = 107
  Width = 435
  Height = 338
  ActiveControl = CheckBox1
  Caption = 'Statistics'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  PixelsPerInch = 96
  TextHeight = 13
  object PageControl: TPageControl
    Left = 0
    Top = 0
    Width = 427
    Height = 304
    ActivePage = TabSheetBlockStats
    Align = alClient
    TabIndex = 0
    TabOrder = 0
    object TabSheetBlockStats: TTabSheet
      Caption = 'Block Statistics'
      ImageIndex = 2
      object Panel2: TPanel
        Left = 0
        Top = 0
        Width = 419
        Height = 33
        Align = alTop
        TabOrder = 0
        object CheckBox1: TCheckBox
          Left = 8
          Top = 8
          Width = 145
          Height = 17
          Caption = 'Compile Block Statistics'
          TabOrder = 0
          OnClick = CheckBox1Click
        end
        object Button1: TButton
          Left = 156
          Top = 4
          Width = 75
          Height = 25
          Caption = 'Reset'
          TabOrder = 1
          OnClick = Button1Click
        end
        object ButtonBProfRead: TButton
          Left = 236
          Top = 4
          Width = 75
          Height = 25
          Caption = 'Read'
          TabOrder = 2
          OnClick = ButtonBProfReadClick
        end
      end
      object ListView: TListView
        Left = 0
        Top = 33
        Width = 419
        Height = 243
        Align = alClient
        Columns = <
          item
            AutoSize = True
            Caption = 'PC'
          end
          item
            AutoSize = True
            Caption = 'Times Called'
          end
          item
            AutoSize = True
            Caption = 'Time spent'
          end
          item
            AutoSize = True
            Caption = '%'
          end>
        RowSelect = True
        SortType = stData
        TabOrder = 1
        ViewStyle = vsReport
        OnColumnClick = ListViewColumnClick
        OnCompare = ListViewCompare
        OnDblClick = ListViewDblClick
      end
    end
    object TabSheetOpcode: TTabSheet
      Caption = 'Opcode Statistics'
      ImageIndex = 1
      object Panel1: TPanel
        Left = 0
        Top = 0
        Width = 419
        Height = 49
        Align = alTop
        Caption = 'Panel1'
        TabOrder = 0
        object CheckBoxToggleOpcodeStats: TCheckBox
          Left = 8
          Top = 8
          Width = 145
          Height = 17
          Caption = 'Compile Opcode Statistics'
          TabOrder = 0
          OnClick = CheckBoxToggleOpcodeStatsClick
        end
        object ButtonResetOpcodeStats: TButton
          Left = 156
          Top = 4
          Width = 75
          Height = 25
          Caption = 'Reset'
          TabOrder = 1
          OnClick = ButtonResetOpcodeStatsClick
        end
        object ButtonReadOpcodeStats: TButton
          Left = 236
          Top = 4
          Width = 75
          Height = 25
          Caption = 'Read'
          TabOrder = 2
          OnClick = ButtonReadOpcodeStatsClick
        end
        object CheckBoxDivideVI: TCheckBox
          Left = 8
          Top = 24
          Width = 97
          Height = 17
          Caption = 'Divide VI Count'
          Checked = True
          State = cbChecked
          TabOrder = 3
        end
      end
      object MemoOpcodeStats: TMemo
        Left = 0
        Top = 49
        Width = 419
        Height = 234
        Align = alClient
        ScrollBars = ssVertical
        TabOrder = 1
      end
    end
    object TabSheetRegStats: TTabSheet
      Caption = 'Register Statistics'
      object Memo: TMemo
        Left = 0
        Top = 33
        Width = 419
        Height = 250
        Align = alClient
        TabOrder = 0
      end
      object Panel: TPanel
        Left = 0
        Top = 0
        Width = 419
        Height = 33
        Align = alTop
        TabOrder = 1
        object CheckBoxRegisterStat: TCheckBox
          Left = 8
          Top = 8
          Width = 145
          Height = 17
          Caption = 'Compile Register Statistics'
          TabOrder = 0
          OnClick = CheckBoxRegisterStatClick
        end
        object ButtonReset: TButton
          Left = 156
          Top = 4
          Width = 75
          Height = 25
          Caption = 'Reset'
          TabOrder = 1
          OnClick = ButtonResetClick
        end
        object ButtonRead: TButton
          Left = 236
          Top = 4
          Width = 75
          Height = 25
          Caption = 'Read'
          TabOrder = 2
          OnClick = ButtonReadClick
        end
      end
    end
  end
  object ThemeManager: TThemeManager
    Left = 332
    Top = 12
  end
end
