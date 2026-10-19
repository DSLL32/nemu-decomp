// Reconstructed Kaillera server browser. Original private names are unknown.
#include "serverbrowser.h"
#include "protocol.h"
#include "resizer.h"
#include "udpconnection.h"
#include "jnetlib/httpget.h"
#include <stdlib.h>
#include <string.h>

// The original controls have trivial lifetime hooks, like the list wrapper.
class BrowserControl {
public:
    BrowserControl() { window=0; }
    ~BrowserControl() {}
    operator HWND() { return window; }
    HWND operator=(HWND value) { return window=value; }
    HWND window;
};

ListView serverList;
BrowserControl usernameEdit;
BrowserControl connectionCombo;
Resizer browserLayout;
HWND browserStatus,refreshButton;
char *nextServer;
int refreshState;
DWORD pingStarted;
int pingRow;
int receivedBytes,serverCount;
static JNL_HTTPGet *serverRequest;
char *serverResponse;
UDPCom *pingConnection;
int pinging;

static BOOL CALLBACK browserDialog(HWND,UINT,WPARAM,LPARAM);
static BOOL CALLBACK addressDialog(HWND,UINT,WPARAM,LPARAM);
static BOOL CALLBACK aboutDialog(HWND,UINT,WPARAM,LPARAM);
static void updateServerList();
static void setBrowserStatus(char *text);
static void browserMessage(char *text);
static int readUsername(HWND parent);
extern int openLobby(HWND parent);

void selectServerDialog(HWND parent)
{
    DialogBoxParam(clientInstance,MAKEINTRESOURCE(101),parent,browserDialog,0);
}

static BOOL CALLBACK browserDialog(HWND window,UINT message,WPARAM wParam,LPARAM lParam)
{
    char address[256];
    switch (message) {
    case WM_WINDOWPOSCHANGED:
        browserLayout.resize();
        break;
    case WM_ERASEBKGND:
        browserLayout.excludeChildren((HDC)wParam);
        break;
    case WM_CLOSE:
        EndDialog(window,0);
        break;
    case WM_DESTROY:
        GetWindowText(usernameEdit,username,31);
        connectionType=SendMessage(connectionCombo,CB_GETCURSEL,0,0)+1;
        browserLayout.savePosition("ServerList");
        settings->writeString("username",username);
        settings->writeInt("conntype",connectionType);
        settings->flush();
        break;
    case WM_NOTIFY:
        if (((NMHDR *)lParam)->hwndFrom==serverList.window) {
            if (((NMHDR *)lParam)->code==NM_DBLCLK) SendMessage(window,WM_COMMAND,IDOK,0);
            else if (((NMHDR *)lParam)->code==LVN_COLUMNCLICK) {
                int column=((NMLISTVIEW *)lParam)->iSubItem;
                int numeric=(column==2 || column==4 || column==5);
                if (column==serverList.sortColumn) serverList.sortDirection=-serverList.sortDirection;
                else serverList.sortDirection=1;
                serverList.numericSort=numeric;
                serverList.sortColumn=column;
                serverList.sort();
            }
        }
        break;
    case WM_INITDIALOG:
        {
            SetWindowText(window,"Kaillera Client v0.84");
            usernameEdit=GetDlgItem(window,1002);
            SetWindowText(usernameEdit,username);
            browserStatus=GetDlgItem(window,1004);
            refreshButton=GetDlgItem(window,1001);
            serverList.window=GetDlgItem(window,1000);
            ListView_SetExtendedListViewStyle(serverList.window,0x820);
            serverList.addColumn("Server name",43);
            serverList.addColumn("Location",25);
            serverList.addColumn("Ping",10);
            serverList.addColumn("Version",10);
            serverList.addColumn("# Users",10);
            serverList.addColumn("# Games",12);
            serverList.addColumn("IP Address");
            serverList.sortColumn=2;
            serverList.numericSort=1;
            serverList.sortDirection=1;
            connectionCombo=GetDlgItem(window,1029);
            if (*connectionTypes) {
                char **type=connectionTypes;
                do { SendMessage(connectionCombo,CB_ADDSTRING,0,(LPARAM)*type); type++; } while (*type);
            }
            SendMessage(connectionCombo,CB_SETCURSEL,connectionType-1,0);
            browserLayout.clear();
            browserLayout.window=window;
            browserLayout.loadPosition("ServerList");
            browserLayout.add(1004,1,0,0,7,6);
            browserLayout.add(0,4,0,0,0,0);
            browserLayout.add(1000,3,7,7,0,0);
            browserLayout.add(0,4,0,0,0,0);
            browserLayout.add(1027,0,6,8,8,0);
            browserLayout.add(1002,1,0,0,4,0);
            browserLayout.add(1030,0,6,2,8,0);
            browserLayout.add(1029,0,2,6,5,0);
            browserLayout.add(0,4,0,0,0,0);
            browserLayout.add(0,1,0,0,0,0);
            browserLayout.add(1,0,4,3,4,7);
            browserLayout.add(1001,0,0,3,4,7);
            browserLayout.add(1005,0,0,3,4,7);
            browserLayout.add(1003,0,0,3,4,7);
            browserLayout.add(2,0,0,7,4,7);
            pinging=0;
            refreshState=0;
            SetTimer(window,0xc001d00d,10,0);
            SetClassLong(window,GCL_HICON,(LONG)LoadIcon(clientInstance,MAKEINTRESOURCE(105)));
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            if (readUsername(window)) {
                connectionType=SendMessage(connectionCombo,CB_GETCURSEL,0,0)+1;
                int row=ListView_GetSelectionMark(serverList.window);
                if (row!=-1) {
                    LVITEM item;
                    item.iSubItem=6;
                    item.cchTextMax=256;
                    item.pszText=address;
                    SendMessage(serverList.window,LVM_GETITEMTEXT,row,(LPARAM)&item);
                    item.iSubItem=0;
                    item.cchTextMax=255;
                    item.pszText=selectedServerName;
                    SendMessage(serverList.window,LVM_GETITEMTEXT,row,(LPARAM)&item);
                    refreshState=9999;
                    if (connectToServer(window,address)) {
                        KillTimer(window,0xc001d00d);
                        openLobby(window);
                        SetTimer(window,0xc001d00d,10,0);
                    }
                }
            }
            break;
        case IDCANCEL:
            refreshState=9999;
            PostMessage(window,WM_CLOSE,0,0);
            break;
        case 1001:
            refreshState=refreshState==-1?0:9999;
            break;
        case 1003:
            DialogBoxParam(clientInstance,MAKEINTRESOURCE(102),window,aboutDialog,0);
            break;
        case 1005:
            if (readUsername(window)) {
                connectionType=SendMessage(connectionCombo,CB_GETCURSEL,0,0)+1;
                KillTimer(window,0xc001d00d);
                refreshState=9999;
                if (DialogBoxParam(clientInstance,MAKEINTRESOURCE(108),window,addressDialog,0)) openLobby(window);
                SetTimer(window,0xc001d00d,10,0);
            }
            break;
        }
        break;
    case WM_TIMER:
        if (wParam==0xc001d00d) {
            updateServerList();
            if (pinging) {
                KillTimer(window,0xc001d00d);
                MSG msg;
                while (pinging) {
                    updateServerList();
                    if (PeekMessage(&msg,0,0,0,PM_REMOVE) && !IsDialogMessage(window,&msg)) {
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                }
                SetTimer(window,0xc001d00d,10,0);
            }
        }
        break;
    }
    return 0;
}

static void updateServerList()
{
    char address[256];
    char text[512];
    switch (refreshState) {
    case 0:
        SetWindowText(refreshButton,"Stop");
        browserMessage("Requesting list...");
        serverResponse=(char *)malloc(65536);
        receivedBytes=0;
        serverRequest=new JNL_HTTPGet;
        serverRequest->connect("http://kaillera.com/raw_server_list2.php?version=0.84");
        refreshState++;
        break;
    case 1:
        {
            int result=serverRequest->run();
            while (serverRequest->bytes_available()) receivedBytes+=serverRequest->get_bytes(serverResponse+receivedBytes,65536-receivedBytes);
            if (result==-1) {
                browserMessage("Error requesting server list!");
                refreshState=9999;
            } else if (result==1) {
                serverResponse[receivedBytes]=0;
                refreshState++;
            }
        }
        break;
    case 2:
        ListView_DeleteAllItems(serverList.window);
        delete serverRequest;
        serverRequest=0;
        nextServer=serverResponse;
        serverCount=0;
        refreshState++;
        setBrowserStatus("Pinging servers...");
        break;
    case 3:
        {
            if (pingConnection) delete pingConnection;
            pingConnection=0;
            char *p=nextServer;
            char *field=p;
            if (p<serverResponse+receivedBytes) {
                while (*p && *p!='\r' && *p!='\n') p++;
                *p++=0;
                int row=serverList.insert(9999,field,serverCount);
                while (*p=='\r' || *p=='\n') p++;
                field=p;
                while (*p && *p!=';' && *p!='\r' && *p!='\n') p++;
                *p++=0;
                strcpy(address,field);
                serverList.setText(row,6,field);
                field=p;
                while (*p && *p!=';' && *p!='\r' && *p!='\n') p++;
                *p++=0;
                serverList.setText(row,4,field);
                field=p;
                while (*p && *p!=';' && *p!='\r' && *p!='\n') p++;
                *p++=0;
                serverList.setText(row,5,field);
                field=p;
                while (*p && *p!=';' && *p!='\r' && *p!='\n') p++;
                *p++=0;
                serverList.setText(row,3,field);
                field=p;
                while (*p && *p!=';' && *p!='\r' && *p!='\n') p++;
                char delimiter=*p;
                *p++=0;
                serverList.setText(row,1,field);
                if (delimiter==';') {
                    while (*p!='\r' && *p!='\n' && *p) p++;
                    while (*p=='\r' || *p=='\n') p++;
                }
                nextServer=p;
                serverCount++;
                p=address;
                while (*p && *p!=':') p++;
                if (*p==':') {
                    *p++=0;
                    pingConnection=new UDPCom;
                    pingConnection->bind(0);
                    pingConnection->setPeer(address,atoi(p));
                    pingConnection->send("PING",5);
                    pingRow=row;
                    pingStarted=GetTickCount();
                    refreshState++;
                    pinging=1;
                }
            } else refreshState=10;
        }
        break;
    case 4:
        UDPCom::selectSockets(0);
        if (pingConnection->dataAvailable()) {
            wsprintf(address,"%i ms",GetTickCount()-pingStarted);
            serverList.setText(pingRow,2,address);
            serverList.sort();
            refreshState=3;
        } else if (pingStarted+1000<GetTickCount()) {
            serverList.setText(pingRow,2,"> 1s");
            serverList.sort();
            refreshState=3;
        }
        break;
    case 10:
        wsprintf(text,"Done. Found %i online servers.",serverCount);
        setBrowserStatus(text);
        refreshState=9999;
        break;
    case 9999:
        SetWindowText(refreshButton,"Refresh list");
        pinging=0;
        delete serverRequest;
        serverRequest=0;
        if (pingConnection) delete pingConnection;
        pingConnection=0;
        if (serverResponse) free(serverResponse);
        serverResponse=0;
        refreshState=-1;
        break;
    }
}

static void setBrowserStatus(char *text)
{
    SetWindowText(browserStatus,text);
}

static void browserMessage(char *text)
{
    ListView_DeleteAllItems(serverList.window);
    serverList.insert(0,text,-1);
    setBrowserStatus(text);
}

static BOOL CALLBACK addressDialog(HWND window,UINT message,WPARAM wParam,LPARAM lParam)
{
    char address[256];
    switch (message) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
            GetWindowText(GetDlgItem(window,1025),address,255);
            if (*address) {
                if (!strstr(address,":")) strcat(address,":27888");
                if (connectToServer(window,address)) EndDialog(window,1);
            }
            break;
        case IDCANCEL:
            PostMessage(window,WM_CLOSE,0,0);
            break;
        }
        return 0;
    case WM_CLOSE:
        EndDialog(window,0);
        break;
    }
    return 0;
}

static BOOL CALLBACK aboutDialog(HWND window,UINT message,WPARAM wParam,LPARAM lParam)
{
    switch (message) {
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK: PostMessage(window,WM_CLOSE,0,0); break;
        }
        break;
    case WM_CLOSE:
        EndDialog(window,0);
        break;
    }
    return 0;
}

static int readUsername(HWND parent)
{
    if (!GetWindowTextLength(usernameEdit)) {
        MessageBox(parent,"You must enter a user name!","Error",0);
        return 0;
    }
    GetWindowText(usernameEdit,username,31);
    return 1;
}
