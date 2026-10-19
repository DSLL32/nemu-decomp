// Inferred shared untyped pointer list, growing in blocks of 32 entries.
#ifndef KAILLERA_PTRLIST_H
#define KAILLERA_PTRLIST_H
#include <stdlib.h>
#include <string.h>

class PtrList {
public:
    PtrList() { count=0; items=0; }
    ~PtrList() { if (items) free(items); }
    int size() { return count; }
    void *get(int index) { if (index<0 || index>=count) return 0; return items[index]; }
    void *add(void *item) {
        if (!items || !(count&31)) items=(void **)realloc(items,(count+32)*sizeof(void *));
        items[count++]=item; return item;
    }
    void remove(int index) {
        if (items && index>=0 && index<count) {
            count--;
            if (index!=count) memcpy(items+index,items+index+1,(count-index)*sizeof(void *));
            if (!(count&31) && count) items=(void **)realloc(items,count*sizeof(void *));
        }
    }
private:
    void **items;
    int count;
};
#endif
