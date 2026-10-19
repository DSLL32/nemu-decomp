#ifndef NEMU_STATIC_CRASH_H
#define NEMU_STATIC_CRASH_H
#include <windows.h>
/* Private to the ordinary static user build. All APIs used exist on XP. */
extern "C" void __cdecl NemuStaticCrashInitialize(HINSTANCE instance);
extern "C" void __cdecl NemuStaticCrashShutdown(void);
/* Cache the UI preference while healthy; the reporter never contacts the UI thread. */
extern "C" void __cdecl NemuStaticCrashSetDark(BOOL enabled);
extern "C" LONG __cdecl NemuStaticCrashReport(EXCEPTION_POINTERS *exception);
extern "C" void __cdecl NemuStaticCrashDelphi(void *address, const char *type, const char *message);
extern "C" BOOL __cdecl NemuStaticCrashIsGeneratedCode(DWORD address);
typedef BOOL (__cdecl *NemuStaticDescribeCode)(DWORD address, char *text, unsigned capacity);
extern "C" void __cdecl NemuStaticCrashSetCodeDescription(NemuStaticDescribeCode describe);
#endif
