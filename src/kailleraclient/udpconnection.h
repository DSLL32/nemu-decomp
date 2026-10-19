// Reconstructed UDP transport used by Kaillera 0.84.
#ifndef KAILLERA_UDPCONNECTION_H
#define KAILLERA_UDPCONNECTION_H
#include <windows.h>
#include "itemlist.h"

class UDPCom {
public:
    UDPCom();
    virtual ~UDPCom();
    virtual int shareSocket(UDPCom *other);
    virtual int bind(int port);
    virtual int getPort();
    virtual int setPeer(char *hostname,int port);
    virtual int setPeer(sockaddr_in *address);
    virtual int setPeerPort(int port);
    virtual int send(char *data,int length);
    virtual int receive(char *data,int *length,char peek=0,sockaddr_in *peer=0);
    virtual int dataAvailable();
    virtual void sendEmpty();
    static int selectSockets(int seconds);
private:
    static C_ItemList<UDPCom> connections;
    static fd_set sockets;
    static fd_set selected;
    static SOCKET maximumSocket;
protected:
    int port;
    SOCKET socketHandle;
    int readable;
    sockaddr_in peer;
};
#endif
