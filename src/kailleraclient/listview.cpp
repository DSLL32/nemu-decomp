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
#include <windows.h>
#include <commctrl.h>
#include <string.h>
#include <stdlib.h>
#include "listview.h"

void W_ListView::AddCol(char *title,int percent)
{
    RECT rect;
    GetClientRect(m_hwnd,&rect);
    rect.right-=rect.left+2;
    LVCOLUMN column={0};
    column.mask=LVCF_TEXT|LVCF_WIDTH;
    column.pszText=title;
    if (percent) column.cx=MulDiv(rect.right,percent,100);
    ListView_InsertColumn(m_hwnd,m_col,&column);
    m_col++;
}

int W_ListView::GetParam(int index)
{
    LVITEM item={0};
    item.mask=LVIF_PARAM;
    item.iItem=index;
    ListView_GetItem(m_hwnd,&item);
    return item.lParam;
}

int W_ListView::InsertItem(int index,char *text,int data)
{
    LVITEM item={0};
    item.mask=LVIF_TEXT|LVIF_PARAM;
    item.iItem=index;
    item.pszText=text;
    item.cchTextMax=strlen(text);
    item.lParam=data;
    return ListView_InsertItem(m_hwnd,&item);
}

void W_ListView::SetItemText(int index,int column,char *text)
{
    LVITEM item={0};
    item.mask=LVIF_TEXT;
    item.iItem=index;
    item.iSubItem=column;
    item.pszText=text;
    item.cchTextMax=strlen(text);
    ListView_SetItem(m_hwnd,&item);
}

int W_ListView::_docompare(char *first,char *second)
{
    if (m_sort_type) {
        if (*first=='>') return m_sort_dir==-1?-1:1;
        if (*second=='>') return m_sort_dir==-1?1:-1;
        int result=atoi(first)-atoi(second);
        if (m_sort_dir==-1) return -result;
        return result;
    }
    int result=stricmp(first,second);
    if (m_sort_dir==-1) return -result;
    return result;
}

int CALLBACK W_ListView::sortfunc(LPARAM first,LPARAM second,LPARAM context)
{
    W_ListView *view=(W_ListView *)context;
    int a=view->FindItemByParam(first);
    int b=view->FindItemByParam(second);
    if (a==-1) return view->m_sort_dir;
    if (b==-1) return -view->m_sort_dir;
    char text1[128],text2[128];
    ListView_GetItemText(view->m_hwnd,a,view->m_sort_col,text1,sizeof(text1));
    ListView_GetItemText(view->m_hwnd,b,view->m_sort_col,text2,sizeof(text2));
    text2[127]=0;
    text1[127]=0;
    return view->_docompare(text1,text2);
}

void W_ListView::Resort()
{
    ListView_SortItems(m_hwnd,sortfunc,(LPARAM)this);
}
