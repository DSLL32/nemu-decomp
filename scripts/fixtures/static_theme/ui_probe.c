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
int main(int argc,char **argv) {
    STARTUPINFOA startup;
    HWND main_window,list,toolbar,status,settings;
    DWORD code;
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
    capture(main_window,"main-dark.bmp");
    set_mode(main_window,'l');
    require(message(list,LVM_GETBKCOLOR,0,0)==GetSysColor(COLOR_WINDOW),"live light palette");
    capture(main_window,"main-light.bmp");
    set_mode(main_window,'h');
    require(message(list,LVM_GETBKCOLOR,0,0)==GetSysColor(COLOR_WINDOW),"high contrast disables dark palette");
    set_mode(main_window,'d');
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
    require(PostMessageA(main_window,WM_CLOSE,0,0),"normal shutdown");
    require(WaitForSingleObject(process.hProcess,10000)==WAIT_OBJECT_0,"process exits");
    require(GetExitCodeProcess(process.hProcess,&code)&&code==0,"successful exit");
    puts("PASS: full static UI dark pixels, live light/high-contrast restoration, Delphi settings and input");
    return 0;
}
