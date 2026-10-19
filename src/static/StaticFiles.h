#ifndef NEMU_STATIC_FILES_H
#define NEMU_STATIC_FILES_H

/* Shared by the static frontend and generated copies of bundled components. */
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
const char *__cdecl NemuStaticDefaultData(const char *filename, unsigned int *size);
DWORD WINAPI NemuStaticGetPrivateProfileStringA(LPCSTR, LPCSTR, LPCSTR, LPSTR, DWORD, LPCSTR);
BOOL WINAPI NemuStaticGetPrivateProfileStructA(LPCSTR, LPCSTR, LPVOID, UINT, LPCSTR);
BOOL WINAPI NemuStaticWritePrivateProfileStringA(LPCSTR, LPCSTR, LPCSTR, LPCSTR);
BOOL WINAPI NemuStaticWritePrivateProfileStructA(LPCSTR, LPCSTR, LPVOID, UINT, LPCSTR);
typedef struct NemuStaticText NemuStaticText;
NemuStaticText *__cdecl NemuStaticTextOpen(const char *filename);
int __cdecl NemuStaticTextEof(NemuStaticText *file);
int __cdecl NemuStaticTextToken(NemuStaticText *file, char *buffer, unsigned int size);
char *__cdecl NemuStaticTextLine(NemuStaticText *file, char *buffer, unsigned int size);
void __cdecl NemuStaticTextClose(NemuStaticText *file);
#ifdef __cplusplus
}
#endif

#ifdef NEMU_STATIC_PROFILE_REDIRECT
#define GetPrivateProfileStringA NemuStaticGetPrivateProfileStringA
#define GetPrivateProfileStructA NemuStaticGetPrivateProfileStructA
#define WritePrivateProfileStringA NemuStaticWritePrivateProfileStringA
#define WritePrivateProfileStructA NemuStaticWritePrivateProfileStructA
#endif
#endif
