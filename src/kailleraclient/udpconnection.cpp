#include "udpconnection.h"
#include "ptrlist.h"

UDPConnection::UDPConnection()
{
    socketHandle=0;
    connections.add(this);
    readable=0;
    if (!maximumSocket) {
        FD_ZERO(&sockets);
        FD_ZERO(&selected);
    }
}

UDPConnection::~UDPConnection()
{
    if (socketHandle) {
        shutdown(socketHandle,2);
        closesocket(socketHandle);
        FD_CLR(socketHandle,&sockets);
        if (socketHandle==maximumSocket) {
            maximumSocket=0;
            for (int i=0;i<connections.size();i++) {
                if (connections.get(i)!=this) {
                    SOCKET handle=((UDPConnection *)connections.get(i))->socketHandle;
                    if (handle>maximumSocket) {
                        maximumSocket=handle;
                    }
                }
            }
        }
    }
    for (int i=0;i<connections.size();i++) {
        if (connections.get(i)==this) { connections.remove(i); i--; }
    }
}

int UDPConnection::bind(int value)
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

int UDPConnection::shareSocket(UDPConnection *other)
{
    port=other->port;
    socketHandle=other->socketHandle;
    return 0;
}

int UDPConnection::getPort() { return port; }

int UDPConnection::setPeer(char *hostname,int value)
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

int UDPConnection::setPeer(sockaddr_in *address) { peer=*address; return 0; }
int UDPConnection::setPeerPort(int value) { peer.sin_port=htons(value); return !peer.sin_port; }

int UDPConnection::send(char *data,int length)
{
    return sendto(socketHandle,data,length,0,(sockaddr *)&peer,sizeof(peer))==SOCKET_ERROR;
}

int UDPConnection::receive(char *data,int *length,char peek,sockaddr_in *address)
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

int UDPConnection::dataAvailable() { return readable; }
void UDPConnection::sendEmpty()
{
    char data[16];
    sendto(socketHandle,data,0,0,(sockaddr *)&peer,sizeof(peer));
}

int UDPConnection::selectSockets(int seconds)
{
    int descriptorCount=maximumSocket+1;
    selected=sockets;
    timeval timeout={seconds,0};
    if (!::select(descriptorCount,&selected,0,0,&timeout)) return 0;
    int count=connections.size();
    for (int i=0;i<count;i++) {
        UDPConnection *connection=(UDPConnection *)connections.get(i);
        if (FD_ISSET(connection->socketHandle,&selected)) connection->readable=1;
    }
    return 1;
}

PtrList UDPConnection::connections;
fd_set UDPConnection::sockets;
fd_set UDPConnection::selected;
SOCKET UDPConnection::maximumSocket=0;
