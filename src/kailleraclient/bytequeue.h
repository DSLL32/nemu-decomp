// Growable byte queues used by the game input caches.
#ifndef KAILLERA_BYTEQUEUE_H
#define KAILLERA_BYTEQUEUE_H
#include <stdlib.h>
#include <string.h>
#include "itemlist.h"

class ByteQueue {
public:
    ByteQueue();
    inline ~ByteQueue();
    inline void append(void *bytes,int length);
    inline void peek(void *bytes,int length);
    inline void read(void *bytes,int length);
    char *data;
    int used,capacity;
};

class ByteQueueList : public C_ItemList<void> {
public:
    ByteQueueList();
    inline void clear();
    ~ByteQueueList();
    inline int find(void *data,int length);
    inline int dataSize(int index);
    inline char *dataAt(int index);
    inline void addData(void *data,int length);
};
#endif
