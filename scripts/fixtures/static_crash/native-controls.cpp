/* Check the actual crash dialog's native controls and keyboard/scroll behavior.
   Theme entry points are injected because Wine need not implement dark themes. */
#include "StaticCrash.cpp"
#ifndef BS_TYPEMASK
#define BS_TYPEMASK 0x0000000fL
#endif
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); ExitProcess(5); } } while (0)
static HWND themed[4];
static WNDPROC procedures[4];
static unsigned themedCount;
static bool rejectTheme;
static HRESULT WINAPI theme(HWND window,LPCWSTR name,LPCWSTR)
{
    if(rejectTheme && !lstrcmpW(name,L"DarkMode_DarkTheme"))return E_FAIL;
    REQUIRE(!lstrcmpW(name,crashNativeTheme && !rejectTheme ? L"DarkMode_DarkTheme" : L"DarkMode_Explorer"));
    REQUIRE(themedCount<4);
    themed[themedCount]=window;
    procedures[themedCount++]=(WNDPROC)GetWindowLongA(window,GWL_WNDPROC);
    return S_OK;
}
int main(void)
{
    setvbuf(stdout,NULL,_IONBF,0); setvbuf(stderr,NULL,_IONBF,0);
    application=GetModuleHandleA(NULL);
    for (unsigned line=0;line<80;++line)
        lstrcatA(report,"A report line long enough to require horizontal scrolling in the details control. "
            "The module, function, address and other diagnostic columns remain available across a long report row.\r\n");
    for (unsigned capability=0;capability<4;++capability) {
        NemuStaticCrashSetDark(TRUE); themedCount=0;
        crashSetWindowTheme=capability ? theme : NULL;
        crashNativeTheme=capability>=2;rejectTheme=capability==3;
        struct { DLGTEMPLATE dialog; WORD menu,cls,title; } layout;
        ZeroMemory(&layout,sizeof(layout));
        layout.dialog.style=WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME;
        HWND window=CreateDialogIndirectParamA(application,&layout.dialog,NULL,dialogProc,0);
        REQUIRE(window!=NULL);
        REQUIRE((GetWindowLong(GetDlgItem(window,IDOK),GWL_STYLE)&BS_TYPEMASK)==BS_DEFPUSHBUTTON);
        REQUIRE((GetWindowLong(GetDlgItem(window,ID_DETAILS),GWL_STYLE)&BS_TYPEMASK)==BS_PUSHBUTTON);
        REQUIRE((GetWindowLong(GetDlgItem(window,ID_COPY),GWL_STYLE)&BS_TYPEMASK)==BS_PUSHBUTTON);
        REQUIRE(GetNextDlgTabItem(window,NULL,FALSE)==GetDlgItem(window,IDOK));
        HDC dc=GetDC(GetDlgItem(window,IDOK));SetTextColor(dc,RGB(17,35,53));SetBkColor(dc,RGB(255,255,255));
        HBRUSH brush=(HBRUSH)SendMessage(window,WM_CTLCOLORBTN,(WPARAM)dc,(LPARAM)GetDlgItem(window,IDOK));
        LOGBRUSH background;REQUIRE(GetObject(brush,sizeof(background),&background));
        REQUIRE(background.lbColor==dialogBackground && GetBkColor(dc)==dialogBackground);
        REQUIRE(GetTextColor(dc)==RGB(17,35,53));ReleaseDC(GetDlgItem(window,IDOK),dc);
        SendMessage(window,WM_COMMAND,ID_DETAILS,0); REQUIRE(IsWindowVisible(detailEdit));
        SendMessage(detailEdit,WM_VSCROLL,SB_BOTTOM,0);
        REQUIRE(SendMessage(detailEdit,EM_GETFIRSTVISIBLELINE,0,0)>0);
        SendMessage(detailEdit,WM_KEYDOWN,VK_HOME,0);
        SendMessage(detailEdit,WM_VSCROLL,SB_TOP,0);
        REQUIRE(SendMessage(detailEdit,EM_GETFIRSTVISIBLELINE,0,0)==0);
        SCROLLINFO scroll; ZeroMemory(&scroll,sizeof(scroll)); scroll.cbSize=sizeof(scroll); scroll.fMask=SIF_POS;
        SendMessage(detailEdit,WM_HSCROLL,SB_RIGHT,0); REQUIRE(GetScrollInfo(detailEdit,SB_HORZ,&scroll)); REQUIRE(scroll.nPos>0);
        if (capability) {
            REQUIRE(themedCount==4);
            for (unsigned i=0;i<themedCount;++i)
                REQUIRE((WNDPROC)GetWindowLongA(themed[i],GWL_WNDPROC)==procedures[i]);
        }
        SendMessage(window,WM_COMMAND,ID_DETAILS,0); REQUIRE(!IsWindowVisible(detailEdit));
        DestroyWindow(window);
    }
    puts("PASS: crash dialog keeps native buttons and edit/scroll procedures, with and without theme APIs");
    return 0;
}
