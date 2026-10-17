object FormWait: TFormWait
  Left = 541
  Top = 136
  BorderStyle = bsDialog
  Caption = 'Input'
  ClientHeight = 146
  ClientWidth = 234
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnClose = FormClose
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object SpeedButtonCancel: TSpeedButton
    Left = 144
    Top = 108
    Width = 75
    Height = 25
    Caption = 'Cancel'
    OnClick = SpeedButtonCancelClick
  end
  object Label1: TLabel
    Left = 12
    Top = 12
    Width = 186
    Height = 26
    Caption = '(warning: this will not use the num-keys properly)'
    WordWrap = True
  end
  object PanelStatus: TPanel
    Left = 12
    Top = 52
    Width = 209
    Height = 45
    BevelInner = bvRaised
    BevelOuter = bvLowered
    Caption = 'Waiting for input...'
    TabOrder = 0
  end
  object Timer: TTimer
    Interval = 55
    OnTimer = TimerTimer
    Left = 8
    Top = 104
  end
end
