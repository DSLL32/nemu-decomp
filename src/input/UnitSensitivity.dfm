object FormSensitivity: TFormSensitivity
  Left = 218
  Top = 244
  BorderStyle = bsDialog
  Caption = 'Button --> Analog sensitivity'
  ClientHeight = 289
  ClientWidth = 288
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  PixelsPerInch = 96
  TextHeight = 13
  object LabelDefault: TLabel
    Left = 32
    Top = 108
    Width = 37
    Height = 13
    Caption = '&Default:'
    FocusControl = EditDefault
  end
  object Label2: TLabel
    Left = 32
    Top = 136
    Width = 49
    Height = 13
    Caption = '&Modifier 1:'
    FocusControl = Edit1
  end
  object Label3: TLabel
    Left = 32
    Top = 164
    Width = 49
    Height = 13
    Caption = 'M&odifier 2:'
    FocusControl = Edit2
  end
  object Label4: TLabel
    Left = 32
    Top = 192
    Width = 49
    Height = 13
    Caption = 'Mod&ifier 3:'
    FocusControl = Edit3
  end
  object BevelBottom: TBevel
    Left = -99
    Top = 237
    Width = 499
    Height = 5
    Shape = bsTopLine
  end
  object LabelName: TLabel
    Left = 8
    Top = 230
    Width = 46
    Height = 13
    Caption = ' Nemu64 '
    Enabled = False
  end
  object EditDefault: TEdit
    Left = 172
    Top = 104
    Width = 57
    Height = 21
    TabOrder = 0
    Text = '50'
  end
  object UpDownDefault: TUpDown
    Left = 229
    Top = 104
    Width = 15
    Height = 21
    Associate = EditDefault
    Min = 1
    Increment = 5
    Position = 50
    TabOrder = 1
    Wrap = False
  end
  object Edit1: TEdit
    Left = 172
    Top = 132
    Width = 57
    Height = 21
    TabOrder = 2
    Text = '15'
  end
  object UpDown1: TUpDown
    Left = 229
    Top = 132
    Width = 15
    Height = 21
    Associate = Edit1
    Min = 1
    Increment = 5
    Position = 15
    TabOrder = 3
    Wrap = False
  end
  object Edit2: TEdit
    Left = 172
    Top = 160
    Width = 57
    Height = 21
    TabOrder = 4
    Text = '25'
  end
  object UpDown2: TUpDown
    Left = 229
    Top = 160
    Width = 15
    Height = 21
    Associate = Edit2
    Min = 1
    Increment = 5
    Position = 25
    TabOrder = 5
    Wrap = False
  end
  object Edit3: TEdit
    Left = 172
    Top = 188
    Width = 57
    Height = 21
    TabOrder = 6
    Text = '100'
  end
  object UpDown3: TUpDown
    Left = 229
    Top = 188
    Width = 15
    Height = 21
    Associate = Edit3
    Min = 1
    Increment = 5
    Position = 100
    TabOrder = 7
    Wrap = False
  end
  object ButtonOK: TButton
    Left = 123
    Top = 252
    Width = 75
    Height = 25
    Caption = 'OK'
    Default = True
    ModalResult = 1
    TabOrder = 8
  end
  object ButtonCancel: TButton
    Left = 203
    Top = 252
    Width = 75
    Height = 25
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 9
  end
  object GroupBox1: TGroupBox
    Left = -65
    Top = -24
    Width = 521
    Height = 101
    Caption = 'GroupBoxTop'
    Color = clWhite
    ParentColor = False
    TabOrder = 10
    object LabelTitle: TLabel
      Left = 80
      Top = 32
      Width = 160
      Height = 13
      Caption = 'Button --> Analog sensitivity'
      Font.Charset = ANSI_CHARSET
      Font.Color = clMaroon
      Font.Height = -11
      Font.Name = 'MS Sans Serif'
      Font.Style = [fsBold]
      ParentFont = False
    end
    object LabelMsg: TLabel
      Left = 92
      Top = 48
      Width = 204
      Height = 41
      AutoSize = False
      Caption = 
        'These settings control the sensitivity of the analog stick when ' +
        'used in "2 Buttons"-mode. Values are in percent.'
      WordWrap = True
    end
    object ImageStick: TImage
      Left = 304
      Top = 47
      Width = 32
      Height = 32
      AutoSize = True
      Picture.Data = {
        07544269746D617076020000424D760200000000000076000000280000002000
        000020000000010004000000000000020000C30E0000C30E0000100000000000
        000000000000605E600070707000000681000000FF000C9E0000A83000008080
        800094929400AFB0AF00BFB8BF0080FFFF00CFCCCF00DFD8DF00DFE3DF00F0F0
        F000BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB000BBBBBB
        BBBBBBBBBBBBBBBBBBBB00099800BBBBBBBBBBBBBBBBBBBBB000999A988800BB
        BBBBBBBBBBBBBB000999CACA98888700BBBBBBBBBBBB00999CCCCCAA98888722
        00BBBBBBBBB799ECDCCCCACFF8888272110BBBBBBBB7EECECDDCFFFFCCC88721
        2110BBBBBBB7EEECEFFFFDCCCCCCA2721110BBBBBBB7EEFFFFEDDCCCCCCACAA1
        2110BBBBBBB7FFFFEEDDA77700AC73399110BB000BB7FEEEEDEA7DDA870A3A40
        9910B7ED70BB77EEEED7FD000720A3399910B7FE70BBBB77EDE7F7FA7020CA99
        111BB7FD70BBBBBB77DE27FA200CAA11BBBB7FEE70BBBBBBBB7777FA707111BB
        BBBB7FEDE7000BBBBBBBB000001BBBBBBBBB7FEEDDD7200BBBBB7FC9820BBBBB
        BBBB7FEDE27CC7200BBB7FC9820BBBBBBBBB7FEE7A07CCA720BB7FC9820BBBBB
        BBBB7FEA9F70CACAA70B7FC9820BBBBBBBBBB7FD7907CCACA9707FC9820BBBBB
        BBBBB7FDC72CCC55AA927FC9820BBBBBBBBBB7FFDCCCCA55A9927FD9820BBBBB
        BBBBBB7FDCCCCCAAA6627FC9820BBBBBBBBBBB7FFCCCCA33A6627FC9820BBBBB
        BBBBBBB7FFCCCC439A927FC9820BBBBBBBBBBBBB77FFCAAAA9FC7FC9820BBBBB
        BBBBBBBBBB77FFFFFF777FC9820BBBBBBBBBBBBBBBBB777777BB7FFF820BBBBB
        BBBBBBBBBBBBBBBBBBBB7AAAAA0BBBBBBBBBBBBBBBBBBBBBBBBBB77777BBBBBB
        BBBB}
      Transparent = True
    end
  end
end
