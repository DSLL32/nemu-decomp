/*
    WASTE - listview.h (Class for management of Windows listviews)
    Copyright (C) 2003 Nullsoft, Inc.

    WASTE is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    WASTE  is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with WASTE; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

// Adapted to the Kaillera revision; see analysis/kaillera-shared-helper-sources.md.
#ifndef KAILLERA_LISTVIEW_H
#define KAILLERA_LISTVIEW_H
#include <windows.h>
#include <commctrl.h>

class W_ListView {
public:
    W_ListView() { m_hwnd=NULL; m_sort_type=0; m_sort_col=0; m_sort_dir=1; m_col=0; }
    ~W_ListView() {}
    void AddCol(char *title,int percent=0);
    int GetParam(int index);
    int InsertItem(int index,char *text,int data);
    void SetItemText(int index,int column,char *text);
    void GetText(int index,int column,char *text,int size) {
        ListView_GetItemText(m_hwnd,index,column,text,size);
    }
    int _docompare(char *first,char *second);
    static int CALLBACK sortfunc(LPARAM first,LPARAM second,LPARAM context);
    void Resort();
    int FindItemByParam(LPARAM data) {
        LVFINDINFO find={LVFI_PARAM,0,data};
        return ListView_FindItem(m_hwnd,-1,&find);
    }
    HWND m_hwnd;
    int m_col;
    int m_sort_col;
    int m_sort_dir;
    int m_sort_type;
};
#endif
