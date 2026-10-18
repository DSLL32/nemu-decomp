#include "game.h"
#include "messages.h"

ByteQueue::ByteQueue() { used=0; data=0; capacity=0; }
ByteQueue::~ByteQueue() { if (data) free(data); }
void ByteQueue::append(void *bytes,int length) {
        if (length<1) return;
        if (!data) { capacity=length<<4; data=(char *)malloc(capacity); }
        else if (used+length>capacity) {
            while (used+length>capacity) capacity*=2;
            data=(char *)realloc(data,capacity);
        }
        memcpy(data+used,bytes,length);
        used+=length;
    }
void ByteQueue::peek(void *bytes,int length) {
        if (length>used) length=used;
        if (length>0) memcpy(bytes,data,length);
    }
void ByteQueue::read(void *bytes,int length) {
        if (length>used) length=used;
        if (length>0) {
            memcpy(bytes,data,length);
            used-=length;
            memcpy(data,data+length,used);
        }
    }

HWND gameInput;
char currentGameName[128];
int initialSynced,syncCancelled,frameDelayCount,frameWithinPacket;
ByteQueue sendByteQueue,receiveByteQueue;
ByteQueueList receivedDataCache,sentDataCache;
static HWND savedForeground;
static DWORD syncStartTick;

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
        playersList.window=GetDlgItem(window,1019);
        ListView_SetExtendedListViewStyle(playersList.window,0x820);
        playersList.addColumn("Nick",55);
        playersList.addColumn("Ping",20);
        playersList.addColumn("Connection",25);
        gameChat=GetDlgItem(window,1006);
        gameInput=GetDlgItem(window,1005);
        SetWindowText(GetDlgItem(window,1014),currentGameName);
        SetFocus(GetDlgItem(window,1005));
        break;
    case WM_COMMAND:
        switch (LOWORD(command)) {
        case 1013: {
            GameStartedMessage packet;
            connection->sendMessage(&packet);
            break;
        }
        case 1026:
            if (GetWindowTextLength(gameInput)) {
                char text[128]; GetWindowText(gameInput,text,127);
                {
                    ChatMessage packet(8,text);
                    connection->sendMessage(&packet);
                }
                SetWindowText(gameInput,defaultUsername);
            }
            break;
        case 1020: {
            int index=ListView_GetSelectionMark(playersList.window);
            if (index!=-1) {
                int id=playersList.itemData(index);
                if (id==localUser) MessageBox(window,"You can't kick yourself!","Error",0);
                else { KickMessage packet(id); connection->sendMessage(&packet); }
            }
            break;
        }
        case IDCANCEL:
            {
                GameUserLeftMessage packet(-1);
                connection->sendMessage(&packet);
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
        playersList.window=GetDlgItem(window,1019);
        ListView_SetExtendedListViewStyle(playersList.window,0x820);
        playersList.addColumn("Nick",55);
        playersList.addColumn("Ping",20);
        playersList.addColumn("Connection",25);
        gameChat=GetDlgItem(window,1006);
        gameInput=GetDlgItem(window,1005);
        SetWindowText(GetDlgItem(window,1014),currentGameName);
        SetFocus(GetDlgItem(window,1005));
        break;
    case WM_COMMAND:
        switch (LOWORD(command)) {
        case 1026:
            if (GetWindowTextLength(gameInput)) {
                char text[128]; GetWindowText(gameInput,text,127);
                {
                    ChatMessage packet(8,text);
                    connection->sendMessage(&packet);
                }
                SetWindowText(gameInput,defaultUsername);
            }
            break;
        case IDCANCEL:
            {
                GameUserLeftMessage packet(-1);
                connection->sendMessage(&packet);
            }
            showGameButtons(1); currentGame=-1; EndDialog(window,0);
            break;
        }
    }
    return 0;
}

void showGameContextMenu(int x,int y)
{
    if (moreInfosCallback) {
        RECT bounds; GetWindowRect(gamesList.window,&bounds);
        int count=ListView_GetItemCount(gamesList.window);
        for (int i=0;i<count;i++) {
            RECT item; ListView_GetItemRect(gamesList.window,i,&item,0);
            if (item.left<=x-bounds.left && x-bounds.left<=item.right &&
                item.top<=y-bounds.top && y-bounds.top<=item.bottom) {
                HMENU menu=CreatePopupMenu();
                AppendMenu(menu,0,1,"More infos about this game...");
                if (TrackPopupMenu(menu,TPM_RETURNCMD,x,y,0,gamesList.window,0)==1) {
                    char name[256]; ListView_GetItemText(gamesList.window,i,0,name,255);
                    moreInfosCallback(name);
                }
                DestroyMenu(menu);
                break;
            }
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
        {
            SimpleMessage packet(21);
            connection->sendMessage(&packet);
        }
        showSyncDialog();
        DWORD start=GetTickCount(); bool ready=false; int retries=0;
        while (!syncCancelled) {
            UDPConnection::selectSockets(0);
            if (connection->dataAvailable()) {
                Message packet;
                if (!connection->receiveMessage(&packet)) {
                    if (packet.type==21) ready=true;
                    else dispatchMessage(&packet);
                }
                if (ready) break;
            }
            if (start+10000<GetTickCount()) {
                connection->setRedundancy(1); start=GetTickCount();
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
    if (++frameWithinPacket==connectionType) {
        frameWithinPacket=0;
        int bytes=sendByteQueue.used;
        char *data=sendByteQueue.data;
        char temporary[1024];
        int index;
        for (index=0;index<sentDataCache.size();index++) {
            sentDataCache.get(index)->peek(temporary,bytes);
            if (!memcmp(temporary,data,bytes)) break;
        }
        if (index<sentDataCache.size() && index!=-1) {
            CachedGameDataMessage packet(index); connection->sendMessage(&packet);
        } else {
            {
                GameDataMessage packet(data,bytes); connection->sendMessage(&packet);
            }
            if (sentDataCache.size()==256) { delete sentDataCache.get(0); sentDataCache.remove(0); }
            ByteQueue *queue=new ByteQueue; queue->append(data,bytes); sentDataCache.add(queue);
        }
        sendByteQueue.used=0;
        if (++frameDelayCount>=syncDelay+1) {
            frameDelayCount--;
            bool control;
            do {
                control=false;
                Message packet;
                int retries=0; DWORD start=GetTickCount();
                for (;;) {
                    UDPConnection::selectSockets(1);
                    if (connection->dataAvailable() && !connection->receiveMessage(&packet)) break;
                    if (start+2000<GetTickCount()) {
                        connection->setRedundancy(syncDelay+4); start=GetTickCount();
                        if (++retries==3) {
                            runningGame=0; MessageBox(GetForegroundWindow(),"Lost connection :(","Error",0);
                            return -1;
                        }
                    }
                    pumpGameMessages();
                }
                switch (packet.type) {
                case 8: {
                    ChatMessage chat(packet);
                    if (chatCallback) chatCallback(chat.nick,chat.text);
                    dispatchMessage(&packet); control=true; break;
                }
                case 18: {
                    GameDataMessage message(packet);
                    unsigned int length=message.size;
                    char *data=message.buffer;
                    if (receivedDataCache.size()==256) { delete receivedDataCache.get(0); receivedDataCache.remove(0); }
                    ByteQueue *queue=new ByteQueue; queue->append(data,length); receivedDataCache.add(queue);
                    receiveByteQueue.append(data,length);
                    result=(int)((__int64)(unsigned __int64)length/connectionType);
                    receiveByteQueue.read(values,result); break;
                }
                case 19: {
                    CachedGameDataMessage message(packet);
                    int length; char *data;
                    if (message.index<receivedDataCache.size()) {
                        ByteQueue *queue=receivedDataCache.get(message.index);
                        length=queue->used; data=queue->data;
                    } else { length=0; data=0; }
                    receiveByteQueue.append(data,length);
                    result=length/connectionType;
                    receiveByteQueue.read(values,result); break;
                }
                default: dispatchMessage(&packet); control=true;
                }
            } while (control);
        }
    } else if (receiveByteQueue.used) {
        result=receiveByteQueue.used/(connectionType-frameWithinPacket);
        receiveByteQueue.read(values,result);
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
