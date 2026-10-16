object FormMemory: TFormMemory
  Left = 258
  Top = 114
  Width = 531
  Height = 226
  Caption = 'Memory'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = False
  OnClose = FormClose
  OnKeyUp = FormKeyUp
  PixelsPerInch = 96
  TextHeight = 13
  object PanelTop: TPanel
    Left = 0
    Top = 0
    Width = 523
    Height = 85
    Align = alTop
    BevelInner = bvLowered
    BorderWidth = 3
    TabOrder = 0
    object LabelAddress: TLabel
      Left = 8
      Top = 14
      Width = 41
      Height = 13
      Caption = 'Address:'
    end
    object SpeedButtonPredefined: TSpeedButton
      Left = 120
      Top = 8
      Width = 77
      Height = 25
      Caption = 'Predefined'
      Glyph.Data = {
        D6000000424DD60000000000000076000000280000000C0000000C0000000100
        04000000000060000000C30E0000C30E00001000000000000000000000000000
        8000008000000080800080000000800080008080000080808000C0C0C0000000
        FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00888888888888
        0000888888888888000088888888888800008888888888880000888880888888
        0000888800088888000088800000888800008800000008880000888888888888
        0000888888888888000088888888888800008888888888880000}
      Layout = blGlyphRight
      Margin = 2
      OnClick = SpeedButtonPredefinedClick
    end
    object EditMemAddress: TEdit
      Left = 56
      Top = 10
      Width = 61
      Height = 21
      TabOrder = 0
      Text = '00000000'
      OnChange = EditMemAddressChange
    end
    object ButtonRefresh: TButton
      Left = 200
      Top = 8
      Width = 61
      Height = 25
      Caption = 'Refresh'
      TabOrder = 1
      OnClick = ButtonRefreshClick
    end
    object ButtonClose: TButton
      Left = 464
      Top = 8
      Width = 49
      Height = 25
      Caption = 'Close'
      TabOrder = 2
      OnClick = ButtonCloseClick
    end
    object ButtonDump: TButton
      Left = 264
      Top = 8
      Width = 61
      Height = 25
      Caption = 'Dump...'
      TabOrder = 3
      OnClick = ButtonDumpClick
    end
    object ButtonSearch: TButton
      Left = 328
      Top = 8
      Width = 61
      Height = 25
      Caption = 'Search...'
      TabOrder = 4
      OnClick = ButtonSearchClick
    end
    object CheckBoxAddress: TCheckBox
      Left = 8
      Top = 40
      Width = 85
      Height = 17
      Caption = 'Address'
      Checked = True
      State = cbChecked
      TabOrder = 5
      OnClick = CheckBoxClick
    end
    object CheckBoxHex: TCheckBox
      Left = 96
      Top = 40
      Width = 77
      Height = 17
      Caption = 'Hex'
      Checked = True
      State = cbChecked
      TabOrder = 6
      OnClick = CheckBoxClick
    end
    object CheckBoxAscii: TCheckBox
      Left = 176
      Top = 40
      Width = 73
      Height = 17
      Caption = 'ASCII'
      Checked = True
      State = cbChecked
      TabOrder = 7
      OnClick = CheckBoxClick
    end
    object CheckBoxWords: TCheckBox
      Left = 8
      Top = 60
      Width = 73
      Height = 17
      Caption = 'Words'
      TabOrder = 8
      OnClick = CheckBoxClick
    end
    object CheckBoxHalfwords: TCheckBox
      Left = 96
      Top = 60
      Width = 73
      Height = 17
      Caption = 'Halfwords'
      TabOrder = 9
      OnClick = CheckBoxClick
    end
    object CheckBoxBytes: TCheckBox
      Left = 176
      Top = 60
      Width = 73
      Height = 17
      Caption = 'Bytes'
      TabOrder = 10
      OnClick = CheckBoxClick
    end
    object ButtonBookmark: TButton
      Left = 392
      Top = 8
      Width = 69
      Height = 25
      Caption = 'Bookmark...'
      TabOrder = 11
      OnClick = ButtonBookmarkClick
    end
    object GroupBoxWatchType: TGroupBox
      Left = 312
      Top = 36
      Width = 201
      Height = 41
      Caption = 'Watch type on set'
      TabOrder = 12
      object CheckBoxRead: TCheckBox
        Left = 8
        Top = 16
        Width = 61
        Height = 17
        Caption = 'Read'
        Checked = True
        Color = clBtnFace
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clWindowText
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentColor = False
        ParentFont = False
        State = cbChecked
        TabOrder = 0
      end
      object CheckBoxWrite: TCheckBox
        Left = 96
        Top = 16
        Width = 97
        Height = 17
        Caption = 'Write'
        Checked = True
        State = cbChecked
        TabOrder = 1
      end
    end
  end
  object PanelMem: TPanel
    Left = 0
    Top = 85
    Width = 523
    Height = 107
    Align = alClient
    BevelInner = bvLowered
    BorderWidth = 3
    TabOrder = 1
    object PaintBox: TPaintBox
      Left = 5
      Top = 5
      Width = 513
      Height = 97
      Align = alClient
      Color = 14155694
      Font.Charset = ANSI_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'Courier'
      Font.Style = []
      ParentColor = False
      ParentFont = False
      OnMouseDown = PaintBoxMouseDown
      OnPaint = PaintBoxPaint
    end
    object EditMem: TEdit
      Left = 68
      Top = 64
      Width = 73
      Height = 21
      Font.Charset = ANSI_CHARSET
      Font.Color = clWindowText
      Font.Height = -13
      Font.Name = 'Courier'
      Font.Style = []
      ParentFont = False
      TabOrder = 0
      Text = '12345678'
      Visible = False
      OnExit = EditMemExit
      OnKeyPress = EditMemKeyPress
    end
  end
  object PopupMenu: TPopupMenu
    Left = 416
    Top = 80
    object MenuItemStackPointerSP: TMenuItem
      Caption = 'Stack Pointer (SP)'
      OnClick = MenuItemStackPointerSPClick
    end
    object N4: TMenuItem
      Caption = '-'
    end
    object RDRAM: TMenuItem
      Caption = '$0000 0000: RDRAM memory'
      OnClick = MemoryAddressClick
    end
    object RDRAMregisters: TMenuItem
      Caption = '$03F0 0000: RDRAM registers'
      OnClick = MemoryAddressClick
    end
    object N1: TMenuItem
      Caption = '-'
    end
    object SPregisters1: TMenuItem
      Caption = '$0400 0000: SP registers'
      OnClick = MemoryAddressClick
    end
    object DPcommandregisters1: TMenuItem
      Caption = '$0410 0000: DP command registers'
      OnClick = MemoryAddressClick
    end
    object DPspanregisters042000001: TMenuItem
      Caption = '$0420 0000: DP span registers'
      OnClick = MemoryAddressClick
    end
    object N04300000MIregisters1: TMenuItem
      Caption = '$0430 0000: MI registers'
      OnClick = MemoryAddressClick
    end
    object N04400000VIregisters1: TMenuItem
      Caption = '$0440 0000: VI registers'
      OnClick = MemoryAddressClick
    end
    object N04500000AIregisters1: TMenuItem
      Caption = '$0450 0000: AI registers'
      OnClick = MemoryAddressClick
    end
    object N04600000PIregisters1: TMenuItem
      Caption = '$0460 0000: PI registers'
      OnClick = MemoryAddressClick
    end
    object N04700000RIregisters1: TMenuItem
      Caption = '$0470 0000: RI registers'
      OnClick = MemoryAddressClick
    end
    object N04800000SIregisters1: TMenuItem
      Caption = '$0480 0000: SI registers'
      OnClick = MemoryAddressClick
    end
    object N2: TMenuItem
      Caption = '-'
    end
    object N05000000cartridgedomain21: TMenuItem
      Caption = '$0500 0000: cartridge domain 2'
      OnClick = MemoryAddressClick
    end
    object N06000000cartridgedomain11: TMenuItem
      Caption = '$0600 0000: cartridge domain 1'
      OnClick = MemoryAddressClick
    end
    object N08000000cartridgedomain22: TMenuItem
      Caption = '$0800 0000: cartridge domain 2'
      OnClick = MemoryAddressClick
    end
    object N10000000cartridgedomain11: TMenuItem
      Caption = '$1000 0000: cartridge domain 1'
      OnClick = MemoryAddressClick
    end
    object N3: TMenuItem
      Caption = '-'
    end
    object PIFBootRom1: TMenuItem
      Caption = '$1FC0 0000: PIF Boot Rom'
      OnClick = MemoryAddressClick
    end
    object N1FC00000PIFBootRom1: TMenuItem
      Caption = '$1FC0 07C0: PIF Ram'
      OnClick = MemoryAddressClick
    end
    object N1FD00000cartridgedomain11: TMenuItem
      Caption = '$1FD0 0000: cartridge domain 1'
      OnClick = MemoryAddressClick
    end
  end
end
