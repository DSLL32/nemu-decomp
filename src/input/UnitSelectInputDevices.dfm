object FormSelectInputDevices: TFormSelectInputDevices
  Left = 309
  Top = 121
  HorzScrollBar.Visible = False
  VertScrollBar.Smooth = True
  VertScrollBar.Visible = False
  BorderIcons = []
  BorderStyle = bsDialog
  Caption = 'Select input devices'
  ClientHeight = 345
  ClientWidth = 344
  Color = clBtnFace
  Constraints.MinHeight = 351
  Constraints.MinWidth = 300
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  DesignSize = (
    344
    345)
  PixelsPerInch = 96
  TextHeight = 13
  object BevelBottom: TBevel
    Left = -211
    Top = 293
    Width = 590
    Height = 6
    Anchors = [akLeft, akRight, akBottom]
    Shape = bsTopLine
  end
  object LabelName: TLabel
    Left = 8
    Top = 286
    Width = 46
    Height = 13
    Anchors = [akLeft, akBottom]
    Caption = ' Nemu64 '
    Enabled = False
  end
  object ButtonOK: TButton
    Left = 179
    Top = 309
    Width = 75
    Height = 25
    Anchors = [akRight, akBottom]
    Caption = 'OK'
    Default = True
    ModalResult = 1
    TabOrder = 1
    OnClick = ButtonOKClick
  end
  object ButtonCancel: TButton
    Left = 259
    Top = 309
    Width = 75
    Height = 25
    Anchors = [akRight, akBottom]
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 2
  end
  object GroupBoxTop: TGroupBox
    Left = -65
    Top = -24
    Width = 579
    Height = 101
    Anchors = [akLeft, akTop, akRight]
    Caption = 'GroupBoxTop'
    Color = clWhite
    ParentColor = False
    TabOrder = 0
    DesignSize = (
      579
      101)
    object LabelTitle: TLabel
      Left = 80
      Top = 32
      Width = 117
      Height = 13
      Caption = 'Select input devices'
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
      Width = 262
      Height = 41
      Anchors = [akLeft, akTop, akRight]
      AutoSize = False
      Caption = 
        'Select those devices you want to use in Nemu while playing. Enab' +
        'ling all will usually have very little effect on performance.'
      WordWrap = True
    end
    object ImageStick: TImage
      Left = 362
      Top = 47
      Width = 32
      Height = 32
      Anchors = [akTop, akRight]
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
  object CheckListBox: TCheckListBox
    Left = 12
    Top = 88
    Width = 323
    Height = 133
    Anchors = [akLeft, akTop, akRight, akBottom]
    ItemHeight = 13
    TabOrder = 3
    OnClick = CheckListBoxClick
  end
  object GroupBox1: TGroupBox
    Left = 12
    Top = 228
    Width = 323
    Height = 57
    Anchors = [akLeft, akRight, akBottom]
    Caption = 'Settings for selected device'
    TabOrder = 4
    object LabelDeadzone: TLabel
      Left = 12
      Top = 24
      Width = 52
      Height = 13
      Caption = 'Deadzone:'
    end
    object EditDeadzone: TEdit
      Left = 80
      Top = 21
      Width = 57
      Height = 21
      TabOrder = 0
      Text = '50'
    end
    object UpDownDeadzone: TUpDown
      Left = 137
      Top = 21
      Width = 16
      Height = 21
      Associate = EditDeadzone
      Min = 0
      Max = 95
      Increment = 5
      Position = 50
      TabOrder = 1
      Wrap = False
      OnClick = UpDownDeadzoneClick
    end
    object CheckBoxForceFeedback: TCheckBox
      Left = 164
      Top = 24
      Width = 145
      Height = 17
      Caption = 'Activate Force Feedback'
      TabOrder = 2
      OnClick = CheckBoxForceFeedbackClick
    end
  end
end
