#include "udpconnection.h"
#include "itemlist.h"

UDPCom::UDPCom()
{
    socketHandle=0;
    connections.Add(this);
    readable=0;
    if (!maximumSocket) {
        FD_ZERO(&sockets);
        FD_ZERO(&selected);
    }
}

UDPCom::~UDPCom()
{
    if (socketHandle) {
        shutdown(socketHandle,2);
        closesocket(socketHandle);
        FD_CLR(socketHandle,&sockets);
        if (socketHandle==maximumSocket) {
            maximumSocket=0;
            for (int i=0;i<connections.GetSize();i++) {
                if (connections.Get(i)!=this) {
                    SOCKET handle=((UDPCom *)connections.Get(i))->socketHandle;
                    if (handle>maximumSocket) {
                        maximumSocket=handle;
                    }
                }
            }
        }
    }
    for (int i=0;i<connections.GetSize();i++) {
        if (connections.Get(i)==this) { connections.Del(i); i--; }
    }
}

int UDPCom::bind(int value)
{
    port=value;
    sockaddr_in local;
    memset(&local,0,sizeof(local));
    local.sin_family=AF_INET;
    local.sin_port=htons((short)port);
    socketHandle=socket(AF_INET,SOCK_DGRAM,0);
    if (socketHandle==INVALID_SOCKET) return 1;
    local.sin_addr.s_addr=htonl(INADDR_ANY);
    if (::bind(socketHandle,(sockaddr *)&local,sizeof(local))) return 1;
    int temporary=1;
    ioctlsocket(socketHandle,FIONBIO,(unsigned long *)&temporary);
    FD_SET(socketHandle,&sockets);
    if (socketHandle>maximumSocket) maximumSocket=socketHandle;
    if (!port) {
        sockaddr_in bound;
        temporary=sizeof(bound);
        getsockname(socketHandle,(sockaddr *)&bound,&temporary);
        port=ntohs(bound.sin_port);
    }
    return 0;
}

int UDPCom::shareSocket(UDPCom *other)
{
    port=other->port;
    socketHandle=other->socketHandle;
    return 0;
}

int UDPCom::getPort() { return port; }

int UDPCom::setPeer(char *hostname,int value)
{
    memset(&peer,0,sizeof(peer));
    peer.sin_family=AF_INET;
    peer.sin_port=htons(value);
    if (!peer.sin_port) return 1;
    peer.sin_addr.s_addr=inet_addr(hostname);
    if (peer.sin_addr.s_addr==INADDR_NONE) {
        hostent *host=gethostbyname(hostname);
        if (!host) return 1;
        memcpy(&peer.sin_addr,host->h_addr,host->h_length);
    }
    return 0;
}

int UDPCom::setPeer(sockaddr_in *address) { peer=*address; return 0; }
int UDPCom::setPeerPort(int value) { peer.sin_port=htons(value); return !peer.sin_port; }

int UDPCom::send(char *data,int length)
{
    return sendto(socketHandle,data,length,0,(sockaddr *)&peer,sizeof(peer))==SOCKET_ERROR;
}

int UDPCom::receive(char *data,int *length,char peek,sockaddr_in *address)
{
    sockaddr_in sender;
    int addressLength=sizeof(sender);
    readable=0;
    int result=recvfrom(socketHandle,data,*length,peek?MSG_PEEK:0,(sockaddr *)&sender,&addressLength);
    if (result==SOCKET_ERROR) return 1;
    *length=result;
    if (address) *address=sender;
    return 0;
}

int UDPCom::dataAvailable() { return readable; }
void UDPCom::sendEmpty()
{
    char data[16];
    sendto(socketHandle,data,0,0,(sockaddr *)&peer,sizeof(peer));
}

int UDPCom::selectSockets(int seconds)
{
    int descriptorCount=maximumSocket+1;
    selected=sockets;
    timeval timeout={seconds,0};
    if (!::select(descriptorCount,&selected,0,0,&timeout)) return 0;
    int count=connections.GetSize();
    for (int i=0;i<count;i++) {
        UDPCom *connection=(UDPCom *)connections.Get(i);
        if (FD_ISSET(connection->socketHandle,&selected)) connection->readable=1;
    }
    return 1;
}

C_ItemList<void> UDPCom::connections;
fd_set UDPCom::sockets;
fd_set UDPCom::selected;
SOCKET UDPCom::maximumSocket=0;
