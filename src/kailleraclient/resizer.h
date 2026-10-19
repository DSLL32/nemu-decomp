// Reconstructed layout manager; original private names are unknown.
#ifndef KAILLERA_RESIZER_H
#define KAILLERA_RESIZER_H
#include <windows.h>
#include "itemlist.h"

class Resizer {
public:
    enum { StretchX=1,StretchY=2,NewRow=4 };
    Resizer();
    ~Resizer();
    void clear();
    void add(HWND child,int flags,int left,int right,int top,int bottom);
    void add(int childID,int flags,int left,int right,int top,int bottom);
    void resize();
    void excludeChildren(HDC dc);
    void loadPosition(char *name);
    void savePosition(char *name);
    HWND window;
private:
    struct Item {
        HWND child;
        int flags;
        int x,y,width,height;
        int left,right,top,bottom;
        int invalidate;
    };
    C_ItemList<Item> controls;
};
#endif
