object FormBookmarks: TFormBookmarks
  Left = 190
  Top = 101
  Width = 448
  Height = 242
  Caption = 'Bookmarks'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  PixelsPerInch = 96
  TextHeight = 13
  object PanelBottom: TPanel
    Left = 0
    Top = 167
    Width = 440
    Height = 41
    Align = alBottom
    BevelInner = bvRaised
    BevelOuter = bvLowered
    TabOrder = 0
    object Notebook: TNotebook
      Left = 2
      Top = 2
      Width = 436
      Height = 37
      Align = alClient
      TabOrder = 0
      object TPage
        Left = 0
        Top = 0
        Caption = 'Default'
        object ButtonSave: TButton
          Left = 335
          Top = 7
          Width = 75
          Height = 25
          Caption = 'Save'
          TabOrder = 0
          OnClick = ButtonSaveClick
        end
        object ButtonLoad: TButton
          Left = 255
          Top = 7
          Width = 75
          Height = 25
          Caption = 'Load'
          TabOrder = 1
          OnClick = ButtonLoadClick
        end
        object ButtonRemove: TButton
          Left = 175
          Top = 7
          Width = 75
          Height = 25
          Caption = 'Remove'
          TabOrder = 2
          OnClick = ButtonRemoveClick
        end
        object ButtonNewFolder: TButton
          Left = 95
          Top = 7
          Width = 75
          Height = 25
          Caption = 'New folder'
          TabOrder = 3
          OnClick = ButtonNewFolderClick
        end
        object ButtonRootFolder: TButton
          Left = 7
          Top = 7
          Width = 83
          Height = 25
          Caption = 'New root folder'
          TabOrder = 4
          OnClick = ButtonRootFolderClick
        end
      end
      object TPage
        Left = 0
        Top = 0
        Caption = 'Create'
        object ButtonCreate: TButton
          Left = 7
          Top = 7
          Width = 75
          Height = 25
          Caption = 'Create...'
          TabOrder = 0
          OnClick = ButtonCreateClick
        end
      end
    end
  end
  object TreeViewBookmarks: TTreeView
    Left = 0
    Top = 29
    Width = 440
    Height = 138
    Align = alClient
    DragMode = dmAutomatic
    Indent = 19
    RowSelect = True
    ShowRoot = False
    SortType = stText
    TabOrder = 1
    OnDblClick = TreeViewBookmarksDblClick
    OnDragDrop = TreeViewBookmarksDragDrop
    OnDragOver = TreeViewBookmarksDragOver
    Items.Data = {
      010000001D0000000000000000000000FFFFFFFFFFFFFFFF0000000003000000
      04526F6F74280000000000000000000000FFFFFFFFFFFFFFFF00000000000000
      000F43505520427265616B706F696E7473290000000000000000000000FFFFFF
      FFFFFFFFFF0000000000000000104D656D6F7279206C6F636174696F6E732800
      00000000000000000000FFFFFFFFFFFFFFFF00000000000000000F5253502042
      7265616B706F696E7473}
  end
  object PanelRomname: TPanel
    Left = 0
    Top = 0
    Width = 440
    Height = 29
    Align = alTop
    BevelInner = bvRaised
    BevelOuter = bvLowered
    Caption = 'Nothing loaded...'
    TabOrder = 2
  end
end
