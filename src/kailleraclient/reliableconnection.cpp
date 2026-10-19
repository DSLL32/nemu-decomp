#include "reliableconnection.h"
#include <stdlib.h>
#include <string.h>

KLRCom::KLRCom()
{
    sendNumber=receiveNumber=0;
    redundancy=3;
}

KLRCom::~KLRCom()
{

}

void KLRCom::sendMessage(Message *message)
{
    char data[32768];
    int length=message->pack(data,32767);
    send(data,length);
}

int KLRCom::send(char *data,int length)
{
    unsigned short number=sendNumber++;
    outgoing.add(number,data,length);
    resend(redundancy);
    return 0;
}

int KLRCom::resend(int count)
{
    char data[32768];
    int offset=1;
    int messages=count>outgoing.count?outgoing.count:count;
    data[0]=(char)messages;
    for (int i=0;i<messages;i++) {
        offset+=outgoing.pack(outgoing.count-i-1,data+offset);
    }
    UDPCom::send(data,offset);
    return 0;
}

int KLRCom::receiveMessage(Message *message,char peek,sockaddr_in *peer)
{
    char data[32768];
    int length=32768;
    if (receive(data,&length,peek,peer)) return 1;
    message->unpack(data,length);
    return 0;
}

int KLRCom::receive(char *data,int *length,char peek,sockaddr_in *peer)
{
    PacketCache *cache=&incoming;
    int size;
    if (!cache->get(receiveNumber,data,length)) goto found;
    {
        char packet[32768];
        memset(packet,0,sizeof(packet));
        size=32767;
        if (UDPCom::receive(packet,&size,0,peer)) return 1;
        if (size<2) return 1;
        int count=*(unsigned int *)packet&255;
        char *p=packet+1;
        for (int n=0;n<count;n++) {
            unsigned short number=*(unsigned short *)p;
            int bytes=*(unsigned short *)(p+2);
            if (number<receiveNumber) break;
            cache->add(number,p+4,bytes);
            p+=bytes+4;
        }
    }
    if (cache->get(receiveNumber,data,length)) return 1;
found:
    if (!peek) receiveNumber++;
    cache->removeEarlier(receiveNumber);
    return 0;
}

int KLRCom::dataAvailable()
{
    if (incoming.count) return 1;
    return UDPCom::dataAvailable();
}

void KLRCom::forceResend(int count) { resend(count); }
