#include "reliableconnection.h"
#include <stdlib.h>
#include <string.h>

ReliableConnection::ReliableConnection()
{
    sendNumber=receiveNumber=0;
    redundancy=3;
}

ReliableConnection::~ReliableConnection()
{

}

void ReliableConnection::sendMessage(Message *message)
{
    char data[32768];
    int length=message->pack(data,32767);
    send(data,length);
}

int ReliableConnection::send(char *data,int length)
{
    unsigned short number=sendNumber++;
    outgoing.add(number,data,length);
    resend(redundancy);
    return 0;
}

int ReliableConnection::resend(int count)
{
    char data[32768];
    int offset=1;
    int messages=count>outgoing.count?outgoing.count:count;
    data[0]=(char)messages;
    for (int i=0;i<messages;i++) {
        PacketCache::Record *record=&outgoing.entries[outgoing.count-i-1];
        offset+=record->pack(data+offset);
    }
    UDPConnection::send(data,offset);
    return 0;
}

int ReliableConnection::receiveMessage(Message *message,char peek,sockaddr_in *peer)
{
    char data[32768];
    int length=32768;
    if (receive(data,&length,peek,peer)) return 1;
    message->unpack(data,length);
    return 0;
}

int ReliableConnection::dataAvailable()
{
    if (incoming.count) return 1;
    return UDPConnection::dataAvailable();
}

int ReliableConnection::receive(char *data,int *length,char peek,sockaddr_in *peer)
{
    PacketCache *cache=&incoming;
    int i;
    int next=receiveNumber;
    for (i=0;i<cache->count;i++) if (cache->entries[i].number==next) goto found;
    {
        char packet[32768];
        memset(packet,0,sizeof(packet));
        int size=32767;
        if (UDPConnection::receive(packet,&size,0,peer)) return 1;
        if (size<2) return 1;
        int count=*(unsigned int *)packet&255;
        char *p=packet+1;
        for (int n=0;n<count;n++) {
            unsigned short number=*(unsigned short *)p;
            unsigned short bytes=*(unsigned short *)(p+2);
            if (number<receiveNumber) break;
            cache->add(number,p+4,bytes);
            p+=bytes+4;
        }
    }
    next=receiveNumber;
    for (i=0;i<cache->count;i++) if (cache->entries[i].number==next) goto found;
    return 1;
found:
    *length=cache->entries[i].size;
    memcpy(data,cache->entries[i].data,*length);
    if (!peek) receiveNumber++;
    cache->removeEarlier(receiveNumber);
    return 0;
}
