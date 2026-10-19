// Reconstructed reliable message layer over UDP.
#ifndef KAILLERA_RELIABLECONNECTION_H
#define KAILLERA_RELIABLECONNECTION_H
#include "udpconnection.h"
#include "message.h"
#include "packetcache.h"

class ReliableConnection : public UDPConnection {
public:
    ReliableConnection();
    virtual ~ReliableConnection();
    void sendMessage(Message *message);
    virtual int send(char *data,int length);
    int resend(int count);
    int receiveMessage(Message *message,char peek=0,sockaddr_in *peer=0);
    virtual int receive(char *data,int *length,char peek=0,sockaddr_in *peer=0);
    virtual int dataAvailable();
    void forceResend(int count);
private:
    unsigned short sendNumber,receiveNumber;
    PacketCache outgoing;
    PacketCache incoming;
    int redundancy;
};
#endif
