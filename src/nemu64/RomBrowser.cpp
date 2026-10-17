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

int WindowHeight(HWND window)
{
    RECT rectangle;
    GetWindowRect(window, &rectangle);
    return rectangle.bottom - rectangle.top;
}

void SetStatusText(unsigned int part, const char* text)
{
    SendMessageA(StatusWindow, SB_SETTEXTA, part, (LPARAM)text);
}

void __stdcall NetplayMessage(const char* text)
{
    MessageBoxA(MainWindow, text, "", 0);
}

void RebuildFavouritesMenu()
{
    MENUITEMINFOA item;
    char text[256], filename[256], firstLine[256];
    if (FavouritesMenu)
        DestroyMenu(FavouritesMenu);
    FavouritesMenu = CreatePopupMenu();
    item.cbSize = sizeof(item);
    item.fMask = MIIM_TYPE | MIIM_ID;
    item.fType = MFT_STRING;
    item.wID = 1;
    item.dwTypeData = text;
    strcpy(text, "Add this folder to favourites");
    InsertMenuItemA(FavouritesMenu, 0, TRUE, &item);
    strcpy(filename, ApplicationDirectory);
    strcat(filename, "fav.dat");
    FILE* file = fopen(filename, "rt");
    bool first = true;
    if (file) {
        while (!feof(file) && fgets(text, 256, file)) {
            if (first) {
                strcpy(firstLine, text);
                first = false;
                item.wID = 2;
                strcpy(text, "Remove a folder from favourites...");
                InsertMenuItemA(FavouritesMenu, 1, TRUE, &item);
                item.fType = MFT_SEPARATOR;
                item.wID = 3;
                InsertMenuItemA(FavouritesMenu, 2, TRUE, &item);
                item.wID = 4;
                item.fType = MFT_STRING;
                strcpy(text, firstLine);
            }
            if (text[strlen(text) - 1] == '\n')
                text[strlen(text) - 1] = '\0';
            InsertMenuItemA(FavouritesMenu, item.wID, TRUE, &item);
            ++item.wID;
        }
        fclose(file);
    }
}

void ShowFavouritesMenu()
{
    POINT point;
    RECT button;
    MENUITEMINFOA item;
    char folder[256], filename[256];
    RebuildFavouritesMenu();
    int index = SendMessageA(ToolbarWindow, TB_COMMANDTOINDEX, 0x801f, 0);
    SendMessageA(ToolbarWindow, TB_GETITEMRECT, index, (LPARAM)&button);
    point.x = button.left;
    point.y = button.bottom + 2;
    ClientToScreen(MainWindow, &point);
    int selection = TrackPopupMenuEx(FavouritesMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                     point.x, point.y, MainWindow, NULL);
    switch (selection) {
    case 0:
    case 3:
        return;
    case 1: {
        strcpy(filename, ApplicationDirectory);
        strcat(filename, "fav.dat");
        FILE* file = fopen(filename, "at");
        if (file) {
            strcpy(folder, RomDirectory);
            if (strlen(folder) != 3)
                folder[strlen(folder) - 1] = '\0';
            fprintf(file, "%s\n", folder);
            fclose(file);
        } else {
            MessageBoxA(MainWindow, "There was an error opening the favourites file", "Error", 0);
        }
        return;
    }
    case 2:
        DialogBoxParamA(ApplicationInstance, MAKEINTRESOURCEA(IDD_REMOVE_FAVOURITES),
                        MainWindow, RemoveFavouritesDialogProc, 0);
        return;
    default:
        item.cbSize = sizeof(item);
        item.fMask = MIIM_TYPE;
        item.dwTypeData = RomDirectory;
        item.cch = 256;
        GetMenuItemInfoA(FavouritesMenu, selection - 1, TRUE, &item);
        if (lstrlenA(RomDirectory) && RomDirectory[lstrlenA(RomDirectory) - 1] != '\\')
            lstrcatA(RomDirectory, "\\");
        RefreshRomBrowser();
        return;
    }
}

void ShowDriveMenu()
{
    POINT point;
    RECT button;
    int index = SendMessageA(ToolbarWindow, TB_COMMANDTOINDEX, 0x801e, 0);
    SendMessageA(ToolbarWindow, TB_GETITEMRECT, index, (LPARAM)&button);
    point.x = button.left;
    point.y = button.bottom + 2;
    ClientToScreen(MainWindow, &point);
    int selection = TrackPopupMenuEx(DriveMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                     point.x, point.y, MainWindow, NULL);
    if (selection) {
        strcpy(RomDirectory, "X:\\");
        RomDirectory[0] = selection + '@';
        RefreshRomBrowser();
    }
}

BOOL CALLBACK RemoveFavouritesDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM)
{
    char oldName[256], newName[256], text[256], filename[256], line[256];
    switch (message) {
    case WM_INITDIALOG: {
        strcpy(filename, ApplicationDirectory);
        strcat(filename, "fav.dat");
        FILE* file = fopen(filename, "rt");
        if (file) {
            while (!feof(file)) {
                if (!fgets(text, 256, file))
                    break;
                if (text[strlen(text) - 1] == '\n')
                    text[strlen(text) - 1] = '\0';
                SendMessageA(FindWindowExA(window, NULL, "LISTBOX", NULL), LB_ADDSTRING, 0, (LPARAM)text);
            }
            fclose(file);
        }
        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1035:
            EndDialog(window, 1035);
            return TRUE;
        case 1034: {
            strcpy(oldName, ApplicationDirectory);
            strcat(oldName, "fav.dat");
            FILE* input = fopen(oldName, "rt");
            strcpy(newName, oldName);
            newName[strlen(newName) - 1] = 'n';
            FILE* output = fopen(newName, "w+t");
            int index = 0;
            if (input && output) {
                while (!feof(input)) {
                    if (!fgets(line, 256, input))
                        break;
                    if (SendMessageA(FindWindowExA(window, NULL, "LISTBOX", NULL), LB_GETSEL, index, 0) <= 0)
                        fprintf(output, "%s", line);
                    ++index;
                }
                fclose(output);
                fclose(input);
                DeleteFileA(oldName);
                MoveFileA(newName, oldName);
            }
            EndDialog(window, 1034);
            return TRUE;
        }
        }
    }
    return FALSE;
}

BOOL CALLBACK AboutDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(window, LOWORD(wParam));
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_DISCLAIMER) {
            DialogBoxParamA(ApplicationInstance, MAKEINTRESOURCEA(IDD_DISCLAIMER), window, DisclaimerDialogProc, 0);
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_WEBSITE) {
            ShellExecuteA(window, "open", "http://www.nemu.com", "", "", SW_SHOW);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL CALLBACK DisclaimerDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM)
{
    char text[1000];
    switch (message) {
    case WM_INITDIALOG:
        LoadStringA(ApplicationInstance, IDS_STRING110, text, 1000);
        SendMessageA(FindWindowExA(window, NULL, "Edit", NULL), WM_SETTEXT, 0, (LPARAM)text);
        return TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            EndDialog(window, IDOK);
            return TRUE;
        }
    }
    return FALSE;
}

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
    a.iItem = firstIndex;
    a.pszText = aText;
    b.cchTextMax = a.cchTextMax = 100;
    b.mask = a.mask = LVIF_TEXT | LVIF_IMAGE;
    b.iSubItem = a.iSubItem = 0;
    b.pszText = bText;
    ListView_GetItem(BrowserWindow, &a);
    ListView_GetItem(BrowserWindow, &b);
    if (a.iImage == 5) {
        if (b.iImage != 5)
            return -BrowserSortMode;
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
        if (left > right) return -BrowserSortMode;
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
    do {
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
        if (search != INVALID_HANDLE_VALUE)
            break;
        MessageBoxA(MainWindow, "Error accessing drive or invalid folder. Setting to C:\\", "Error", MB_ICONERROR);
        strcpy(RomDirectory, "C:\\");
    } while (true);
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
}
