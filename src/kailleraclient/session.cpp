// Kaillera partyline and game session. Original private names are inferred.
#include "game.h"
#include "messages.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string.h>

BOOL CALLBACK lobbyDialog(HWND,UINT,WPARAM,LPARAM);
void pollLobby();

W_ListView usersList;
W_ListView gamesList;
static TextControl lobbyInput;
TextControl serverChat;
W_ListView playersList;
TextControl gameInput;
TextControl gameChat;
ByteQueueList receivedDataCache,sentDataCache;
ByteQueue sendByteQueue;
ByteQueue receiveByteQueue;
HWND lobbyWindow,gameWindow,syncWindow;
int onlineUsers;
int currentGame=-1,localUser=-1,localPlayer=-1;
int totalPlayers,runningGame,syncDelay;
DWORD lastPacketTime;
char currentGameName[128];
int initialSynced,syncCancelled,frameDelayCount,frameWithinPacket;
static HWND savedForeground;
static DWORD syncStartTick;
static int lobbyClosed;
static int gameStartFlag=1;
static HMENU gameMenu;




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

int openLobby(HWND parent)
{
    HWND window=CreateDialogParam(clientInstance,MAKEINTRESOURCE(104),parent,lobbyDialog,0);
    ShowWindow(window,5);
    MSG message;
    while (!lobbyClosed) {
        if (GetMessage(&message,0,0,0) && !IsDialogMessage(gameWindow,&message) &&
            !IsDialogMessage(window,&message)) {
            TranslateMessage(&message); DispatchMessage(&message);
        }
    }
    DestroyWindow(window);
    return 0;
}

BOOL CALLBACK lobbyDialog(HWND window,UINT message,WPARAM command,LPARAM data)
{
    char joinText[256];
    switch (message) {
    case WM_NOTIFY: {
        NMLISTVIEW *notice=(NMLISTVIEW *)data;
        if (notice->hdr.hwndFrom==usersList.m_hwnd) {
            if (notice->hdr.code==LVN_COLUMNCLICK) {
            int column=notice->iSubItem;
            int numeric=(column==1);
            if (column==usersList.m_sort_col) usersList.m_sort_dir=-usersList.m_sort_dir;
            else usersList.m_sort_dir=1;
            usersList.m_sort_type=numeric; usersList.m_sort_col=column; usersList.Resort();
        }
        }
        if (notice->hdr.hwndFrom==gamesList.m_hwnd) {
            if (notice->hdr.code==LVN_COLUMNCLICK) {
            int column=notice->iSubItem;
            int numeric=(column==4);
            if (column==gamesList.m_sort_col) gamesList.m_sort_dir=-gamesList.m_sort_dir;
            else gamesList.m_sort_dir=1;
            gamesList.m_sort_col=column; gamesList.m_sort_type=numeric; gamesList.Resort();
        }
        }
        break;
    }
    case WM_CONTEXTMENU:
        if ((HWND)command==GetDlgItem(window,1008)) showGameContextMenu(LOWORD(data),HIWORD(data));
        break;
    case WM_CLOSE: lobbyClosed=1; break;
    case WM_COMMAND:
        switch (LOWORD(command)) {
        case 1012:
            if (GetWindowTextLength(lobbyInput)) {
                char text[128]; GetWindowText(lobbyInput,text,127);
                { connection->sendMessage(&ChatMessage(7,text)); }
                SetWindowText(lobbyInput,"");
            }
            break;
        case 1010: {
            RECT rect; GetWindowRect(GetDlgItem(window,1010),&rect);
            int selected=TrackPopupMenu(gameMenu,0x182,rect.right,rect.top,0,window,0);
            if (selected) {
                char *name=gameList;
                for (int i=selected-1;i>0;i--) name+=strlen(name)+1;
                strncpy(currentGameName,name,128); currentGameName[127]=0;
                showHostGamePanel();
                {
                    connection->sendMessage(&GameCreatedMessage(currentGameName));
                }
                currentGame=0x12345;
            }
            break;
        }
        case 1009: {
            int index=ListView_GetSelectionMark(gamesList.m_hwnd);
            if (index==-1) break;
            gamesList.GetText(index,3,joinText,255);
            if (strcmp(joinText,"Waiting")) {
                MessageBox(window,"This game is currently playing!\r\n(Joining playing games is not supported yet)","Can't join",0);
                break;
            }
            gamesList.GetText(index,0,currentGameName,127);
            int found=0;
            for (char *name=gameList;*name && !found;name+=strlen(name)+1)
                if (!strcmp(name,currentGameName)) found=1;
            if (!found) {
                MessageBox(window,"This ROM is not available on your computer!","Can't join",0);
                break;
            }
            gamesList.GetText(index,1,joinText,255);
            if (strcmp(joinText,applicationName)) MessageBox(window,
                "This game seems to be running an incompatible emulator versions.\r\nDesynch MAY occur...",
                "Warning",0);
            currentGame=gamesList.GetParam(index);
            connection->sendMessage(&GameUserJoinedMessage(currentGame));
            showJoinedGamePanel();
            return 0;
        }
        }
        break;
    case WM_DESTROY: {
        {
            connection->sendMessage(&UserLeftMessage(-1,"Normal client exit"));
        }
        delete connection; connection=0;
        break;
    }
    case WM_INITDIALOG: {
        onlineUsers=0; lobbyWindow=window; currentGameName[0]=0; localUser=-1; lobbyClosed=0;
        updateCaption();
        usersList.m_hwnd=GetDlgItem(window,1011);
        ListView_SetExtendedListViewStyle(usersList.m_hwnd,0x820);
        gamesList.m_hwnd=GetDlgItem(window,1008);
        ListView_SetExtendedListViewStyle(gamesList.m_hwnd,0x820);
        lobbyInput=GetDlgItem(window,1005); serverChat=GetDlgItem(window,1006);
        usersList.AddCol("Nick",30); usersList.AddCol("Ping",25);
        usersList.AddCol("Connection",30); usersList.AddCol("Status",15);
        usersList.m_sort_col=1; usersList.m_sort_type=1; usersList.m_sort_dir=1;
        gamesList.AddCol("Game",40); gamesList.AddCol("Version",20);
        gamesList.AddCol("Owner",20); gamesList.AddCol("Status",10); gamesList.AddCol("# Users",10);
        gamesList.m_sort_col=3; gamesList.m_sort_type=0; gamesList.m_sort_dir=-1;
        lastPacketTime=GetTickCount();
        MENUITEMINFO item={sizeof(MENUITEMINFO)};
        gameMenu=CreatePopupMenu();
        item.fMask=MIIM_TYPE|MIIM_ID|MIIM_DATA; item.fType=MFT_STRING;
        UINT id=1; char previous=0; HMENU submenu=0;
        for (char *name=gameList;*name;name+=strlen(name)+1) {
            item.wID=id; item.dwTypeData=name; item.dwItemData=(DWORD)name; item.cch=strlen(name);
            char current=*name;
            if (current!=previous) {
                submenu=CreatePopupMenu(); char initial[2]={*name,0};
                AppendMenu(gameMenu,MF_POPUP,(UINT)submenu,initial); previous=*name;
            }
            InsertMenuItem(submenu,id++,TRUE,&item);
        }
        SetTimer(window,0xd1ed1e,10,0);
        break;
    }
    case WM_TIMER:
        if (command==0xd1ed1e) pollLobby();
        break;
    case 0x523:
        KillTimer(window,0xd1ed1e); runningGame=1; frameDelayCount=0; frameWithinPacket=0;
        sendByteQueue.used=0; receiveByteQueue.used=0; gameStartFlag=1; initialSynced=0; syncCancelled=0;
        receivedDataCache.clear(); sentDataCache.clear();
        gameCallback(currentGameName,localPlayer,totalPlayers);
        break;
    }
    return 0;
}

void updateCaption()
{
    char text[512];
    wsprintf(text,"Kaillera v%s: %s (%i online users)","0.84",selectedServerName,onlineUsers);
    SetWindowText(lobbyWindow,text);
}

void pollLobby()
{
    UDPCom::selectSockets(0);
    if (connection->dataAvailable()) {
        Message packet;
        if (connection->receiveMessage(&packet)) return;
        dispatchMessage(&packet);
    }
    if (lastPacketTime+60000<GetTickCount()) {
        {
            SimpleMessage packet(9); connection->sendMessage(&packet);
        }
        lastPacketTime=GetTickCount();
    }
}

void dispatchMessage(Message *packet)
{
    char text[512];
    switch (packet->type) {
    case 5:
        {
            connection->sendMessage(&PongMessage());
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
    case 1:
        {
            UserLeftMessage message(*packet);
            wsprintf(text,"* %s left the partyline (%s).\r\n",packet->nick,message.reason);
            appendChat(serverChat,text);
            removeUser(message.id);
            updateCaption();
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
    case 10:
        {
            GameCreatedMessage message(*packet);
            addGame(message.game,message.id,message.application,message.nick,1,2,0);
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
    case 20:
        {
            PlayerDroppedMessage message(*packet);
            wsprintf(text,"* Player %i (%s) dropped from the current game.\r\n",message.player,message.nick);
            appendChat(gameChat,text);
            if (runningGame && droppedCallback) droppedCallback(message.nick,message.player);
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
            localPlayer=message.player;
            totalPlayers=message.players;
            PostMessage(lobbyWindow,0x523,0,0);
        }
        break;
    case 14:
        {
            GameStatusMessage message(*packet);
            updateGame(message.id,message.status,message.users,message.maximum);
        }
        break;
    }
}

void addUser(char *name,int ping,int status,int id,int connection)
{
    char text[512];
    int item=usersList.InsertItem(0,name,id);
    wsprintf(text,"%i ms",ping);
    usersList.SetItemText(item,1,text);
    usersList.SetItemText(item,2,connectionTypes[connection-1]);
    usersList.SetItemText(item,3,status==1?"Idle":"Playing");
    usersList.Resort();
    onlineUsers++;
}

void removeUser(int id)
{
    int item;
    while ((item=usersList.FindItemByParam(id))!=-1) {
        ListView_DeleteItem(usersList.m_hwnd,item);
        onlineUsers--;
    }
}

void addGameUser(char *name,int ping,int id,int connection)
{
    char text[256];
    int item=playersList.InsertItem(0,name,id);
    wsprintf(text,"%i ms",ping);
    playersList.SetItemText(item,1,text);
    playersList.SetItemText(item,2,connectionTypes[connection-1]);
}

void removeGameUser(int id)
{
    int item;
    while ((item=playersList.FindItemByParam(id))!=-1) ListView_DeleteItem(playersList.m_hwnd,item);
}

void updateGame(int id,int status,int users,int maximum)
{
    char text[32];
    int item=gamesList.FindItemByParam(id);
    if (item!=-1) {
        gamesList.SetItemText(item,3,status==0?"Waiting":"Playing");
        wsprintf(text,"%i/%i",users,maximum);
        gamesList.SetItemText(item,4,text);
        gamesList.Resort();
    }
}

void addGame(char *name,int id,char *application,char *owner,int users,int maximum,int status)
{
    int item=gamesList.InsertItem(0,name,id);
    gamesList.SetItemText(item,1,application);
    gamesList.SetItemText(item,2,owner);
    updateGame(id,status,users,maximum);
}

void removeGame(int id)
{
    for (int i=0;i<ListView_GetItemCount(gamesList.m_hwnd);i++) {
        if (gamesList.GetParam(i)==id) { ListView_DeleteItem(gamesList.m_hwnd,i); i--; }
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

void showHostGamePanel()
{
    showGameButtons(0);
    RECT rect;
    GetWindowRect(GetDlgItem(lobbyWindow,1015),&rect);
    ScreenToClient(lobbyWindow,(POINT *)&rect);
    gameWindow=CreateDialogParam(clientInstance,MAKEINTRESOURCE(106),lobbyWindow,hostGameDialog,0);
    SetWindowPos(gameWindow,0,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,0);
    ShowWindow(gameWindow,1);
}

BOOL CALLBACK hostGameDialog(HWND window,UINT message,WPARAM command,LPARAM data)
{
    switch (message) {
    case WM_INITDIALOG:
        playersList.m_hwnd=GetDlgItem(window,1019);
        ListView_SetExtendedListViewStyle(playersList.m_hwnd,0x820);
        playersList.AddCol("Nick",55);
        playersList.AddCol("Ping",20);
        playersList.AddCol("Connection",25);
        gameChat=GetDlgItem(window,1006);
        gameInput=GetDlgItem(window,1005);
        SetWindowText(GetDlgItem(window,1014),currentGameName);
        SetFocus(GetDlgItem(window,1005));
        break;
    case WM_COMMAND:
        switch (LOWORD(command)) {
        case 1013: {
            connection->sendMessage(&GameStartedMessage());
            break;
        }
        case 1026:
            if (GetWindowTextLength(gameInput)) {
                char text[128]; GetWindowText(gameInput,text,127);
                {
                    connection->sendMessage(&ChatMessage(8,text));
                }
                SetWindowText(gameInput,"");
            }
            break;
        case 1020: {
            int index=ListView_GetSelectionMark(playersList.m_hwnd);
            if (index!=-1) {
                int id=playersList.GetParam(index);
                if (id==localUser) MessageBox(window,"You can't kick yourself!","Error",0);
                else connection->sendMessage(&KickMessage(id));
            }
            break;
        }
        case IDCANCEL:
            {
                connection->sendMessage(&GameUserLeftMessage(-1));
            }
            showGameButtons(1); currentGame=-1; EndDialog(window,0);
            break;
        }
    }
    return 0;
}

void showJoinedGamePanel()
{
    showGameButtons(0);
    RECT rect;
    GetWindowRect(GetDlgItem(lobbyWindow,1015),&rect);
    ScreenToClient(lobbyWindow,(POINT *)&rect);
    gameWindow=CreateDialogParam(clientInstance,MAKEINTRESOURCE(107),lobbyWindow,joinedGameDialog,0);
    SetWindowPos(gameWindow,0,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,0);
    ShowWindow(gameWindow,1);
}

BOOL CALLBACK joinedGameDialog(HWND window,UINT message,WPARAM command,LPARAM data)
{
    switch (message) {
    case WM_INITDIALOG:
        playersList.m_hwnd=GetDlgItem(window,1019);
        ListView_SetExtendedListViewStyle(playersList.m_hwnd,0x820);
        playersList.AddCol("Nick",55);
        playersList.AddCol("Ping",20);
        playersList.AddCol("Connection",25);
        gameChat=GetDlgItem(window,1006);
        gameInput=GetDlgItem(window,1005);
        SetWindowText(GetDlgItem(window,1014),currentGameName);
        SetFocus(GetDlgItem(window,1005));
        return 0;
    case WM_COMMAND: {
        int id=LOWORD(command);
        switch (id) {
        case IDCANCEL: break;
        case 1026: if (GetWindowTextLength(gameInput)) break;
        default: return 0;
        }
        if (id==1026) {
        {
            char text[128]; GetWindowText(gameInput,text,127);
            { connection->sendMessage(&ChatMessage(8,text)); }
            SetWindowText(gameInput,"");
        }
        } else {
        { connection->sendMessage(&GameUserLeftMessage(-1)); }
        showGameButtons(1); currentGame=-1; EndDialog(window,0);
        }
        return 0;
    }
    }
    return 0;
}

void showGameContextMenu(int x,int y)
{
    if (moreInfosCallback) {
        RECT bounds; GetWindowRect(gamesList.m_hwnd,&bounds);
        int relativeY=y-bounds.top;
        int relativeX=x-bounds.left;
        int count=ListView_GetItemCount(gamesList.m_hwnd);
        int found=-1;
        for (int i=0;i<count;i++) {
            RECT item; ListView_GetItemRect(gamesList.m_hwnd,i,&item,0);
            if (item.left<=relativeX && item.right>=relativeX &&
                item.top<=relativeY && item.bottom>=relativeY) {
                found=i;
                break;
            }
        }
        if (found!=-1) {
            HMENU menu=CreatePopupMenu();
            AppendMenu(menu,0,1,"More infos about this game...");
            switch (TrackPopupMenu(menu,TPM_RETURNCMD,x,y,0,gamesList.m_hwnd,0)) {
            case 1: {
                char name[256]; ListView_GetItemText(gamesList.m_hwnd,found,0,name,255);
                moreInfosCallback(name);
                break;
            }
            }
            DestroyMenu(menu);
        }
    }
}

void pumpGameMessages()
{
    MSG message;
    if (PeekMessage(&message,0,0,0,PM_REMOVE) && !IsDialogMessage(gameWindow,&message) &&
        !IsDialogMessage(lobbyWindow,&message)) {
        TranslateMessage(&message); DispatchMessage(&message);
    }
}

void sendChat(char *text)
{
    if (currentGame!=-1) {
        ChatMessage packet(8,text);
        connection->sendMessage(&packet);
    }
}

int modifyPlayValues(void *values,int size)
{
    int result=0;
    if (!runningGame) return -1;
    if (!initialSynced) {
        connection->sendMessage(&SimpleMessage(21));
        showSyncDialog();
        DWORD start=GetTickCount(); int ready=0; int retries=0;
        while (1) {
            if (syncCancelled) break;
            UDPCom::selectSockets(0);
            if (connection->dataAvailable()) {
                {
                    Message packet;
                    if (!connection->receiveMessage(&packet)) {
                        if (packet.type==21) ready=1;
                        else dispatchMessage(&packet);
                    }
                }
                if (ready) break;
            }
            if (GetTickCount()>start+10000) {
                connection->forceResend(1); start=GetTickCount();
                if (++retries==12) {
                    runningGame=0; closeSyncDialog();
                    MessageBox(GetForegroundWindow(),"Timeout while waiting for initial netsync","Error",0);
                    return -1;
                }
            }
            pumpGameMessages();
        }
        closeSyncDialog();
        if (syncCancelled) { runningGame=0; return -1; }
        initialSynced=1;
    }
    sendByteQueue.append(values,size);
    if (++frameWithinPacket!=connectionType) {
        if (receiveByteQueue.used) {
            result=receiveByteQueue.used/(connectionType-frameWithinPacket);
            receiveByteQueue.read(values,result);
        }
    } else {
        frameWithinPacket=0;
        int bytes=sendByteQueue.used;
        char *data=sendByteQueue.data;
        int index=sentDataCache.find(data,bytes);
        if (index!=-1) {
            connection->sendMessage(&CachedGameDataMessage(index));
        } else {
            connection->sendMessage(&GameDataMessage(data,bytes));
            sentDataCache.addData(data,bytes);
        }
        sendByteQueue.used=0;
        if (++frameDelayCount>=syncDelay+1) {
            frameDelayCount--;
            int control;
            do {
                control=0;
                Message packet;
                int retries=0; DWORD start=GetTickCount();
                while (1) {
                    UDPCom::selectSockets(1);
                    if (connection->dataAvailable() && !connection->receiveMessage(&packet)) break;
                    if (GetTickCount()>start+2000) {
                        connection->forceResend(syncDelay+4); start=GetTickCount();
                        if (++retries==3) {
                            runningGame=0; MessageBox(GetForegroundWindow(),"Lost connection :(","Error",0);
                            return -1;
                        }
                    }
                    pumpGameMessages();
                }
                switch (packet.type) {
                case 18: {
                    GameDataMessage message(packet);
                    int length=message.size;
                    char *data=message.buffer;
                    receivedDataCache.addData(data,length);
                    receiveByteQueue.append(data,length);
                    result=length/connectionType;
                    receiveByteQueue.read(values,result); break;
                }
                case 19: {
                    CachedGameDataMessage message(packet);
                    int length=receivedDataCache.dataSize(message.index);
                    char *data=receivedDataCache.dataAt(message.index);
                    receiveByteQueue.append(data,length);
                    result=length/connectionType;
                    receiveByteQueue.read(values,result); break;
                }
                case 8: {
                    ChatMessage chat(packet);
                    if (chatCallback) chatCallback(chat.nick,chat.text);
                    dispatchMessage(&packet); control=1; break;
                }
                default: dispatchMessage(&packet); control=1;
                }
            } while (control);
        }
    }
    return result;
}

void showSyncDialog()
{
    savedForeground=GetForegroundWindow();
    syncWindow=CreateDialogParam(clientInstance,MAKEINTRESOURCE(109),savedForeground,syncDialog,0);
    RECT inner,outer;
    GetWindowRect(savedForeground,&outer); GetWindowRect(syncWindow,&inner);
    int width=inner.right-inner.left,height=inner.bottom-inner.top;
    MoveWindow(syncWindow,((outer.right-outer.left)-width)/2+outer.left,
        ((outer.bottom-outer.top)-height)/2+outer.top,width,height,0);
    ShowWindow(syncWindow,5);
}

BOOL CALLBACK syncDialog(HWND window,UINT message,WPARAM command,LPARAM data)
{
    char text[128];
    switch (message) {
    case WM_INITDIALOG:
        SetTimer(window,0xa55face,100,0); syncStartTick=GetTickCount();
        break;
    case WM_COMMAND:
        switch (LOWORD(command)) {
        case 2: syncCancelled=1; return 0;
        }
        break;
    case WM_TIMER:
        if (command==0xa55face) {
            int seconds=(syncStartTick-GetTickCount()+120000)/1000;
            if (seconds>0) wsprintf(text,"Waiting for other players: %is",seconds);
            else wsprintf(text,"Netsync timeout!");
            SetWindowText(GetDlgItem(window,1004),text);
            return 0;
        }
        break;
    }
    return 0;
}

void closeSyncDialog()
{
    DestroyWindow(syncWindow); SetFocus(savedForeground); syncWindow=0;
}



inline int ByteQueueList::find(void *data,int length) {
        char bytes[1024];
        for (int index=0;index<m_size;index++) {
            ((ByteQueue *)Get(index))->peek(bytes,length);
            if (!memcmp(bytes,data,length)) return index;
        }
        return -1;
    }

inline int ByteQueueList::dataSize(int index) { if (index>=m_size) return 0; return ((ByteQueue *)Get(index))->used; }

inline char *ByteQueueList::dataAt(int index) { if (index>=m_size) return 0; return ((ByteQueue *)Get(index))->data; }

inline void ByteQueueList::addData(void *data,int length) {
        if (m_size==256) { delete (ByteQueue *)Get(0); Del(0); }
        ByteQueue *queue=new ByteQueue;
        queue->append(data,length);
        Add(queue);
    }

ByteQueue::ByteQueue() { used=0; data=0; capacity=0; }

inline void ByteQueue::append(void *bytes,int length) {
        if (length<=0) return;
        if (!data) { capacity=length<<4; data=(char *)malloc(capacity); }
        else if (used+length>capacity) {
            while (used+length>capacity) capacity*=2;
            data=(char *)realloc(data,capacity);
        }
        memcpy(data+used,bytes,length);
        used+=length;
    }

inline void ByteQueue::read(void *bytes,int length) {
        if (length>used) length=used;
        if (length>0) {
            memcpy(bytes,data,length);
            used-=length;
            memcpy(data,data+length,used);
        }
    }

inline void ByteQueue::peek(void *bytes,int length) {
        int size=length<=used?length:used;
        if (size>0) memcpy(bytes,data,size);
    }

ByteQueueList::ByteQueueList() {}

inline void ByteQueueList::clear() { while (m_size>0) { delete (ByteQueue *)Get(0); Del(0); } }

ByteQueueList::~ByteQueueList() { clear(); }

inline ByteQueue::~ByteQueue() { if (data) free(data); }
