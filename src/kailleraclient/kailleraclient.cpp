#define KAILLERA_DLL
#include "kailleraclient.h"
#include "client.h"
#include "jnetlib/util.h"
#include <commctrl.h>
#include <stdlib.h>
#include <string.h>

HINSTANCE clientInstance;
HWND hostWindow;
Config *settings;
char *applicationName;
char *gameList;
char username[32];
char selectedServerName[256];
char serverAddress[256];
int serverPort;
char defaultUsername[1];
ReliableConnection *connection;
int localPort;
int connectionType;
FARPROC flashWindowEx;
int (WINAPI *gameCallback)(char *,int,int);
void (WINAPI *chatCallback)(char *,char *);
void (WINAPI *droppedCallback)(char *,int);
void (WINAPI *moreInfosCallback)(char *);

BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved)
{
    clientInstance=instance;
    return TRUE;
}

DLLEXP kailleraGetVersion(char *version)
{
    strcpy(version,"0.84");
}

DLLEXP kailleraInit()
{
    connection=0;
    settings=new Config("kaillera.ini","KailleraClient");
    applicationName="Unknown";
    strcpy(username,settings->readString("username",defaultUsername));
    gameList=0;
    localPort=27886;
    connectionType=settings->readInt("conntype",3);
    JNL::open_socketlib();
    InitCommonControls();
    HMODULE module=LoadLibrary("USER32.DLL");
    flashWindowEx=GetProcAddress(module,"FlashWindowEx");
    FreeLibrary(module);
}

DLLEXP kailleraShutdown()
{
    JNL::close_socketlib();
    settings->flush();
    delete settings;
}

DLLEXP kailleraSetInfos(kailleraInfos *infos)
{
    applicationName=infos->appName;
    if (gameList) free(gameList);
    int length=0;
    char *p=infos->gameList;
    while (*p || p[1]) { p++; length++; }
    length+=2;
    gameList=(char *)malloc(length);
    memcpy(gameList,infos->gameList,length);
    gameCallback=infos->gameCallback;
    chatCallback=infos->chatReceivedCallback;
    droppedCallback=infos->clientDroppedCallback;
    moreInfosCallback=infos->moreInfosCallback;
}

DLLEXP kailleraSelectServerDialog(HWND parent)
{
    hostWindow=parent;
    selectServerDialog(parent);
}

DLLEXP kailleraModifyPlayValues(void *values,int size)
{
    return modifyPlayValues(values,size);
}

DLLEXP kailleraChatSend(char *text)
{
    sendChat(text);
}

DLLEXP kailleraEndGame()
{
    endGame();
}
