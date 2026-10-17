#ifndef NEMU64_KAILLERA_H
#define NEMU64_KAILLERA_H

#include <windows.h>

struct KailleraInfos {
    const char *AppName;
    char *GameList;
    int (__stdcall *GameCallback)(char *, int, int);
    void (__stdcall *ChatReceivedCallback)(char *, char *);
    void (__stdcall *ClientDroppedCallback)(char *, int);
    void (__stdcall *MoreInfosCallback)(const char *);
};

extern "C" {
__declspec(dllimport) void __stdcall kailleraInit();
__declspec(dllimport) void __stdcall kailleraShutdown();
__declspec(dllimport) void __stdcall kailleraSetInfos(KailleraInfos *infos);
__declspec(dllimport) void __stdcall kailleraSelectServerDialog(HWND window);
}

#endif
