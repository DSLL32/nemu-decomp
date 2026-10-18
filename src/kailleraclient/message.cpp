#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "message.h"

Message::Message()
{
    type=0;
    nick[0]=0;
    length=0;
    capacity=16;
    buffer=(char *)malloc(capacity);
    buffer[0]=0;
}

Message::~Message() { free(buffer); }

Message::Message(const Message &other)
{
    type=other.type;
    strcpy(nick,other.nick);
    capacity=other.capacity;
    buffer=(char *)malloc(capacity);
    memcpy(buffer,other.buffer,other.length);
    length=other.length;
}

void Message::reserve(int size)
{
    if (size>capacity) {
        while (size>capacity) capacity*=2;
        buffer=(char *)realloc(buffer,capacity);
    }
}

void Message::setNick(char *text)
{
    int size=min(strlen(text),31);
    strncpy(nick,text,size);
    nick[size]=0;
}

void Message::append(void *data,int size)
{
    reserve(length+size);
    memcpy(buffer+length,data,size);
    length+=size;
}

void Message::read(void *data,int size)
{
    if (length) {
        if (size>length) size=length;
        memcpy(data,buffer,size);
        length-=size;
        memcpy(buffer,buffer+size,length);
    }
}

void Message::appendString(char *text) { append(text,strlen(text)+1); }

void Message::readString(char *text,int size)
{
    if (size>(int)strlen(buffer)) size=strlen(buffer);
    if (length+1<=size) size=length+1;
    strncpy(text,buffer,size);
    *(size+text)=0;
    length-=size+1;
    memcpy(buffer,buffer+size+1,length);
}

void Message::appendInt(int value) { append(&value,4); }
int Message::readInt() { int value=0; read(&value,4); return value; }
void Message::appendByte(char value) { append(&value,1); }
unsigned char Message::readByte() { unsigned char value=0; read(&value,1); return value; }
void Message::appendWord(short value) { append(&value,2); }
unsigned short Message::readWord() { unsigned short value=0; read(&value,2); return value; }

int Message::pack(char *data,int size)
{
    data[0]=(char)type;
    strcpy(data+1,nick);
    int header=strlen(nick)+2;
    size-=header;
    int bytes=length>size?size:length;
    memcpy(data+header,buffer,bytes);
    return header+bytes;
}

void Message::unpack(char *data,int size)
{
    type=*data;
    char *p=data+1;
    int bytes=0;
    while (*p) {
        char c=*p++;
        nick[bytes++]=c;
    }
    nick[bytes]=0;
    int count=size-bytes-2;
    reserve(count);
    memcpy(buffer,p+1,count);
    length=count;
}
