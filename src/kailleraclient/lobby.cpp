#include "game.h"
#include "messages.h"

static int lobbyClosed;
static int gameStartFlag=1;
static HWND lobbyInput;
static HMENU gameMenu;
BOOL CALLBACK lobbyDialog(HWND,UINT,WPARAM,LPARAM);
void pollLobby();

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
    switch (message) {
    case WM_INITDIALOG: {
        onlineUsers=0; lobbyWindow=window; currentGameName[0]=0; localUser=-1; lobbyClosed=0;
        updateCaption();
        usersList.window=GetDlgItem(window,1011);
        ListView_SetExtendedListViewStyle(usersList.window,0x820);
        gamesList.window=GetDlgItem(window,1008);
        ListView_SetExtendedListViewStyle(gamesList.window,0x820);
        lobbyInput=GetDlgItem(window,1005); serverChat=GetDlgItem(window,1006);
        usersList.addColumn("Nick",30); usersList.addColumn("Ping",25);
        usersList.addColumn("Connection",30); usersList.addColumn("Status",15);
        usersList.sortColumn=1; usersList.numericSort=1; usersList.sortDirection=1;
        gamesList.addColumn("Game",40); gamesList.addColumn("Version",20);
        gamesList.addColumn("Owner",20); gamesList.addColumn("Status",10); gamesList.addColumn("# Users",10);
        gamesList.sortColumn=3; gamesList.numericSort=0; gamesList.sortDirection=-1;
        lastPacketTime=GetTickCount();
        MENUITEMINFO item={sizeof(MENUITEMINFO)};
        gameMenu=CreatePopupMenu();
        item.fMask=MIIM_TYPE|MIIM_ID|MIIM_DATA; item.fType=MFT_STRING;
        UINT id=1; char previous=0; HMENU submenu=0;
        for (char *name=gameList;*name;name+=strlen(name)+1,id++) {
            item.cch=strlen(name); item.wID=id; item.dwItemData=(DWORD)name; item.dwTypeData=name;
            if (*name!=previous) {
                submenu=CreatePopupMenu(); char initial[2]={*name,0};
                AppendMenu(gameMenu,MF_POPUP,(UINT)submenu,initial); previous=*name;
            }
            InsertMenuItem(submenu,id,TRUE,&item);
        }
        SetTimer(window,0xd1ed1e,10,0);
        break;
    }
    case WM_DESTROY: {
        {
            UserLeftMessage packet(-1,"Normal client exit"); connection->sendMessage(&packet);
        }
        delete connection; connection=0;
        break;
    }
    case WM_CLOSE: lobbyClosed=1; break;
    case WM_NOTIFY: {
        NMLISTVIEW *notice=(NMLISTVIEW *)data;
        if (notice->hdr.hwndFrom==usersList.window && notice->hdr.code==LVN_COLUMNCLICK) {
            int column=notice->iSubItem;
            usersList.numericSort=(column==1);
            if (column==usersList.sortColumn) usersList.sortDirection=-usersList.sortDirection;
            else usersList.sortDirection=1;
            usersList.sortColumn=column; usersList.sort();
        }
        if (notice->hdr.hwndFrom==gamesList.window && notice->hdr.code==LVN_COLUMNCLICK) {
            int column=notice->iSubItem;
            gamesList.numericSort=(column==4);
            if (column==gamesList.sortColumn) gamesList.sortDirection=-gamesList.sortDirection;
            else gamesList.sortDirection=1;
            gamesList.sortColumn=column; gamesList.sort();
        }
        break;
    }
    case WM_CONTEXTMENU:
        if ((HWND)command==GetDlgItem(window,1008)) showGameContextMenu(LOWORD(data),HIWORD(data));
        break;
    case WM_COMMAND:
        switch (LOWORD(command)) {
        case 1012:
            if (GetWindowTextLength(lobbyInput)) {
                char text[128]; GetWindowText(lobbyInput,text,127);
                { ChatMessage packet(7,text); connection->sendMessage(&packet); }
                SetWindowText(lobbyInput,defaultUsername);
            }
            break;
        case 1010: {
            RECT rect; GetWindowRect(GetDlgItem(window,1010),&rect);
            int selected=TrackPopupMenu(gameMenu,0x182,rect.left,rect.bottom,0,window,0);
            if (selected) {
                char *name=gameList;
                for (int i=selected-1;i>0;i--) name+=strlen(name)+1;
                strncpy(currentGameName,name,128); currentGameName[127]=0;
                showHostGamePanel();
                {
                    GameCreatedMessage packet(currentGameName); connection->sendMessage(&packet);
                }
                currentGame=0x12345;
            }
            break;
        }
        case 1009: {
            int index=ListView_GetSelectionMark(gamesList.window);
            if (index!=-1) {
                char text[256]; ListView_GetItemText(gamesList.window,index,3,text,255);
                if (!strcmp(text,"Waiting")) {
                    ListView_GetItemText(gamesList.window,index,0,currentGameName,127);
                    bool found=false;
                    for (char *name=gameList;*name && !found;name+=strlen(name)+1)
                        if (!strcmp(name,currentGameName)) found=true;
                    if (found) {
                        ListView_GetItemText(gamesList.window,index,1,text,255);
                        if (strcmp(text,applicationName)) MessageBox(window,
                            "This game seems to be running an incompatible emulator versions.\r\nDesynch MAY occur...",
                            "Warning",0);
                        currentGame=gamesList.itemData(index);
                        {
                            GameUserJoinedMessage packet(currentGame); connection->sendMessage(&packet);
                        }
                        showJoinedGamePanel();
                        return 0;
                    }
                    MessageBox(window,"This ROM is not available on your computer!","Can't join",0);
                } else MessageBox(window,"This game is currently playing!\r\n(Joining playing games is not supported yet)","Can't join",0);
            }
            break;
        }
        }
        break;
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

void pollLobby()
{
    UDPConnection::selectSockets(0);
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
