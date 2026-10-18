// Growable byte queues used by the game input caches.
#ifndef KAILLERA_BYTEQUEUE_H
#define KAILLERA_BYTEQUEUE_H
#include <stdlib.h>
#include <string.h>
#include "ptrlist.h"

class ByteQueue {
public:
    ByteQueue();
    ~ByteQueue();
    void append(void *bytes,int length);
    void peek(void *bytes,int length);
    void read(void *bytes,int length);
    char *data;
    int used,capacity;
};

class ByteQueueList : public PtrList<ByteQueue> {
public:
    void clear() { while (size()>0) { delete get(0); remove(0); } }
    ~ByteQueueList() { clear(); }
};
#endif
