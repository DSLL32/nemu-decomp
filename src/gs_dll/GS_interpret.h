#if !defined(AFX_GS_INTERPRET_H__INCLUDED_)
#define AFX_GS_INTERPRET_H__INCLUDED_

__declspec(dllexport) void GSInitialize(char *Header, char *Ram, char *IniDirectory);
__declspec(dllexport) void GSConfigure();
__declspec(dllexport) void GSUpdate();

#endif
