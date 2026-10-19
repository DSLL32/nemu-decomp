/* Reuse the external UI fixture's process/window helpers. */
#define main unused_smoke_main
#include "nemu_probe.c"
#undef main

static void capture(HWND window, const char *filename) {
    RECT rect;
    HDC dc, memory;
    HBITMAP bitmap;
    HGDIOBJ original;
    BITMAPINFO info;
    BITMAPFILEHEADER header;
    unsigned char *pixels;
    DWORD size;
    FILE *file;
    GetWindowRect(window,&rect);
    dc=GetDC(NULL);memory=CreateCompatibleDC(dc);
    bitmap=CreateCompatibleBitmap(dc,rect.right-rect.left,rect.bottom-rect.top);
    original=SelectObject(memory,bitmap);
    require(BitBlt(memory,0,0,rect.right-rect.left,rect.bottom-rect.top,dc,rect.left,rect.top,SRCCOPY),"capture UI");
    ZeroMemory(&info,sizeof(info));info.bmiHeader.biSize=sizeof(info.bmiHeader);
    info.bmiHeader.biWidth=rect.right-rect.left;info.bmiHeader.biHeight=rect.bottom-rect.top;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    size=info.bmiHeader.biWidth*info.bmiHeader.biHeight*4;
    pixels=(unsigned char *)HeapAlloc(GetProcessHeap(),0,size);
    SelectObject(memory,original);
    require(GetDIBits(dc,bitmap,0,info.bmiHeader.biHeight,pixels,&info,DIB_RGB_COLORS)!=0,"capture bitmap data");
    ZeroMemory(&header,sizeof(header));header.bfType=0x4d42;
    header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader);header.bfSize=header.bfOffBits+size;
    file=fopen(filename,"wb");require(file!=NULL,"save UI image");
    fwrite(&header,sizeof(header),1,file);fwrite(&info.bmiHeader,sizeof(info.bmiHeader),1,file);
    fwrite(pixels,size,1,file);fclose(file);
    HeapFree(GetProcessHeap(),0,pixels);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(NULL,dc);
}
static void set_mode(HWND window, char value) {
    FILE *file=fopen("theme-mode.txt","w");require(file!=NULL,"write theme preference");
    fputc(value,file);fclose(file);
    message(window,WM_SETTINGCHANGE,0,0);Sleep(200);
}
static void pixel(HWND window,int x,int y,COLORREF expected) {
    POINT point;HDC dc;COLORREF actual;
    point.x=x;point.y=y;ClientToScreen(window,&point);
    /* Parent DCs clip child controls, including the Delphi banner panel. */
    dc=GetDC(NULL);actual=GetPixel(dc,point.x,point.y);ReleaseDC(NULL,dc);
    if(actual!=expected)fprintf(stderr,"pixel %d,%d: got %06lx expected %06lx\n",x,y,actual,expected);
    require(actual==expected,"rendered client pixel");
}
static void debugger(HWND main_window, int action) {
    HANDLE result;
    message(main_window,WM_APP+0x487,action,0);
    result=GetPropA(main_window,"DebuggerProbeResult");
    if(result!=(HANDLE)1) { fprintf(stderr,"Debugger action %d: %ld\n",action,(LONG)result); capture(main_window,"painter-failure.bmp"); }
    require(result==(HANDLE)1,"debugger painter and watch colors");
}
int main(int argc,char **argv) {
    STARTUPINFOA startup;
    HWND main_window,list,toolbar,status,settings;
    DWORD code;
    RECT bounds;POINT point;HDC screen;COLORREF color;
    require(argc==2,"executable argument");
    ZeroMemory(&startup,sizeof(startup));startup.cb=sizeof(startup);
    require(CreateProcessA(argv[1],NULL,NULL,NULL,FALSE,0,NULL,NULL,&startup,&process),"start themed build");
    WaitForInputIdle(process.hProcess,20000);
    main_window=await_window("Nemu64",NULL);Sleep(400);
    list=FindWindowExA(main_window,NULL,WC_LISTVIEWA,NULL);
    toolbar=FindWindowExA(main_window,NULL,TOOLBARCLASSNAMEA,NULL);
    status=FindWindowExA(main_window,NULL,STATUSCLASSNAMEA,NULL);
    require(list && toolbar && status,"main controls exist");
    require(message(list,LVM_GETBKCOLOR,0,0)==RGB(25,25,25),"dark ROM list palette");
    pixel(list,30,60,RGB(25,25,25));
    pixel(toolbar,600,5,RGB(40,40,40));
    pixel(status,600,5,RGB(40,40,40));
    debugger(main_window,101);Sleep(150);debugger(main_window,102);
    /* The divider above the toolbar must use the same dark face. */
    GetWindowRect(toolbar,&bounds);point.x=bounds.left+600;point.y=bounds.top;
    screen=GetDC(NULL);color=GetPixel(screen,point.x,point.y);ReleaseDC(NULL,screen);
    if(color!=RGB(40,40,40)) {
        fprintf(stderr,"divider %ld,%ld %ld,%ld: %06lx\n",bounds.left,bounds.top,bounds.right,bounds.bottom,color);
        capture(main_window,"divider-failure.bmp");
    }
    require(color==RGB(40,40,40),"dark toolbar divider");
    debugger(main_window,103);debugger(main_window,104);
    capture(main_window,"main-disabled-dark.bmp");
    debugger(main_window,100);
    capture(main_window,"main-dark.bmp");
    set_mode(main_window,'l');
    require(message(list,LVM_GETBKCOLOR,0,0)==GetSysColor(COLOR_WINDOW),"live light palette");
    capture(main_window,"main-light.bmp");
    set_mode(main_window,'h');
    require(message(list,LVM_GETBKCOLOR,0,0)==GetSysColor(COLOR_WINDOW),"high contrast disables dark palette");
    set_mode(main_window,'d');
    require(PostMessageA(main_window,WM_COMMAND,32776,0),"open native audio settings");
    settings=await_window("#32770","      Audio Configuration");Sleep(300);
    debugger(settings,105);capture(settings,"audio-dark.bmp");
    set_mode(settings,'l');debugger(settings,105);
    set_mode(settings,'h');debugger(settings,105);
    set_mode(settings,'d');debugger(settings,105);
    message(GetDlgItem(settings,1036),TBM_SETPOS,TRUE,0);
    require(PostMessageA(settings,WM_COMMAND,IDOK,0),"apply changed audio buffer size");await_closed(settings);
    settings=await_window("#32770","Audio Warning");Sleep(300);
    debugger(settings,106);capture(settings,"audio-warning-dark.bmp");
    set_mode(settings,'l');debugger(settings,106);
    set_mode(settings,'h');debugger(settings,106);
    set_mode(settings,'d');debugger(settings,106);
    require(PostMessageA(settings,WM_COMMAND,IDOK,0),"dismiss audio warning normally");await_closed(settings);
    require(PostMessageA(main_window,WM_COMMAND,32780,0),"open Delphi settings");
    settings=await_window("TFormNemuSettings","Nemu Settings");Sleep(400);
    pixel(settings,5,65,RGB(40,40,40));pixel(settings,5,5,RGB(25,25,25));capture(settings,"settings-dark.bmp");
    set_mode(settings,'l');
    pixel(settings,5,65,GetSysColor(COLOR_BTNFACE));pixel(settings,5,5,RGB(255,255,255));
    capture(settings,"settings-light.bmp");
    set_mode(settings,'d');
    require(PostMessageA(settings,WM_CLOSE,0,0),"close settings");await_closed(settings);
    require(PostMessageA(main_window,WM_COMMAND,32778,0),"open Delphi input");
    settings=await_window("TFormSettings","Input settings");Sleep(400);
    capture(settings,"input-dark.bmp");
    require(PostMessageA(settings,WM_CLOSE,0,0),"close input");await_closed(settings);
    debugger(main_window,0);
    settings=await_window("TFormMemory","Memory");Sleep(300);
    debugger(main_window,10);capture(settings,"memory-dark.bmp");
    set_mode(settings,'l');debugger(main_window,20);capture(settings,"memory-light.bmp");
    set_mode(settings,'d');
    debugger(main_window,1);
    settings=await_window("TFormDebug","Commands");Sleep(300);
    debugger(main_window,11);capture(settings,"commands-dark.bmp");
    debugger(main_window,2);
    settings=await_window("TFormRegisters","Registers");Sleep(300);
    debugger(main_window,12);capture(settings,"registers-dark.bmp");
    debugger(main_window,9);
    require(PostMessageA(main_window,WM_CLOSE,0,0),"normal shutdown");
    require(WaitForSingleObject(process.hProcess,10000)==WAIT_OBJECT_0,"process exits");
    require(GetExitCodeProcess(process.hProcess,&code)&&code==0,"successful exit");
    puts("PASS: full static UI dark pixels, live light/high-contrast restoration, Delphi settings and input");
    return 0;
}
