/* Simulate a native scrollbar's animation paints around a real edit control.
   The reporter must preserve them, including paints from native timers. */
#include "StaticCrash.cpp"
#include <stdlib.h>
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); ExitProcess(5); } } while (0)
static WNDPROC editProcedure;
static unsigned nativeCalls;
static const COLORREF nativeMarker=RGB(11,71,123);
static bool paints(UINT message)
{
    return message==WM_NCPAINT || message==WM_PAINT || message==WM_HSCROLL || message==WM_VSCROLL ||
        message==WM_NCMOUSEMOVE || message==WM_NCLBUTTONUP || message==WM_TIMER;
}
static LRESULT CALLBACK nativeProcedure(HWND window,UINT message,WPARAM w,LPARAM l)
{
    LRESULT result=CallWindowProcA(editProcedure,window,message,w,l);
    if (paints(message)) {
        ++nativeCalls;
        RECT outer; GetWindowRect(window,&outer);
        int width=outer.right-outer.left,height=outer.bottom-outer.top;
        RECT vertical={width-GetSystemMetrics(SM_CXVSCROLL),20,width-3,height-20};
        RECT horizontal={20,height-GetSystemMetrics(SM_CYHSCROLL),width-20,height-3};
        HDC dc=GetWindowDC(window); HBRUSH brush=CreateSolidBrush(nativeMarker);
        FillRect(dc,&vertical,brush); FillRect(dc,&horizontal,brush);
        DeleteObject(brush); ReleaseDC(window,dc);
    }
    return result;
}
static bool nativePixels(HWND window)
{
    RECT outer; GetWindowRect(window,&outer);
    int width=outer.right-outer.left,height=outer.bottom-outer.top;
    HDC dc=GetWindowDC(window);
    bool preserved=GetPixel(dc,width-8,height/2)==nativeMarker && GetPixel(dc,width/2,height-8)==nativeMarker;
    ReleaseDC(window,dc); return preserved;
}
int main(void)
{
    application=GetModuleHandleA(NULL);
    HWND parent=CreateWindowExA(0,"STATIC","Scroll paint probe",WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        20,20,340,280,NULL,NULL,application,NULL);
    REQUIRE(parent!=NULL);
    char text[10000]="";
    for (unsigned line=0;line<80;++line)
        lstrcatA(text,"A report line long enough to require horizontal scrolling in a small details control.\r\n");
    HWND edit=CreateWindowExA(WS_EX_CLIENTEDGE,"EDIT",text,WS_CHILD|WS_VISIBLE|WS_VSCROLL|WS_HSCROLL|
        ES_MULTILINE|ES_READONLY|ES_AUTOHSCROLL|ES_AUTOVSCROLL,10,10,300,200,parent,NULL,application,NULL);
    REQUIRE(edit!=NULL);
    dialogBrush=CreateSolidBrush(dialogBackground); reportBrush=CreateSolidBrush(reportBackground);
    editProcedure=(WNDPROC)GetWindowLongA(edit,GWL_WNDPROC); originalReportProc=nativeProcedure;
    nativeReportScrollbars=true;
    SetWindowLongA(edit,GWL_WNDPROC,(LONG)reportProc);
    UINT messages[]={WM_NCPAINT,WM_PAINT,WM_VSCROLL,WM_HSCROLL,WM_NCMOUSEMOVE,WM_NCLBUTTONUP,WM_TIMER};
    WPARAM params[]={1,0,SB_LINEDOWN,SB_LINERIGHT,HTVSCROLL,HTVSCROLL,reportScrollTimer};
    for (unsigned pass=0;pass<8;++pass) for (unsigned i=0;i<sizeof(messages)/sizeof(messages[0]);++i) {
        unsigned before=nativeCalls;
        SendMessage(edit,messages[i],params[i],0);
        REQUIRE(nativeCalls>before); REQUIRE(nativePixels(edit));
        MSG message;
        while (PeekMessage(&message,NULL,0,0,PM_REMOVE)) DispatchMessage(&message);
        REQUIRE(nativePixels(edit));
    }
    REQUIRE(SendMessage(edit,EM_GETFIRSTVISIBLELINE,0,0)>0);
    nativeReportScrollbars=false;
    SendMessage(edit,WM_VSCROLL,SB_LINEUP,0);
    REQUIRE(!nativePixels(edit));
    DestroyWindow(parent); DeleteObject(dialogBrush); DeleteObject(reportBrush);
    puts("PASS: native scrollbar paints survive scrolling, hover and timers; custom fallback still paints");
    return 0;
}
