object FormSearch: TFormSearch
  Left = 190
  Top = 101
  Width = 396
  Height = 353
  Caption = 'Search'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object GroupBoxWhere: TGroupBox
    Left = 0
    Top = 0
    Width = 388
    Height = 81
    Align = alTop
    Caption = 'Where'
    TabOrder = 0
    object LabelSearchWhere: TLabel
      Left = 8
      Top = 16
      Width = 146
      Height = 13
      Caption = 'Search in (physical addresses):'
    end
    object ButtonRDRAM: TButton
      Left = 8
      Top = 44
      Width = 75
      Height = 25
      Caption = 'RDRAM'
      TabOrder = 0
      OnClick = ButtonRDRAMClick
    end
    object ButtonRom: TButton
      Left = 88
      Top = 44
      Width = 75
      Height = 25
      Caption = 'ROM'
      TabOrder = 1
      OnClick = ButtonRomClick
    end
    object EditSearchInStart: TEdit
      Left = 156
      Top = 12
      Width = 73
      Height = 21
      TabOrder = 2
      OnChange = EditValuesChange
    end
    object EditSearchInEnd: TEdit
      Left = 236
      Top = 12
      Width = 73
      Height = 21
      TabOrder = 3
      OnChange = EditValuesChange
    end
    object ButtonSPMEM: TButton
      Left = 168
      Top = 44
      Width = 75
      Height = 25
      Caption = 'SPMEM'
      TabOrder = 4
      OnClick = ButtonSPMEMClick
    end
  end
  object GroupBoxWhat: TGroupBox
    Left = 0
    Top = 81
    Width = 388
    Height = 88
    Align = alTop
    Caption = 'What'
    TabOrder = 1
    object RadioButtonSearchValue: TRadioButton
      Left = 8
      Top = 16
      Width = 89
      Height = 17
      Caption = 'Search value:'
      Checked = True
      TabOrder = 0
      TabStop = True
      OnClick = RadioButtonSearchClick
    end
    object EditSearchValue: TEdit
      Left = 96
      Top = 12
      Width = 121
      Height = 21
      TabOrder = 1
      Text = '0'
      OnChange = EditValuesChange
    end
    object CheckBoxHex: TCheckBox
      Left = 220
      Top = 16
      Width = 41
      Height = 17
      Caption = 'hex'
      Checked = True
      State = cbChecked
      TabOrder = 2
      OnClick = EditValuesChange
    end
    object ComboBoxBits: TComboBox
      Left = 260
      Top = 14
      Width = 113
      Height = 21
      Style = csDropDownList
      ItemHeight = 13
      TabOrder = 3
      Items.Strings = (
        '8 bits'
        '16 bits (aligned)'
        '32 bits (aligned)')
    end
    object RadioButtonSearchText: TRadioButton
      Left = 8
      Top = 40
      Width = 81
      Height = 17
      Caption = 'Search text:'
      TabOrder = 4
      OnClick = RadioButtonSearchClick
    end
    object EditSearchText: TEdit
      Left = 96
      Top = 36
      Width = 121
      Height = 21
      TabOrder = 5
      OnChange = EditValuesChange
    end
    object CheckBoxCaseSensitive: TCheckBox
      Left = 220
      Top = 44
      Width = 97
      Height = 13
      Caption = 'Case-sensitive'
      TabOrder = 6
      OnClick = EditValuesChange
    end
    object RadioButtonJALTo: TRadioButton
      Left = 8
      Top = 64
      Width = 81
      Height = 17
      Caption = 'JAL to:'
      TabOrder = 7
      OnClick = RadioButtonSearchClick
    end
    object EditJAL: TEdit
      Left = 96
      Top = 60
      Width = 121
      Height = 21
      TabOrder = 8
      OnChange = EditValuesChange
    end
    object CheckBoxFast: TCheckBox
      Left = 316
      Top = 44
      Width = 45
      Height = 13
      Caption = 'Fast'
      Checked = True
      State = cbChecked
      TabOrder = 9
    end
  end
  object GroupBoxResults: TGroupBox
    Left = 0
    Top = 169
    Width = 388
    Height = 150
    Align = alClient
    Caption = 'Results'
    TabOrder = 2
    object PanelResultsTop: TPanel
      Left = 2
      Top = 15
      Width = 384
      Height = 50
      Align = alTop
      BevelOuter = bvNone
      TabOrder = 0
      object LabelMax: TLabel
        Left = 164
        Top = 4
        Width = 130
        Height = 13
        Caption = 'Maximum number of results:'
      end
      object ButtonSearch: TButton
        Left = 4
        Top = 0
        Width = 75
        Height = 25
        Caption = 'Search'
        Enabled = False
        TabOrder = 0
        OnClick = ButtonSearchClick
      end
      object ButtonClearResults: TButton
        Left = 84
        Top = 0
        Width = 75
        Height = 25
        Caption = 'Clear results'
        TabOrder = 1
        OnClick = ButtonClearResultsClick
      end
      object CheckBoxDblClickCommands: TCheckBox
        Left = 4
        Top = 28
        Width = 189
        Height = 17
        Caption = 'Doubleclick --> Commands window'
        TabOrder = 2
      end
    end
    object ListBoxSearchResults: TListBox
      Left = 2
      Top = 65
      Width = 384
      Height = 83
      Align = alClient
      ItemHeight = 13
      TabOrder = 1
      OnDblClick = ListBoxSearchResultsDblClick
    end
    object EditMaxCount: TEdit
      Left = 300
      Top = 16
      Width = 81
      Height = 21
      TabOrder = 2
      Text = '1000'
      OnChange = EditValuesChange
    end
  end
end
