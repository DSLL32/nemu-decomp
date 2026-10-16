unit UnitDebugMemory;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls, Menus, Buttons;

type
  TFormMemory = class(TForm)
    PanelTop: TPanel;
    PanelMem: TPanel;
    LabelAddress: TLabel;
    EditMemAddress: TEdit;
    SpeedButtonPredefined: TSpeedButton;
    PopupMenu: TPopupMenu;
    RDRAM: TMenuItem;
    RDRAMregisters: TMenuItem;
    N1: TMenuItem;
    SPregisters1: TMenuItem;
    DPcommandregisters1: TMenuItem;
    DPspanregisters042000001: TMenuItem;
    N04300000MIregisters1: TMenuItem;
    N04400000VIregisters1: TMenuItem;
    N04500000AIregisters1: TMenuItem;
    N04600000PIregisters1: TMenuItem;
    N04700000RIregisters1: TMenuItem;
    N04800000SIregisters1: TMenuItem;
    N2: TMenuItem;
    N05000000cartridgedomain21: TMenuItem;
    N06000000cartridgedomain11: TMenuItem;
    N08000000cartridgedomain22: TMenuItem;
    N10000000cartridgedomain11: TMenuItem;
    N3: TMenuItem;
    PIFBootRom1: TMenuItem;
    N1FC00000PIFBootRom1: TMenuItem;
    N1FD00000cartridgedomain11: TMenuItem;
    ButtonRefresh: TButton;
    EditMem: TEdit;
    PaintBox: TPaintBox;
    MenuItemStackPointerSP: TMenuItem;
    N4: TMenuItem;
    ButtonClose: TButton;
    ButtonDump: TButton;
    ButtonSearch: TButton;
    CheckBoxAddress: TCheckBox;
    CheckBoxHex: TCheckBox;
    CheckBoxAscii: TCheckBox;
    CheckBoxWords: TCheckBox;
    CheckBoxHalfwords: TCheckBox;
    CheckBoxBytes: TCheckBox;
    ButtonBookmark: TButton;
    GroupBoxWatchType: TGroupBox;
    CheckBoxRead: TCheckBox;
    CheckBoxWrite: TCheckBox;
    procedure PaintBoxPaint(Sender: TObject);
    procedure EditMemAddressChange(Sender: TObject);
    procedure MemoryAddressClick(Sender: TObject);
    procedure SpeedButtonPredefinedClick(Sender: TObject);
    procedure ButtonRefreshClick(Sender: TObject);
    procedure PaintBoxMouseDown(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
    procedure EditMemKeyPress(Sender: TObject; var Key: Char);
    procedure MenuItemStackPointerSPClick(Sender: TObject);
    procedure ButtonCloseClick(Sender: TObject);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure ButtonDumpClick(Sender: TObject);
    procedure ButtonSearchClick(Sender: TObject);
    procedure CheckBoxClick(Sender: TObject);
    procedure EditMemExit(Sender: TObject);
    procedure ButtonBookmarkClick(Sender: TObject);
    procedure FormKeyUp(Sender: TObject; var Key: Word; Shift: TShiftState);
  public
    procedure RefreshMemory;
  end;

var
  FormMemory: TFormMemory;

function ResolvePhysicalAddress(Address: Cardinal): Cardinal;

implementation

uses UnitDecs, UnitDump, UnitSearch, UnitBookmarks;

{$R *.dfm}

var
  WatchColor: TColor;

function ResolvePhysicalAddress(Address: Cardinal): Cardinal;
begin
  Result := TranslateAddress(Address);
  if Nemu64_API.TranslateError^ then
    Result := Address;
  Nemu64_API.TranslateError^ := False;
end;

procedure TFormMemory.RefreshMemory;
begin
  EditMem.Hide;
  PaintBoxPaint(PaintBox);
end;

function IsWatched(Address: Cardinal): Boolean;
var
  I: Cardinal;
begin
  Result := False;
  Address := ResolvePhysicalAddress(Address);
  I := 0;
  while I < Nemu64_API.WatchCount^ do
  begin
    if Address = Nemu64_API.Watches^[I].Address then
    begin
      case Nemu64_API.Watches^[I].WatchType of
        1: WatchColor := $AEFFFF;
        2: WatchColor := $FFAEAE;
        3: WatchColor := $D7FFAE;
      end;
      Result := True;
      Break;
    end;
    Inc(I);
  end;
end;

procedure TFormMemory.PaintBoxPaint(Sender: TObject);
var
  Address, Value: Cardinal;
  HexText, AsciiText, WordText, HalfwordText, ByteText: string;
  Row: Cardinal;
  H, X, Y: Integer;

  procedure AppendWord(Offset: Cardinal; Separator: string);
  begin
    Value := ReadWord(ResolvePhysicalAddress(Address + Offset));
    if not Nemu64_API.TranslateError^ and not Nemu64_API.ReadError^ then
    begin
      HexText := HexText + IntToHex(Value, 8) + Separator;
      AsciiText := AsciiText + Char(Value shr 24) + Char(Value shr 16) +
        Char(Value shr 8) + Chr(Value);
      WordText := WordText + Format('%10d', [Int64(Value)]) + Separator;
      HalfwordText := HalfwordText + Format('%5d %5d',
        [Value shr 16, Value and $FFFF]) + Separator;
      ByteText := ByteText + Format('%3d %3d %3d %3d',
        [(Value shr 24) and $FF, (Value shr 16) and $FF,
         (Value shr 8) and $FF, Value and $FF]) + Separator;
    end
    else
    begin
      HexText := HexText + '********' + Separator;
      AsciiText := AsciiText + '****';
      WordText := WordText + '**********' + Separator;
      HalfwordText := HalfwordText + '***** *****' + Separator;
      ByteText := ByteText + '** ** ** ** **' + Separator;
      Nemu64_API.TranslateError^ := False;
      Nemu64_API.ReadError^ := False;
    end;
  end;

begin
  with (Sender as TPaintBox).Canvas do
  begin
    Brush.Color := clWhite;
    FillRect((Sender as TPaintBox).ClientRect);
    H := TextHeight('A');
    if ((Sender as TPaintBox).Height - 1) div H - 1 >= 0 then
      for Row := 0 to ((Sender as TPaintBox).Height - 1) div H - 1 do
      begin
        Address := (StrToIntDef('$' + EditMemAddress.Text, 0) and $FFFFFFFC) + Row * 16;
        X := 0;
        Y := Row * H;
        HexText := '';
        AsciiText := '';
        WordText := '';
        HalfwordText := '';
        ByteText := '';
        AppendWord(0, ' ');
        AppendWord(4, ' - ');
        AppendWord(8, ' ');
        AppendWord(12, '');
        if CheckBoxAddress.Checked then
        begin
          TextOut(X, Y, IntToHex(Address, 8) + ':');
          Inc(X, TextWidth('00000000:'));
        end;
        if CheckBoxHex.Checked then
        begin
          Brush.Color := $40FFFB;
          if FormSearch.ListBoxSearchResults.Items.IndexOf('0x' + IntToHex(Address, 8)) <> -1 then
            FillRect(Rect(X, Y, X + TextWidth('00000000'), Y + H));
          if FormSearch.ListBoxSearchResults.Items.IndexOf('0x' + IntToHex(Address + 4, 8)) <> -1 then
            FillRect(Rect(X + 72, Y, X + 72 + TextWidth('00000000'), Y + H));
          if FormSearch.ListBoxSearchResults.Items.IndexOf('0x' + IntToHex(Address + 8, 8)) <> -1 then
            FillRect(Rect(X + 160, Y, X + 160 + TextWidth('00000000'), Y + H));
          if FormSearch.ListBoxSearchResults.Items.IndexOf('0x' + IntToHex(Address + 12, 8)) <> -1 then
            FillRect(Rect(X + 232, Y, X + 232 + TextWidth('00000000'), Y + H));
          if IsWatched(Address) then
          begin
            Brush.Color := WatchColor;
            FillRect(Rect(X, Y, X + TextWidth('00000000'), Y + H));
          end;
          if IsWatched(Address + 4) then
          begin
            Brush.Color := WatchColor;
            FillRect(Rect(X + 72, Y, X + 72 + TextWidth('00000000'), Y + H));
          end;
          if IsWatched(Address + 8) then
          begin
            Brush.Color := WatchColor;
            FillRect(Rect(X + 160, Y, X + 160 + TextWidth('00000000'), Y + H));
          end;
          if IsWatched(Address + 12) then
          begin
            Brush.Color := WatchColor;
            FillRect(Rect(X + 232, Y, X + 232 + TextWidth('00000000'), Y + H));
          end;
          Brush.Color := clWhite;
          Brush.Style := bsClear;
          TextOut(X, Y, HexText);
          Brush.Style := bsSolid;
          Inc(X, TextWidth(HexText + ' '));
        end;
        if CheckBoxAscii.Checked then
        begin
          TextOut(X, Y, AsciiText);
          Inc(X, TextWidth(AsciiText + ' '));
        end;
        if CheckBoxWords.Checked then
        begin
          TextOut(X, Y, WordText);
          Inc(X, TextWidth(WordText + ' '));
        end;
        if CheckBoxHalfwords.Checked then
        begin
          TextOut(X, Y, HalfwordText);
          Inc(X, TextWidth(HalfwordText + ' '));
        end;
        if CheckBoxBytes.Checked then
          TextOut(X, Y, ByteText);
      end;
  end;
end;

procedure TFormMemory.EditMemAddressChange(Sender: TObject);
begin
  RefreshMemory;
end;

procedure TFormMemory.MemoryAddressClick(Sender: TObject);
var
  S: string;
begin
  S := (Sender as TMenuItem).Caption;
  Delete(S, 1, 1);
  Delete(S, 5, 1);
  EditMemAddress.Text := Copy(S, 1, 8);
end;

procedure TFormMemory.SpeedButtonPredefinedClick(Sender: TObject);
var
  P: TPoint;
begin
  P := (Sender as TSpeedButton).ClientToScreen(Point(0, (Sender as TSpeedButton).ClientRect.Bottom));
  PopupMenu.Popup(P.X, P.Y);
end;

procedure TFormMemory.ButtonRefreshClick(Sender: TObject);
begin
  PaintBox.Refresh;
end;

procedure TFormMemory.PaintBoxMouseDown(Sender: TObject; Button: TMouseButton; Shift: TShiftState; X, Y: Integer);
var
  H, W, Offset: Integer;
  Address: Cardinal;
  I, WatchType: Integer;
  Found: Cardinal;
begin
  with (Sender as TPaintBox).Canvas do
  begin
    if CheckBoxHex.Checked then
    begin
      H := TextHeight('0');
      W := TextWidth('0');
      if Button = mbLeft then
      begin
        if CheckBoxAddress.Checked then
          Offset := X - PaintBox.Canvas.TextWidth('00000000:')
        else
          Offset := X;
        if Offset < 0 then Exit;
        if Offset > W * 8 then Dec(Offset, W);
        if Offset > W * 16 then Dec(Offset, W * 3);
        if Offset > W * 24 then Dec(Offset, W);
        if Offset > W * 32 then Exit;
        Offset := (Offset div W div 2) and $FFFFFFFC;
        Address := StrToIntDef('$' + EditMemAddress.Text, 0) and $FFFFFFFC;
        Inc(Address, (Y div H) * 16 + Offset);
        EditMem.Text := IntToHex(ReadWord(ResolvePhysicalAddress(Address)), 8);
        EditMem.Tag := Address;
        case Offset of
          0: EditMem.Left := 2;
          4: EditMem.Left := 74;
          8: EditMem.Left := 162;
          12: EditMem.Left := 234;
        end;
        if CheckBoxAddress.Checked then
          EditMem.Left := EditMem.Left + PaintBox.Canvas.TextWidth('00000000:');
        EditMem.Top := Y - Y mod H;
        EditMem.Show;
        EditMem.SetFocus;
        EditMem.SelectAll;
      end
      else
      begin
        if CheckBoxAddress.Checked then
          Offset := X - PaintBox.Canvas.TextWidth('00000000:')
        else
          Offset := X;
        if Offset < 0 then Exit;
        if Offset > W * 8 then Dec(Offset, W);
        if Offset > W * 16 then Dec(Offset, W * 3);
        if Offset > W * 24 then Dec(Offset, W);
        if Offset > W * 32 then Exit;
        Offset := (Offset div W div 2) and $FFFFFFFC;
        Address := StrToIntDef('$' + EditMemAddress.Text, 0) and $FFFFFFFC;
        Inc(Address, Offset + (Y div H) * 16);
        if IsWatched(Address) then
        begin
          Address := ResolvePhysicalAddress(Address);
          Found := 0;
          for I := 0 to Nemu64_API.WatchCount^ - 1 do
            if Address = Nemu64_API.Watches^[I].Address then
            begin
              Found := I;
              Break;
            end;
          Dec(Nemu64_API.WatchCount^);
          if Nemu64_API.WatchCount^ - Found <> 0 then
            for I := Found + 1 to Nemu64_API.WatchCount^ do
              Nemu64_API.Watches^[I - 1] := Nemu64_API.Watches^[I];
        end
        else
        begin
          if CheckBoxRead.Checked then WatchType := 1 else WatchType := 0;
          if CheckBoxWrite.Checked then WatchType := WatchType or 2;
          if WatchType = 0 then
            MessageBox(0, 'Set watch type', 'Watch', 0)
          else
          begin
            Nemu64_API.Watches^[Nemu64_API.WatchCount^].Address := ResolvePhysicalAddress(Address);
            Nemu64_API.Watches^[Nemu64_API.WatchCount^].WatchType := WatchType;
            Inc(Nemu64_API.WatchCount^);
          end;
        end;
        PaintBox.Refresh;
        Nemu64_API.DebugFlags^ := Nemu64_API.DebugFlags^ or $400;
      end;
    end;
  end;
end;

procedure TFormMemory.EditMemKeyPress(Sender: TObject; var Key: Char);
begin
  if Key = #13 then
  begin
    WriteWord(ResolvePhysicalAddress(EditMem.Tag), StrToIntDef('$' + EditMem.Text, 0));
    EditMem.Hide;
  end;
  if Key = #27 then EditMem.Hide;
end;

procedure TFormMemory.MenuItemStackPointerSPClick(Sender: TObject);
begin
  EditMemAddress.Text := IntToHex(Cardinal(Nemu64_API.CPURegisters^[29]), 8);
end;

procedure TFormMemory.ButtonCloseClick(Sender: TObject);
begin
  Close;
end;

procedure TFormMemory.FormClose(Sender: TObject; var Action: TCloseAction);
begin
  Action := caHide;
end;

procedure TFormMemory.ButtonDumpClick(Sender: TObject);
begin
  FormDump.Show;
end;

procedure TFormMemory.ButtonSearchClick(Sender: TObject);
begin
  FormSearch.Show;
end;

procedure TFormMemory.CheckBoxClick(Sender: TObject);
begin
  PaintBox.Refresh;
end;

procedure TFormMemory.EditMemExit(Sender: TObject);
begin
  EditMem.Hide;
end;

procedure TFormMemory.ButtonBookmarkClick(Sender: TObject);
var
  Node: TTreeNode;
begin
  FormBookmarks.Show;
  if FormBookmarks.Notebook.PageIndex = 1 then
    FormBookmarks.ButtonCreate.Click;
  if FormBookmarks.TreeViewBookmarks.Selected = nil then
    Node := FormBookmarks.TreeViewBookmarks.Items.AddFirst(nil, '')
  else
    Node := FormBookmarks.TreeViewBookmarks.Items.AddChildFirst(FormBookmarks.TreeViewBookmarks.Selected, '');
  if Node.Parent <> nil then
    Node.Parent.Expand(False);
  Node.Text := 'MEM 0x' + EditMemAddress.Text + ': ';
  Node.EditText;
end;

procedure TFormMemory.FormKeyUp(Sender: TObject; var Key: Word; Shift: TShiftState);
begin
  if Key = VK_F5 then RefreshMemory;
end;

end.
