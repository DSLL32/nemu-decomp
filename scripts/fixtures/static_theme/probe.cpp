/* Run the production adapter against simulated OS exports, with real HWNDs,
   thread hooks and message delivery. No dark-mode capability is assumed in Wine. */
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define REQUIRE(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); ExitProcess(5); } } while (0)
static DWORD testMajor, testBuild;
static bool ntMissing, dllMissing, exportsMissing, shouldMissing, appDark, highContrast, spiFails;
static DWORD registryType, registrySize, registryValue;
static unsigned loads, ordinalLookups, oldAppCalls, newAppCalls, flushCalls, windowCalls, dwmCalls, legacyCalls;
static int lastMode;
static HWND lastWindow;
static BOOL lastDark;
static HRESULT attribute20Result;
static LONG WINAPI testVersion(OSVERSIONINFOW *v) { v->dwMajorVersion=testMajor;v->dwBuildNumber=testBuild;return 0; }
static bool WINAPI testShould() { return appDark; }
static bool WINAPI testAllowWindow(HWND w,bool d) { ++windowCalls;lastWindow=w;lastDark=d;return true; }
static bool WINAPI testOldApp(bool d) { ++oldAppCalls;lastMode=d?1:0;return true; }
static int WINAPI testNewApp(int mode) { ++newAppCalls;lastMode=mode;return 0; }
static void WINAPI testRefresh() {}
static void WINAPI testFlush() { ++flushCalls; }
static HRESULT WINAPI testAttribute(HWND w,DWORD attribute,LPCVOID value,DWORD size) {
 REQUIRE(size==sizeof(BOOL));REQUIRE(IsWindow(w));
 REQUIRE(!(GetWindowLongA(w,GWL_STYLE)&WS_CHILD));
 ++dwmCalls;lastWindow=w;lastDark=*(const BOOL *)value;
 if(attribute==20)return attribute20Result;
 REQUIRE(attribute==19);++legacyCalls;return S_OK;
}
static HMODULE WINAPI testModule(LPCSTR name) {
 if(!name)return GetModuleHandleA(NULL);
 if(!strcmp(name,"comctl32.dll") || !strcmp(name,"user32.dll"))return GetModuleHandleA(name);
 REQUIRE(!strcmp(name,"ntdll.dll"));return ntMissing?NULL:(HMODULE)1;
}
static HMODULE WINAPI testLoad(LPCSTR name) {
 char system[MAX_PATH];GetSystemDirectoryA(system,sizeof(system));
 REQUIRE(!_strnicmp(name,system,strlen(system)));++loads;
 if(dllMissing)return NULL;
 if(strstr(name,"\\dwmapi.dll"))return (HMODULE)2;
 REQUIRE(strstr(name,"\\uxtheme.dll")!=NULL);return (HMODULE)3;
}
static BOOL WINAPI testFree(HMODULE) { return TRUE; }
static FARPROC WINAPI testAddress(HMODULE module,LPCSTR name) {
 if(module==(HMODULE)1) { REQUIRE(!strcmp(name,"RtlGetVersion"));return (FARPROC)testVersion; }
 if(module==(HMODULE)2) { REQUIRE(!strcmp(name,"DwmSetWindowAttribute"));return exportsMissing?NULL:(FARPROC)testAttribute; }
 if(module!=(HMODULE)3)return GetProcAddress(module,name);
 if((ULONG_PTR)name>65535) { REQUIRE(!strcmp(name,"SetWindowTheme"));return NULL; }
 ++ordinalLookups;
 if(exportsMissing)return NULL;
 switch((ULONG_PTR)name) {
 case 132:return shouldMissing?NULL:(FARPROC)testShould;
 case 133:return (FARPROC)testAllowWindow;
 case 135:return testBuild>=18362?(FARPROC)testNewApp:(FARPROC)testOldApp;
 case 104:return (FARPROC)testRefresh;
 case 136:return (FARPROC)testFlush;
 default:REQUIRE(0);return NULL;
 }
}
static BOOL WINAPI testSpi(UINT action,UINT,PVOID data,UINT) {
 if(action==SPI_GETNONCLIENTMETRICS)return FALSE;
 REQUIRE(action==SPI_GETHIGHCONTRAST);
 ((HIGHCONTRASTA *)data)->dwFlags=highContrast?HCF_HIGHCONTRASTON:0;
 return !spiFails;
}
static LONG WINAPI testRegOpen(HKEY,LPCSTR name,DWORD,REGSAM,HKEY *key) {
 REQUIRE(strstr(name,"Themes\\Personalize")!=NULL);*key=(HKEY)1;return ERROR_SUCCESS;
}
static LONG WINAPI testRegRead(HKEY,LPCSTR name,LPDWORD,LPDWORD type,LPBYTE value,LPDWORD size) {
 REQUIRE(!strcmp(name,"AppsUseLightTheme"));*type=registryType;
 memcpy(value,&registryValue,sizeof(registryValue));*size=registrySize;return ERROR_SUCCESS;
}
static LONG WINAPI testRegClose(HKEY) { return ERROR_SUCCESS; }
#define GetModuleHandleA testModule
#define LoadLibraryA testLoad
#define FreeLibrary testFree
#define GetProcAddress testAddress
#define SystemParametersInfoA testSpi
#define RegOpenKeyExA testRegOpen
#define RegQueryValueExA testRegRead
#define RegCloseKey testRegClose
static unsigned vclRefreshCalls;
static BOOL crashDark;
extern "C" void __cdecl NemuStaticCrashSetDark(BOOL enabled) { crashDark=enabled; }
extern "C" void __cdecl NemuStaticVclThemeRefresh(void) { ++vclRefreshCalls; }
#include "StaticTheme.cpp"

static void reset(DWORD major,DWORD build) {
 NemuStaticThemeShutdown();testMajor=major;testBuild=build;
 ntMissing=dllMissing=exportsMissing=shouldMissing=highContrast=spiFails=false;appDark=true;
 registryType=REG_DWORD;registrySize=4;registryValue=0;
 loads=ordinalLookups=oldAppCalls=newAppCalls=flushCalls=windowCalls=dwmCalls=legacyCalls=0;
 lastMode=-1;lastWindow=NULL;lastDark=FALSE;attribute20Result=S_OK;
}
static HWND create(const char *cls,HWND parent) {
 HWND w=CreateWindowExA(0,cls,"Theme probe",parent?WS_CHILD:WS_OVERLAPPEDWINDOW,
  0,0,100,100,parent,NULL,GetModuleHandleA(NULL),NULL);
 REQUIRE(w!=NULL);return w;
}
static BOOL CALLBACK dialog(HWND,UINT,WPARAM,LPARAM) { return FALSE; }
int main() {
 setvbuf(stdout,NULL,_IONBF,0);setvbuf(stderr,NULL,_IONBF,0);
 WNDCLASSA wc;HWND before,after,child,dlg;unsigned count;
 struct { DLGTEMPLATE header;WORD menu,cls,title; } resource;
 ZeroMemory(&wc,sizeof(wc));wc.lpfnWndProc=DefWindowProcA;wc.hInstance=GetModuleHandleA(NULL);
 wc.lpszClassName="TFormThemeProbe";REQUIRE(RegisterClassA(&wc)!=0);
 InitCommonControls();
 before=create("TFormThemeProbe",NULL);
 HMENU menu=CreateMenu();AppendMenuA(menu,MF_STRING,101,"&Probe");SetMenu(before,menu);
 reset(5,2600);NemuStaticThemeInitialize();REQUIRE(!loads&&!ordinalLookups&&!windowHook&&!crashDark);
 reset(6,7601);NemuStaticThemeInitialize();REQUIRE(!loads&&!ordinalLookups&&!windowHook);
 reset(10,17134);NemuStaticThemeInitialize();REQUIRE(!loads&&!ordinalLookups&&!windowHook);
 reset(10,22000);ntMissing=true;NemuStaticThemeInitialize();REQUIRE(!loads&&!windowHook);
 puts("PASS: pre-dark-mode Windows never loads DLLs or calls ordinals");
 reset(10,17763);attribute20Result=E_INVALIDARG;NemuStaticThemeInitialize();
 REQUIRE(oldAppCalls==1&&!newAppCalls&&lastMode==1&&legacyCalls&&lastDark&&windowHook&&crashDark);
 MENUITEMINFOA info;ZeroMemory(&info,sizeof(info));info.cbSize=sizeof(info);info.fMask=MIIM_FTYPE;
 REQUIRE(GetMenuItemInfoA(menu,0,TRUE,&info)&&(info.fType&MFT_OWNERDRAW));
 count=dwmCalls;after=create("TFormThemeProbe",NULL);REQUIRE(dwmCalls>count&&lastWindow==after);
 count=dwmCalls;child=create("EDIT",after);REQUIRE(dwmCalls==count);
 ZeroMemory(&resource,sizeof(resource));resource.header.style=WS_POPUP|WS_CAPTION;resource.header.cx=100;resource.header.cy=100;
 dlg=CreateDialogIndirectParamA(GetModuleHandleA(NULL),&resource.header,NULL,dialog,0);
 REQUIRE(dlg&&dwmCalls>count&&lastWindow==dlg);
 appDark=false;SendMessageA(after,WM_SETTINGCHANGE,0,0);REQUIRE(lastMode==0&&!lastDark&&!crashDark);
 REQUIRE(GetMenuItemInfoA(menu,0,TRUE,&info)&&!(info.fType&MFT_OWNERDRAW));
 appDark=true;SendMessageA(after,WM_THEMECHANGED,0,0);REQUIRE(lastMode==1&&lastDark&&crashDark);
 highContrast=true;SendMessageA(after,WM_SETTINGCHANGE,0,0);REQUIRE(lastMode==0&&!lastDark&&!crashDark);
 highContrast=false;SendMessageA(after,WM_SYSCOLORCHANGE,0,0);REQUIRE(lastMode==1&&lastDark);
 REQUIRE(DestroyWindow(dlg));REQUIRE(DestroyWindow(child));REQUIRE(DestroyWindow(after));
 puts("PASS: 1809 ABI, legacy attribute, existing/new/dialog handles, child exclusion, live theme and high contrast");
 // Delphi modal forms can have WS_CHILD and a caption (ParentWindow).
 NemuStaticThemeVclReady(TRUE);
 HWND owned=create("TFormThemeProbe",before);
 SetWindowLongA(owned,GWL_STYLE,GetWindowLongA(owned,GWL_STYLE)|WS_CAPTION);
 count=vclRefreshCalls;ShowWindow(owned,SW_SHOW);
 REQUIRE(vclRefreshCalls>count);DestroyWindow(owned);
 NemuStaticThemeVclReady(FALSE);
 // Real control colors and paint output, then restore original palette.
 reset(10,22000);NemuStaticThemeInitialize();
 HWND list=create(WC_LISTVIEWA,before), bar=create(STATUSCLASSNAMEA,before);
 HWND button=create("BUTTON",before), edit=create("EDIT",before);
 REQUIRE(ListView_GetBkColor(list)==RGB(25,25,25));
 REQUIRE(ListView_GetTextColor(list)==RGB(232,232,232));
 HDC dc=GetDC(before), memory=CreateCompatibleDC(dc);
 HBITMAP bitmap=CreateCompatibleBitmap(dc,100,100);
 HGDIOBJ original=SelectObject(memory,bitmap);
 SendMessageA(bar,WM_PRINTCLIENT,(WPARAM)memory,PRF_CLIENT);
 REQUIRE(GetPixel(memory,3,3)==RGB(40,40,40));
 RECT face={0,0,20,20};
 REQUIRE(NemuStaticDrawFrameControl(memory,&face,DFC_BUTTON,DFCS_BUTTONPUSH|DFCS_ADJUSTRECT));
 REQUIRE(GetPixel(memory,10,10)==RGB(40,40,40)&&face.left==2&&face.right==18);
 face.left=face.top=0;face.right=face.bottom=20;
 REQUIRE(NemuStaticDrawEdge(memory,&face,EDGE_RAISED,BF_RECT|BF_MIDDLE|BF_ADJUST));
 REQUIRE(GetPixel(memory,10,10)==RGB(40,40,40));
 SendMessageA(button,WM_PRINTCLIENT,(WPARAM)memory,PRF_CLIENT);
 REQUIRE(GetPixel(memory,3,3)==RGB(40,40,40));
 HBRUSH brush=(HBRUSH)SendMessageA(before,WM_CTLCOLOREDIT,(WPARAM)memory,(LPARAM)edit);
 REQUIRE(brush==NemuStaticGetSysColorBrush(COLOR_WINDOW));
 REQUIRE(GetTextColor(memory)==RGB(232,232,232));
 REQUIRE(GetBkColor(memory)==RGB(25,25,25));
 highContrast=true;SendMessageA(before,WM_SETTINGCHANGE,0,0);
 REQUIRE(ListView_GetBkColor(list)==GetSysColor(COLOR_WINDOW));
 REQUIRE(NemuStaticGetSysColor(COLOR_BTNTEXT)==GetSysColor(COLOR_BTNTEXT));
 highContrast=false;SendMessageA(before,WM_SETTINGCHANGE,0,0);
 REQUIRE(ListView_GetBkColor(list)==RGB(25,25,25));
 // A dark toolbar owns its inverted copy; the browser's shared list is intact.
 wc.lpszClassName="Nemu64";REQUIRE(RegisterClassA(&wc)!=0);
 HWND mainWindow=create("Nemu64",NULL),toolbar=create(TOOLBARCLASSNAMEA,mainWindow);
 HIMAGELIST sourceImages=ImageList_Create(16,16,ILC_COLOR24|ILC_MASK,1,1);
 RECT imageRect={0,0,16,16};
 HBITMAP sample=CreateCompatibleBitmap(dc,16,16);HGDIOBJ savedSample=SelectObject(memory,sample);
 HBRUSH maskKey=CreateSolidBrush(RGB(255,0,255));FillRect(memory,&imageRect,maskKey);DeleteObject(maskKey);
 RECT glyph={4,4,12,12};FillRect(memory,&glyph,(HBRUSH)GetStockObject(BLACK_BRUSH));SelectObject(memory,savedSample);
 REQUIRE(ImageList_AddMasked(sourceImages,sample,RGB(255,0,255))==0);DeleteObject(sample);
 // A masked source may have an opaque background color. Preserve its mask
 // independently of that drawing preference when deriving dark icons.
 ImageList_SetBkColor(sourceImages,RGB(192,192,192));
 SendMessage(toolbar,TB_SETIMAGELIST,0,(LPARAM)sourceImages);
 SendMessage(toolbar,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);
 TBBUTTON item;ZeroMemory(&item,sizeof(item));item.idCommand=ID_PAUSE;item.fsState=TBSTATE_ENABLED;
 SendMessage(toolbar,TB_ADDBUTTONS,1,(LPARAM)&item);
 SendMessage(toolbar,WM_PAINT,0,0);
 HIMAGELIST darkImages=(HIMAGELIST)SendMessage(toolbar,TB_GETIMAGELIST,0,0);
 REQUIRE(darkImages!=sourceImages);
 FillRect(memory,&imageRect,(HBRUSH)GetStockObject(BLACK_BRUSH));
 REQUIRE(ImageList_Draw(darkImages,0,memory,0,0,ILD_NORMAL));
 REQUIRE(GetPixel(memory,8,8)==RGB(255,255,255));
 REQUIRE(GetPixel(memory,0,0)==RGB(0,0,0));
 HIMAGELIST disabledImages=(HIMAGELIST)SendMessage(toolbar,TB_GETDISABLEDIMAGELIST,0,0);
 REQUIRE(disabledImages!=NULL);FillRect(memory,&imageRect,(HBRUSH)GetStockObject(BLACK_BRUSH));
 REQUIRE(ImageList_Draw(disabledImages,0,memory,0,0,ILD_NORMAL));
 REQUIRE(GetPixel(memory,8,8)==RGB(150,150,150)&&GetPixel(memory,0,0)==RGB(0,0,0));
 appDark=false;SendMessage(mainWindow,WM_SETTINGCHANGE,0,0);
 REQUIRE((HIMAGELIST)SendMessage(toolbar,TB_GETIMAGELIST,0,0)==sourceImages);
 REQUIRE((HIMAGELIST)SendMessage(toolbar,TB_GETDISABLEDIMAGELIST,0,0)!=disabledImages);
 REQUIRE(DestroyWindow(mainWindow));ImageList_Destroy(sourceImages);
 SelectObject(memory,original);DeleteObject(bitmap);DeleteDC(memory);ReleaseDC(before,dc);
 DestroyWindow(list);DestroyWindow(bar);DestroyWindow(button);DestroyWindow(edit);
 puts("PASS: dark list colors, status/button pixels, edit contrast and high-contrast restoration");
 reset(10,18362);NemuStaticThemeInitialize();REQUIRE(newAppCalls==1&&!oldAppCalls&&lastMode==1);
 reset(10,22000);NemuStaticThemeInitialize();REQUIRE(newAppCalls==1&&!oldAppCalls&&!legacyCalls&&lastDark);
 highContrast=true;SendMessageA(before,WM_SETTINGCHANGE,0,0);REQUIRE(lastMode==0&&!lastDark);
 reset(10,22000);shouldMissing=true;NemuStaticThemeInitialize();REQUIRE(lastDark&&lastMode==1);
 registryValue=1;SendMessageA(before,WM_SETTINGCHANGE,0,0);REQUIRE(!lastDark&&lastMode==0);
 registryValue=0;registryType=REG_SZ;SendMessageA(before,WM_SETTINGCHANGE,0,0);REQUIRE(!lastDark);
 registryType=REG_DWORD;registrySize=2;SendMessageA(before,WM_SETTINGCHANGE,0,0);REQUIRE(!lastDark);
 reset(10,22000);spiFails=true;NemuStaticThemeInitialize();REQUIRE(!lastDark&&lastMode==0);
 puts("PASS: 1903+/Windows 11 ABI, registry fallback, invalid preferences and failed accessibility query");
 reset(10,22000);attribute20Result=E_INVALIDARG;NemuStaticThemeInitialize();REQUIRE(!legacyCalls);
 reset(10,22000);dllMissing=true;NemuStaticThemeInitialize();REQUIRE(!windowHook&&!ordinalLookups&&!dwmCalls);
 reset(10,22000);exportsMissing=true;NemuStaticThemeInitialize();REQUIRE(!windowHook&&!dwmCalls&&!oldAppCalls&&!newAppCalls);
 NemuStaticThemeShutdown();REQUIRE(!windowHook&&!started&&!setAttribute&&!preferredApp);
 NemuStaticThemeShutdown();REQUIRE(DestroyWindow(before));
 puts("PASS: unsupported attributes, absent DLLs/exports and repeatable cleanup");
 return 0;
}
