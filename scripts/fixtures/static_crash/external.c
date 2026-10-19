#include <windows.h>
__declspec(dllexport) __declspec(noinline) void __cdecl CrashExternal(void) { *(volatile unsigned *)0=1; }
static DWORD WINAPI worker(void *unused) { Sleep(1000);CrashExternal();return 0; }
__declspec(dllexport) unsigned __cdecl ToolInit(void *host) { CloseHandle(CreateThread(NULL,0,worker,NULL,0,NULL));return 1; }
__declspec(dllexport) void __cdecl ToolFree(void) {}
__declspec(dllexport) void __cdecl ToolFunc(unsigned reason,unsigned index) {}
__declspec(dllexport) void __cdecl ToolGetMenuText(char *text) { lstrcpyA(text,"Crash reporting fixture"); }
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved) { return TRUE; }
