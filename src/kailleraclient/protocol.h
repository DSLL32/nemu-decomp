#ifndef KAILLERA_PROTOCOL_H
#define KAILLERA_PROTOCOL_H
#include "client.h"
#include "listview.h"
#include "control.h"

extern W_ListView gamesList,usersList,playersList;
extern HWND lobbyWindow,gameWindow,syncWindow;
extern TextControl gameChat,serverChat;
extern int onlineUsers,currentGame,localUser,localPlayer,totalPlayers;
extern int runningGame,syncDelay;
extern DWORD lastPacketTime;
extern char *connectionTypes[];

inline void appendChat(TextControl &window,char *text) {
    window.select(GetWindowTextLength(window.window),GetWindowTextLength(window.window));
    window.replace(text);
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
