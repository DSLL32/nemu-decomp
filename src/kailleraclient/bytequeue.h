// Growable byte queues used by the game input caches.
#ifndef KAILLERA_BYTEQUEUE_H
#define KAILLERA_BYTEQUEUE_H
#include <stdlib.h>
#include <string.h>
#include "itemlist.h"

class ByteQueue {
public:
    ByteQueue() { used=0; data=0; capacity=0; }
    inline ~ByteQueue();
    inline void append(void *bytes,int length);
    inline void peek(void *bytes,int length);
    inline void read(void *bytes,int length);
    char *data;
    int used,capacity;
};

// Cached entries have an out-of-line constructor in the original code.
// The private type names and this hierarchy are inferred from native emission.
class CachedByteQueue : public ByteQueue {
public:
    CachedByteQueue();
};

class ByteQueueList : public C_ItemList<ByteQueue> {
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
