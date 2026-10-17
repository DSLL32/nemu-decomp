object FormProfiler: TFormProfiler
  Left = 190
  Top = 102
  BorderIcons = [biSystemMenu]
  BorderStyle = bsDialog
  Caption = 'Profiler'
  ClientHeight = 126
  ClientWidth = 250
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  FormStyle = fsStayOnTop
  OldCreateOrder = False
  Position = poDefaultPosOnly
  OnClose = FormClose
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object Profiler: TOptimizer
    Left = 4
    Top = 28
    Width = 242
    Height = 93
    Processes.Strings = (
      'CPU'
      'GFX-HLE'
      'VI'
      'Audio-HLE'
      'DCT-HLE'
      'Compiler'
      'RSP LLE')
  end
  object ButtonReset: TButton
    Left = 0
    Top = 0
    Width = 75
    Height = 25
    Caption = 'Reset'
    TabOrder = 0
    OnClick = ButtonResetClick
  end
  object CheckBoxEnabled: TCheckBox
    Left = 80
    Top = 4
    Width = 97
    Height = 17
    Caption = 'Enabled'
    TabOrder = 1
    OnClick = CheckBoxEnabledClick
  end
  object CheckBoxAccumulate: TCheckBox
    Left = 144
    Top = 4
    Width = 97
    Height = 17
    Caption = 'Accumulate'
    Checked = True
    State = cbChecked
    TabOrder = 2
  end
  object Timer: TTimer
    Enabled = False
    Interval = 1500
    OnTimer = TimerTimer
    Left = 120
    Top = 64
  end
end
