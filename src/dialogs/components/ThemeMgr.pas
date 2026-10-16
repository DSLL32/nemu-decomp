unit ThemeMgr;

// Reconstructed linked behavior for the April 2002 dialogs.dll, using the
// newer reference below. See README.txt for changes and provenance.
// The version/history and author notices below describe that reference.

//----------------------------------------------------------------------------------------------------------------------
// Version 1.10.1
//
// Windows XP Theme Manager is freeware. You may freely use it in any software, including commercial software, provided
// you accept the following conditions:
//
// 1) The software may not be included into component collections and similar compilations which are sold. If you want
//    to distribute this software for money then contact me first and ask for my permission.
// 2) My copyright notices in the source code may not be removed or modified.
// 3) If you modify and/or distribute the code to any third party then you must not veil the original author. It must
//    always be clearly identifiable that I, Mike Lischke, am the original author.
// Although it is not required it would be a nice move to recognize my work by adding a citation to the application's
// about box or a similar place.
//
// The original code is ThemeMgr.pas, released 01. January 2002.
//
// The initial developer of the original code is:
//   Mike Lischke (public@soft-gems.net, www.soft-gems.net).
//
// Portions created by Mike Lischke are
// (C) 2001-2005 Mike Lischke. All Rights Reserved.
//----------------------------------------------------------------------------------------------------------------------
//
// This unit contains the implementation of TThemeManager which is designed to fix certain VCL components to enable
// XP theme support in Delphi and BCB applications (version 6 and lower).
//
// TThemeManager uses global theming (all windows in the application use the same theme). Hence you don't
// need more than one instance in an application (except for DLLs). Having more than one instance in the same module
// (application, DLL) will disable subclassing of controls by all other but the first instance.
//
// Note: If you are using a Theme Manager in a DLL then make sure the handle of the application object in the DLL (which
//       is usually not allocated) is set to that of the main application, e.g. by passing it via an exported function.
//----------------------------------------------------------------------------------------------------------------------
//
// January 2005
//   - Bug fix: Test for Windows XP was wrong.
//
// For full development history see help file.
//
// Credits for their valuable help go to:
//   Bert Moorthaemer, Rob Schoenaker, John W. Long, Vassiliev V.V., Steve Moss, Torsten Detsch, Milan Vandrovec
//----------------------------------------------------------------------------------------------------------------------

interface

{$I Compilers.inc}

uses
  Windows, Classes, Messages, Graphics, Controls, StdCtrls, Buttons, Forms, ThemeSrv;

type
  TThemeOption = (
    toAllowNonClientArea, toAllowControls, toAllowWebContent,
    toSubclassAnimate, toSubclassButtons, toSubclassGroupBox,
    toSubclassListView, toSubclassTabSheet, toSubclassStatusBar,
    toSubclassTrackBar, toResetMouseCapture, toSetTransparency
  );
  TThemeOptions = set of TThemeOption;

const
  DefaultThemeOptions = [toAllowNonClientArea..toAllowWebContent, toSubclassButtons..toSetTransparency];

type
  TThemeManager = class;

  TWindowProcList = class(TList)
  private
    FOwner: TThemeManager;
    FNewWindowProc: TWndMethod;
    FControlClass: TControlClass;
  public
    constructor Create(Owner: TThemeManager; WindowProc: TWndMethod; ControlClass: TControlClass);
    function Add(Control: TControl): Integer;
    procedure Clear; override;
    procedure DispatchMessage(Control: TControl; var Message: TMessage);
    function Find(Control: TControl; out Index: Integer): Boolean;
    procedure Remove(Control: TControl);
  end;

  TThemeManager = class(TComponent)
  private
    FOptions: TThemeOptions;
    FListViewList, FTabSheetList, FGroupBoxList, FButtonControlList,
    FSpeedButtonList, FTrackBarList, FAnimateList, FStatusBarList,
    FFormList: TWindowProcList;
    FPendingFormsList: TList;
    FSubclassingDisabled: Boolean;
    FThemeServices: TThemeServices;
    FOnThemeChange: TNotifyEvent;
    procedure AnimateWindowProc(Control: TControl; var Message: TMessage);
    procedure ButtonControlWindowProc(Control: TControl; var Message: TMessage);
    procedure FormWindowProc(Control: TControl; var Message: TMessage);
    procedure GroupBoxWindowProc(Control: TControl; var Message: TMessage);
    procedure ListviewWindowProc(Control: TControl; var Message: TMessage);
    function MainWindowHook(var Message: TMessage): Boolean;
    procedure SetThemeOptions(const Value: TThemeOptions);
    procedure SpeedButtonWindowProc(Control: TControl; var Message: TMessage);
    procedure StatusBarWindowProc(Control: TControl; var Message: TMessage);
    procedure TabSheetWindowProc(Control: TControl; var Message: TMessage);
    procedure TrackBarWindowProc(Control: TControl; var Message: TMessage);
    procedure PreAnimateWindowProc(var Message: TMessage);
    procedure PreButtonControlWindowProc(var Message: TMessage);
    procedure PreFormWindowProc(var Message: TMessage);
    procedure PreGroupBoxWindowProc(var Message: TMessage);
    procedure PreListviewWindowProc(var Message: TMessage);
    procedure PreSpeedButtonWindowProc(var Message: TMessage);
    procedure PreStatusBarWindowProc(var Message: TMessage);
    procedure PreTabSheetWindowProc(var Message: TMessage);
    procedure PreTrackBarWindowProc(var Message: TMessage);
  protected
    procedure BroadcastThemeChange;
    procedure DoOnThemeChange; virtual;
    procedure DrawBitBtn(Control: TBitBtn; var DrawItemStruct: TDrawItemStruct);
    procedure DrawButton(Control: TControl; Button: TThemedButton; DC: HDC; R: TRect; Focused: Boolean);
    procedure FixControls(Form: TCustomForm = nil);
    procedure HandleControlChange(Control: TControl; Inserting: Boolean); virtual;
    procedure Loaded; override;
    procedure Notification(AComponent: TComponent; Operation: TOperation); override;
    procedure RemoveChildSubclassing(Control: TWinControl);
    procedure UpdateThemes;
  public
    constructor Create(AOwner: TComponent); override;
    destructor Destroy; override;
    procedure CollectForms(Form: TCustomForm = nil);
    procedure CollectControls(Parent: TWinControl);
    procedure PerformEraseBackground(Control: TControl; DC: HDC);
    property ThemeServices: TThemeServices read FThemeServices;
  published
    property Options: TThemeOptions read FOptions write SetThemeOptions default DefaultThemeOptions;
    property OnThemeChange: TNotifyEvent read FOnThemeChange write FOnThemeChange;
  end;

implementation

uses
  ComCtrls, SysUtils, CommCtrl, SyncObjs, ExtCtrls, JwaUxTheme;

const
  WM_MAINMANAGERRELEASED = CN_NOTIFY + 100;

type
  PWindowProcEntry = ^TWindowProcEntry;
  TWindowProcEntry = record
    Control: TControl;
    OldWndProc: TWndMethod;
  end;
  TControlCast = class(TControl);
  TWinControlCast = class(TWinControl);
  TGroupBoxCast = class(TCustomGroupBox);
  TSpeedButtonCast = class(TSpeedButton);
  TCustomStatusBarCast = class(TCustomStatusBar);
  TLabelCast = class(TCustomLabel);
  TWMPrint = packed record
    Msg: Cardinal;
    DC: HDC;
    Flags: Cardinal;
    Result: Integer;
  end;
  TWMPrintClient = TWMPrint;

var
  Lock: TCriticalSection;
  MainManager: TThemeManager;

//----------------------------------------------------------------------------------------------------------------------
procedure CalcButtonLayout(Control: TControl; DC: HDC; const Client: TRect; const Offset: TPoint; var GlyphPos: TPoint;
  var TextBounds: TRect; BiDiFlags: Integer);

// This routine is nearly the same as the same named version in TButtonGlyph. The inclusion here is necessary
// because we need the same layout as in the VCL but the implementation of TButtonGlyph is hidden in Buttons and
// cannot be made accessible from here.

var
  TextPos: TPoint;
  ClientSize,
  GlyphSize,
  TextSize: TPoint;
  TotalSize: TPoint;
  Layout: TButtonLayout;
  Spacing: Integer;
  Margin: Integer;
  Glyph: TBitmap;
  NumGlyphs: Integer;
  Caption: TCaption;

begin
  if Control is TBitBtn then
  begin
    Layout := TBitBtn(Control).Layout;
    Spacing := TBitBtn(Control).Spacing;
    Margin := TBitBtn(Control).Margin;
    Glyph := TBitBtn(Control).Glyph;
    NumGlyphs := TBitBtn(Control).NumGlyphs;
    Caption := TBitBtn(Control).Caption;
  end
  else
  begin
    Layout := TSpeedButton(Control).Layout;
    Spacing := TSpeedButton(Control).Spacing;
    Margin := TSpeedButton(Control).Margin;
    Glyph := TSpeedButton(Control).Glyph;
    NumGlyphs := TSpeedButton(Control).NumGlyphs;
    Caption := TSpeedButton(Control).Caption;
  end;

  if (BiDiFlags and DT_RIGHT) = DT_RIGHT then
    if Layout = blGlyphLeft then
      Layout := blGlyphRight
    else
      if Layout = blGlyphRight then
        Layout := blGlyphLeft;

  // Calculate the item sizes.
  ClientSize := Point(Client.Right - Client.Left, Client.Bottom - Client.Top);

  if Assigned(Glyph) then
    GlyphSize := Point(Glyph.Width div NumGlyphs, Glyph.Height)
  else
    GlyphSize := Point(0, 0);

  if Length(Caption) > 0 then
  begin
    TextBounds := Rect(0, 0, Client.Right - Client.Left, 0);
    Windows.DrawText(DC, PChar(Caption), Length(Caption), TextBounds, DT_CALCRECT or BiDiFlags);
    TextSize := Point(TextBounds.Right - TextBounds.Left, TextBounds.Bottom - TextBounds.Top);
  end
  else
  begin
    TextBounds := Rect(0, 0, 0, 0);
    TextSize := Point(0,0);
  end;

  // If the layout has the glyph on the right or the left, then both the text and the glyph are centered vertically.
  // If the glyph is on the top or the bottom, then both the text and the glyph are centered horizontally.
  if Layout in [blGlyphLeft, blGlyphRight] then
  begin
    GlyphPos.Y := (ClientSize.Y - GlyphSize.Y + 1) div 2;
    TextPos.Y := (ClientSize.Y - TextSize.Y + 1) div 2;
  end
  else
  begin
    GlyphPos.X := (ClientSize.X - GlyphSize.X + 1) div 2;
    TextPos.X := (ClientSize.X - TextSize.X + 1) div 2;
  end;

  // If there is no text or no bitmap, then Spacing is irrelevant.
  if (TextSize.X = 0) or (GlyphSize.X = 0) then
    Spacing := 0;

  // Adjust Margin and Spacing.
  if Margin = -1 then
  begin
    if Spacing = -1 then
    begin
      TotalSize := Point(GlyphSize.X + TextSize.X, GlyphSize.Y + TextSize.Y);
      if Layout in [blGlyphLeft, blGlyphRight] then
        Margin := (ClientSize.X - TotalSize.X) div 3
      else
        Margin := (ClientSize.Y - TotalSize.Y) div 3;
      Spacing := Margin;
    end
    else
    begin
      TotalSize := Point(GlyphSize.X + Spacing + TextSize.X, GlyphSize.Y + Spacing + TextSize.Y);
      if Layout in [blGlyphLeft, blGlyphRight] then
        Margin := (ClientSize.X - TotalSize.X + 1) div 2
      else
        Margin := (ClientSize.Y - TotalSize.Y + 1) div 2;
    end;
  end
  else
  begin
    if Spacing = -1 then
    begin
      TotalSize := Point(ClientSize.X - (Margin + GlyphSize.X), ClientSize.Y - (Margin + GlyphSize.Y));
      if Layout in [blGlyphLeft, blGlyphRight] then
        Spacing := (TotalSize.X - TextSize.X) div 2
      else
        Spacing := (TotalSize.Y - TextSize.Y) div 2;
    end;
  end;

  case Layout of
    blGlyphLeft:
      begin
        GlyphPos.X := Margin;
        TextPos.X := GlyphPos.X + GlyphSize.X + Spacing;
      end;
    blGlyphRight:
      begin
        GlyphPos.X := ClientSize.X - Margin - GlyphSize.X;
        TextPos.X := GlyphPos.X - Spacing - TextSize.X;
      end;
    blGlyphTop:
      begin
        GlyphPos.Y := Margin;
        TextPos.Y := GlyphPos.Y + GlyphSize.Y + Spacing;
      end;
    blGlyphBottom:
      begin
        GlyphPos.Y := ClientSize.Y - Margin - GlyphSize.Y;
        TextPos.Y := GlyphPos.Y - Spacing - TextSize.Y;
      end;
  end;

  // Fixup the result variables.
  with GlyphPos do
  begin
    Inc(X, Client.Left + Offset.X);
    Inc(Y, Client.Top + Offset.Y);
  end;
  OffsetRect(TextBounds, TextPos.X + Client.Left + Offset.X, TextPos.Y + Client.Top + Offset.X);
end;

//----------------------------------------------------------------------------------------------------------------------
constructor TWindowProcList.Create(Owner: TThemeManager; WindowProc: TWndMethod; ControlClass: TControlClass);

begin
  inherited Create;

  FOwner := Owner;
  FNewWindowProc := WindowProc;
  FControlClass := ControlClass;
end;

//----------------------------------------------------------------------------------------------------------------------
function Compare(Item1, Item2: Pointer): Integer;

// Helper function for sort and find in window proc lists. They are sorted by control reference.

begin
  Result := Integer(PWindowProcEntry(Item1).Control) - Integer(PWindowProcEntry(Item2).Control);
end;

//----------------------------------------------------------------------------------------------------------------------
function TWindowProcList.Add(Control: TControl): Integer;

var
  I: Integer;
  Entry: PWindowProcEntry;
begin
  Result := -1;
  if (Control is FControlClass) and not Find(Control, I) then
  begin
    New(Entry);
    Entry.Control := Control;
    Entry.OldWndProc := Control.WindowProc;
    Control.WindowProc := FNewWindowProc;
    Result := inherited Add(Entry);
    Sort(Compare);
  end;
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TWindowProcList.Clear;

begin
  while Count > 0 do
    Remove(PWindowProcEntry(Items[Count - 1]).Control);
  inherited;
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TWindowProcList.DispatchMessage(Control: TControl; var Message: TMessage);

var
  I: Integer;
  Entry: PWindowProcEntry;
begin
  if Find(Control, I) then
  begin
    Entry := Items[I];
    Entry.OldWndProc(Message);
    if Message.Msg = WM_DESTROY then
      if csDestroying in Control.ComponentState then
        Remove(Control);
  end;
end;

//----------------------------------------------------------------------------------------------------------------------
function TWindowProcList.Find(Control: TControl; out Index: Integer): Boolean;

var
  L, H, I, C: Integer;
  Dummy: TWindowProcEntry;
begin
  Result := False;
  Dummy.Control := Control;
  L := 0;
  H := Count - 1;
  while L <= H do
  begin
    I := (L + H) shr 1;
    C := Compare(Items[I], @Dummy);
    if C < 0 then
      L := I + 1
    else
    begin
      H := I - 1;
      if C = 0 then
      begin
        Result := True;
        L := I;
      end;
    end;
  end;
  Index := L;
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TWindowProcList.Remove(Control: TControl);

var
  I: Integer;
  Entry: PWindowProcEntry;
begin
  if Find(Control, I) then
  begin
    Entry := Items[I];
    Delete(I);
    Entry.Control.WindowProc := Entry.OldWndProc;
    if Entry.Control is TWinControl then
      FOwner.RemoveChildSubclassing(Entry.Control as TWinControl);
    Dispose(Entry);
  end;
end;

//----------------------------------------------------------------------------------------------------------------------
constructor TThemeManager.Create(AOwner: TComponent);

begin
  inherited;
  FThemeServices := TThemeServices.Create(0);
  FOptions := DefaultThemeOptions;
  FPendingFormsList := TList.Create;
  FListViewList := TWindowProcList.Create(Self, PreListviewWindowProc, TCustomListView);
  FTabSheetList := TWindowProcList.Create(Self, PreTabSheetWindowProc, TTabSheet);
  FGroupBoxList := TWindowProcList.Create(Self, PreGroupBoxWindowProc, TCustomGroupBox);
  FButtonControlList := TWindowProcList.Create(Self, PreButtonControlWindowProc, TButtonControl);
  FSpeedButtonList := TWindowProcList.Create(Self, PreSpeedButtonWindowProc, TSpeedButton);
  FTrackBarList := TWindowProcList.Create(Self, PreTrackBarWindowProc, TTrackBar);
  FAnimateList := TWindowProcList.Create(Self, PreAnimateWindowProc, TAnimate);
  FStatusBarList := TWindowProcList.Create(Self, PreStatusBarWindowProc, TCustomStatusBar);
  FFormList := TWindowProcList.Create(Self, PreFormWindowProc, TCustomForm);
  if csDesigning in ComponentState then
    FSubclassingDisabled := True
  else
  begin
    Application.HookMainWindow(MainWindowHook);
    Lock.Enter;
    try
      if MainManager = nil then
        MainManager := Self
      else
        FSubclassingDisabled := True;
    finally
      Lock.Leave;
    end;
  end;
end;

//----------------------------------------------------------------------------------------------------------------------
destructor TThemeManager.Destroy;

begin
  FPendingFormsList.Free;
  FFormList.Free;
  FStatusBarList.Free;
  FAnimateList.Free;
  FTrackBarList.Free;
  FSpeedButtonList.Free;
  FButtonControlList.Free;
  FListViewList.Free;
  FTabSheetList.Free;
  FGroupBoxList.Free;
  if not (csDesigning in ComponentState) then
  begin
    Application.UnhookMainWindow(MainWindowHook);
    Lock.Enter;
    try
      if MainManager = Self then
      begin
        MainManager := nil;
        if Application.Handle <> 0 then
          PostMessage(Application.Handle, WM_MAINMANAGERRELEASED, 0, 0);
      end;
    finally
      Lock.Leave;
    end;
  end;
  FThemeServices.Free;
  inherited;
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.AnimateWindowProc(Control: TControl; var Message: TMessage);

begin
    if ThemeServices.ThemesEnabled and (toAllowControls in FOptions) then
    begin
      case Message.Msg of
        WM_ERASEBKGND:
          Message.Result := 1;
        CN_CTLCOLORSTATIC:
          if TAnimate(Control).Transparent then
            with TWMCtlColorStatic(Message) do
            begin
              // Return a brush corresponding to the control's fixed background color.
              // The animation control insists on always erasing its background.
              Result := GetSysColorBrush(TWinControlCast(Control).Color and not $80000000);
              {ThemeServices.DrawParentBackground(TWinControl(Control).Handle, ChildDC, nil, False);
              SetBkMode(ChildDC, TRANSPARENT);
              // Return an empty brush to prevent Windows from overpainting we just have created.
              Result := GetStockObject(NULL_BRUSH);}
            end
          else
            FAnimateList.DispatchMessage(Control, Message);
      else
        FAnimateList.DispatchMessage(Control, Message);
      end;
    end
    else
      FAnimateList.DispatchMessage(Control, Message);
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.ButtonControlWindowProc(Control: TControl; var Message: TMessage);

var
  Details: TThemedElementDetails;

begin
    if ThemeServices.ThemesEnabled and (toAllowControls in FOptions) then
    begin
      case Message.Msg of
        WM_ERASEBKGND:
          Message.Result := 1;
        CN_CTLCOLORBTN: // TButton background erasing. Necessary for some themes (like EclipseOSX).
          with TWMCtlColorBtn(Message) do
          begin
            if TWinControl(Control.Parent).DoubleBuffered then
              PerformEraseBackground(Control, ChildDC)
            else
              ThemeServices.DrawParentBackground(TWinControl(Control).Handle, ChildDC, nil, False);
            // Return an empty brush to prevent Windows from overpainting we just have created.
            Result := GetStockObject(NULL_BRUSH);
          end;
        CN_CTLCOLORSTATIC: // Background erasing for check boxes and radio buttons.
          with TWMCtlColorStatic(Message) do
          begin
            if TWinControl(Control.Parent).DoubleBuffered then
              PerformEraseBackground(Control, ChildDC)
            else
              ThemeServices.DrawParentBackground(TWinControl(Control).Handle, ChildDC, nil, False);
            // Return an empty brush to prevent Windows from overpainting we just have created.
            Result := GetStockObject(NULL_BRUSH);
          end;
        CM_MOUSEENTER,
        CM_MOUSELEAVE:
          begin
            // Hot tracking for owner drawn buttons seems to be unsupported by Windows. So we have to work around that.
            if Control is TBitBtn then
              Control.Invalidate;
          end;
        CN_DRAWITEM: // Painting for owner drawn buttons.
          with TWMDrawItem(Message) do
          begin
            // This message is sent for bit buttons (TBitBtn) when they must be drawn. Since a bit button is a normal
            // Windows button (but with custom draw enabled) it is handled here too.
            // TSpeedButton is a TGraphicControl descentant and handled separately.
            Details := ThemeServices.GetElementDetails(tbPushButtonNormal);
            ThemeServices.DrawParentBackground(TWinControl(Control).Handle, DrawItemStruct.hDC, @Details, True);

            DrawBitBtn(TBitBtn(Control), DrawItemStruct^);
          end;
      else
        FButtonControlList.DispatchMessage(Control, Message);
      end;
    end
    else
      FButtonControlList.DispatchMessage(Control, Message);
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.FormWindowProc(Control: TControl; var Message: TMessage);

begin
  case Message.Msg of
    CM_CONTROLLISTCHANGE:
      with TCMControlListChange(Message) do
        HandleControlChange(Control, Inserting);
  end;
  FFormList.DispatchMessage(Control, Message);
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.GroupBoxWindowProc(Control: TControl; var Message: TMessage);

  //--------------- local function --------------------------------------------

  procedure NewPaint(DC: HDC);

  var
    CaptionRect,
    OuterRect: TRect;
    Size: TSize;
    LastFont: HFONT;
    Box: TThemedButton;
    Details: TThemedElementDetails;

  begin
    with TGroupBoxCast(Control) do
    begin
      LastFont := SelectObject(DC, Font.Handle);
      if Text <> '' then
      begin
        SetTextColor(DC, Graphics.ColorToRGB(Font.Color));
        // Determine size and position of text rectangle.
        // This must be clipped out before painting the frame.
        GetTextExtentPoint32(DC, PChar(Text), Length(Text), Size);
        CaptionRect := Rect(0, 0, Size.cx, Size.cy);
        if not UseRightToLeftAlignment then
          OffsetRect(CaptionRect, 8, 0)
        else
          OffsetRect(CaptionRect, Width - 8 - CaptionRect.Right, 0);
      end
      else
        CaptionRect := Rect(0, 0, 0, 0);

      OuterRect := ClientRect;
      OuterRect.Top := (CaptionRect.Bottom - CaptionRect.Top) div 2;
      with CaptionRect do
        ExcludeClipRect(DC, Left, Top, Right, Bottom);
      if Control.Enabled then
        Box := tbGroupBoxNormal
      else
        Box := tbGroupBoxDisabled;
      Details := ThemeServices.GetElementDetails(Box);
      ThemeServices.DrawElement(DC, Details, OuterRect);

      SelectClipRgn(DC, 0);
      if Text <> '' then
        ThemeServices.DrawText(DC, Details, Text, CaptionRect, DT_LEFT, 0);
      SelectObject(DC, LastFont);
    end;
  end;

  //--------------- local function --------------------------------------------

var
  PS: TPaintStruct;
  Details: TThemedElementDetails;

begin
    if ThemeServices.ThemesEnabled and (toAllowControls in FOptions) then
    begin
      case Message.Msg of
        WM_ERASEBKGND:
          with TWMEraseBkGnd(Message) do
          begin
            // Get the parent to draw its background into the control's background.
            if TWinControl(Control.Parent).DoubleBuffered then
              PerformEraseBackground(Control, DC)
            else
            begin
              Details := ThemeServices.GetElementDetails(tbGroupBoxNormal);
              ThemeServices.DrawParentBackground(TGroupBoxCast(Control).Handle, DC, @Details, True);
            end;
            Result := 1;
          end;
        WM_PAINT:
          begin
            BeginPaint(TGroupBoxCast(Control).Handle, PS);
            NewPaint(PS.hdc);
            TGroupBoxCast(Control).PaintControls(PS.hdc, nil);
            EndPaint(TGroupBoxCast(Control).Handle, PS);
            Message.Result := 0;
          end;
      else
        FGroupBoxList.DispatchMessage(Control, Message);
      end;
    end
    else
      FGroupBoxList.DispatchMessage(Control, Message);
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.ListviewWindowProc(Control: TControl; var Message: TMessage);

begin
  if (Message.Msg = LVM_SETCOLUMN) or (Message.Msg = LVM_INSERTCOLUMN) then
    with PLVColumn(Message.LParam)^ do
      if iImage = -1 then
        Mask := Mask and not LVCF_IMAGE;
  FListViewList.DispatchMessage(Control, Message);
end;

//----------------------------------------------------------------------------------------------------------------------
function TThemeManager.MainWindowHook(var Message: TMessage): Boolean;

// Listens to messages sent to the application to know when a theme change occured.

var
  Form: TCustomForm;
  I: Integer;

begin
  Result := False;

  // If the main manager was destroyed then it posted this message to the application so all still existing
  // theme managers know a new election is due. Well, it is not purely democratic. The earlier a manager was created
  // the higher is the probability to get this message first and become the new main manager.
  if Message.Msg = WM_MAINMANAGERRELEASED then
  begin
    Lock.Enter;
    try
      // Check if the main manager role is still vacant.
      if MainManager = nil then
      begin
        MainManager := Self;
        FSubclassingDisabled := False;
        CollectForms;
      end;
    finally
      Lock.Leave;
    end;
  end;

  if FPendingFormsList.Count > 0 then
  begin
    for I := 0 to FPendingFormsList.Count - 1 do
    begin
      Form := TCustomForm(FPendingFormsList[I]);
      FFormList.Add(Form);
      if MainManager = Self then
        CollectControls(Form);
    end;
    FPendingFormsList.Clear;
  end;

  if Message.Msg = WM_THEMECHANGED then
  begin
    UpdateThemes;
    DoOnThemeChange;
  end;
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.SetThemeOptions(const Value: TThemeOptions);

var
  Flags: Cardinal;
  I: Integer;

begin
  // If this instance is the main manager then apply the options directly. Otherwise let the current main manager do it.
  Lock.Enter;
  try
    if Assigned(MainManager) and (MainManager <> Self) then
      MainManager.Options := Value
    else
    begin
      if FOptions <> Value then
      begin
        FOptions := Value;

        if ThemeServices.ThemesEnabled and not FSubclassingDisabled and not (csDesigning in ComponentState) then
        begin
          Flags := 0;
          if toAllowNonClientArea in FOptions then
            Flags := Flags or STAP_ALLOW_NONCLIENT;
          if toAllowControls in FOptions then
            Flags := Flags or STAP_ALLOW_CONTROLS;
          if toAllowWebContent in FOptions then
            Flags := Flags or STAP_ALLOW_WEBCONTENT;
          SetThemeAppProperties(Flags);
          UpdateThemes;

          // Tell the application that we changed the options.
          BroadcastThemeChange;
          // Notify all theme manager instances about the change.
          SendAppMessage(WM_THEMECHANGED, 0, 0);

          for I := 0 to Screen.FormCount - 1 do
            RedrawWindow(Screen.Forms[I].Handle, nil, 0, RDW_ERASE or RDW_FRAME or RDW_INVALIDATE or RDW_INTERNALPAINT or
              RDW_ERASENOW or RDW_UPDATENOW or RDW_ALLCHILDREN);
        end;
      end;
    end;
  finally
    Lock.Leave;
  end;
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.SpeedButtonWindowProc(Control: TControl; var Message: TMessage);

var
  Button: TThemedButton;
begin
  if ThemeServices.ThemesEnabled and (toAllowControls in FOptions) then
  begin
    case Message.Msg of
      WM_PAINT:
        with TWMPaint(Message) do
        begin
          PerformEraseBackground(Control, DC);
          if not Control.Enabled then
            Button := tbPushButtonDisabled
          else
            with TSpeedButtonCast(Control) do
              if FState in [bsDown, bsExclusive] then
                Button := tbPushButtonPressed
              else if MouseInControl then
                Button := tbPushButtonHot
              else
                Button := tbPushButtonNormal;
          DrawButton(Control, Button, DC, Control.ClientRect, False);
          Message.Result := 0;
        end;
    else
      FSpeedButtonList.DispatchMessage(Control, Message);
    end;
  end
  else
    FSpeedButtonList.DispatchMessage(Control, Message);
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.StatusBarWindowProc(Control: TControl; var Message: TMessage);

begin
  if ThemeServices.ThemesEnabled and (toAllowControls in FOptions) then
  begin
    case Message.Msg of
      WM_NCCALCSIZE:
        with TWMNCCalcSize(Message) do
        begin
          FStatusBarList.DispatchMessage(Control, Message);
          if CalcValidRects then
            Result := Result or WVR_HREDRAW or WVR_VREDRAW;
        end;
      WM_ERASEBKGND:
        with TWMEraseBkGnd(Message) do
        begin
          DrawThemeBackground(ThemeServices.Theme[teStatus], DC, 0, 0, Control.ClientRect, nil);
          Result := 1;
        end;
    else
      FStatusBarList.DispatchMessage(Control, Message);
    end;
  end
  else
    FStatusBarList.DispatchMessage(Control, Message);
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.TabSheetWindowProc(Control: TControl; var Message: TMessage);

var
  DrawRect: TRect;
  Details: TThemedElementDetails;
  DC: HDC;
begin
  if ThemeServices.ThemesEnabled and (toAllowControls in FOptions) then
  begin
    case Message.Msg of
      WM_PRINTCLIENT, WM_ERASEBKGND:
        begin
          if Message.Msg = WM_PRINTCLIENT then
            DC := TWMPrintClient(Message).DC
          else
            DC := TWMEraseBkGnd(Message).DC;
          DrawRect := Control.ClientRect;
          Details := ThemeServices.GetElementDetails(ttBody);
          ThemeServices.DrawParentBackground(TWinControl(Control).Handle, DC, @Details, False);
          Message.Result := 1;
        end;
    else
      FTabSheetList.DispatchMessage(Control, Message);
    end;
  end
  else
    FTabSheetList.DispatchMessage(Control, Message);
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.TrackBarWindowProc(Control: TControl; var Message: TMessage);

var
  Info: PNMCustomDraw;
  R: TRect;
  Rgn: HRGN;
  Details: TThemedElementDetails;
begin
  if ThemeServices.ThemesEnabled and (toAllowControls in FOptions) then
  begin
    case Message.Msg of
      CN_NOTIFY:
        with TWMNotify(Message) do
          if NMHdr.code = NM_CUSTOMDRAW then
          begin
            Info := Pointer(NMHdr);
            case Info.dwDrawStage of
              CDDS_PREPAINT:
                Result := CDRF_NOTIFYITEMDRAW;
              CDDS_ITEMPREPAINT:
                begin
                  case Info.dwItemSpec of
                    TBCD_TICS:
                      begin
                        Details := ThemeServices.GetElementDetails(ttbThumbTics);
                        ThemeServices.DrawParentBackground(TWinControl(Control).Handle, Info.hDC, @Details, False);
                      end;
                    TBCD_CHANNEL:
                      begin
                        SendMessage(TWinControl(Control).Handle, TBM_GETTHUMBRECT, 0, Integer(@R));
                        if TTrackBar(Control).Orientation = trHorizontal then
                        begin
                          R.Left := Control.ClientRect.Left;
                          R.Right := Control.ClientRect.Right;
                        end
                        else
                        begin
                          R.Top := Control.ClientRect.Top;
                          R.Bottom := Control.ClientRect.Bottom;
                        end;
                        with R do
                          Rgn := CreateRectRgn(Left, Top, Right, Bottom);
                        SelectClipRgn(Info.hDC, Rgn);
                        Details := ThemeServices.GetElementDetails(ttbThumbTics);
                        ThemeServices.DrawParentBackground(TWinControl(Control).Handle, Info.hDC, @Details, False);
                        DeleteObject(Rgn);
                        SelectClipRgn(Info.hDC, 0);
                      end;
                  end;
                  Result := CDRF_DODEFAULT;
                end;
            else
              Result := CDRF_DODEFAULT;
            end;
          end;
    else
      FTrackBarList.DispatchMessage(Control, Message);
    end;
  end
  else
    FTrackBarList.DispatchMessage(Control, Message);
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.PreAnimateWindowProc(var Message: TMessage);

asm
  PUSH EBX
  PUSH EAX
  PUSH EBX
  PUSH ECX
  PUSH EDX
  PUSH 4
  PUSH EBX
  CALL IsBadReadPtr
  OR AL, AL
  POP EDX
  POP ECX
  POP EBX
  POP EAX
  JZ @@Valid
  MOV EBX, EBP
@@Valid:
  MOV ECX, EDX
  MOV EDX, EBX
  CALL AnimateWindowProc
  POP EBX
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.PreButtonControlWindowProc(var Message: TMessage);

asm
  PUSH EBX
  PUSH EAX
  PUSH EBX
  PUSH ECX
  PUSH EDX
  PUSH 4
  PUSH EBX
  CALL IsBadReadPtr
  OR AL, AL
  POP EDX
  POP ECX
  POP EBX
  POP EAX
  JZ @@Valid
  MOV EBX, EBP
@@Valid:
  MOV ECX, EDX
  MOV EDX, EBX
  CALL ButtonControlWindowProc
  POP EBX
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.PreFormWindowProc(var Message: TMessage);

asm
  PUSH EBX
  PUSH EAX
  PUSH EBX
  PUSH ECX
  PUSH EDX
  PUSH 4
  PUSH EBX
  CALL IsBadReadPtr
  OR AL, AL
  POP EDX
  POP ECX
  POP EBX
  POP EAX
  JZ @@Valid
  MOV EBX, EBP
@@Valid:
  MOV ECX, EDX
  MOV EDX, EBX
  CALL FormWindowProc
  POP EBX
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.PreGroupBoxWindowProc(var Message: TMessage);

asm
  PUSH EBX
  PUSH EAX
  PUSH EBX
  PUSH ECX
  PUSH EDX
  PUSH 4
  PUSH EBX
  CALL IsBadReadPtr
  OR AL, AL
  POP EDX
  POP ECX
  POP EBX
  POP EAX
  JZ @@Valid
  MOV EBX, EBP
@@Valid:
  MOV ECX, EDX
  MOV EDX, EBX
  CALL GroupBoxWindowProc
  POP EBX
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.PreListviewWindowProc(var Message: TMessage);

asm
  PUSH EBX
  PUSH EAX
  PUSH EBX
  PUSH ECX
  PUSH EDX
  PUSH 4
  PUSH EBX
  CALL IsBadReadPtr
  OR AL, AL
  POP EDX
  POP ECX
  POP EBX
  POP EAX
  JZ @@Valid
  MOV EBX, EBP
@@Valid:
  MOV ECX, EDX
  MOV EDX, EBX
  CALL ListviewWindowProc
  POP EBX
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.PreSpeedButtonWindowProc(var Message: TMessage);

asm
  PUSH EBX
  PUSH EAX
  PUSH EBX
  PUSH ECX
  PUSH EDX
  PUSH 4
  PUSH EBX
  CALL IsBadReadPtr
  OR AL, AL
  POP EDX
  POP ECX
  POP EBX
  POP EAX
  JZ @@Valid
  MOV EBX, EBP
@@Valid:
  MOV ECX, EDX
  MOV EDX, EBX
  CALL SpeedButtonWindowProc
  POP EBX
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.PreStatusBarWindowProc(var Message: TMessage);

asm
  PUSH EBX
  PUSH EAX
  PUSH EBX
  PUSH ECX
  PUSH EDX
  PUSH 4
  PUSH EBX
  CALL IsBadReadPtr
  OR AL, AL
  POP EDX
  POP ECX
  POP EBX
  POP EAX
  JZ @@Valid
  MOV EBX, EBP
@@Valid:
  MOV ECX, EDX
  MOV EDX, EBX
  CALL StatusBarWindowProc
  POP EBX
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.PreTabSheetWindowProc(var Message: TMessage);

asm
  PUSH EBX
  PUSH EAX
  PUSH EBX
  PUSH ECX
  PUSH EDX
  PUSH 4
  PUSH EBX
  CALL IsBadReadPtr
  OR AL, AL
  POP EDX
  POP ECX
  POP EBX
  POP EAX
  JZ @@Valid
  MOV EBX, EBP
@@Valid:
  MOV ECX, EDX
  MOV EDX, EBX
  CALL TabSheetWindowProc
  POP EBX
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.PreTrackBarWindowProc(var Message: TMessage);

asm
  PUSH EBX
  PUSH EAX
  PUSH EBX
  PUSH ECX
  PUSH EDX
  PUSH 4
  PUSH EBX
  CALL IsBadReadPtr
  OR AL, AL
  POP EDX
  POP ECX
  POP EBX
  POP EAX
  JZ @@Valid
  MOV EBX, EBP
@@Valid:
  MOV ECX, EDX
  MOV EDX, EBX
  CALL TrackBarWindowProc
  POP EBX
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.BroadcastThemeChange;

  //--------------- local function --------------------------------------------

  procedure BroadcastChildren(Control: TWinControl);

  var
    I: Integer;
    ChildControl: TWinControl;

  begin
    for I := 0 to Control.ControlCount - 1 do
      if Control.Controls[I] is TWinControl then
      begin
        ChildControl := TWinControl(Control.Controls[I]);
        if ChildControl.HandleAllocated then
          ChildControl.Perform(WM_THEMECHANGED, 0, 0);
        // We must force recreation of some window handles (to reapply all the control settings).
        if (ChildControl is TCustomListView) or (ChildControl is TCoolBar) then
          TWinControlCast(ChildControl).RecreateWnd
        else
          BroadcastChildren(ChildControl);
      end;
  end;

  //--------------- local function --------------------------------------------

var
  I: Integer;
  Form: TCustomForm;

begin
  for I := 0 to Screen.FormCount - 1 do
  begin
    Form := Screen.Forms[I];
    Form.Perform(WM_THEMECHANGED, 0, 0);
    BroadcastChildren(Form);
  end;
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.DoOnThemeChange;

begin
  if Assigned(FOnThemeChange) then
    FOnThemeChange(Self);
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.DrawBitBtn(Control: TBitBtn; var DrawItemStruct: TDrawItemStruct);

var
  Button: TThemedButton;
  R: TRect;

  Wnd: HWND;
  P: TPoint;

begin
  with DrawItemStruct do
  begin
    // For owner drawn buttons we will never get the ODS_HIGHLIGHT flag. This makes it necessary to
    // check ourselves if the button is "hot".
    GetCursorPos(P);
    Wnd := WindowFromPoint(P);
    if Wnd = TWinControl(Control).Handle then
      itemState := itemState or ODS_HOTLIGHT;

    R := rcItem;

    if not Control.Enabled then
      Button := tbPushButtonDisabled
    else
      if (itemState and ODS_SELECTED) <> 0 then
        Button := tbPushButtonPressed
      else
        if (itemState and ODS_HOTLIGHT) <> 0 then
          Button := tbPushButtonHot
        else
          // It seems ODS_DEFAULT is never set, so we have to check the control's properties.
          if Control.Default or ((itemState and ODS_FOCUS) <> 0) then
            Button := tbPushButtonDefaulted
          else
            Button := tbPushButtonNormal;

    DrawButton(Control, Button, hDC, R, itemState and ODS_FOCUS <> 0);
  end;
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.DrawButton(Control: TControl; Button: TThemedButton; DC: HDC; R: TRect; Focused: Boolean);

// Common paint routine for TBitBtn and TSpeedButton.

var
  TextBounds: TRect;
  LastFont: HFONT;
  CaptionText: WideString;
  Glyph: TBitmap;
  GlyphPos: TPoint;
  GlyphWidth: Integer;
  GlyphSourceX: Integer;
  GlyphMask: TBitmap;
  Offset: TPoint;
  ToolButton: TThemedToolBar;
  Details: TThemedElementDetails;

begin
  GlyphSourceX := 0;
  GlyphWidth := 0;

  ToolButton := ttbToolbarDontCare;
  if Control is TBitBtn then
  begin
    Glyph := TBitBtn(Control).Glyph;
    // Determine which image to use (if there is more than one in the glyph).
    with TBitBtn(Control), Glyph do
    begin
      if not Empty then
      begin
        GlyphWidth := Width div NumGlyphs;
        if not Enabled and (NumGlyphs > 1) then
          GlyphSourceX := GlyphWidth
        else
          if (Button = tbPushButtonPressed) and (NumGlyphs > 2) then
            GlyphSourceX := 2 * GlyphWidth;
      end;
    end;
  end
  else
  begin
    Glyph := TSpeedButton(Control).Glyph;
    with TSpeedButtonCast(Control) do
    begin
      // Determine which image to use (if there is more than one in the glyph).
      with Glyph do
        if not Empty then
        begin
          GlyphWidth := Width div NumGlyphs;
          if not Enabled and (NumGlyphs > 1) then
            GlyphSourceX := GlyphWidth
          else
            case FState of
              bsDown:
                if NumGlyphs > 2 then
                  GlyphSourceX := 2 * GlyphWidth;
              bsExclusive:
                if NumGlyphs > 3 then
                  GlyphSourceX := 3 * GlyphWidth;
            end;
        end;
      // If the speed button is flat then we use toolbutton images for drawing.
      if Flat then
      begin
        case Button of
          tbPushButtonDisabled:
            Toolbutton := ttbButtonDisabled;
          tbPushButtonPressed:
            Toolbutton := ttbButtonPressed;
          tbPushButtonHot:
            Toolbutton := ttbButtonHot;
          tbPushButtonNormal:
            Toolbutton := ttbButtonNormal;
        end;
      end;
    end;
  end;

  if ToolButton = ttbToolbarDontCare then
  begin
    Details := ThemeServices.GetElementDetails(Button);
    ThemeServices.DrawElement(DC, Details, R);
    R := ThemeServices.ContentRect(DC, Details, R);
  end
  else
  begin
    Details := ThemeServices.GetElementDetails(ToolButton);
    ThemeServices.DrawElement(DC, Details, R);
    R := ThemeServices.ContentRect(DC, Details, R);
  end;

  // The XP style does no longer indicate pressed buttons by moving the caption one pixel down and right.
  Offset := Point(0, 0);

  with TControlCast(Control) do
  begin
    LastFont := SelectObject(DC, Font.Handle);
    CalcButtonLayout(Control, DC, R, Offset, GlyphPos, TextBounds, DrawTextBidiModeFlags(0));
    CaptionText := Caption;
    ThemeServices.DrawText(DC, Details, CaptionText, TextBounds, DT_CENTER or DT_VCENTER, 0);
    with Glyph do
      if not Empty then
      begin
        GlyphMask := TBitmap.Create;
        GlyphMask.Assign(Glyph);
        GlyphMask.Mask(Glyph.TransparentColor);
        TransparentStretchBlt(DC, GlyphPos.X, GlyphPos.Y, GlyphWidth, Height, Canvas.Handle, GlyphSourceX, 0,
          GlyphWidth, Height, GlyphMask.Canvas.Handle, GlyphSourceX, 0);
        GlyphMask.Free;
      end;
    SelectObject(DC, LastFont);
  end;

  if Focused then
  begin
    SetTextColor(DC, 0);
    DrawFocusRect(DC, R);
  end;
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.FixControls(Form: TCustomForm);

// Iterates through all existing controls in all forms which are registered with Screen and checks for TToolBar and
// TCustomLabel. Both controls will get their Transparent property set to True.

var
  MakeTransparent: Boolean;
  RemoveMouseCapture: Boolean;

  //--------------- local function --------------------------------------------

  procedure IterateControls(Parent: TWinControl);

  var
    I, J: Integer;
    ToolBar: TToolBar;

  begin
    for I := 0 to Parent.ControlCount - 1 do
    begin
      if Parent.Controls[I] is TToolBar then
      begin
        ToolBar := TToolBar(Parent.Controls[I]);
        if MakeTransparent then
          ToolBar.Transparent := True;
        if RemoveMouseCapture then
        begin
          for J := 0 to ToolBar.ButtonCount - 1 do
            if ToolBar.Buttons[J].Style <> tbsDropDown then
              ToolBar.Buttons[J].ControlStyle := ToolBar.Buttons[J].ControlStyle - [csCaptureMouse];
        end;
      end
      else
        if Parent.Controls[I] is TCustomLabel then
        begin
          if MakeTransparent then
            TLabelCast(Parent.Controls[I]).Transparent := True;
        end
        else
          if (Parent.Controls[I] is TWinControl) and (TWinControl(Parent.Controls[I]).ControlCount > 0) then
            IterateControls(Parent.Controls[I] as TWinControl);
    end;
  end;

  //--------------- end local function ----------------------------------------

var
  I: Integer;

begin
  MakeTransparent := toSetTransparency in FOptions;
  RemoveMouseCapture := toResetMouseCapture in FOptions;

  if Form = nil then
  begin
    for I := 0 to Screen.FormCount - 1 do
    begin
      Form := Screen.Forms[I];
      IterateControls(Form);
    end;
  end
  else
    IterateControls(Form);
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.HandleControlChange(Control: TControl; Inserting: Boolean);

var
  List: TWindowProcList;
  Index: Integer;
begin
  if (Control is TCustomListView) and ((toSubclassListView in FOptions) or not Inserting) then
  begin
    List := FListViewList;
    if Inserting and not List.Find(Control, Index) then
      TWinControlCast(Control).DestroyHandle;
  end
  else if (Control is TTabSheet) and ((toSubclassTabSheet in FOptions) or not Inserting) then
  begin
    List := FTabSheetList;
  end
  else if (Control is TCustomGroupBox) and ((toSubclassGroupBox in FOptions) or not Inserting) then
  begin
    List := FGroupBoxList;
  end
  else if (Control is TButtonControl) and ((toSubclassButtons in FOptions) or not Inserting) then
  begin
    List := FButtonControlList;
  end
  else if (Control is TSpeedButton) and ((toSubclassButtons in FOptions) or not Inserting) then
  begin
    List := FSpeedButtonList;
    if Inserting then
      Control.FreeNotification(Self);
  end
  else if (Control is TTrackBar) and ((toSubclassTrackBar in FOptions) or not Inserting) then
  begin
    List := FTrackBarList;
  end
  else if (Control is TAnimate) and ((toSubclassAnimate in FOptions) or not Inserting) then
  begin
    List := FAnimateList;
  end
  else if (Control is TCustomStatusBar) and ((toSubclassStatusBar in FOptions) or not Inserting) then
  begin
    List := FStatusBarList;
  end
  else if (Control is TCustomForm) then
  begin
    List := FFormList;
    if Inserting then
    begin
      Control.FreeNotification(Self);
      CollectControls(Control as TWinControl);
      FPendingFormsList.Remove(Control);
    end;
  end
  else
    List := nil;
  if Assigned(List) then
    if Inserting then
      List.Add(Control)
    else
      List.Remove(Control);
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.Loaded;

begin
  inherited;
  if (MainManager = Self) and not (csDesigning in ComponentState) then
    CollectForms;
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.Notification(AComponent: TComponent; Operation: TOperation);

begin
  if not (csDesigning in ComponentState) then
    case Operation of
      opInsert:
        if (MainManager = Self) and (AComponent is TCustomForm) and (FPendingFormsList.IndexOf(AComponent) < 0) then
          FPendingFormsList.Add(AComponent);
      opRemove:
        if (MainManager = Self) and (AComponent is TControl) then
        begin
          if AComponent is TCustomForm then
            FPendingFormsList.Remove(AComponent);
          HandleControlChange(AComponent as TControl, False);
        end;
    end;
  inherited;
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.PerformEraseBackground(Control: TControl; DC: HDC);

// Repainting the background of a control using theme services relies on the ability of the parent to handle
// WM_PRINT messages. Usually the default behavior of a window is enough to make this possible. However
// double buffered and non-windowed controls are quite different and need so special handling.
// This method uses the WM_ERASEBKGND message to achieve the same effect.

var
  LastOrigin: TPoint;

begin
  GetWindowOrgEx(DC, LastOrigin);
  SetWindowOrgEx(DC, LastOrigin.X + Control.Left, LastOrigin.Y + Control.Top, nil);
  Control.Parent.Perform(WM_ERASEBKGND, Integer(DC), Integer(DC));
  SetWindowOrgEx(DC, LastOrigin.X, LastOrigin.Y, nil);
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.RemoveChildSubclassing(Control: TWinControl);

// Child controls may be released without further notice if their parent control is destroyed.
// One can use the WM_DESTORY message to get notified but if the control haven't even created their window handle
// then also this possibility does not exist anymore.
// Hence when we get notice of a control which is being destroyed then we implicitely remove all subclassed child
// controls from our lists too.

var
  I: Integer;

begin
  for I := 0 to Control.ControlCount - 1 do
    if Control.Controls[I] is TWinControl then
    begin
      RemoveChildSubclassing(TWinControl(Control.Controls[I]));
      HandleControlChange(Control.Controls[I], False);
    end;
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.UpdateThemes;

var
  Flags: Cardinal;

begin
  ThemeServices.UpdateThemes;

  if ThemeServices.ThemesEnabled and not (csDesigning in ComponentState) then
  begin
    Flags := GetThemeAppProperties;
    if (Flags and STAP_ALLOW_NONCLIENT) <> 0 then
      Include(FOptions, toAllowNonClientArea)
    else
      Exclude(FOptions, toAllowNonClientArea);
    if (Flags and STAP_ALLOW_CONTROLS) <> 0 then
      Include(FOptions, toAllowControls)
    else
      Exclude(FOptions, toAllowControls);
    if (Flags and STAP_ALLOW_WEBCONTENT) <> 0 then
      Include(FOptions, toAllowWebContent)
    else
      Exclude(FOptions, toAllowWebContent);
  end;
end;

//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.CollectForms(Form: TCustomForm = nil);

// (Re)initiates collecting all controls which need to be subclassed to fixed one or more problems.

var
  I: Integer;

begin
  if ThemeServices.ThemesEnabled and not FSubclassingDisabled and not (csDesigning in ComponentState) then
  begin
    if Form = nil then
    begin
      FStatusBarList.Clear;
      FAnimateList.Clear;
      FTrackBarList.Clear;
      FSpeedButtonList.Clear;
      FButtonControlList.Clear;
      FListViewList.Clear;
      FTabSheetList.Clear;
      FGroupBoxList.Clear;
      FFormList.Clear;
      for I := 0 to Screen.FormCount - 1 do
      begin
        FFormList.Add(Screen.Forms[I]);
        CollectControls(Screen.Forms[I]);
      end;
    end
    else
    begin
      FFormList.Add(Form);
      CollectControls(Form);
    end;

    if ([toResetMouseCapture, toSetTransparency] * FOptions) <> [] then
      FixControls(Form);
  end;
end;

//----------------------------------------------------------------------------------------------------------------------
procedure TThemeManager.CollectControls(Parent: TWinControl);

var
  I: Integer;

begin
  if ThemeServices.ThemesEnabled and not FSubclassingDisabled and not (csDesigning in ComponentState) then
  begin
    for I := 0 to Parent.ControlCount - 1 do
    begin
      HandleControlChange(Parent.Controls[I], True);
      if (Parent.Controls[I] is TWinControl) and (TWinControl(Parent.Controls[I]).ControlCount > 0) then
        CollectControls(Parent.Controls[I] as TWinControl);
    end;
  end;
end;

//----------------------------------------------------------------------------------------------------------------------
initialization
  Lock := TCriticalSection.Create;
finalization
  Lock.Free;
end.
