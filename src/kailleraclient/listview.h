// Reconstructed list-view wrapper; private names and header boundary inferred.
#ifndef KAILLERA_LISTVIEW_H
#define KAILLERA_LISTVIEW_H
#include <windows.h>
#include <commctrl.h>

class ListView {
public:
    ListView() { window=0; columnCount=0; sortColumn=0; sortDirection=1; numericSort=0; }
    ~ListView() {}
    void addColumn(char *title,int percent=0);
    LPARAM itemData(int index);
    int insert(int index,char *text,LPARAM data);
    void setText(int index,int column,char *text);
    int compare(char *first,char *second);
    static int CALLBACK sortCallback(LPARAM first,LPARAM second,LPARAM context);
    void sort();
    int findData(LPARAM data) {
        LVFINDINFO find={LVFI_PARAM,0,data};
        return ListView_FindItem(window,-1,&find);
    }
    HWND window;
    int columnCount;
    int sortColumn;
    int sortDirection;
    int numericSort;
};
#endif
