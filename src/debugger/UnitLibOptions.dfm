object FormLibOptions: TFormLibOptions
  Left = 190
  Top = 101
  BorderIcons = [biSystemMenu]
  BorderStyle = bsSingle
  Caption = 'Library options'
  ClientHeight = 215
  ClientWidth = 337
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object PanelLoad: TPanel
    Left = 8
    Top = 16
    Width = 317
    Height = 45
    BevelInner = bvRaised
    BevelOuter = bvLowered
    TabOrder = 1
    object LabelCodFolder: TLabel
      Left = 8
      Top = 16
      Width = 53
      Height = 13
      Caption = '.cod folder:'
    end
    object EditCodFolder: TEdit
      Left = 100
      Top = 12
      Width = 205
      Height = 21
      TabOrder = 0
    end
  end
  object CheckBoxAutoloadCodFiles: TCheckBox
    Left = 12
    Top = 8
    Width = 105
    Height = 17
    Caption = 'Auto load cod files'
    TabOrder = 0
  end
  object GroupBoxLFE: TGroupBox
    Left = 8
    Top = 68
    Width = 317
    Height = 97
    Caption = 'LFE'
    TabOrder = 2
    object LabelLFE: TLabel
      Left = 8
      Top = 20
      Width = 22
      Height = 13
      Caption = 'LFE:'
    end
    object LabelLFECommandline: TLabel
      Left = 8
      Top = 44
      Width = 124
      Height = 13
      Caption = 'Commandline (%1 for rom):'
    end
    object EditLFE: TEdit
      Left = 100
      Top = 16
      Width = 205
      Height = 21
      TabOrder = 0
    end
    object EditLFECommandline: TEdit
      Left = 8
      Top = 60
      Width = 297
      Height = 21
      TabOrder = 1
    end
  end
  object ButtonSave: TButton
    Left = 248
    Top = 172
    Width = 75
    Height = 25
    Caption = 'Save'
    TabOrder = 3
    OnClick = ButtonSaveClick
  end
end
