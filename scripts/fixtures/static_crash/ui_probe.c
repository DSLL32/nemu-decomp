#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef BS_TYPEMASK
#define BS_TYPEMASK 0x0000000fL
#endif
static void require(int value,const char *message) {
    if(!value){fprintf(stderr,"FAIL: %s (%lu)\n",message,GetLastError());ExitProcess(5);}
}
static void capture(HWND window,const char *filename) {
    RECT rect;HDC dc,memory;HBITMAP bitmap;HGDIOBJ old;BITMAPINFO info;BITMAPFILEHEADER header;
    DWORD size;unsigned char *pixels;FILE *file;
    GetWindowRect(window,&rect);dc=GetDC(window);memory=CreateCompatibleDC(dc);
    bitmap=CreateCompatibleBitmap(dc,rect.right-rect.left,rect.bottom-rect.top);old=SelectObject(memory,bitmap);
    /* GetWindowDC includes the XP-style title bar. */
    ReleaseDC(window,dc);dc=GetWindowDC(window);
    require(BitBlt(memory,0,0,rect.right-rect.left,rect.bottom-rect.top,dc,0,0,SRCCOPY),"capture dialog");
    ZeroMemory(&info,sizeof(info));info.bmiHeader.biSize=sizeof(info.bmiHeader);
    info.bmiHeader.biWidth=rect.right-rect.left;info.bmiHeader.biHeight=rect.bottom-rect.top;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;size=info.bmiHeader.biWidth*info.bmiHeader.biHeight*4;
    pixels=(unsigned char *)malloc(size);SelectObject(memory,old);
    require(GetDIBits(dc,bitmap,0,info.bmiHeader.biHeight,pixels,&info,DIB_RGB_COLORS)!=0,"read dialog bitmap");
    ZeroMemory(&header,sizeof(header));header.bfType=0x4d42;header.bfOffBits=sizeof(header)+sizeof(info.bmiHeader);
    header.bfSize=header.bfOffBits+size;file=fopen(filename,"wb");require(file!=NULL,"save dialog bitmap");
    fwrite(&header,sizeof(header),1,file);fwrite(&info.bmiHeader,sizeof(info.bmiHeader),1,file);fwrite(pixels,size,1,file);fclose(file);
    free(pixels);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(window,dc);
}
int main(int argc,char **argv) {
    STARTUPINFOA startup;PROCESS_INFORMATION process;DWORD code,start;HWND dialog=NULL,edit;
    char command[2048],text[65536],filename[256];RECT collapsed,expanded;FILE *file;
    setvbuf(stdout,NULL,_IONBF,0);setvbuf(stderr,NULL,_IONBF,0);
    require(argc==3,"exe and mode arguments");ZeroMemory(&startup,sizeof(startup));startup.cb=sizeof(startup);
    _snprintf(command,sizeof(command),"\"%s\" %s",argv[1],argv[2]);
    require(CreateProcessA(NULL,command,NULL,NULL,FALSE,0,NULL,NULL,&startup,&process),"launch crash fixture");
    if(!strcmp(argv[2],"caught")) {
        require(WaitForSingleObject(process.hProcess,10000)==WAIT_OBJECT_0,"handled exception exits");
        GetExitCodeProcess(process.hProcess,&code);require(code==0,"handled exception is not fatal");
        puts("PASS: handled access violation keeps normal exception handling and shutdown");return 0;
    }
    start=GetTickCount();
    while(GetTickCount()-start<20000) {
        dialog=FindWindowA("#32770","Nemu64 - Application Error");
        if(dialog&&IsWindowVisible(dialog)&&GetDlgItem(dialog,1003))break;dialog=NULL;
        GetExitCodeProcess(process.hProcess,&code);if(code!=STILL_ACTIVE)break;Sleep(20);
    }
    GetExitCodeProcess(process.hProcess,&code);if(!dialog)fprintf(stderr,"No dialog, exit %08lx\n",code);
    require(dialog!=NULL,"crash dialog appears");edit=GetDlgItem(dialog,1003);require(edit!=NULL,"details control exists");
    require(!IsWindowVisible(edit),"details initially collapsed");GetWindowRect(dialog,&collapsed);
    SendMessage(dialog,WM_COMMAND,1001,0);GetWindowRect(dialog,&expanded);
    require(IsWindowVisible(edit)&&expanded.bottom-expanded.top>collapsed.bottom-collapsed.top,"Details expands");
    RedrawWindow(dialog,NULL,NULL,RDW_INVALIDATE|RDW_ERASE|RDW_FRAME|RDW_ALLCHILDREN|RDW_UPDATENOW);
    SendMessageA(edit,WM_GETTEXT,sizeof(text),(LPARAM)text);
    _snprintf(filename,sizeof(filename),"report-%s.txt",argv[2]);file=fopen(filename,"wb");require(file!=NULL,"save test report");
    fwrite(text,strlen(text),1,file);fclose(file);
    require(strstr(text,"Exception: C0000005")!=NULL,"exception code");
    require(strstr(text,"Build: ")!=NULL && strstr(text,"symbols unavail")==NULL,"embedded build and symbol table");
    require(strstr(text,"Stack candidates")!=NULL,"uncertain frames labelled separately");
    require(strstr(text,"Fault location:")==NULL && strstr(text,"[fault]")!=NULL,"fault appears once at start of stack");
    {
        int first=(int)SendMessage(edit,EM_LINEINDEX,0,0),second=(int)SendMessage(edit,EM_LINEINDEX,1,0);
        LRESULT a=SendMessage(edit,EM_POSFROMCHAR,first,0),b=SendMessage(edit,EM_POSFROMCHAR,second,0);
        HDC dc=GetDC(dialog);int dpi=GetDeviceCaps(dc,LOGPIXELSY);COLORREF background=GetPixel(dc,5,5);
        int line=(short)HIWORD(b)-(short)HIWORD(a);
        ReleaseDC(dialog,dc);
        require(a!=-1 && b!=-1 && line>=8 && line<=MulDiv(16,dpi,96),"details uses compact readable font");
        if(!strcmp(argv[2],"dark")) {
            RECT client;HWND button=GetDlgItem(dialog,IDOK);
            require(background==RGB(40,40,40),"dark reporter dialog background");
            dc=GetDC(edit);GetClientRect(edit,&client);
            require(GetPixel(dc,client.right-3,3)==RGB(25,25,25),"dark report background");ReleaseDC(edit,dc);
            require((GetWindowLong(button,GWL_STYLE)&BS_TYPEMASK)==BS_DEFPUSHBUTTON,"native default Close button");
            require((GetWindowLong(GetDlgItem(dialog,1001),GWL_STYLE)&BS_TYPEMASK)==BS_PUSHBUTTON,"native Details button");
            SendMessage(edit,WM_VSCROLL,SB_BOTTOM,0);SendMessage(edit,WM_HSCROLL,SB_RIGHT,0);
            SendMessage(edit,WM_VSCROLL,SB_TOP,0);SendMessage(edit,WM_HSCROLL,SB_LEFT,0);
        } else require(background==GetSysColor(COLOR_BTNFACE),"XP/light dialog keeps system palette");
    }
    if(!strcmp(argv[2],"vcl")) {
        require(strstr(text,"Delphi: EAccessViolation")!=NULL,"Delphi exception class/message");
        require(strstr(text,"Captured before exception handlers")!=NULL&&strstr(text,"CrashClick")!=NULL,"pre-unwind Delphi context and stack");
    } else if(!strcmp(argv[2],"external")) {
        require(strstr(text,"external.dll")!=NULL&&strstr(text,"CrashExternal")!=NULL,"loose DLL module/export fallback");
    } else if(strcmp(argv[2],"badbp")) {
        char *candidates=strstr(text,"Stack candidates");*candidates=0;
        require(strstr(text,"CrashLeaf")!=NULL&&strstr(text,"CrashMiddle")!=NULL&&strstr(text,"[FPO")!=NULL,"FPO recovers frameless caller chain");
        *candidates='S';
    } else require(strstr(text,"DEADBEEF")!=NULL&&strstr(text,"BrokenFrame")!=NULL,"corrupt EBP reported safely");
    SendMessage(dialog,WM_COMMAND,1002,0);require(OpenClipboard(NULL),"read copied report");
    {HGLOBAL data=GetClipboardData(CF_TEXT);const char *copied=(const char *)GlobalLock(data);
    require(copied!=NULL&&!strcmp(copied,text),"Copy includes complete report");GlobalUnlock(data);CloseClipboard();}
    Sleep(100);_snprintf(filename,sizeof(filename),"crash-dialog-%s.bmp",argv[2]);capture(dialog,filename);
    SendMessage(dialog,WM_COMMAND,1001,0);require(!IsWindowVisible(edit),"Details collapses");
    PostMessage(dialog,WM_COMMAND,IDOK,0);require(WaitForSingleObject(process.hProcess,10000)==WAIT_OBJECT_0,"Close ends faulted process");
    GetExitCodeProcess(process.hProcess,&code);require(code==0xc0000005,"fatal exception exit code");
    CloseHandle(process.hThread);CloseHandle(process.hProcess);
    printf("PASS: %s crash, embedded names/FPO, details/copy and termination\n",argv[2]);return 0;
}
