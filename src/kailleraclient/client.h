// Shared client state inferred from API and dialog references.
#ifndef KAILLERA_CLIENT_INTERNAL_H
#define KAILLERA_CLIENT_INTERNAL_H
#include <windows.h>
#include "config.h"
#include "reliableconnection.h"

extern HINSTANCE clientInstance;
extern HWND hostWindow;
extern Config *settings;
extern char *applicationName;
extern char *gameList;
extern char username[32];
extern char selectedServerName[256];
extern char serverAddress[256];
extern int serverPort;
extern char defaultUsername[1];
extern ReliableConnection *connection;
extern int localPort;
extern int connectionType;
extern FARPROC flashWindowEx;
extern int (WINAPI *gameCallback)(char *,int,int);
extern void (WINAPI *chatCallback)(char *,char *);
extern void (WINAPI *droppedCallback)(char *,int);
extern void (WINAPI *moreInfosCallback)(char *);

void selectServerDialog(HWND parent);
int connectToServer(HWND parent,char *address);
int modifyPlayValues(void *values,int size);
void sendChat(char *text);
void endGame();
#endif
