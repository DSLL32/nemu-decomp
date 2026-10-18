#ifndef KAILLERA_PROTOCOL_H
#define KAILLERA_PROTOCOL_H
#include "client.h"
#include "listview.h"

extern ListView gamesList,usersList,playersList;
extern HWND lobbyWindow,gameWindow,gameChat,serverChat,syncWindow;
extern int onlineUsers,currentGame,localUser,localPlayer,totalPlayers;
extern int runningGame,syncDelay;
extern DWORD lastPacketTime;
extern char *connectionTypes[];

inline void appendChat(HWND &window,char *text) {
    SendMessage(window,EM_SETSEL,GetWindowTextLength(window),GetWindowTextLength(window));
    SendMessage(window,EM_REPLACESEL,0,(LPARAM)text);
}

void updateCaption();
void dispatchMessage(Message *message);
void addUser(char *name,int ping,int status,int id,int connection);
void removeUser(int id);
void addGameUser(char *name,int ping,int id,int connection);
void removeGameUser(int id);
void updateGame(int id,int status,int users,int maximum);
void addGame(char *name,int id,char *application,char *owner,int users,int maximum,int status);
void removeGame(int id);
void showGameButtons(int show);
#endif
