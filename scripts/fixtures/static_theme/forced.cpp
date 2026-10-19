/* Test-only adapter: force OS preference on Wine while using the production
   client painter and the actual statically linked VCL/plugin code. */
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
static char mode(void) {
    FILE *file = fopen("theme-mode.txt", "r");
    int value = file ? fgetc(file) : 'd';
    if (file) fclose(file);
    return (char)value;
}
static LONG WINAPI version(OSVERSIONINFOW *v) { v->dwMajorVersion=10;v->dwBuildNumber=22000;return 0; }
static bool WINAPI preference() { return mode()=='d'; }
static bool WINAPI allow(HWND,bool) { return true; }
static int WINAPI app(int) { return 0; }
static void WINAPI notify() {}
static HRESULT WINAPI attribute(HWND,DWORD,LPCVOID,DWORD) { return S_OK; }
static HMODULE WINAPI module(LPCSTR name) {
    if(name && !strcmp(name,"ntdll.dll"))return (HMODULE)1;
    return GetModuleHandleA(name);
}
static HMODULE WINAPI load(LPCSTR name) {
    if(strstr(name,"dwmapi.dll"))return (HMODULE)2;
    return LoadLibraryA(name);
}
static BOOL WINAPI release(HMODULE value) { return value==(HMODULE)2 ? TRUE : FreeLibrary(value); }
static FARPROC WINAPI address(HMODULE value,LPCSTR name) {
    if(value==(HMODULE)1)return (FARPROC)version;
    if(value==(HMODULE)2)return (FARPROC)attribute;
    if((ULONG_PTR)name<=65535) switch((ULONG_PTR)name) {
    case 132:return (FARPROC)preference;
    case 133:return (FARPROC)allow;
    case 135:return (FARPROC)app;
    case 104:case 136:return (FARPROC)notify;
    }
    return GetProcAddress(value,name);
}
static BOOL WINAPI spi(UINT action,UINT size,PVOID data,UINT flags) {
    if(action==SPI_GETHIGHCONTRAST) {
        ((HIGHCONTRASTA *)data)->dwFlags=mode()=='h' ? HCF_HIGHCONTRASTON : 0;
        return TRUE;
    }
    return SystemParametersInfoA(action,size,data,flags);
}
extern "C" int __cdecl ProbeDebugger(int action);
static HOOKPROC productionHook;
static int probeToolbar(HWND main) {
    HWND toolbar=FindWindowExA(main,NULL,TOOLBARCLASSNAMEA,NULL);
    int commands[3]={32804,32777,32800};
    HIMAGELIST images=(HIMAGELIST)SendMessage(toolbar,TB_GETIMAGELIST,0,0);
    int width,height; if(!images || !ImageList_GetIconSize(images,&width,&height))return 50;
    HDC screen=GetDC(NULL),dc=CreateCompatibleDC(screen);
    HBITMAP bitmap=CreateCompatibleBitmap(screen,width,height);
    HGDIOBJ original=SelectObject(dc,bitmap);int result=1;
    for(int i=0;i<3;++i) {
        SendMessage(toolbar,TB_ENABLEBUTTON,commands[i],TRUE);
        int index=(int)SendMessage(toolbar,TB_COMMANDTOINDEX,commands[i],0);
        TBBUTTON button;SendMessage(toolbar,TB_GETBUTTON,index,(LPARAM)&button);
        RECT rect={0,0,width,height};HBRUSH background=CreateSolidBrush(RGB(17,35,53));
        FillRect(dc,&rect,background);DeleteObject(background);
        ImageList_Draw(images,button.iBitmap,dc,0,0,ILD_NORMAL);
        if(GetPixel(dc,0,0)!=RGB(17,35,53))result=57+i;
        int visible=0;
        for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
            COLORREF c=GetPixel(dc,x,y);
            if(GetRValue(c)>180 && GetGValue(c)>180 && GetBValue(c)>180)++visible;
        }
        if(visible<3)result=51+i;
    }
    SelectObject(dc,original);DeleteObject(bitmap);DeleteDC(dc);ReleaseDC(NULL,screen);
    UpdateWindow(toolbar);
    return result;
}
static int probeScrollbar(HWND main,bool move) {
    HWND list=FindWindowExA(main,NULL,WC_LISTVIEWA,NULL);
    SCROLLBARINFO bar;ZeroMemory(&bar,sizeof(bar));bar.cbSize=sizeof(bar);
    if(!GetScrollBarInfo(list,OBJID_HSCROLL,&bar))return 61;
    if(bar.rgstate[0]&STATE_SYSTEM_INVISIBLE)return 62;
    HDC dc=GetDC(NULL);COLORREF color=GetPixel(dc,bar.rcScrollBar.left+1,bar.rcScrollBar.top+1);ReleaseDC(NULL,dc);
    if(color!=RGB(40,40,40))return 63;
    if(!move)return 1;
    SCROLLINFO position;ZeroMemory(&position,sizeof(position));position.cbSize=sizeof(position);position.fMask=SIF_POS;
    if(!GetScrollInfo(list,SB_HORZ,&position))return 64;
    int before=position.nPos;SendMessage(list,WM_HSCROLL,SB_LINERIGHT,0);
    if(!GetScrollInfo(list,SB_HORZ,&position)||position.nPos<=before)return 65;
    return 1;
}
static LRESULT CALLBACK fixtureMessages(int code,WPARAM w,LPARAM l) {
    const CWPRETSTRUCT *message=(const CWPRETSTRUCT *)l;
    if(code>=0 && message->message==WM_APP+0x487) {
        int result=message->wParam==100 ? probeToolbar(message->hwnd) :
            message->wParam==101 ? probeScrollbar(message->hwnd,true) :
            message->wParam==102 ? probeScrollbar(message->hwnd,false) : ProbeDebugger((int)message->wParam);
        SetPropA(message->hwnd,"DebuggerProbeResult",(HANDLE)result);
    }
    return productionHook(code,w,l);
}
static HHOOK WINAPI installHook(int type,HOOKPROC proc,HINSTANCE instance,DWORD thread) {
    productionHook=proc;
    return SetWindowsHookExA(type,fixtureMessages,instance,thread);
}
#define SetWindowsHookExA installHook
#define GetModuleHandleA module
#define LoadLibraryA load
#define FreeLibrary release
#define GetProcAddress address
#define SystemParametersInfoA spi
#include "StaticTheme.cpp"
