// Inferred shared pointer-list template, growing in blocks of 32 entries.
#ifndef KAILLERA_PTRLIST_H
#define KAILLERA_PTRLIST_H
#include <stdlib.h>
#include <string.h>

template<class T> class PtrList {
public:
    PtrList() { count=0; items=0; }
    ~PtrList() { if (items) free(items); }
    int size() { return count; }
    T *get(int index) { if (index<0 || index>=count) return 0; return items[index]; }
    T *add(T *item) {
        if (!items || !(count&31)) items=(T **)realloc(items,(count+32)*sizeof(T *));
        items[count++]=item; return item;
    }
    void remove(int index) {
        if (items && index>=0 && index<count) {
            count--;
            if (index!=count) memcpy(items+index,items+index+1,(count-index)*sizeof(T *));
            if (!(count&31) && count) items=(T **)realloc(items,count*sizeof(T *));
        }
    }
private:
    T **items;
    int count;
};
#endif
