unit UnitSearch;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, ExtCtrls, ComCtrls, Menus, Buttons;

type
  TFormSearch = class(TForm)
    GroupBoxWhere: TGroupBox;
    LabelSearchWhere: TLabel;
    ButtonRDRAM: TButton;
    ButtonRom: TButton;
    EditSearchInStart: TEdit;
    EditSearchInEnd: TEdit;
    ButtonSPMEM: TButton;
    GroupBoxWhat: TGroupBox;
    RadioButtonSearchValue: TRadioButton;
    EditSearchValue: TEdit;
    GroupBoxResults: TGroupBox;
    PanelResultsTop: TPanel;
    ButtonSearch: TButton;
    ListBoxSearchResults: TListBox;
    CheckBoxHex: TCheckBox;
    ComboBoxBits: TComboBox;
    EditMaxCount: TEdit;
    LabelMax: TLabel;
    ButtonClearResults: TButton;
    RadioButtonSearchText: TRadioButton;
    EditSearchText: TEdit;
    CheckBoxCaseSensitive: TCheckBox;
    RadioButtonJALTo: TRadioButton;
    EditJAL: TEdit;
    CheckBoxDblClickCommands: TCheckBox;
    CheckBoxFast: TCheckBox;
    procedure ButtonRDRAMClick(Sender: TObject);
    procedure ButtonRomClick(Sender: TObject);
    procedure EditValuesChange(Sender: TObject);
    procedure ButtonSPMEMClick(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure ButtonSearchClick(Sender: TObject);
    procedure ListBoxSearchResultsDblClick(Sender: TObject);
    procedure ButtonClearResultsClick(Sender: TObject);
    procedure RadioButtonSearchClick(Sender: TObject);
  private
    SearchValue: Cardinal;
    SearchHalfWord: Word;
    SearchByte: Byte;
    SearchText: string;
  end;

var
  FormSearch: TFormSearch;

implementation

uses UnitDecs, UnitDebugCommands, UnitDebugMemory;

{$R *.dfm}

procedure TFormSearch.ButtonRDRAMClick(Sender: TObject);
begin
  EditSearchInStart.Text := '0';
  EditSearchInEnd.Text := IntToHex(Nemu64_API.RamSize^, 0);
end;

procedure TFormSearch.ButtonRomClick(Sender: TObject);
begin
  EditSearchInStart.Text := '10000000';
  EditSearchInEnd.Text := IntToHex(Nemu64_API.RomSize^ + $10000000, 8);
end;

procedure TFormSearch.EditValuesChange(Sender: TObject);
var
  S: string;
begin
  if (StrToIntDef('$' + EditSearchInStart.Text, 1) <> 1) and
     (StrToIntDef('$' + EditSearchInEnd.Text, 1) <> 1) and
     (StrToIntDef(EditMaxCount.Text, 0) <> 0) then
    ButtonSearch.Enabled := True
  else
    ButtonSearch.Enabled := False;
  if RadioButtonSearchValue.Checked then
  begin
    if CheckBoxHex.Checked then
      S := '$' + EditSearchValue.Text
    else
      S := EditSearchValue.Text;
    try
      SearchValue := StrToInt(S);
    except
      ButtonSearch.Enabled := False;
    end;
    SearchHalfWord := Word(SearchValue);
    SearchByte := Byte(SearchValue);
  end;
  if RadioButtonSearchText.Checked then
  begin
    if CheckBoxCaseSensitive.Checked then
      SearchText := EditSearchText.Text
    else
      SearchText := UpperCase(EditSearchText.Text);
  end;
  if RadioButtonJALTo.Checked then
  begin
    S := '$' + EditJAL.Text;
    try
      SearchValue := ((StrToInt(S) and $0FFFFFFF) shr 2) or $0C000000;
    except
      ButtonSearch.Enabled := False;
    end;
  end;
end;

procedure TFormSearch.ButtonSPMEMClick(Sender: TObject);
begin
  EditSearchInStart.Text := '04000000';
  EditSearchInEnd.Text := '04002000';
end;

procedure TFormSearch.FormCreate(Sender: TObject);
begin
  ComboBoxBits.ItemIndex := 2;
  RadioButtonSearchClick(RadioButtonSearchValue);
  ButtonRDRAM.Click;
end;

procedure TFormSearch.ButtonSearchClick(Sender: TObject);
var
  Count: Cardinal;
  SkipTable: array[Char] of Byte;
  CachedAddress: Cardinal;
  CachedWord: Cardinal;
  CaseSensitive: Boolean;
  MaxCount, EndAddress, Total, TextLength: Cardinal;
  Address, Progress, I: Cardinal;
  Bits: Integer;

  procedure AddResult(S: string);
  begin
    ListBoxSearchResults.Items.Add(S);
    Inc(Count);
  end;

  procedure MakeSkipTable(const S: string);
  var
    I: Integer;
  begin
    FillChar(SkipTable, SizeOf(SkipTable), Length(S));
    for I := 1 to Length(S) - 1 do
      SkipTable[S[I]] := Length(S) - I;
  end;

  function ReadChar(Address: Cardinal): Char;
  begin
    if Address and $FFFFFFFC <> CachedAddress then
    begin
      CachedWord := ReadWord(Address and $FFFFFFFC);
      CachedAddress := Address and $FFFFFFFC;
    end;
    case Address and 3 of
      3: if CaseSensitive then
           Result := Char(CachedWord)
         else
           Result := UpCase(Char(CachedWord));
      2: if CaseSensitive then
           Result := Char(CachedWord shr 8)
         else
           Result := UpCase(Char(CachedWord shr 8));
      1: if CaseSensitive then
           Result := Char(CachedWord shr 16)
         else
           Result := UpCase(Char(CachedWord shr 16));
      0: if CaseSensitive then
           Result := Char(CachedWord shr 24)
         else
           Result := UpCase(Char(CachedWord shr 24));
    else
      Result := 'A';
    end;
  end;

  procedure FastSearch(const S: string; Start, Size: Cardinal);
  var
    Position, ScanPosition: Cardinal;
    I: Integer;
  begin
    Position := Length(S);
    while Position <= Size do
    begin
      ScanPosition := Position;
      I := Length(S);
      while I >= 1 do
      begin
        if ReadChar(Start + ScanPosition) <> S[I] then
          I := -1
        else
        begin
          Dec(I);
          Dec(ScanPosition);
        end;
      end;
      if I = 0 then
      begin
        AddResult('0x' + IntToHex(Start + ScanPosition + 1, 8));
        if Count >= MaxCount then Break;
      end;
      if CaseSensitive then
        Inc(Position, SkipTable[ReadChar(Start + Position)])
      else
        Inc(Position, SkipTable[UpCase(ReadChar(Start + Position))]);
      if Position and $FFFF = 0 then
        Caption := 'Searching... (' + IntToStr(Position * 100 div Size) + '% complete)';
    end;
  end;

begin
  ListBoxSearchResults.Items.Clear;
  Count := 0;
  Progress := 0;
  Address := StrToIntDef('$' + EditSearchInStart.Text, 0);
  EndAddress := StrToIntDef('$' + EditSearchInEnd.Text, 0);
  MaxCount := StrToInt(EditMaxCount.Text);
  CachedAddress := 0;
  if RadioButtonSearchValue.Checked then
  begin
    Bits := ComboBoxBits.ItemIndex;
    Total := (EndAddress - Address) shr 2;
    while (Address < EndAddress) and (Count < MaxCount) do
    begin
      CachedWord := ReadWord(Address);
      case Bits of
        0: begin
          if Byte(CachedWord shr 24) = SearchByte then
            AddResult('0x' + IntToHex(Address, 8));
          if Byte(CachedWord shr 16) = SearchByte then
            AddResult('0x' + IntToHex(Address + 1, 8));
          if Byte(CachedWord shr 8) = SearchByte then
            AddResult('0x' + IntToHex(Address + 2, 8));
          if Byte(CachedWord) = SearchByte then
            AddResult('0x' + IntToHex(Address + 3, 8));
        end;
        1: begin
          if Word(CachedWord shr 16) = SearchHalfWord then
            AddResult('0x' + IntToHex(Address, 8));
          if SearchHalfWord = Word(CachedWord) then
            AddResult('0x' + IntToHex(Address + 2, 8));
        end;
        2: if SearchValue = CachedWord then
          AddResult('0x' + IntToHex(Address, 8));
      end;
      Inc(Address, 4);
      Inc(Progress);
      if Progress and $FFFFF = 0 then
        Caption := 'Searching... (' + IntToStr(Progress * 100 div Total) + '% complete)';
    end;
  end;
  if RadioButtonSearchText.Checked then
  begin
    CaseSensitive := CheckBoxCaseSensitive.Checked;
    if not CheckBoxFast.Checked then
    begin
      Total := EndAddress - Address;
      TextLength := Length(SearchText);
      while Address < EndAddress do
      begin
        I := 1;
        while (I <= TextLength) and (ReadChar(Address + I - 1) = SearchText[I]) do
          Inc(I);
        if I > TextLength then
        begin
          AddResult('0x' + IntToHex(Address, 8));
          if Count >= MaxCount then Break;
        end;
        Inc(Address);
        Inc(Progress);
        if Progress and $FFFFF = 0 then
          Caption := 'Searching... (' + IntToStr(Progress * 100 div Total) + '% complete)';
      end;
    end
    else if CaseSensitive then
    begin
      MakeSkipTable(SearchText);
      FastSearch(SearchText, Address, EndAddress - Address);
    end
    else
    begin
      MakeSkipTable(UpperCase(SearchText));
      FastSearch(UpperCase(SearchText), Address, EndAddress - Address);
    end;
  end;
  if RadioButtonJALTo.Checked then
  begin
    Total := (EndAddress - Address) shr 2;
    while (Address < EndAddress) and (Count < MaxCount) do
    begin
      CachedWord := ReadWord(Address);
      if SearchValue = CachedWord then
        AddResult('0x' + IntToHex(Address, 8));
      Inc(Address, 4);
      Inc(Progress);
      if Progress and $FFFFF = 0 then
        Caption := 'Searching... (' + IntToStr(Progress * 100 div Total) + '% complete)';
    end;
  end;
  if Count < MaxCount then
    Caption := IntToStr(Count) + ' results found'
  else
    Caption := 'Too many results';
end;

procedure TFormSearch.ListBoxSearchResultsDblClick(Sender: TObject);
begin
  if ListBoxSearchResults.ItemIndex <> -1 then
    if CheckBoxDblClickCommands.Checked then
    begin
      FormDebug.EditCPUPC.Text := Copy(ListBoxSearchResults.Items[ListBoxSearchResults.ItemIndex], 3, 8);
      FormDebug.Show;
    end
    else
    begin
      FormMemory.EditMemAddress.Text := Copy(ListBoxSearchResults.Items[ListBoxSearchResults.ItemIndex], 3, 8);
      FormMemory.Show;
    end;
end;

procedure TFormSearch.ButtonClearResultsClick(Sender: TObject);
begin
  ListBoxSearchResults.Items.Clear;
end;

procedure TFormSearch.RadioButtonSearchClick(Sender: TObject);
begin
  EditSearchValue.Enabled := RadioButtonSearchValue.Checked;
  CheckBoxHex.Enabled := RadioButtonSearchValue.Checked;
  ComboBoxBits.Enabled := RadioButtonSearchValue.Checked;
  EditSearchText.Enabled := RadioButtonSearchText.Checked;
  CheckBoxCaseSensitive.Enabled := RadioButtonSearchText.Checked;
  CheckBoxFast.Enabled := RadioButtonSearchText.Checked;
  EditJAL.Enabled := RadioButtonJALTo.Checked;
end;

end.
