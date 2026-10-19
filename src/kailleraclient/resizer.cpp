#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "client.h"
#include "resizer.h"

Resizer::Resizer()
{
    window=0;
}

Resizer::~Resizer()
{
    clear();
}

void Resizer::clear()
{
    while (controls.GetSize()>0) {
        free(controls.Get(0));
        controls.Del(0);
    }
}

void Resizer::add(HWND child,int flags,int left,int right,int top,int bottom)
{
    Item *item=(Item *)malloc(sizeof(Item));
    item->flags=flags;
    item->left=left;
    item->child=child;
    item->right=right;
    item->top=top;
    item->bottom=bottom;
    if (child) {
        RECT rect;
        GetWindowRect(child,&rect);
        item->x=rect.left;
        item->y=rect.top;
        item->width=rect.right-rect.left;
        item->height=rect.bottom-rect.top;
        char className[256];
        GetClassName(child,className,255);
        if (!strcmp(className,"SysListView32") || !strcmp(className,"Edit"))
            item->invalidate=1;
        else item->invalidate=0;
    } else {
        item->invalidate=0;
        item->height=0;
        item->width=0;
        item->y=0;
        item->x=0;
    }
    controls.Add(item);
}

void Resizer::add(int childID,int flags,int left,int right,int top,int bottom)
{
    add(GetDlgItem(window,childID),flags,left,right,top,bottom);
}

void Resizer::resize()
{
    RECT client;
    GetClientRect(window,&client);
    int clientWidth=client.right-client.left;
    int clientHeight=client.bottom-client.top;
    int height=clientHeight;
    int x=0,y=0,width=clientWidth;
    for (int i=0;i<controls.GetSize();i++) {
        Item *item=(Item *)controls.Get(i);
        if (height<item->height+item->top+item->bottom && !(item->flags&StretchY)) {
            int difference=item->height+item->top+item->bottom-height;
            int j;
            for (j=i-1;j>=0;j--) if (((Item *)controls.Get(j))->flags&StretchY) break;
            if (j>=0) {
                ((Item *)controls.Get(j))->height-=difference;
                while (++j!=i) ((Item *)controls.Get(j))->y-=difference;
                height+=difference;
                y-=difference;
            }
        }
        if (width<item->width+item->left+item->right && !(item->flags&StretchX)) {
            int difference=item->width+item->left+item->right-width;
            int j;
            for (j=i-1;j>=0;j--) if (((Item *)controls.Get(j))->flags&StretchX) break;
            if (j>=0) {
                ((Item *)controls.Get(j))->width-=difference;
                while (++j!=i) ((Item *)controls.Get(j))->x-=difference;
                width+=difference;
                x-=difference;
            }
        }
        item->x=item->left+x;
        item->y=item->top+y;
        if (item->flags&StretchX) item->width=width-item->left-item->right;
        if (item->flags&StretchY) item->height=height-item->top-item->bottom;
        if (item->flags&NewRow) {
            int used=0;
            x=0;
            width=clientWidth;
            for (int j=i-1;j>=0;j--) {
                if (((Item *)controls.Get(j))->flags==NewRow) break;
                if (used<=((Item *)controls.Get(j))->height+((Item *)controls.Get(j))->top+((Item *)controls.Get(j))->bottom)
                    used=((Item *)controls.Get(j))->height+((Item *)controls.Get(j))->top+((Item *)controls.Get(j))->bottom;
            }
            y+=used;
            height-=used;
        } else {
            int used=item->width+item->left+item->right;
            x+=used;
            width-=used;
        }
    }
    HRGN region=CreateRectRgn(0,0,clientWidth,clientHeight);
    HDWP positions=BeginDeferWindowPos(controls.GetSize());
    for (i=0;i<controls.GetSize();i++) {
        Item *item=(Item *)controls.Get(i);
        if (item->child) {
            DeferWindowPos(positions,item->child,0,item->x,item->y,item->width,item->height,SWP_NOZORDER);
            if (item->invalidate) {
                HRGN child=CreateRectRgn(item->x,item->y,item->width,item->height);
                HRGN copy=CreateRectRgn(0,0,1,1);
                CombineRgn(copy,region,0,RGN_COPY);
                CombineRgn(region,copy,child,RGN_DIFF);
                DeleteObject(child);
                DeleteObject(copy);
            }
        }
    }
    EndDeferWindowPos(positions);
    InvalidateRgn(window,region,TRUE);
    DeleteObject(region);
}

void Resizer::excludeChildren(HDC dc)
{
    for (int i=0;i<controls.GetSize();i++) {
        Item *item=(Item *)controls.Get(i);
        if (item->child) ExcludeClipRect(dc,item->x,item->y,item->width+item->x,item->height+item->y);
    }
}

void Resizer::loadPosition(char *name)
{
    RECT rect;
    char key[512];
    GetWindowRect(window,&rect);
    wsprintf(key,"%s_x",name);
    int x=settings->readInt(key,rect.left);
    wsprintf(key,"%s_y",name);
    int y=settings->readInt(key,rect.top);
    wsprintf(key,"%s_cx",name);
    int width=settings->readInt(key,rect.right-rect.left);
    wsprintf(key,"%s_cy",name);
    int height=settings->readInt(key,rect.bottom-rect.top);
    RECT desktop;
    GetWindowRect(GetDesktopWindow(),&desktop);
    if (x<=desktop.right && y<=desktop.bottom)
        SetWindowPos(window,0,x,y,width,height,SWP_NOZORDER);
}

void Resizer::savePosition(char *name)
{
    RECT rect;
    char key[512];
    GetWindowRect(window,&rect);
    wsprintf(key,"%s_x",name);
    settings->writeInt(key,rect.left);
    wsprintf(key,"%s_y",name);
    settings->writeInt(key,rect.top);
    wsprintf(key,"%s_cx",name);
    settings->writeInt(key,rect.right-rect.left);
    wsprintf(key,"%s_cy",name);
    settings->writeInt(key,rect.bottom-rect.top);
    settings->flush();
}
