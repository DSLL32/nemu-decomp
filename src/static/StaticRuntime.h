#ifndef NEMU_STATIC_RUNTIME_H
#define NEMU_STATIC_RUNTIME_H

/* Ordinary user-build integration; historical DLL projects do not use this. */
extern "C" const char *__cdecl NemuStaticPluginName(unsigned int index);
extern "C" int __cdecl NemuStaticPluginIndex(const char *name);
extern "C" FARPROC __cdecl NemuStaticPluginProc(int index, const char *name);
extern "C" bool __cdecl NemuStaticInitialize(HINSTANCE instance);
extern "C" void __cdecl NemuStaticShutdown(void);
extern "C" void *__cdecl NemuStaticRdram(void);

#endif
