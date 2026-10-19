/* The reporter owns its controls and GDI objects. Never call the main UI's
   subclasses, VCL, or theme hooks: that thread may be blocked at the fault. */
typedef HRESULT (WINAPI *CrashDwmAttribute)(HWND,DWORD,LPCVOID,DWORD);
typedef HRESULT (WINAPI *CrashWindowTheme)(HWND,LPCWSTR,LPCWSTR);
typedef bool (WINAPI *CrashAllowWindow)(HWND,bool);
static HMODULE crashDwm, crashTheme;
static CrashDwmAttribute crashSetAttribute;
static CrashWindowTheme crashSetWindowTheme;
static CrashAllowWindow crashAllowWindow;
static bool details, dialogDark, nativeReportScrollbars;
static HWND detailEdit, detailButton;
static WNDPROC originalReportProc;
static HFONT dialogFont, reportFont;
static HBRUSH dialogBrush, reportBrush;
static int dialogDpi;
static const UINT repaintReportFrame=WM_APP+0x54b;
static const UINT_PTR reportScrollTimer=0x4e435241;
enum { ID_DETAILS = 1001, ID_COPY = 1002, ID_REPORT = 1003 };
static const COLORREF dialogBackground = RGB(40,40,40), reportBackground = RGB(25,25,25), dialogText = RGB(232,232,232);

extern "C" void __cdecl NemuStaticCrashSetDark(BOOL enabled) { InterlockedExchange(&preferDark,enabled != FALSE); }
static HMODULE crashSystemLibrary(const char *name)
{
    char path[MAX_PATH];
    UINT length = GetSystemDirectoryA(path,sizeof(path));
    if (!length || length+1+lstrlenA(name) >= sizeof(path)) return NULL;
    path[length++] = '\\'; lstrcpyA(path+length,name);
    return LoadLibraryA(path);
}
static void initializeDialog(void)
{
    // Resolve optional modern exports before a fault can hold the loader lock.
    // XP never looks up undocumented ordinals or needs either DLL.
    typedef LONG (WINAPI *Version)(OSVERSIONINFOW *);
    Version version = (Version)GetProcAddress(GetModuleHandleA("ntdll.dll"),"RtlGetVersion");
    OSVERSIONINFOW os; ZeroMemory(&os,sizeof(os)); os.dwOSVersionInfoSize=sizeof(os);
    if (!version || version(&os) || os.dwMajorVersion < 10 || os.dwBuildNumber < 17763) return;
    crashDwm = crashSystemLibrary("dwmapi.dll");
    if (crashDwm) crashSetAttribute=(CrashDwmAttribute)GetProcAddress(crashDwm,"DwmSetWindowAttribute");
    crashTheme = crashSystemLibrary("uxtheme.dll");
    if (crashTheme) {
        crashSetWindowTheme=(CrashWindowTheme)GetProcAddress(crashTheme,"SetWindowTheme");
        crashAllowWindow=(CrashAllowWindow)GetProcAddress(crashTheme,MAKEINTRESOURCEA(133));
    }
}
static void shutdownDialog(void)
{
    if (crashDwm) FreeLibrary(crashDwm);
    if (crashTheme) FreeLibrary(crashTheme);
    crashDwm=crashTheme=NULL;
    crashSetAttribute=NULL; crashSetWindowTheme=NULL; crashAllowWindow=NULL;
    NemuStaticCrashSetDark(FALSE);
}
static int dialogUnits(int value) { return MulDiv(value,dialogDpi,96); }
static void dialogSize(HWND window, bool center)
{
    RECT rect={0,0,dialogUnits(680),dialogUnits(details ? 438 : 150)};
    DWORD style=GetWindowLong(window,GWL_STYLE), extended=GetWindowLong(window,GWL_EXSTYLE);
    AdjustWindowRectEx(&rect,style,FALSE,extended);
    RECT work={0,0,GetSystemMetrics(SM_CXSCREEN),GetSystemMetrics(SM_CYSCREEN)};
    SystemParametersInfoA(SPI_GETWORKAREA,0,&work,0);
    int width=rect.right-rect.left,height=rect.bottom-rect.top;
    SetWindowPos(window,center ? HWND_TOPMOST : NULL,
        work.left+((work.right-work.left)-width)/2,work.top+((work.bottom-work.top)-height)/2,
        width,height,center ? SWP_SHOWWINDOW : SWP_NOMOVE|SWP_NOZORDER);
}
static HWND dialogControl(HWND window, const char *cls, const char *text, DWORD style, DWORD extended,
                          int x,int y,int width,int height,int id)
{
    HWND control=CreateWindowExA(extended,cls,text,WS_CHILD|style,dialogUnits(x),dialogUnits(y),
        dialogUnits(width),dialogUnits(height),window,(HMENU)id,application,NULL);
    SendMessage(control,WM_SETFONT,(WPARAM)dialogFont,FALSE);
    if (dialogDark && crashAllowWindow) crashAllowWindow(control,true);
    return control;
}
static void dialogFonts(HWND window)
{
    HDC dc=GetDC(window); dialogDpi=GetDeviceCaps(dc,LOGPIXELSY); ReleaseDC(window,dc);
    if (dialogDpi <= 0) dialogDpi=96;
    NONCLIENTMETRICSA metrics; ZeroMemory(&metrics,sizeof(metrics)); metrics.cbSize=sizeof(metrics);
    LOGFONTA font; ZeroMemory(&font,sizeof(font));
    if (SystemParametersInfoA(SPI_GETNONCLIENTMETRICS,sizeof(metrics),&metrics,0)) font=metrics.lfMessageFont;
    else GetObject(GetStockObject(DEFAULT_GUI_FONT),sizeof(font),&font);
    dialogFont=CreateFontIndirectA(&font);
    // Match the system message font's size; ANSI_FIXED_FONT is a large legacy
    // stock Courier font. Lucida Console is available on XP as well.
    font.lfWidth=0; font.lfWeight=FW_NORMAL; font.lfItalic=FALSE;
    font.lfPitchAndFamily=FIXED_PITCH|FF_MODERN; lstrcpyA(font.lfFaceName,"Lucida Console");
    reportFont=CreateFontIndirectA(&font);
}
static void reportScrollBar(HWND window,HDC dc,LONG object,const RECT &outer)
{
    SCROLLBARINFO info; ZeroMemory(&info,sizeof(info)); info.cbSize=sizeof(info);
    if (!GetScrollBarInfo(window,object,&info) ||
        (info.rgstate[0]&(STATE_SYSTEM_INVISIBLE|STATE_SYSTEM_OFFSCREEN))) return;
    RECT rect=info.rcScrollBar; OffsetRect(&rect,-outer.left,-outer.top);
    if (IsRectEmpty(&rect)) return;
    FillRect(dc,&rect,reportBrush);
    bool vertical=object==OBJID_VSCROLL;
    int arrowSize=info.dxyLineButton;
    // Fall back when an implementation reports invalid button geometry.
    int thickness=vertical ? rect.right-rect.left : rect.bottom-rect.top;
    if (arrowSize<4 || arrowSize>2*thickness)
        arrowSize=GetSystemMetrics(vertical ? SM_CYVSCROLL : SM_CXHSCROLL);
    RECT first=rect,last=rect,thumb=rect;
    if (vertical) {
        first.bottom=first.top+arrowSize; last.top=last.bottom-arrowSize;
        thumb.top=rect.top+info.xyThumbTop; thumb.bottom=rect.top+info.xyThumbBottom;
    } else {
        first.right=first.left+arrowSize; last.left=last.right-arrowSize;
        thumb.left=rect.left+info.xyThumbTop; thumb.right=rect.left+info.xyThumbBottom;
    }
    for (unsigned end=0;end<2;++end) {
        RECT arrow=end ? last : first; FillRect(dc,&arrow,dialogBrush);
        HPEN pen=CreatePen(PS_SOLID,1,(info.rgstate[end ? 5 : 1]&STATE_SYSTEM_UNAVAILABLE) ? RGB(150,150,150) : dialogText);
        HGDIOBJ old=SelectObject(dc,pen);
        int x=(arrow.left+arrow.right)/2,y=(arrow.top+arrow.bottom)/2;
        for (int i=0;i<4;++i) {
            int direction=end ? i : -i;
            if (vertical) { MoveToEx(dc,x-3+i,y-1+direction,NULL); LineTo(dc,x+4-i,y-1+direction); }
            else { MoveToEx(dc,x-1+direction,y-3+i,NULL); LineTo(dc,x-1+direction,y+4-i); }
        }
        SelectObject(dc,old); DeleteObject(pen);
    }
    if (info.xyThumbBottom>info.xyThumbTop && !(info.rgstate[3]&STATE_SYSTEM_INVISIBLE)) {
        InflateRect(&thumb,-2,-2);
        HBRUSH brush=CreateSolidBrush((info.rgstate[3]&STATE_SYSTEM_PRESSED) ? RGB(140,140,140) : RGB(100,100,100));
        FillRect(dc,&thumb,brush); DeleteObject(brush);
    }
}
static void reportFrame(HWND window)
{
    HDC dc=GetWindowDC(window); if (!dc) return;
    RECT outer; GetWindowRect(window,&outer);
    // Native themes paint and animate their own scrollbars during tracking.
    // Overpainting them after native drawing alternates two thumb shapes.
    if (!nativeReportScrollbars) {
        reportScrollBar(window,dc,OBJID_HSCROLL,outer); reportScrollBar(window,dc,OBJID_VSCROLL,outer);
        SCROLLBARINFO horizontal,vertical; ZeroMemory(&horizontal,sizeof(horizontal)); ZeroMemory(&vertical,sizeof(vertical));
        horizontal.cbSize=sizeof(horizontal); vertical.cbSize=sizeof(vertical);
        if (GetScrollBarInfo(window,OBJID_HSCROLL,&horizontal) && GetScrollBarInfo(window,OBJID_VSCROLL,&vertical) &&
            !((horizontal.rgstate[0]|vertical.rgstate[0])&(STATE_SYSTEM_INVISIBLE|STATE_SYSTEM_OFFSCREEN))) {
            RECT corner={vertical.rcScrollBar.left,horizontal.rcScrollBar.top,vertical.rcScrollBar.right,horizontal.rcScrollBar.bottom};
            OffsetRect(&corner,-outer.left,-outer.top); FillRect(dc,&corner,dialogBrush);
        }
    }
    RECT edge={0,0,outer.right-outer.left,outer.bottom-outer.top};
    HBRUSH border=CreateSolidBrush(RGB(80,80,80));
    FrameRect(dc,&edge,border); InflateRect(&edge,-1,-1); FrameRect(dc,&edge,border); DeleteObject(border);
    ReleaseDC(window,dc);
}
static LRESULT CALLBACK reportProc(HWND window,UINT message,WPARAM w,LPARAM l)
{
    if (!nativeReportScrollbars && (message==repaintReportFrame || (message==WM_TIMER && w==reportScrollTimer))) {
        reportFrame(window); return 0;
    }
    bool tracking=!nativeReportScrollbars && message==WM_NCLBUTTONDOWN && (w==HTHSCROLL || w==HTVSCROLL);
    if (tracking) SetTimer(window,reportScrollTimer,30,NULL);
    LRESULT result=CallWindowProcA(originalReportProc,window,message,w,l);
    if (tracking && IsWindow(window)) { KillTimer(window,reportScrollTimer); reportFrame(window); }
    if (message==WM_PAINT || message==WM_NCPAINT || message==WM_SIZE || (!nativeReportScrollbars &&
        (message==WM_HSCROLL || message==WM_VSCROLL || message==WM_NCMOUSEMOVE || message==WM_NCLBUTTONUP))) reportFrame(window);
    if (!nativeReportScrollbars && (message==WM_HSCROLL || message==WM_VSCROLL || message==WM_NCMOUSEMOVE || message==WM_NCLBUTTONUP))
        PostMessage(window,repaintReportFrame,0,0);
    return result;
}
static INT_PTR CALLBACK dialogProc(HWND window, UINT message, WPARAM w, LPARAM l)
{
    if (message == WM_INITDIALOG) {
        SetWindowTextA(window,"Nemu64 - Application Error"); details=false;
        dialogDark=InterlockedCompareExchange(&preferDark,0,0)!=0;
        dialogFonts(window);
        if (dialogDark) {
            dialogBrush=CreateSolidBrush(dialogBackground); reportBrush=CreateSolidBrush(reportBackground);
            if (crashAllowWindow) crashAllowWindow(window,true);
            if (crashSetAttribute) {
                BOOL enabled=TRUE;
                if (FAILED(crashSetAttribute(window,20,&enabled,sizeof(enabled))))
                    crashSetAttribute(window,19,&enabled,sizeof(enabled));
            }
        }
        HWND icon=dialogControl(window,"STATIC",NULL,WS_VISIBLE|SS_ICON,0,18,18,32,32,0);
        SendMessage(icon,STM_SETICON,(WPARAM)LoadIcon(NULL,IDI_ERROR),0);
        dialogControl(window,"STATIC","Nemu64 encountered an error and must close.\r\n\r\nPlease copy the details when reporting this problem.",
            WS_VISIBLE,0,64,18,480,80,0);
        DWORD button=WS_VISIBLE|WS_TABSTOP|(dialogDark ? BS_OWNERDRAW : BS_PUSHBUTTON);
        dialogControl(window,"BUTTON","Close",button|(dialogDark ? 0 : BS_DEFPUSHBUTTON),0,572,18,90,26,IDOK);
        detailButton=dialogControl(window,"BUTTON","Details >>",button,0,572,58,90,26,ID_DETAILS);
        dialogControl(window,"BUTTON","Copy report",button,0,18,110,112,26,ID_COPY);
        detailEdit=dialogControl(window,"EDIT",report,WS_TABSTOP|WS_VSCROLL|WS_HSCROLL|
            ES_READONLY|ES_MULTILINE|ES_AUTOVSCROLL|ES_AUTOHSCROLL,WS_EX_CLIENTEDGE,18,150,644,270,ID_REPORT);
        SendMessage(detailEdit,WM_SETFONT,(WPARAM)reportFont,FALSE);
        SendMessage(detailEdit,EM_SETLIMITTEXT,sizeof(report),0);
        nativeReportScrollbars=dialogDark && crashSetWindowTheme &&
            SUCCEEDED(crashSetWindowTheme(detailEdit,L"DarkMode_Explorer",NULL));
        if (dialogDark) originalReportProc=(WNDPROC)SetWindowLongA(detailEdit,GWL_WNDPROC,(LONG)reportProc);
        dialogSize(window,true); return TRUE;
    }
    if (dialogDark) {
        if (message == WM_CTLCOLORDLG) return (INT_PTR)dialogBrush;
        if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT) {
            bool edit=(HWND)l==detailEdit;
            SetTextColor((HDC)w,dialogText); SetBkColor((HDC)w,edit ? reportBackground : dialogBackground);
            return (INT_PTR)(edit ? reportBrush : dialogBrush);
        }
        if (message == WM_DRAWITEM) {
            DRAWITEMSTRUCT *item=(DRAWITEMSTRUCT *)l;
            if (item->CtlType != ODT_BUTTON) return FALSE;
            HDC dc=item->hDC; RECT rect=item->rcItem;
            HBRUSH face=CreateSolidBrush((item->itemState&ODS_SELECTED) ? RGB(65,65,65) : RGB(50,50,50));
            FillRect(dc,&rect,face); DeleteObject(face);
            HBRUSH edge=CreateSolidBrush((item->itemState&ODS_FOCUS) ? RGB(130,160,190) : RGB(90,90,90));
            FrameRect(dc,&rect,edge); DeleteObject(edge);
            char text[64]; GetWindowTextA(item->hwndItem,text,sizeof(text));
            HGDIOBJ previous=SelectObject(dc,dialogFont); SetBkMode(dc,TRANSPARENT);
            SetTextColor(dc,(item->itemState&ODS_DISABLED) ? RGB(150,150,150) : dialogText);
            if (item->itemState&ODS_SELECTED) OffsetRect(&rect,1,1);
            DrawTextA(dc,text,-1,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
            if ((item->itemState&ODS_FOCUS) && !(item->itemState&ODS_NOFOCUSRECT)) {
                InflateRect(&rect,-4,-4); DrawFocusRect(dc,&rect);
            }
            SelectObject(dc,previous); return TRUE;
        }
    }
    if (message == WM_COMMAND) {
        if (LOWORD(w)==IDOK || LOWORD(w)==IDCANCEL) { EndDialog(window,IDOK); return TRUE; }
        if (LOWORD(w)==ID_COPY) { copyReport(window); return TRUE; }
        if (LOWORD(w)==ID_DETAILS) {
            details=!details; dialogSize(window,false);
            ShowWindow(detailEdit,details ? SW_SHOW : SW_HIDE);
            SetWindowTextA(detailButton,details ? "Details <<" : "Details >>"); return TRUE;
        }
    }
    if (message == WM_CLOSE) { EndDialog(window,IDOK); return TRUE; }
    if (message == WM_DESTROY) {
        if (dialogFont) DeleteObject(dialogFont);
        if (reportFont) DeleteObject(reportFont);
        if (dialogBrush) DeleteObject(dialogBrush);
        if (reportBrush) DeleteObject(reportBrush);
        dialogFont=reportFont=NULL; dialogBrush=reportBrush=NULL;
    }
    return FALSE;
}
