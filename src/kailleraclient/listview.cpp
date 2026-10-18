#include <windows.h>
#include <commctrl.h>
#include <string.h>
#include <stdlib.h>
#include "listview.h"

void ListView::addColumn(char *title,int percent)
{
    RECT rect;
    GetClientRect(window,&rect);
    rect.right-=rect.left+2;
    LVCOLUMN column={0};
    column.mask=LVCF_TEXT|LVCF_WIDTH;
    column.pszText=title;
    if (percent) column.cx=MulDiv(rect.right,percent,100);
    ListView_InsertColumn(window,columnCount,&column);
    columnCount++;
}

LPARAM ListView::itemData(int index)
{
    LVITEM item={0};
    item.mask=LVIF_PARAM;
    item.iItem=index;
    ListView_GetItem(window,&item);
    return item.lParam;
}

int ListView::insert(int index,char *text,LPARAM data)
{
    LVITEM item={0};
    item.mask=LVIF_TEXT|LVIF_PARAM;
    item.iItem=index;
    item.pszText=text;
    item.cchTextMax=strlen(text);
    item.lParam=data;
    return ListView_InsertItem(window,&item);
}

void ListView::setText(int index,int column,char *text)
{
    LVITEM item={0};
    item.mask=LVIF_TEXT;
    item.iItem=index;
    item.iSubItem=column;
    item.pszText=text;
    item.cchTextMax=strlen(text);
    ListView_SetItem(window,&item);
}

int ListView::compare(char *first,char *second)
{
    if (numericSort) {
        if (*first=='>') return sortDirection==-1?-1:1;
        if (*second=='>') return sortDirection==-1?1:-1;
        int result=atoi(first)-atoi(second);
        if (sortDirection==-1) return -result;
        return result;
    }
    int result=stricmp(first,second);
    if (sortDirection==-1) return -result;
    return result;
}

int CALLBACK ListView::sortCallback(LPARAM first,LPARAM second,LPARAM context)
{
    ListView *view=(ListView *)context;
    int a=view->findData(first);
    int b=view->findData(second);
    if (a==-1) return view->sortDirection;
    if (b==-1) return -view->sortDirection;
    char text1[128],text2[128];
    ListView_GetItemText(view->window,a,view->sortColumn,text1,sizeof(text1));
    ListView_GetItemText(view->window,b,view->sortColumn,text2,sizeof(text2));
    text2[127]=0;
    text1[127]=0;
    return view->compare(text1,text2);
}

void ListView::sort()
{
    ListView_SortItems(window,sortCallback,(LPARAM)this);
}
