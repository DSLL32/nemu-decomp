#include "protocol.h"
#include "messages.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>

ListView gamesList,usersList,playersList;
HWND lobbyWindow,gameWindow,gameChat,serverChat,syncWindow;
int onlineUsers;
int currentGame=-1,localUser=-1,localPlayer=-1;
int totalPlayers,runningGame,syncDelay;
DWORD lastPacketTime;

char *connectionTypes[]={"LAN (60 keyframes/s)","Excellent (30 keyframes/s)",
    "Good (20 keyframes/s)","Average (15 keyframes/s)",
    "Low (12 keyframes/s)","Bad (10 keyframes/s)",0};

void endGame()
{
    runningGame=0;
    lastPacketTime=GetTickCount();
    {
        PlayerDroppedMessage message(0);
        connection->sendMessage(&message);
    }
    if (syncWindow) DestroyWindow(syncWindow);
    syncWindow=0;
    SetFocus(lobbyWindow);
    SetTimer(lobbyWindow,0xd1ed1e,10,0);
}

void updateCaption()
{
    char text[512];
    wsprintf(text,"Kaillera v%s: %s (%i online users)","0.84",selectedServerName,onlineUsers);
    SetWindowText(lobbyWindow,text);
}

void dispatchMessage(Message *packet)
{
    char text[512];
    switch (packet->type) {
    case 1:
        {
            UserLeftMessage message(*packet);
            wsprintf(text,"* %s left the partyline (%s).\r\n",packet->nick,message.reason);
            appendChat(serverChat,text);
            removeUser(message.id);
            updateCaption();
        }
        break;
    case 2:
        {
            UserJoinedMessage message(*packet);
            if (localUser==-1) localUser=message.id;
            addUser(message.nick,message.ping,1,message.id,message.connection);
            updateCaption();
            wsprintf(text,"* %s joined the partyline!\r\n",message.nick);
            appendChat(serverChat,text);
        }
        break;
    case 4:
        {
            ServerStatusMessage message(*packet);
            int i;
            for (i=0;i<message.users;i++) {
                char name[32];
                message.readString(name,31);
                int ping=message.readInt();
                int status=message.readByte();
                int id=message.readWord();
                int connection=message.readByte();
                addUser(name,ping,status,id,connection);
            }
            for (i=0;i<message.games;i++) {
                char game[128],application[128],owner[32],players[10];
                message.readString(game,127);
                int id=message.readInt();
                message.readString(application,127);
                message.readString(owner,31);
                message.readString(players,9);
                int status=message.readByte();
                addGame(game,id,application,owner,atoi(players),2,status);
            }
        }
        break;
    case 5:
        {
            PongMessage message;
            connection->sendMessage(&message);
        }
        break;
    case 7:
        {
            ChatMessage message(*packet);
            wsprintf(text,"<%s> %s\r\n",message.nick,message.text);
            appendChat(serverChat,text);
        }
        break;
    case 8:
        {
            ChatMessage message(*packet);
            wsprintf(text,"<%s> %s\r\n",message.nick,message.text);
            appendChat(gameChat,text);
        }
        break;
    case 10:
        {
            GameCreatedMessage message(*packet);
            addGame(message.game,message.id,message.application,message.nick,1,2,0);
        }
        break;
    case 11:
        {
            if (currentGame==-1) return;
            GameUserLeftMessage message(*packet);
            if (message.id==localUser) {
                appendChat(serverChat,"* You have been kicked out of the game...\r\n");
                EndDialog(gameWindow,0);
                showGameButtons(1);
                currentGame=-1;
            } else {
                wsprintf(text,"* %s left the game's partyline.\r\n",packet->nick);
                appendChat(gameChat,text);
                removeGameUser(message.id);
            }
        }
        break;
    case 12:
        {
            GameUserJoinedMessage message(*packet);
            addGameUser(message.name,message.ping,message.id,message.connection);
            wsprintf(text,"* %s joined the game's partyline!\r\n",message.name);
            appendChat(gameChat,text);
            if (strcmp(message.name,username)) {
                PlaySound("join.wav",0,SND_FILENAME);
                if (flashWindowEx) {
                    struct FlashInfo { UINT size; HWND window; DWORD flags; UINT count; DWORD timeout; };
                    FlashInfo info={sizeof(FlashInfo),hostWindow,14,999,0};
                    ((BOOL (WINAPI *)(FlashInfo *))flashWindowEx)(&info);
                }
            }
        }
        break;
    case 13:
        {
            GameUsersMessage message(*packet);
            for (int i=0;i<message.users;i++) {
                char name[32];
                message.readString(name,31);
                int ping=message.readInt();
                int id=message.readWord();
                int connection=message.readByte();
                addGameUser(name,ping,id,connection);
            }
        }
        break;
    case 14:
        {
            GameStatusMessage message(*packet);
            updateGame(message.id,message.status,message.users,message.maximum);
        }
        break;
    case 16:
        {
            GameDeletedMessage message(*packet);
            removeGame(message.id);
            if (message.id==currentGame) {
                EndDialog(gameWindow,0);
                showGameButtons(1);
                currentGame=-1;
            }
        }
        break;
    case 17:
        {
            GameStartedMessage message(*packet);
            syncDelay=message.delay;
            totalPlayers=message.players;
            localPlayer=message.player;
            PostMessage(lobbyWindow,0x523,0,0);
        }
        break;
    case 20:
        {
            PlayerDroppedMessage message(*packet);
            wsprintf(text,"* Player %i (%s) dropped from the current game.\r\n",message.player,message.nick);
            appendChat(gameChat,text);
            if (runningGame && droppedCallback) droppedCallback(message.nick,message.player);
        }
        break;
    }
}

void addUser(char *name,int ping,int status,int id,int connection)
{
    char text[512];
    int item=usersList.insert(0,name,id);
    wsprintf(text,"%i ms",ping);
    usersList.setText(item,1,text);
    usersList.setText(item,2,connectionTypes[connection-1]);
    usersList.setText(item,3,status==1?"Idle":"Playing");
    usersList.sort();
    onlineUsers++;
}

void removeUser(int id)
{
    int item;
    while ((item=usersList.findData(id))!=-1) {
        ListView_DeleteItem(usersList.window,item);
        onlineUsers--;
    }
}

void addGameUser(char *name,int ping,int id,int connection)
{
    char text[256];
    int item=playersList.insert(0,name,id);
    wsprintf(text,"%i ms",ping);
    playersList.setText(item,1,text);
    playersList.setText(item,2,connectionTypes[connection-1]);
}

void removeGameUser(int id)
{
    int item;
    while ((item=playersList.findData(id))!=-1) ListView_DeleteItem(playersList.window,item);
}

void updateGame(int id,int status,int users,int maximum)
{
    char text[32];
    int item=gamesList.findData(id);
    if (item!=-1) {
        gamesList.setText(item,3,status==0?"Waiting":"Playing");
        wsprintf(text,"%i/%i",users,maximum);
        gamesList.setText(item,4,text);
        gamesList.sort();
    }
}

void addGame(char *name,int id,char *application,char *owner,int users,int maximum,int status)
{
    int item=gamesList.insert(0,name,id);
    gamesList.setText(item,1,application);
    gamesList.setText(item,2,owner);
    updateGame(id,status,users,maximum);
}

void removeGame(int id)
{
    for (int i=0;i<ListView_GetItemCount(gamesList.window);i++) {
        if (gamesList.itemData(i)==id) { ListView_DeleteItem(gamesList.window,i); i--; }
    }
}

void showGameButtons(int show)
{
    int command=show?SW_SHOWNORMAL:SW_HIDE;
    ShowWindow(GetDlgItem(lobbyWindow,1008),command);
    ShowWindow(GetDlgItem(lobbyWindow,1009),command);
    ShowWindow(GetDlgItem(lobbyWindow,1010),command);
    ShowWindow(GetDlgItem(lobbyWindow,1015),command);
}
