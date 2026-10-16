object FormSymbols: TFormSymbols
  Left = 216
  Top = 121
  Width = 307
  Height = 224
  Caption = 'Symbols'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object ListBoxSymbols: TListBox
    Left = 0
    Top = 0
    Width = 299
    Height = 156
    Align = alClient
    ItemHeight = 13
    Sorted = True
    TabOrder = 0
    OnDblClick = ListBoxSymbolsDblClick
  end
  object PanelBottom: TPanel
    Left = 0
    Top = 156
    Width = 299
    Height = 41
    Align = alBottom
    TabOrder = 1
    object ButtonClose: TButton
      Left = 8
      Top = 8
      Width = 75
      Height = 25
      Caption = 'Close'
      TabOrder = 0
      OnClick = ButtonCloseClick
    end
    object ButtonDebug: TButton
      Left = 88
      Top = 8
      Width = 75
      Height = 25
      Caption = 'Debug'
      TabOrder = 1
      OnClick = ListBoxSymbolsDblClick
    end
  end
end
