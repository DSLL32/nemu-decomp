object FormImageView: TFormImageView
  Left = 243
  Top = 128
  Width = 524
  Height = 421
  Caption = 'Image Viewer'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnClose = FormClose
  OnCreate = FormCreate
  OnResize = FormResize
  PixelsPerInch = 96
  TextHeight = 13
  object ImageSizer: TImage
    Left = 292
    Top = 292
    Width = 24
    Height = 24
    AutoSize = True
    Picture.Data = {
      07544269746D617096010000424D960100000000000076000000280000001800
      000018000000010004000000000020010000C40E0000C40E0000100000000000
      000000000000000080000080000000808000800000008000800080800000C0C0
      C000808080000000FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFF
      FF00077777777777F887F887F8870777777777777F887F887F88077777777777
      77F887F887F8077777777777777F887F887F0777777777777777F887F8870777
      7777777777777F887F88077777777777777777F887F80777777777777777777F
      887F07777777777777777777F887077777777777777777777F88077777777777
      7777777777F807777777777777777777777F0777777777777777777777770777
      7777777777777777777707777777777777777777777707777777777777777777
      7777077777777777777777777777077777777777777777777777077777777777
      7777777777770777777777777777777777770777777777777777777777770777
      7777777777777777777707777777777777777777777700000000000000000000
      0000}
    OnMouseDown = ImageSizerMouseDown
    OnMouseMove = ImageSizerMouseMove
    OnMouseUp = ImageSizerMouseUp
  end
  object PaintBox: TPaintBox
    Left = 0
    Top = 37
    Width = 289
    Height = 257
    OnPaint = PaintBoxPaint
  end
  object PanelTop: TPanel
    Left = 0
    Top = 0
    Width = 516
    Height = 37
    Align = alTop
    BevelInner = bvRaised
    BevelOuter = bvLowered
    TabOrder = 0
    object LabelAddress: TLabel
      Left = 8
      Top = 10
      Width = 41
      Height = 13
      Caption = 'Address:'
    end
    object EditMemAddress: TEdit
      Left = 56
      Top = 8
      Width = 61
      Height = 21
      TabOrder = 0
      Text = '00000000'
      OnChange = EditMemAddressChange
    end
    object chkRefreshEveryFrame: TCheckBox
      Left = 120
      Top = 11
      Width = 117
      Height = 17
      Caption = 'Refresh every frame'
      TabOrder = 1
      OnClick = chkRefreshEveryFrameClick
    end
    object ComboBoxFormat: TComboBox
      Left = 244
      Top = 8
      Width = 145
      Height = 21
      Style = csDropDownList
      ItemHeight = 13
      ItemIndex = 2
      TabOrder = 2
      Text = '16 bit grayscale'
      OnChange = ComboBoxFormatChange
      Items.Strings = (
        '4 bit grayscale'
        '8 bit grayscale'
        '16 bit grayscale')
    end
    object Button1: TButton
      Left = 392
      Top = 8
      Width = 75
      Height = 25
      Caption = 'Export VRML'
      TabOrder = 3
    end
  end
  object ScrollBar: TScrollBar
    Left = 500
    Top = 37
    Width = 16
    Height = 350
    Align = alRight
    Kind = sbVertical
    LargeChange = 10
    Max = 2097152
    PageSize = 0
    TabOrder = 1
    TabStop = False
    OnChange = ScrollBarChange
  end
end
