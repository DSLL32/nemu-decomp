#include "client.h"
#include <stdlib.h>
#include <string.h>

static int connectState=-1;
static HWND connectWindow;
static HWND statusControl;
static char *connectAddress;
static UDPConnection *handshake;
static int cancelled;

static int pollConnection();
static void closeHandshake();
static void setStatus(char *text);
static BOOL CALLBACK connectionDialog(HWND,UINT,WPARAM,LPARAM);

int connectToServer(HWND parent,char *address)
{
    connectAddress=address;
    return DialogBoxParam(clientInstance,MAKEINTRESOURCE(103),parent,connectionDialog,0);
}

static BOOL CALLBACK connectionDialog(HWND window,UINT message,WPARAM wparam,LPARAM lparam)
{
    switch (message) {
    case WM_INITDIALOG:
        connectWindow=window;
        if (connection) { delete connection; connection=0; }
        statusControl=GetDlgItem(window,1004);
        connectState=0;
        cancelled=0;
        SetTimer(window,0xdeadc0de,1,0);
        break;
    case WM_DESTROY:
        closeHandshake();
        break;
    case WM_CLOSE:
        if (connection) { delete connection; connection=0; }
        EndDialog(window,0);
        break;
    case WM_COMMAND:
        if (LOWORD(wparam)==IDCANCEL) {
            cancelled=1;
            PostMessage(window,WM_CLOSE,0,0);
        }
        break;
    case WM_TIMER:
        if (wparam==0xdeadc0de && pollConnection()) EndDialog(window,1);
        break;
    }
    return 0;
}

static void closeHandshake()
{
    if (handshake) delete handshake;
    handshake=0;
    connectState=-1;
}

static int pollConnection()
{
    char *error;
    switch (connectState) {
    case 0:
        {
            setStatus("Connecting...");
            char *p=connectAddress;
            while (*p && *p!=':') p++;
            if (*p!=':') return 0;
            *p=0;
            handshake=new UDPConnection;
            handshake->bind(0);
            handshake->setPeer(connectAddress,atoi(p+1));
            char greeting[128];
            wsprintf(greeting,"HELLO%s","0.83");
            handshake->send(greeting,strlen(greeting)+1);
        }
        break;
    case 1:
        UDPConnection::selectSockets(0);
        if (!handshake->dataAvailable()) return 0;
        break;
    case 2:
        {
            char reply[256];
            int length=256;
            handshake->receive(reply,&length,0,0);
            if (!strncmp(reply,"VER",3)) {
                error="Can't connect: Version mismatch";
                goto failed;
            }
            if (!strncmp(reply,"TOO",3)) {
                error="Can't connect: Too many users";
                goto failed;
            }
            if (strncmp(reply,"HELLOD00D",9)) {
                connectState=1;
                return 0;
            }
            setStatus("Connected. Logging in...");
            strcpy(serverAddress,connectAddress);
            serverPort=atoi(reply+9);
            if (handshake) delete handshake;
            handshake=0;
            connection=new ReliableConnection;
            connection->bind(0);
            connection->setPeer(serverAddress,serverPort);
            {
                Message login;
                login.type=3;
                login.appendString(applicationName);
                login.setNick(username);
                login.appendByte((char)connectionType);
                connection->sendMessage(&login);
            }
            KillTimer(connectWindow,0xdeadc0de);
            MSG message;
            while (!cancelled) {
                if (PeekMessage(&message,0,0,0,PM_REMOVE)) {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
                if (cancelled) return 0;
                if (!connection) continue;
                UDPConnection::selectSockets(0);
                if (connection && connection->dataAvailable()) {
                    Message packet;
                    if (!connection->receiveMessage(&packet,1,0)) {
                        switch (packet.type) {
                        case 4:
                            connectState=-1;
                            return 1;
                        case 5:
                            connection->receiveMessage(&packet,0,0);
                            {
                                Message pong;
                                pong.type=6;
                                for (int i=0;i<4;i++) pong.appendInt(i);
                                connection->sendMessage(&pong);
                            }
                            break;
                        default:
                            connection->receiveMessage(&packet,0,0);
                            break;
                        }
                    }
                }
            }
            return 0;
        }
    default:
        return 0;
    }
    connectState++;
    return 0;
failed:
    setStatus(error);
    connectState=-1;
    return 0;
}

static void setStatus(char *text)
{
    SetWindowText(statusControl,text);
}
