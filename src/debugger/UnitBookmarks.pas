unit UnitBookmarks;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls, Menus, Buttons;

type
  TFormBookmarks = class(TForm)
    PanelBottom: TPanel;
    TreeViewBookmarks: TTreeView;
    PanelRomname: TPanel;
    Notebook: TNotebook;
    ButtonSave: TButton;
    ButtonLoad: TButton;
    ButtonRemove: TButton;
    ButtonNewFolder: TButton;
    ButtonRootFolder: TButton;
    ButtonCreate: TButton;
    procedure ButtonNewFolderClick(Sender: TObject);
    procedure ButtonRemoveClick(Sender: TObject);
    procedure ButtonRootFolderClick(Sender: TObject);
    procedure TreeViewBookmarksDragOver(Sender: TObject; Source: TObject; X, Y: Integer; State: TDragState; var Accept: Boolean);
    procedure TreeViewBookmarksDragDrop(Sender: TObject; Source: TObject; X, Y: Integer);
    procedure ButtonSaveClick(Sender: TObject);
    procedure ButtonLoadClick(Sender: TObject);
    procedure TreeViewBookmarksDblClick(Sender: TObject);
    procedure FormDestroy(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure ButtonCreateClick(Sender: TObject);
  private
    BookmarkFileName: string;
    procedure WMReloadBookmarks(var Message: TMessage); message WM_USER + 1;
  end;

var
  FormBookmarks: TFormBookmarks;
  BookmarksFormHandle: THandle;

implementation

uses UnitDecs, UnitDebugCommands, UnitDebugMemory, UnitLibOptions;

{$R *.dfm}

procedure TFormBookmarks.WMReloadBookmarks(var Message: TMessage);
begin
  FormBookmarks.ButtonSave.Click;
  FormBookmarks.ButtonLoad.Click;
end;

procedure TFormBookmarks.ButtonNewFolderClick(Sender: TObject);
var
  Node, NewNode: TTreeNode;
begin
  Node := TreeViewBookmarks.Selected;
  if Node <> nil then
  begin
    NewNode := TreeViewBookmarks.Items.AddChildFirst(Node, '');
    Node.Expand(False);
    NewNode.EditText;
  end;
end;

procedure TFormBookmarks.ButtonRemoveClick(Sender: TObject);
var
  Node: TTreeNode;
begin
  Node := TreeViewBookmarks.Selected;
  if Node <> nil then
    if Application.MessageBox(PChar('Delete ' + Node.Text +
      ' and all its children? Cannot be undone'), 'Confirm delete', MB_YESNO) = IDYES then
      Node.Delete;
end;

procedure TFormBookmarks.ButtonRootFolderClick(Sender: TObject);
var
  Node: TTreeNode;
begin
  Node := TreeViewBookmarks.Items.Add(nil, '');
  Node.EditText;
end;

procedure TFormBookmarks.TreeViewBookmarksDragOver(Sender: TObject; Source: TObject; X, Y: Integer; State: TDragState; var Accept: Boolean);
begin
  Accept := True;
end;

procedure TFormBookmarks.TreeViewBookmarksDragDrop(Sender: TObject; Source: TObject; X, Y: Integer);
begin
  TreeViewBookmarks.Selected.MoveTo(TreeViewBookmarks.GetNodeAt(X, Y), naAddChild);
  TreeViewBookmarks.AlphaSort;
end;

procedure TFormBookmarks.ButtonSaveClick(Sender: TObject);
begin
  if BookmarkFileName <> '' then
    if Notebook.PageIndex = 0 then
      TreeViewBookmarks.SaveToFile(BookmarkFileName);
end;

procedure TFormBookmarks.ButtonLoadClick(Sender: TObject);
var
  RomFileName: string;
  Root: TTreeNode;
begin
  try
    RomFileName := Nemu64_API.RomFilename;
    if RomFileName <> '' then
    begin
      RomFileName := ExtractFileName(RomFileName);
      SetLength(RomFileName, Length(RomFileName) - Length(ExtractFileExt(RomFileName)));
      BookmarkFileName := NemuFileDir + RomFileName + '.nbm';
      if FileExists(BookmarkFileName) then
      begin
        TreeViewBookmarks.LoadFromFile(BookmarkFileName);
        Notebook.PageIndex := 0;
        TreeViewBookmarks.Enabled := True;
      end
      else
      begin
        TreeViewBookmarks.Items.Clear;
        Root := TreeViewBookmarks.Items.Add(nil, 'Root');
        TreeViewBookmarks.Items.AddChild(Root, 'CPU');
        TreeViewBookmarks.Items.AddChild(Root, 'Memory');
        TreeViewBookmarks.Items.AddChild(Root, 'RSP');
        TreeViewBookmarks.Enabled := False;
        Notebook.PageIndex := 1;
      end;
      PanelBottom.Enabled := True;
      PanelRomname.Caption := 'Bookmark file: ' + BookmarkFileName;
    end
    else
    begin
      BookmarkFileName := '';
      PanelRomname.Caption := 'Nothing loaded...';
      PanelBottom.Enabled := False;
      TreeViewBookmarks.Enabled := False;
      Notebook.PageIndex := 0;
    end;
  finally
    TreeViewBookmarks.FullExpand;
  end;
end;

procedure TFormBookmarks.TreeViewBookmarksDblClick(Sender: TObject);
var
  Node: TTreeNode;
  Address: string;
begin
  try
    Node := TreeViewBookmarks.Selected;
    if Node <> nil then
    begin
      Address := Copy(Node.Text, 7, 8);
      case Node.Text[1] of
        'R': begin
          FormDebug.Show;
          FormDebug.EditRSPPC.Text := Copy(Address, Length(Address) - 2, 3);
          FormDebug.PageControl.ActivePage := FormDebug.TabSheetRSP;
        end;
        'C': begin
          FormDebug.Show;
          FormDebug.EditCPUPC.Text := Address;
          FormDebug.PageControl.ActivePage := FormDebug.TabSheetCPU;
        end;
        'M': begin
          FormMemory.Show;
          FormMemory.EditMemAddress.Text := Address;
        end;
      end;
    end;
  except
  end;
end;

procedure TFormBookmarks.FormDestroy(Sender: TObject);
begin
  ButtonSave.Click;
end;

procedure TFormBookmarks.FormCreate(Sender: TObject);
begin
  BookmarksFormHandle := Handle;
  ButtonLoad.Click;
end;

procedure TFormBookmarks.ButtonCreateClick(Sender: TObject);
begin
  Notebook.PageIndex := 0;
  TreeViewBookmarks.Enabled := True;
end;

end.
