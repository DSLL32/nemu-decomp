/* The reporter owns its controls and GDI objects. Never call the main UI's
   subclasses, VCL, or theme hooks: that thread may be blocked at the fault. */
typedef HRESULT (WINAPI *CrashDwmAttribute)(HWND,DWORD,LPCVOID,DWORD);
typedef HRESULT (WINAPI *CrashWindowTheme)(HWND,LPCWSTR,LPCWSTR);
typedef bool (WINAPI *CrashAllowWindow)(HWND,bool);
static HMODULE crashDwm, crashTheme;
static CrashDwmAttribute crashSetAttribute;
static CrashWindowTheme crashSetWindowTheme;
static CrashAllowWindow crashAllowWindow;
static bool details, dialogDark, crashNativeTheme;
static HWND detailEdit, detailButton;
static HFONT dialogFont, reportFont;
static HBRUSH dialogBrush, reportBrush;
static int dialogDpi;
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
    crashNativeTheme = os.dwBuildNumber >= 26200;
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
    crashNativeTheme=false;
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
    if (dialogDark) {
        if (crashAllowWindow) crashAllowWindow(control,true);
        if (crashSetWindowTheme && (!lstrcmpA(cls,"BUTTON") || !lstrcmpA(cls,"EDIT"))) {
            if (!crashNativeTheme || FAILED(crashSetWindowTheme(control,L"DarkMode_DarkTheme",NULL)))
                crashSetWindowTheme(control,L"DarkMode_Explorer",NULL);
        }
    }
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
        DWORD button=WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON;
        dialogControl(window,"BUTTON","Close",button|BS_DEFPUSHBUTTON,0,572,18,90,26,IDOK);
        detailButton=dialogControl(window,"BUTTON","Details >>",button,0,572,58,90,26,ID_DETAILS);
        dialogControl(window,"BUTTON","Copy report",button,0,18,110,112,26,ID_COPY);
        detailEdit=dialogControl(window,"EDIT",report,WS_TABSTOP|WS_VSCROLL|WS_HSCROLL|
            ES_READONLY|ES_MULTILINE|ES_AUTOVSCROLL|ES_AUTOHSCROLL,WS_EX_CLIENTEDGE,18,150,644,270,ID_REPORT);
        SendMessage(detailEdit,WM_SETFONT,(WPARAM)reportFont,FALSE);
        SendMessage(detailEdit,EM_SETLIMITTEXT,sizeof(report),0);
        dialogSize(window,true); return TRUE;
    }
    if (dialogDark) {
        if (message == WM_CTLCOLORDLG) return (INT_PTR)dialogBrush;
        if (message == WM_CTLCOLORBTN) {
            SetBkColor((HDC)w,dialogBackground);
            return (INT_PTR)dialogBrush;
        }
        if (message == WM_CTLCOLORSTATIC || message == WM_CTLCOLOREDIT) {
            bool edit=(HWND)l==detailEdit;
            SetTextColor((HDC)w,dialogText); SetBkColor((HDC)w,edit ? reportBackground : dialogBackground);
            return (INT_PTR)(edit ? reportBrush : dialogBrush);
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
