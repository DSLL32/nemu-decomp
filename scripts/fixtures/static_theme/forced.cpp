/* Test-only adapter: force OS preference on Wine while using the production
   client painter and the actual statically linked VCL/plugin code. */
#define _WIN32_WINNT 0x0501
#include <windows.h>
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
#define GetModuleHandleA module
#define LoadLibraryA load
#define FreeLibrary release
#define GetProcAddress address
#define SystemParametersInfoA spi
#include "StaticTheme.cpp"
