object FormDump: TFormDump
  Left = 294
  Top = 103
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'Dump memory'
  ClientHeight = 231
  ClientWidth = 306
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnClose = FormClose
  PixelsPerInch = 96
  TextHeight = 13
  object GroupBoxRange: TGroupBox
    Left = 12
    Top = 8
    Width = 285
    Height = 97
    Caption = '&Range'
    TabOrder = 0
    object LabelStart: TLabel
      Left = 12
      Top = 20
      Width = 25
      Height = 13
      Caption = '&Start:'
      FocusControl = EditStart
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clRed
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object LabelEnd: TLabel
      Left = 12
      Top = 44
      Width = 22
      Height = 13
      Caption = '&End:'
      FocusControl = EditEnd
      Font.Charset = DEFAULT_CHARSET
      Font.Color = clRed
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = []
      ParentFont = False
    end
    object EditStart: TEdit
      Left = 60
      Top = 16
      Width = 109
      Height = 21
      TabOrder = 0
      OnChange = EditStartChange
    end
    object EditEnd: TEdit
      Left = 60
      Top = 40
      Width = 109
      Height = 21
      TabOrder = 1
      OnChange = EditEndChange
    end
    object CheckBoxAlternatePC: TCheckBox
      Left = 12
      Top = 68
      Width = 105
      Height = 17
      Caption = 'Use alternate PC:'
      TabOrder = 2
      OnClick = CheckBoxAlternatePCClick
    end
    object EditAlternateStartPC: TEdit
      Left = 116
      Top = 64
      Width = 121
      Height = 21
      Enabled = False
      TabOrder = 3
    end
  end
  object GroupBoxFile: TGroupBox
    Left = 12
    Top = 112
    Width = 285
    Height = 69
    Caption = '&File'
    TabOrder = 1
    object LabelFilename: TLabel
      Left = 8
      Top = 20
      Width = 45
      Height = 13
      Caption = 'File&name:'
      FocusControl = EditFilename
    end
    object LabelFormat: TLabel
      Left = 8
      Top = 44
      Width = 35
      Height = 13
      Caption = 'Format:'
    end
    object EditFilename: TEdit
      Left = 60
      Top = 16
      Width = 193
      Height = 21
      TabOrder = 0
    end
    object ButtonBrowse: TButton
      Left = 256
      Top = 16
      Width = 19
      Height = 21
      Caption = '...'
      TabOrder = 1
      OnClick = ButtonBrowseClick
    end
    object ComboBoxEndian: TComboBox
      Left = 60
      Top = 40
      Width = 217
      Height = 21
      Style = csDropDownList
      ItemHeight = 13
      ItemIndex = 1
      TabOrder = 2
      Text = 'RAW - Big endian (n64)'
      Items.Strings = (
        'RAW - Little endian (PC)'
        'RAW - Big endian (n64)'
        'TEXT - Disassembled'
        'TEXT - Disassembled + PC'
        'TEXT - RSP Disassembled'
        'TEXT - RSP Disassembled + PC'
        'TEXT - 16 Bit HEX Data'
        'TEXT - 32 Bit HEX Data'
        'C Code (which will need some work)')
    end
  end
  object ButtonDump: TButton
    Left = 144
    Top = 192
    Width = 75
    Height = 25
    Caption = 'Dump'
    Default = True
    TabOrder = 2
    OnClick = ButtonDumpClick
  end
  object ButtonClose: TButton
    Left = 224
    Top = 192
    Width = 75
    Height = 25
    Cancel = True
    Caption = 'Close'
    TabOrder = 3
    OnClick = ButtonCloseClick
  end
  object ButtonAutoRSPUcode: TButton
    Left = 12
    Top = 192
    Width = 125
    Height = 25
    Caption = 'Auto: RSP UCode dump'
    TabOrder = 4
    OnClick = ButtonAutoRSPUcodeClick
  end
  object SaveDialogDumpfile: TSaveDialog
    Filter = 'All files (*.*)|*.*'
    Options = [ofOverwritePrompt, ofHideReadOnly, ofNoChangeDir, ofEnableSizing]
    Title = 'Save dumpfile as...'
    Left = 240
    Top = 128
  end
end
