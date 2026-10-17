#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "WinMain.h"
#include "RomBrowser.h"
#include "FrontendIni.h"
#include "Rom.h"
#include "resource.h"

RomInformation CurrentRom;
char RomDirectory[260];
int BrowserSortMode;
unsigned long BrowserRomCount;










int CALLBACK CompareBrowserItems(LPARAM first, LPARAM second, LPARAM)
{
    LVFINDINFOA find;
    LVITEMA a, b;
    char aText[100], bText[100];
    char* end;
    find.flags = LVFI_PARAM;
    find.lParam = first;
    int firstIndex = ListView_FindItem(BrowserWindow, -1, &find);
    find.lParam = second;
    b.iItem = ListView_FindItem(BrowserWindow, -1, &find);
    a.pszText = aText;
    b.cchTextMax = a.cchTextMax = 100;
    b.mask = a.mask = LVIF_TEXT | LVIF_IMAGE;
    a.iItem = firstIndex;
    b.iSubItem = a.iSubItem = 0;
    b.pszText = bText;
    ListView_GetItem(BrowserWindow, &a);
    ListView_GetItem(BrowserWindow, &b);
    if (a.iImage == 5) {
        if (b.iImage != 5)
            goto negative;
        goto compareText;
    }
    if (b.iImage == 5)
        return BrowserSortMode;
    switch (BrowserSortMode) {
    case -2: case 2:
        b.iSubItem = a.iSubItem = 1;
        ListView_GetItem(BrowserWindow, &a);
        ListView_GetItem(BrowserWindow, &b);
    case -1: case 1:
compareText:
        return stricmp(a.pszText, b.pszText) * BrowserSortMode;
    case -3: case 3: {
        b.iSubItem = a.iSubItem = 2;
        ListView_GetItem(BrowserWindow, &a);
        ListView_GetItem(BrowserWindow, &b);
        int left = strtol(a.pszText, &end, 10);
        int right = strtol(b.pszText, &end, 10);
        if (left < right) return BrowserSortMode;
        if (left > right) { negative: return -BrowserSortMode; }
        return 0;
    }
    case -4: case 4:
        b.iSubItem = a.iSubItem = 3;
        ListView_GetItem(BrowserWindow, &a);
        ListView_GetItem(BrowserWindow, &b);
        return stricmp(a.pszText, b.pszText) * BrowserSortMode;
    case -5: case 5:
        b.iSubItem = a.iSubItem = 4;
        ListView_GetItem(BrowserWindow, &a);
        ListView_GetItem(BrowserWindow, &b);
        return stricmp(a.pszText, b.pszText) * BrowserSortMode;
    }
    return 0;
}

void RefreshRomBrowser()
{
    LVITEMA item;
    HANDLE search;
    WIN32_FIND_DATAA data;
    char text[100], status[100], title[100];
retry:
    sprintf(title, "Nemu64 - Exploring %s", RomDirectory);
    SetWindowTextA(MainWindow, title);
    ListView_DeleteAllItems(BrowserWindow);
    item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
    item.iItem = 0;
    item.iSubItem = 0;
    item.lParam = 1;
    BrowserRomCount = 0;
    strcpy(text, RomDirectory);
    strcat(text, "*.*");
    search = FindFirstFileA(text, &data);
    if (search == INVALID_HANDLE_VALUE)
        goto invalidDirectory;

    do {
        item.pszText = text;
        item.iSubItem = 0;
        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (stricmp(data.cFileName, ".") && stricmp(data.cFileName, "..")) {
                strcpy(text, data.cFileName);
                item.iImage = 5;
                ListView_InsertItem(BrowserWindow, &item);
                ++item.iItem;
                ++item.lParam;
            }
        } else if ((!stricmp(data.cFileName + strlen(data.cFileName) - 4, ".bin") ||
                    !stricmp(data.cFileName + strlen(data.cFileName) - 4, ".rom") ||
                    !stricmp(data.cFileName + strlen(data.cFileName) - 4, ".pal") ||
                    !stricmp(data.cFileName + strlen(data.cFileName) - 4, ".usa") ||
                    !stricmp(data.cFileName + strlen(data.cFileName) - 4, ".eur") ||
                    !stricmp(data.cFileName + strlen(data.cFileName) - 4, ".v64") ||
                    !stricmp(data.cFileName + strlen(data.cFileName) - 4, ".z64") ||
                    !stricmp(data.cFileName + strlen(data.cFileName) - 4, ".n64") ||
                    !stricmp(data.cFileName + strlen(data.cFileName) - 4, ".zip")) && data.nFileSizeLow > 1024) {
            item.iImage = 6;
            sprintf(status, "Getting info for %s...", data.cFileName);
            SetStatusText(0, status);
            UpdateWindow(MainWindow);
            strcpy(text, RomDirectory);
            strcat(text, data.cFileName);
            if (ReadRomImage(NULL, text, false, 0x40)) {
                ReadRomString("FriendlyName", CurrentRom.InternalName, CurrentRom.DisplayName);
                strcpy(text, CurrentRom.DisplayName);
                item.iItem = ListView_InsertItem(BrowserWindow, &item);
                item.mask &= ~LVIF_PARAM;
                strcpy(text, data.cFileName);
                item.iSubItem = 1;
                ListView_SetItem(BrowserWindow, &item);
                if (!stricmp(data.cFileName + strlen(data.cFileName) - 4, ".zip")) {
                    if (CurrentRom.FileSize)
                        sprintf(text, "%u MBIT (Ratio %u%%)", CurrentRom.FileSize >> 17,
                                (CurrentRom.FileSize - CurrentRom.CompressedSize) * 100 / CurrentRom.FileSize);
                    else
                        sprintf(text, "0 MBIT");
                } else
                    sprintf(text, "%u MBIT", data.nFileSizeLow >> 17);
                item.iSubItem = 2;
                ListView_SetItem(BrowserWindow, &item);
                ReadRomString("Comment", "(none)", text);
                int position = (int)strchr(text, '|');
                if (position) {
                    position -= (int)text;
                    text[position - 1] = '\0';
                    item.iSubItem = 3;
                    ListView_SetItem(BrowserWindow, &item);
                    item.pszText = text + position + 2;
                }
                item.iSubItem = 4;
                ListView_SetItem(BrowserWindow, &item);
                ++BrowserRomCount;
                item.pszText = CurrentRom.InternalName;
                item.iSubItem = 5;
                ListView_SetItem(BrowserWindow, &item);
                item.pszText = CurrentRom.IniSection;
                item.iSubItem = 6;
                ListView_SetItem(BrowserWindow, &item);
                ++item.iItem;
                ++item.lParam;
                item.mask |= LVIF_PARAM;
                FreeRomImage();
                memset(&CurrentRom, 0, sizeof(CurrentRom));
            }
        }
    } while (FindNextFileA(search, &data));
    FindClose(search);
    SetStatusText(0, "I am Bender - please insert Gurder");
    ListView_SortItems(BrowserWindow, CompareBrowserItems, BrowserSortMode);
    return;
invalidDirectory:
    MessageBoxA(MainWindow, "Error accessing drive or invalid folder. Setting to C:\\", "Error", MB_ICONERROR);
    strcpy(RomDirectory, "C:\\");
    goto retry;
}
