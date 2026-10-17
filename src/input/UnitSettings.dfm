object FormSettings: TFormSettings
  Left = 193
  Top = 106
  Width = 644
  Height = 390
  BorderIcons = []
  Caption = 'Input settings'
  Color = clBtnFace
  Constraints.MinHeight = 341
  Constraints.MinWidth = 483
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnShow = FormShow
  DesignSize = (
    636
    356)
  PixelsPerInch = 96
  TextHeight = 13
  object Image1: TImage
    Left = 592
    Top = 8
    Width = 32
    Height = 32
    Anchors = [akTop, akRight]
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
  object ButtonOK: TButton
    Left = 552
    Top = 324
    Width = 75
    Height = 25
    Anchors = [akRight, akBottom]
    Caption = 'Close'
    Default = True
    ModalResult = 1
    TabOrder = 3
    OnClick = ButtonOKClick
  end
  object ButtonSelectInput: TButton
    Left = 12
    Top = 8
    Width = 129
    Height = 25
    Caption = '&Select input devices...'
    TabOrder = 0
    OnClick = ButtonSelectInputClick
  end
  object GroupBoxSettings: TGroupBox
    Left = 12
    Top = 52
    Width = 617
    Height = 261
    Anchors = [akLeft, akTop, akRight, akBottom]
    Caption = 'Controller settings'
    TabOrder = 2
    OnClick = GroupBoxSettingsClick
    DesignSize = (
      617
      261)
    object LabelSelect: TLabel
      Left = 12
      Top = 24
      Width = 88
      Height = 13
      Caption = 'Select a &controller:'
    end
    object GroupBoxGeneral: TGroupBox
      Left = 12
      Top = 48
      Width = 129
      Height = 89
      Caption = 'General'
      TabOrder = 1
      object CheckBoxMemPack: TCheckBox
        Left = 12
        Top = 40
        Width = 97
        Height = 17
        Caption = 'Has &mempack'
        TabOrder = 1
        OnClick = CheckBoxMemPackClick
      end
      object CheckBoxRumblePack: TCheckBox
        Left = 12
        Top = 60
        Width = 97
        Height = 17
        Caption = 'Has &rumblepack'
        TabOrder = 2
        OnClick = CheckBoxRumblePackClick
      end
      object CheckBoxPluggedIn: TCheckBox
        Left = 12
        Top = 20
        Width = 82
        Height = 17
        Caption = '&Plugged in'
        TabOrder = 0
        OnClick = CheckBoxPluggedInClick
      end
    end
    object GroupBoxMapping: TGroupBox
      Left = 148
      Top = 48
      Width = 461
      Height = 201
      Anchors = [akLeft, akTop, akRight, akBottom]
      Caption = 'Mapping'
      TabOrder = 2
      DesignSize = (
        461
        201)
      object ListBoxButtons: TListBox
        Left = 16
        Top = 20
        Width = 133
        Height = 165
        Style = lbOwnerDrawFixed
        Anchors = [akLeft, akTop, akBottom]
        Constraints.MaxHeight = 165
        Constraints.MinHeight = 165
        ItemHeight = 13
        Items.Strings = (
          'BA'
          'BB'
          'BZ'
          'BStart'
          'BPan Left'
          'BPan Right'
          'AAnalog X'
          'AAnalog Y'
          'ADigital Pad Left/Right'
          'ADigital Pad Up/Down'
          'ACButtons Left/Right'
          'ACButtons Up/Down'
          'DRumble Pack')
        TabOrder = 0
        OnClick = ListBoxButtonsClick
        OnDrawItem = ListBoxButtonsDrawItem
      end
      object NotebookMap: TNotebook
        Left = 152
        Top = 16
        Width = 304
        Height = 181
        Anchors = [akLeft, akTop, akRight, akBottom]
        PageIndex = 2
        TabOrder = 1
        object TPage
          Left = 0
          Top = 0
          Caption = 'Axis'
          DesignSize = (
            304
            181)
          object NotebookAxis: TNotebook
            Left = 0
            Top = 0
            Width = 305
            Height = 177
            Anchors = [akLeft, akTop, akRight, akBottom]
            TabOrder = 1
            object TPage
              Left = 0
              Top = 0
              Caption = 'Axis-Axis'
              DesignSize = (
                305
                177)
              object LabelAxis: TLabel
                Left = 0
                Top = 60
                Width = 22
                Height = 13
                Caption = 'Axis:'
              end
              object ComboBoxAxis: TComboBox
                Left = 44
                Top = 56
                Width = 257
                Height = 21
                Style = csDropDownList
                Anchors = [akLeft, akTop, akRight]
                DropDownCount = 20
                ItemHeight = 13
                Sorted = True
                TabOrder = 0
                OnChange = ComboBoxAxisChange
              end
            end
            object TPage
              Left = 0
              Top = 0
              Caption = 'Axis-2 Buttons'
              DesignSize = (
                305
                177)
              object LabelAxisButton1: TLabel
                Left = 0
                Top = 60
                Width = 43
                Height = 13
                Caption = 'Button 1:'
              end
              object LabelAxisButton2: TLabel
                Left = 0
                Top = 84
                Width = 43
                Height = 13
                Caption = 'Button 2:'
              end
              object LabelAxisModifier2: TLabel
                Left = 0
                Top = 132
                Width = 43
                Height = 13
                Alignment = taRightJustify
                AutoSize = False
                Caption = '25%:'
              end
              object LabelAxisModifier3: TLabel
                Left = 0
                Top = 156
                Width = 43
                Height = 13
                Alignment = taRightJustify
                AutoSize = False
                Caption = '100%'
              end
              object LabelAxisModifier1: TLabel
                Left = 0
                Top = 108
                Width = 43
                Height = 13
                Alignment = taRightJustify
                AutoSize = False
                Caption = '15%:'
              end
              object LabelModifierExplanation: TLabel
                Left = 100
                Top = 8
                Width = 197
                Height = 33
                Anchors = [akLeft, akTop, akRight]
                AutoSize = False
                Caption = 'Without modifiers, analog stick is at 50% when button is pressed'
                WordWrap = True
              end
              object ComboBoxAxisButton1: TComboBox
                Left = 44
                Top = 56
                Width = 235
                Height = 21
                Style = csDropDownList
                Anchors = [akLeft, akTop, akRight]
                DropDownCount = 20
                ItemHeight = 13
                Sorted = True
                TabOrder = 0
                OnChange = ComboBoxAxisButton1Change
              end
              object ComboBoxAxisButton2: TComboBox
                Left = 44
                Top = 80
                Width = 235
                Height = 21
                Style = csDropDownList
                Anchors = [akLeft, akTop, akRight]
                DropDownCount = 20
                ItemHeight = 13
                Sorted = True
                TabOrder = 2
                OnChange = ComboBoxAxisButton2Change
              end
              object ComboBoxModifier2: TComboBox
                Left = 44
                Top = 128
                Width = 235
                Height = 21
                Style = csDropDownList
                Anchors = [akLeft, akTop, akRight]
                DropDownCount = 20
                ItemHeight = 13
                Sorted = True
                TabOrder = 6
                OnChange = ComboBoxModifier2Change
              end
              object ComboBoxModifier1: TComboBox
                Left = 44
                Top = 104
                Width = 235
                Height = 21
                Style = csDropDownList
                Anchors = [akLeft, akTop, akRight]
                DropDownCount = 20
                ItemHeight = 13
                Sorted = True
                TabOrder = 4
                OnChange = ComboBoxModifier1Change
              end
              object ComboBoxModifier3: TComboBox
                Left = 44
                Top = 152
                Width = 235
                Height = 21
                Style = csDropDownList
                Anchors = [akLeft, akTop, akRight]
                DropDownCount = 20
                ItemHeight = 13
                Sorted = True
                TabOrder = 8
                OnChange = ComboBoxModifier3Change
              end
              object ButtonWaitAxisButton1: TButton
                Left = 284
                Top = 56
                Width = 20
                Height = 21
                Anchors = [akTop, akRight]
                Caption = '...'
                TabOrder = 1
                OnClick = ButtonWaitAxisButton1Click
              end
              object ButtonWaitAxisButton2: TButton
                Left = 284
                Top = 80
                Width = 20
                Height = 21
                Anchors = [akTop, akRight]
                Caption = '...'
                TabOrder = 3
                OnClick = ButtonWaitAxisButton2Click
              end
              object ButtonWaitAxisModifier1: TButton
                Left = 284
                Top = 104
                Width = 20
                Height = 21
                Anchors = [akTop, akRight]
                Caption = '...'
                TabOrder = 5
                OnClick = ButtonWaitAxisModifier1Click
              end
              object ButtonWaitAxisModifier2: TButton
                Left = 284
                Top = 128
                Width = 20
                Height = 21
                Anchors = [akTop, akRight]
                Caption = '...'
                TabOrder = 7
                OnClick = ButtonWaitAxisModifier2Click
              end
              object ButtonWaitAxisModifier3: TButton
                Left = 284
                Top = 152
                Width = 20
                Height = 21
                Anchors = [akTop, akRight]
                Caption = '...'
                TabOrder = 9
                OnClick = ButtonWaitAxisModifier3Click
              end
            end
          end
          object RadioGroupAxis: TRadioGroup
            Left = 0
            Top = 0
            Width = 97
            Height = 49
            Caption = 'Axis'
            ItemIndex = 0
            Items.Strings = (
              'Axis'
              '2 Buttons')
            TabOrder = 0
            OnClick = RadioGroupAxisClick
          end
        end
        object TPage
          Left = 0
          Top = 0
          Caption = 'Button'
          DesignSize = (
            304
            181)
          object LabelButtonButton: TLabel
            Left = 0
            Top = 8
            Width = 34
            Height = 13
            Caption = 'Button:'
          end
          object ComboBoxButton: TComboBox
            Left = 44
            Top = 4
            Width = 231
            Height = 21
            Style = csDropDownList
            Anchors = [akLeft, akTop, akRight]
            DropDownCount = 20
            ItemHeight = 13
            Sorted = True
            TabOrder = 0
            OnChange = ComboBoxButtonChange
          end
          object CheckBoxTurboFire: TCheckBox
            Left = 44
            Top = 32
            Width = 97
            Height = 13
            Caption = '&Turbo fire'
            TabOrder = 2
            OnClick = CheckBoxTurboFireClick
          end
          object ButtonWaitButton: TButton
            Left = 280
            Top = 4
            Width = 20
            Height = 21
            Anchors = [akTop, akRight]
            Caption = '...'
            TabOrder = 1
            OnClick = ButtonWaitButtonClick
          end
        end
        object TPage
          Left = 0
          Top = 0
          Caption = 'Device'
          DesignSize = (
            304
            181)
          object Label1: TLabel
            Left = 0
            Top = 8
            Width = 37
            Height = 13
            Caption = 'Device:'
          end
          object ComboBoxDDevice: TComboBox
            Left = 44
            Top = 4
            Width = 255
            Height = 21
            Style = csDropDownList
            Anchors = [akLeft, akTop, akRight]
            ItemHeight = 13
            TabOrder = 0
            OnChange = ComboBoxDDeviceChange
          end
        end
      end
    end
    object ButtonAutoConfigureAdaptoid: TButton
      Left = 12
      Top = 176
      Width = 129
      Height = 25
      Caption = 'Auto configure Adaptoid'
      TabOrder = 4
      OnClick = ButtonAutoConfigureAdaptoidClick
    end
    object ButtonAutoconfigureKeyboard: TButton
      Left = 12
      Top = 144
      Width = 129
      Height = 25
      Caption = 'Auto configure keyboard'
      TabOrder = 3
      OnClick = ButtonAutoconfigureKeyboardClick
    end
    object TabControlController: TTabControl
      Left = 104
      Top = 20
      Width = 349
      Height = 21
      Style = tsFlatButtons
      TabOrder = 0
      Tabs.Strings = (
        'Controller 1'
        'Controller 2'
        'Controller 3'
        'Controller 4')
      TabIndex = 0
      OnChange = TabControlControllerChange
    end
  end
  object ButtonSensitivity: TButton
    Left = 144
    Top = 8
    Width = 101
    Height = 25
    Caption = 'Set Sensitivity...'
    TabOrder = 1
    OnClick = ButtonSensitivityClick
  end
  object PopupMenuAdaptoid: TPopupMenu
    Left = 80
    Top = 260
  end
end
