object FormCombiners: TFormCombiners
  Left = 195
  Top = 127
  Width = 443
  Height = 317
  Caption = 'Combiners'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  OnResize = FormResize
  PixelsPerInch = 96
  TextHeight = 13
  object CheckListBox: TCheckListBox
    Left = 0
    Top = 0
    Width = 435
    Height = 201
    OnClickCheck = CheckListBoxClickCheck
    Align = alClient
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    ItemHeight = 13
    ParentFont = False
    Style = lbOwnerDrawFixed
    TabOrder = 0
    OnClick = CheckListBoxClick
    OnDrawItem = CheckListBoxDrawItem
  end
  object PanelTop: TPanel
    Left = 0
    Top = 201
    Width = 435
    Height = 82
    Align = alBottom
    BevelInner = bvRaised
    BevelOuter = bvLowered
    TabOrder = 1
    object LabelColor: TLabel
      Left = 88
      Top = 14
      Width = 27
      Height = 13
      BiDiMode = bdLeftToRight
      Caption = 'Color:'
      ParentBiDiMode = False
    end
    object LabelAlpha: TLabel
      Left = 88
      Top = 38
      Width = 30
      Height = 13
      Caption = 'Alpha:'
    end
    object Label1: TLabel
      Left = 88
      Top = 60
      Width = 32
      Height = 13
      Caption = 'Extras:'
    end
    object ButtonEdit: TButton
      Left = 8
      Top = 8
      Width = 75
      Height = 25
      Caption = 'Edit'
      TabOrder = 0
      OnClick = ButtonEditClick
    end
    object EditColor: TEdit
      Left = 124
      Top = 10
      Width = 300
      Height = 21
      Color = clBtnFace
      ReadOnly = True
      TabOrder = 1
    end
    object EditAlpha: TEdit
      Left = 124
      Top = 34
      Width = 300
      Height = 21
      Color = clBtnFace
      ReadOnly = True
      TabOrder = 2
    end
    object ButtonDelete: TButton
      Left = 8
      Top = 36
      Width = 75
      Height = 25
      Caption = 'Delete mode'
      Enabled = False
      TabOrder = 3
      OnClick = ButtonDeleteClick
    end
    object EditExtras: TEdit
      Left = 124
      Top = 58
      Width = 300
      Height = 21
      Color = clBtnFace
      ReadOnly = True
      TabOrder = 4
    end
  end
end
