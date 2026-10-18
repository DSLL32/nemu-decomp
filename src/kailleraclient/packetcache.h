// The two inline cache members are inferred from their independent lifecycle
// ordering and the repeated member-relative insertion code in the DLL.
#ifndef KAILLERA_PACKETCACHE_H
#define KAILLERA_PACKETCACHE_H
#include <stdlib.h>
#include <string.h>

class PacketCache {
public:
    struct Record {
        unsigned short number,size;
        char *data;
        int pack(char *buffer) {
            int bytes=size+4;
            memcpy(buffer,this,4);
            buffer+=4;
            memcpy(buffer,data,size);
            return bytes;
        }
    };
    PacketCache() { count=0; }
    ~PacketCache() { for (int i=0;i<count;i++) free(entries[i].data); }
    void add(unsigned short number,char *data,int length) {
        for (int i=0;i<count;i++) if (entries[i].number==number) return;
        char *buffer;
        if (count==256) {
            if (length>entries[0].size) buffer=(char *)realloc(entries[0].data,length);
            else buffer=entries[0].data;
            memcpy(entries,entries+1,255*sizeof(Record));
            count--;
        } else buffer=(char *)malloc(length);
        Record record;
        record.number=number;
        record.size=(unsigned short)length;
        memcpy(buffer,data,length);
        record.data=buffer;
        entries[count++]=record;
    }
    void removeEarlier(int number) {
        for (int i=0;i<count;i++) {
            if (entries[i].number<number) {
                free(entries[i].data);
                memcpy(entries+i,entries+i+1,(count-i)*sizeof(Record)-sizeof(Record));
                count--;
                i--;
            }
        }
    }
    int get(int number,char *buffer,int *length) {
        for (int i=0;i<count;i++) {
            if (entries[i].number==number) {
                *length=entries[i].size;
                memcpy(buffer,entries[i].data,*length);
                return 0;
            }
        }
        return 1;
    }
    int pack(int index,char *buffer) { Record *record=entries+index; int bytes=record->size+4; memcpy(buffer,record,4); buffer+=4; memcpy(buffer,record->data,record->size); return bytes; }
    Record entries[256];
    int count;
};
#endif
