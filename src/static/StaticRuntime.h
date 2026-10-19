#ifndef NEMU_STATIC_RUNTIME_H
#define NEMU_STATIC_RUNTIME_H

/* User-build integration only; the reconstructed DLL projects do not use this. */
extern "C" __declspec(dllimport) const char *__cdecl NemuStaticPluginName(unsigned int index);
extern "C" __declspec(dllimport) void *__cdecl NemuStaticRdram(void);

#endif
