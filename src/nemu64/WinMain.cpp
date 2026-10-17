#include "WinMain.h"
#include "resource.h"
#include "Plugins.h"
#include "CPU.h"
#include "Memory.h"
#include "FrontendIni.h"
#include "RomBrowser.h"
#include "Rom.h"
#include "State.h"
#include "PIF.h"
#include "Kaillera.h"
#include "Dynarec.h"
#include <shellapi.h>
#include <commdlg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "EasterText.h"

HWND MainWindow;
HINSTANCE ApplicationInstance;
char ApplicationDirectory[MAX_PATH];
HWND ToolbarWindow;
HWND StatusWindow;
HWND BrowserWindow;
int MainWindowX;
int MainWindowY;
int DisplayResolution;
unsigned short StatePreview[120][160];
char StateFileName[256];
char EasterText[10000];
int EasterTextLength;
HIMAGELIST BrowserImageList;
HMENU DriveMenu;
HMENU FavouritesMenu;
bool BrowserInitialized;
bool Closing;
bool FullScreen;
unsigned int EasterTextPosition;
int ActivePause;
bool FrontendInitialized;

// Browser helpers use an internal register calling convention in the linked
// program. The declarations here describe their C++ parameters; /GL and LTCG
// decide that private convention when the complete program is linked.
extern int WindowHeight(HWND window);
__declspec(dllimport) void GSConfigure();

void ResizeMainWindow(unsigned int width, int height)
{
    RECT rect;
    rect.left = MainWindowX;
    rect.top = MainWindowY;
    rect.right = rect.left + width;
    rect.bottom = rect.top + height;
    if (SelectedGraphicsPlugin->SetResolution)
        SelectedGraphicsPlugin->SetResolution(width, height);
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);
    MoveWindow(MainWindow, rect.left, rect.top - 27,
        rect.right - rect.left,
        rect.bottom - rect.top + WindowHeight(StatusWindow) + WindowHeight(ToolbarWindow), TRUE);
    switch (width)
    {
    case 320: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_320X240, MF_BYCOMMAND); break;
    case 400: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_400X300, MF_BYCOMMAND); break;
    case 640: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_640X480, MF_BYCOMMAND); break;
    case 800: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_800X600, MF_BYCOMMAND); break;
    case 1024: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_1024X768, MF_BYCOMMAND); break;
    case 1152: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_1152X864, MF_BYCOMMAND); break;
    case 1280: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_1280X960, MF_BYCOMMAND); break;
    case 1600: CheckMenuRadioItem(GetMenu(MainWindow), ID_320X240, ID_1600X1200, ID_1600X1200, MF_BYCOMMAND); break;
    }
}

void SetEmulationMenus(unsigned char running)
{
    unsigned int stopped = !running;
    EnableMenuItem(GetMenu(MainWindow), ID_CLOSE_ROM, stopped);
    EnableMenuItem(GetMenu(MainWindow), 0x8026, running);
    EnableMenuItem(GetMenu(MainWindow), ID_RESET, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_PAUSE, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_ACTIVE_PAUSE, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_SAVE_STATE, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_LOAD_STATE, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_QUICK_SAVESTATE, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_QUICK_LOADSTATE, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_START_RECORD, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_PLAY_START, stopped);
    EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_STOP_RECORD, MF_GRAYED);
    EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_PLAY_STOP, MF_GRAYED);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, ID_DIRECTORY_UP, stopped);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, 0x801e, stopped);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, 0x801f, stopped);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, 0x8026, stopped);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, ID_CLOSE_ROM, running);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, ID_PAUSE, running);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, ID_CLOSE_ROM, running);
    SendMessageA(ToolbarWindow, TB_ENABLEBUTTON, ID_RESET, running);
    if (running)
    {
        ShowWindow(BrowserWindow, SW_HIDE);
        EnableWindow(BrowserWindow, FALSE);
        HWND junk = CreateWindowExA(0, "EDIT", "Junk", 0, 0, 0, 0, 0,
            MainWindow, NULL, ApplicationInstance, NULL);
        SetFocus(junk);
        DestroyWindow(junk);
    }
    else
    {
        ShowWindow(BrowserWindow, SW_SHOW);
        EnableWindow(BrowserWindow, TRUE);
        SetFocus(BrowserWindow);
    }
    DrawMenuBar(MainWindow);
    InvalidateRect(MainWindow, NULL, TRUE);
}

void DrawStatePreview(HWND dialog)
{
    HWND child = GetWindow(dialog, GW_CHILD);
    HDC dc = GetDC(child);
    for (unsigned int y = 0; y < 120; y++)
        for (unsigned int x = 0; x < 160; x++)
        {
            unsigned int value = StatePreview[y][x] >> 1;
            BYTE red = (BYTE)(value >> 10);
            BYTE green = (BYTE)(value >> 5);
            BYTE blue = (BYTE)value;
            red <<= 3;
            green <<= 3;
            blue <<= 3;
            SetPixelV(dc, x, y, RGB(red, green, blue));
        }
    // The original releases this DC with a null HWND.
    ReleaseDC(NULL, dc);
}

void CaptureStatePreview()
{
    if (SelectedGraphicsPlugin->DrawScreen) SelectedGraphicsPlugin->DrawScreen();
    Sleep(100);
    SetStatusText(0, "Creating screenshot...");
    HDC dc = GetDC(MainWindow);
    RECT rect;
    GetClientRect(MainWindow, &rect);
    unsigned int width = rect.right - rect.left;
    unsigned int height = rect.bottom - rect.top - 45;
    unsigned int scaledX = 0;
    unsigned short* column = &StatePreview[0][0];
    for (int x = 0; x < 160; x++)
    {
        unsigned int pixelX = scaledX / 160;
        unsigned int scaledY = 0;
        unsigned short* pixel = column;
        for (int y = 0; y < 120; y++)
        {
            COLORREF value = GetPixel(dc, pixelX, scaledY / 120 + 27);
            *pixel = (unsigned short)(((GetRValue(value) >> 3) << 11) |
                ((GetGValue(value) >> 3) << 6) | ((GetBValue(value) >> 3) << 1));
            scaledY += height;
            pixel += 160;
        }
        scaledX += width;
        column++;
    }
    ReleaseDC(MainWindow, dc);
    SetStatusText(0, "Compressing and saving state...");
    CPU_ActionFlags |= 0x1000;
}

void AddToolbarButton(TBBUTTON* buttons, int* count, BYTE style, int bitmap,
                      BYTE state, int command)
{
    buttons[*count].fsState = state;
    buttons[*count].fsStyle = style;
    buttons[*count].iBitmap = bitmap;
    buttons[*count].idCommand = command;
    buttons[*count].dwData = 0;
    buttons[*count].iString = *count;
    ++*count;
}

UINT CALLBACK StatePreviewHook(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message != WM_PAINT)
    {
        if (message != WM_NOTIFY) return 0;
        OFNOTIFYA* notification = (OFNOTIFYA*)lParam;
        if (notification->hdr.code != CDN_SELCHANGE)
        {
            if (notification->hdr.code != CDN_INITDONE) return 0;
            SetWindowPos(GetWindow(dialog, GW_CHILD), NULL, 0, 0, 164, 124,
                SWP_NOMOVE | SWP_NOZORDER);
            return 0;
        }
        SendMessageA(notification->hdr.hwndFrom, CDM_GETFILEPATH, MAX_PATH, (LPARAM)StateFileName);
        if (!LoadState(true)) memset(StatePreview, 0, sizeof(StatePreview));
    }
    DrawStatePreview(dialog);
    return 0;
}

LRESULT CALLBACK MainWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    char text[500];
    char filename[256];
    PAINTSTRUCT paint;
    switch (message)
    {
    case WM_DESTROY:
        SaveBrowserSettings();
        Closing = true;
        if (CPU_Running)
        {
            CPU_ActionFlags |= 0x400000;
            ResumeThread(CPU_Thread);
            int timeout = 30;
            do
            {
                Sleep(100);
                timeout--;
                if (!CPU_Thread) break;
            } while (timeout >= 0);
        }
        DestroyMenu(DriveMenu);
        if (FavouritesMenu)
        {
            DestroyMenu(FavouritesMenu);
            FavouritesMenu = NULL;
        }
        PostQuitMessage(0);
        return 0;
    case WM_MOVE:
        MainWindowX = LOWORD(lParam);
        MainWindowY = HIWORD(lParam) + 27;
        if (SelectedGraphicsPlugin->MoveScreen)
            SelectedGraphicsPlugin->MoveScreen(MainWindowX, MainWindowY);
        return 0;
    case WM_SIZE:
        {
            int toolbarHeight = WindowHeight(ToolbarWindow);
            int statusHeight = WindowHeight(StatusWindow);
            MoveWindow(BrowserWindow, 0, toolbarHeight, LOWORD(lParam),
                HIWORD(lParam) - statusHeight - toolbarHeight, TRUE);
            MoveWindow(StatusWindow, 0, 0, 0, 0, FALSE);
            if (!BrowserInitialized)
            {
                BrowserInitialized = true;
                RefreshRomBrowser();
            }
            return 0;
        }
    case WM_CREATE:
        {
            MainWindow = window;
            InitCommonControls();
            BrowserWindow = CreateWindowExA(WS_EX_CLIENTEDGE, WC_LISTVIEWA, "RomSelector",
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_AUTOARRANGE, 0, 0, 100, 100,
                MainWindow, NULL, ApplicationInstance, NULL);
            StatusWindow = CreateWindowExA(0, STATUSCLASSNAMEA, "StatusBar", WS_CHILD | WS_VISIBLE,
                0, 0, 100, 100, MainWindow, NULL, ApplicationInstance, NULL);
            ToolbarWindow = CreateWindowExA(0, TOOLBARCLASSNAMEA, "ToolBar", WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT,
                0, 0, 100, 100, MainWindow, NULL, ApplicationInstance, NULL);
            BrowserImageList = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 15, 1);
            HBITMAP bitmap = (HBITMAP)LoadImageA(ApplicationInstance, MAKEINTRESOURCEA(IDB_BITMAP143),
                IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT);
            ImageList_AddMasked(BrowserImageList, bitmap, RGB(255, 0, 255));
            DeleteObject(bitmap);
            DriveMenu = CreatePopupMenu();
            MENUITEMINFOA menuItem;
            char menuText[256], drivePath[256], volumeName[256], fileSystemName[256];
            DWORD maximumNameLength, fileSystemFlags;
            menuItem.cbSize = sizeof(menuItem);
            menuItem.fMask = MIIM_TYPE | MIIM_ID;
            menuItem.fType = MFT_STRING;
            menuItem.dwTypeData = menuText;
            DWORD drives = GetLogicalDrives();
            for (unsigned int drive = 0; drive <= 26; drive++)
                if (drives & (1 << drive))
                {
                    menuItem.wID = drive + 1;
                    strcpy(text, "A:");
                    text[0] = (char)(drive + 'A');
                    strcpy(drivePath, text);
                    strcat(drivePath, "\\");
                    UINT type = GetDriveTypeA(drivePath);
                    if (type >= DRIVE_FIXED && type <= DRIVE_CDROM &&
                        GetVolumeInformationA(drivePath, volumeName, 256, NULL,
                            &maximumNameLength, &fileSystemFlags, fileSystemName, 256))
                    {
                        strcpy(menuText, volumeName);
                        strcat(menuText, " (");
                        strcat(menuText, text);
                        strcat(menuText, ")");
                    }
                    else strcpy(menuText, text);
                    InsertMenuItemA(DriveMenu, drive, TRUE, &menuItem);
                }
            FavouritesMenu = NULL;
            SendMessageA(ToolbarWindow, TB_SETIMAGELIST, 0, (LPARAM)BrowserImageList);
            TBBUTTON buttons[20];
            memset(buttons, 0, sizeof(buttons));
            int count = 0;
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 4, TBSTATE_ENABLED, ID_DIRECTORY_UP);
            AddToolbarButton(buttons, &count, TBSTYLE_DROPDOWN, 1, TBSTATE_ENABLED, 0x801e);
            AddToolbarButton(buttons, &count, TBSTYLE_DROPDOWN, 7, TBSTATE_ENABLED, 0x801f);
            AddToolbarButton(buttons, &count, TBSTYLE_SEP, 0, TBSTATE_ENABLED, ID_DIRECTORY_UP);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 12, TBSTATE_ENABLED, 0x8026);
            AddToolbarButton(buttons, &count, TBSTYLE_CHECK, 13, TBSTATE_ENABLED, ID_PAUSE);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 14, TBSTATE_ENABLED, ID_CLOSE_ROM);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 15, TBSTATE_ENABLED, ID_RESET);
            AddToolbarButton(buttons, &count, TBSTYLE_SEP, 0, TBSTATE_ENABLED, ID_DIRECTORY_UP);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 8, TBSTATE_ENABLED, ID_SETTINGS);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 9, TBSTATE_ENABLED, ID_CONFIGURE_AUDIO_PLUGIN);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 10, TBSTATE_ENABLED, ID_CONFIGURE_INPUT_PLUGIN);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 11, TBSTATE_ENABLED, ID_CONFIGURE_VIDEO_PLUGIN);
            AddToolbarButton(buttons, &count, TBSTYLE_SEP, 0, TBSTATE_ENABLED, ID_DIRECTORY_UP);
            AddToolbarButton(buttons, &count, TBSTYLE_BUTTON, 16, TBSTATE_ENABLED, ID_ABOUT);
            SendMessageA(ToolbarWindow, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
            SendMessageA(ToolbarWindow, TB_ADDBUTTONSA, count, (LPARAM)buttons);
            SendMessageA(ToolbarWindow, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);
            SendMessageA(ToolbarWindow, TB_AUTOSIZE, 0, 0);
            SendMessageA(BrowserWindow, LVM_SETEXTENDEDLISTVIEWSTYLE, 0x30, 0x30);
            SendMessageA(BrowserWindow, LVM_SETIMAGELIST, LVSIL_SMALL, (LPARAM)BrowserImageList);
            SendMessageA(BrowserWindow, LVM_SETIMAGELIST, LVSIL_NORMAL, (LPARAM)BrowserImageList);
            LVCOLUMNA column;
            char columnName[100];
            column.mask = LVCF_TEXT | LVCF_WIDTH;
            column.pszText = columnName;
            strcpy(columnName, "Rom name");
            column.cx = ReadIniInteger("Configuration", "Column0Width", 120, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 0, (LPARAM)&column);
            strcpy(columnName, "Filename");
            column.cx = ReadIniInteger("Configuration", "Column1Width", 120, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 1, (LPARAM)&column);
            strcpy(columnName, "Size");
            column.cx = ReadIniInteger("Configuration", "Column2Width", 59, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 2, (LPARAM)&column);
            strcpy(columnName, "Rating");
            column.cx = ReadIniInteger("Configuration", "Column3Width", 43, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 3, (LPARAM)&column);
            strcpy(columnName, "Comment");
            column.cx = ReadIniInteger("Configuration", "Column4Width", 240, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 4, (LPARAM)&column);
            strcpy(columnName, "INI Section");
            column.cx = ReadIniInteger("Configuration", "Column5Width", 0, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 5, (LPARAM)&column);
            strcpy(columnName, "CRC1");
            column.cx = ReadIniInteger("Configuration", "Column6Width", 0, ConfigurationIniPath);
            SendMessageA(BrowserWindow, LVM_INSERTCOLUMNA, 6, (LPARAM)&column);
            int columnOrder[7];
            if (GetPrivateProfileStructA("Configuration", "ColumnOrder", columnOrder,
                sizeof(columnOrder), ConfigurationIniPath))
                SendMessageA(BrowserWindow, LVM_SETCOLUMNORDERARRAY, 7, (LPARAM)columnOrder);
            CheckMenuItem(GetMenu(MainWindow), ID_SHOW_ERRORS, ShowErrors ? MF_CHECKED : MF_UNCHECKED);
            EnableMenuItem(GetMenu(MainWindow), ID_USE_VIRTUAL_SAVE_FILES, MF_GRAYED);
            DragAcceptFiles(MainWindow, TRUE);
            return 1;
        }
    case WM_PAINT:
        {
            BeginPaint(window, &paint);
            EndPaint(window, &paint);
            if (CPU_Running && SelectedGraphicsPlugin->DrawScreen)
                SelectedGraphicsPlugin->DrawScreen();
            return 0;
        }
    case WM_COMMAND:
        return MainWindowCommand(window, message, wParam, lParam);
    case WM_NOTIFY:
        {
            NMHDR* notification = (NMHDR*)lParam;
            if (notification->hwndFrom == BrowserWindow)
            {
                NMLISTVIEW* list = (NMLISTVIEW*)notification;
                switch (notification->code)
                {
                case LVN_COLUMNCLICK:
                    {
                        int order = list->iSubItem + 1;
                        if (BrowserSortMode == order) order = -order;
                        BrowserSortMode = order;
                        SendMessageA(BrowserWindow, LVM_SORTITEMS, order, (LPARAM)CompareBrowserItems);
                        return 0;
                    }
                case LVN_ITEMCHANGED:
                    {
                        LVITEMA item;
                        item.iSubItem = 5;
                        item.cchTextMax = 100;
                        item.pszText = CurrentRom.InternalName;
                        SendMessageA(BrowserWindow, LVM_GETITEMTEXTA, list->iItem, (LPARAM)&item);
                        item.iSubItem = 6;
                        item.cchTextMax = 11;
                        item.pszText = CurrentRom.IniSection;
                        SendMessageA(BrowserWindow, LVM_GETITEMTEXTA, list->iItem, (LPARAM)&item);
                        return 0;
                    }
                case NM_RETURN:
                case NM_DBLCLK:
                    if (list->iItem != -1)
                    {
                        LVITEMA selected, item;
                        selected.iItem = SendMessageA(BrowserWindow, LVM_GETHOTITEM, 0, 0);
                        selected.iSubItem = 0;
                        selected.mask = LVIF_IMAGE;
                        SendMessageA(BrowserWindow, LVM_GETITEMA, 0, (LPARAM)&selected);
                        if (selected.iImage != 6)
                        {
                            item.pszText = text;
                            item.iSubItem = 0;
                            item.cchTextMax = MAX_PATH;
                            SendMessageA(BrowserWindow, LVM_GETITEMTEXTA, selected.iItem, (LPARAM)&item);
                            strcat(RomDirectory, text);
                            strcat(RomDirectory, "\\");
                            RefreshRomBrowser();
                        }
                        else if (!CPU_Running)
                        {
                            item.pszText = text;
                            item.iSubItem = 1;
                            item.cchTextMax = 256;
                            SendMessageA(BrowserWindow, LVM_GETITEMTEXTA, selected.iItem, (LPARAM)&item);
                            SetCurrentDirectoryA(RomDirectory);
                            strcpy(SelectedRomFilename, RomDirectory);
                            strcat(SelectedRomFilename, text);
                            StartCPUThread();
                        }
                    }
                    return 0;
                }
            }
            else if (notification->hwndFrom == ToolbarWindow)
            {
                NMTOOLBARA* toolbar = (NMTOOLBARA*)notification;
                if (notification->code == TBN_DROPDOWN)
                {
                    if (toolbar->iItem == 0x801e) ShowDriveMenu();
                    else if (toolbar->iItem == 0x801f) ShowFavouritesMenu();
                }
                else if (notification->code == NM_RCLICK && toolbar->iItem == ID_ABOUT)
                {
                    unsigned int position = EasterTextPosition;
                    unsigned int previous;
                    do
                    {
                        previous = position;
                        position++;
                        if ((unsigned int)EasterTextLength <= position) break;
                    } while (EasterText[previous + 1] != '\n');
                    strncpy(text, EasterText + EasterTextPosition, position - EasterTextPosition);
                    text[position - EasterTextPosition] = 0;
                    EasterTextPosition = previous + 2;
                    if ((unsigned int)EasterTextLength < EasterTextPosition) EasterTextPosition = 0;
                    SetStatusText(0, text);
                }
            }
            return 0;
        }
    case WM_DROPFILES:
        {
            DragQueryFileA((HDROP)wParam, 0, filename, 256);
            DragFinish((HDROP)wParam);
            if (!CPU_Running)
            {
                sprintf(text, "Load rom file %s?", filename);
                if (MessageBoxA(MainWindow, text, "Load rom file", MB_YESNO | MB_ICONQUESTION) == IDYES)
                {
                    strcpy(SelectedRomFilename, filename);
                    StartCPUThread();
                }
            }
            else
            {
                sprintf(text, "Load savestate file %s?", filename);
                if (MessageBoxA(MainWindow, text, "Load state file", MB_YESNO | MB_ICONQUESTION) == IDYES)
                {
                    strcpy(StateFileName, filename);
                    SetStatusText(0, "Loading state...");
                    CPU_ActionFlags |= 0x2000;
                }
            }
            return 0;
        }
    case WM_USER + 10:
        return 0;
    default:
        return DefWindowProcA(window, message, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine, int showCommand)
{
    ApplicationInstance = instance;
    WNDCLASSEXA windowClass;
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = MainWindowProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(IDI_NEMU64));
    windowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    windowClass.lpszMenuName = MAKEINTRESOURCEA(IDR_MAINMENU);
    windowClass.lpszClassName = "Nemu64";
    windowClass.hIconSm = LoadIconA(windowClass.hInstance, MAKEINTRESOURCEA(108));
    if (!RegisterClassExA(&windowClass))
    {
        MessageBoxA(NULL, "Error initializing class", "Error", MB_ICONSTOP);
        return 0;
    }
    if (!CIC6105Table)
    {
        CIC6105Table = (unsigned int*)malloc(0x400010);
        if (CIC6105Table)
        {
            HRSRC resource = FindResourceA(instance, MAKEINTRESOURCEA(IDR_CIC6105_TABLE), RT_RCDATA);
            GetLastError();
            HGLOBAL loaded = LoadResource(instance, resource);
            void* data = LockResource(loaded);
            if (data)
                memcpy(CIC6105Table, data, 0x400000);
        }
    }
    sprintf(EasterText, EasterTextFirst);
    strcat(EasterText, EasterTextSecond);
    EasterTextLength = strlen(EasterText);
    kailleraInit();
    NetPlayPlayer = 0;
    NetPlayActive = 0;
    GetCurrentDirectoryA(MAX_PATH, ApplicationDirectory);
    if (ApplicationDirectory[strlen(ApplicationDirectory) - 1] != '\\')
        strcat(ApplicationDirectory, "\\");
    InitializeIniPaths();
    RDRAMSize = 0;
    RDRAM = (unsigned char*)VirtualAlloc(NULL, 0x20000000, MEM_RESERVE, PAGE_READWRITE);
    if (!RDRAM)
        MessageBoxA(MainWindow, "Couldn't reserve rdram-memory", "Error", MB_ICONSTOP);
    ReloadConfiguration();
    ReadIniString("Configuration", "Folder", "C:\\", RomDirectory, ConfigurationIniPath);
    if (RomDirectory[strlen(RomDirectory) - 1] != '\\')
        strcat(RomDirectory, "\\");
    BrowserSortMode = ReadIniInteger("Configuration", "SortMode", 1, ConfigurationIniPath);
    DisplayResolution = ReadIniInteger("Configuration", "Resolution", 640, ConfigurationIniPath);
    FrontendInitialized = false;
    ReadIniString("Configuration", "PifNTSC", "", PIFNTSCFilename, ConfigurationIniPath);
    ReadIniString("Configuration", "PifPAL", "", PIFPALFilename, ConfigurationIniPath);
    if (!LoadPlugins())
    {
        MessageBoxA(NULL, "Error initializing plugins - can't continue execution", "Error", MB_ICONSTOP);
        return 0;
    }
    if (!CIInit)
    {
        MessageBoxA(NULL, "No input plugin found - can't continue execution", "Error", MB_ICONSTOP);
        return 0;
    }
    if (!AIInit)
    {
        MessageBoxA(NULL, "No audio plugin found - can't continue execution", "Error", MB_ICONSTOP);
        return 0;
    }
    if (!DCTList)
    {
        MessageBoxA(NULL, "No dct (jpeg) plugin found - jpeg disabled", "Error", MB_ICONSTOP);
        return 0;
    }
    if (!PluginCount)
    {
        MessageBoxA(NULL, "No zilmar-standard compatible plugin found", "Error", MB_ICONSTOP);
        return 0;
    }
    SelectGraphicsPlugin();
    if (!SelectedGraphicsPlugin)
    {
        MessageBoxA(NULL, "No zilmar-standard compatible graphics plugin found", "Error", MB_ICONSTOP);
        return 0;
    }
    CPU_Running = 0;
    MainWindow = CreateWindowExA(0, "Nemu64", "Nemu64", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, instance, NULL);
    if (!MainWindow)
        return 0;
    SetEmulationMenus(0);
    ShowWindow(MainWindow, showCommand);
    UpdateWindow(MainWindow);
    ResizeMainWindow(640, 480);
    InitializeGraphicsPlugin();
    if (SelectedGraphicsPlugin->MoveScreen)
    {
        RECT rect;
        GetWindowRect(MainWindow, &rect);
        SelectedGraphicsPlugin->MoveScreen(rect.left + 4, rect.top + 69);
    }
    int parts[3] = {350, 420, -1};
    SendMessageA(StatusWindow, SB_SETPARTS, 3, (LPARAM)parts);
    HACCEL accelerators = LoadAcceleratorsA(instance, MAKEINTRESOURCEA(IDR_MAINMENU));
    RSPBreakpointCount = 0;
    OpcodeBreakpointCount = 0;
    InitializeTools();
    CIInit(MainWindow, Controllers);
    MSG message;
    while (GetMessageA(&message, NULL, 0, 0))
    {
        if (!TranslateAcceleratorA(message.hwnd, accelerators, &message))
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
    CIFree();
    if (SelectedGraphicsPlugin->CloseDLL)
        SelectedGraphicsPlugin->CloseDLL();
    FreeTools();
    kailleraShutdown();
    UnloadPlugins();
    DecommitRDRAM();
    if (!VirtualFree(RDRAM, 0, MEM_RELEASE))
        MessageBoxA(MainWindow, "Couldn't release rdram-memory", "Error", MB_ICONSTOP);
    return message.wParam;
}

LRESULT MainWindowCommand(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    int command = LOWORD(wParam);
    if (command > 65000)
    {
        ToolCatalog[(int)(command - 65001) / 30].Invoke(1, (int)(command - 65001) % 30);
        return 1;
    }
    OPENFILENAMEA file;
    switch (command)
    {
    case ID_ABOUT:
        DialogBoxParamA(ApplicationInstance, MAKEINTRESOURCEA(IDD_ABOUT), window, AboutDialogProc, 0);
        return 1;
    case ID_EXIT:
        DestroyWindow(MainWindow);
        return 1;
    case ID_CONFIGURE_VIDEO_PLUGIN:
        if (SelectedGraphicsPlugin->DllConfig)
            SelectedGraphicsPlugin->DllConfig(MainWindow);
        return 1;
    case ID_CONFIGURE_AUDIO_PLUGIN:
        AIConfigure(window);
        return 1;
    case ID_CLOSE_ROM:
        CPU_ActionFlags = (CPU_ActionFlags & ~0x10000) | 0x400000;
        ResumeThread(CPU_Thread);
        SetStatusText(0, "Closing rom...");
        return 1;
    case ID_CONFIGURE_INPUT_PLUGIN:
        CIConfigure(window);
        return 1;
    case ID_CHEAT_DP_INT:
        CPU_ActionFlags |= 0x20;
        return 1;
    case ID_SETTINGS:
        {
            if (!NemuSettings)
                return 1;
            NemuSettings(PluginCount, PluginCatalog);
            PluginDescriptor* previous = SelectedGraphicsPlugin;
            if (CPU_Running)
                SuspendThread(CPU_Thread);
            ReloadConfiguration();
            if (previous != SelectedGraphicsPlugin)
            {
                if (!CPU_Running)
                {
                    if (previous->CloseDLL)
                        previous->CloseDLL();
                    InitializeGraphicsPlugin();
                }
                else
                {
                    PendingGraphicsPlugin = SelectedGraphicsPlugin;
                    CPU_ActionFlags |= 0x4000000;
                    SelectedGraphicsPlugin = previous;
                }
            }
            if (CPU_Running)
                ResumeThread(CPU_Thread);
        }
        return 1;
    case ID_RESET_DYNREC:
        CPU_ActionFlags |= 0x400;
        return 1;
    case ID_320X240: ResizeMainWindow(320, 240); return 1;
    case ID_400X300: ResizeMainWindow(400, 300); return 1;
    case ID_640X480: ResizeMainWindow(640, 480); return 1;
    case ID_800X600: ResizeMainWindow(800, 600); return 1;
    case ID_1024X768: ResizeMainWindow(1024, 768); return 1;
    case ID_1152X864: ResizeMainWindow(1152, 864); return 1;
    case ID_1280X960: ResizeMainWindow(1280, 960); return 1;
    case ID_1600X1200: ResizeMainWindow(1600, 1200); return 1;
    case 0x801e:
        ShowDriveMenu();
        return 1;
    case 0x801f:
        ShowFavouritesMenu();
        return 1;
    case ID_RESET:
        CPU_ActionFlags |= 0x800;
        return 1;
    case ID_FULL_SCREEN:
        FullScreen = !FullScreen;
        if (SelectedGraphicsPlugin->ChangeWindow)
            SelectedGraphicsPlugin->ChangeWindow();
        return 1;
    case ID_PAUSE:
        if (!(CPU_ActionFlags & 0x200000))
        {
            CPU_ActionFlags |= 0x200000;
            ActivePause = 0;
            SendMessageA(ToolbarWindow, TB_SETSTATE, ID_PAUSE, TBSTATE_ENABLED | TBSTATE_CHECKED);
        }
        else
        {
            CPU_ActionFlags &= ~0x200000;
            ResumeThread(CPU_Thread);
            SendMessageA(ToolbarWindow, TB_SETSTATE, ID_PAUSE, TBSTATE_ENABLED);
        }
        return 1;
    case ID_DIRECTORY_UP:
        if (strlen(RomDirectory) != 3)
        {
            int i = strlen(RomDirectory) - 2;
            while (i >= 0 && RomDirectory[i] != '\\')
                --i;
            RomDirectory[i + 1] = 0;
            RefreshRomBrowser();
        }
        return 1;
    case 0x8026:
    case ID_LOAD_ROM:
        memset(&file, 0, sizeof(file));
        SelectedRomFilename[0] = 0;
        file.lStructSize = sizeof(file);
        file.nMaxFile = 256;
        file.lpstrTitle = "Open rom...";
        file.lpstrFilter = "Rom and Zip files\0*.zip;*.?64;*.rom;*.usa;*.pal;*.eur;*.bin\0All files\0*.*\0\0";
        file.lpstrFile = SelectedRomFilename;
        file.hwndOwner = window;
        if (GetOpenFileNameA(&file))
            StartCPUThread();
        return 1;
    case ID_1: CPU_CycleShift = 0; return 1;
    case ID_2: CPU_CycleShift = 1; return 1;
    case ID_4: CPU_CycleShift = 2; return 1;
    case ID_8: CPU_CycleShift = 3; return 1;
    case ID_LOAD_STATE:
        memset(&file, 0, sizeof(file));
        StateFileName[0] = 0;
        file.lStructSize = sizeof(file);
        file.nMaxFile = 256;
        file.lpstrTitle = "Load state...";
        file.lpstrFilter = "Nemu save state (*.nss)\0*.nss\0All files\0*.*\0\0";
        file.lpstrFile = StateFileName;
        file.lpstrDefExt = "nss";
        file.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
        file.lpfnHook = StatePreviewHook;
        file.hInstance = ApplicationInstance;
        file.lpTemplateName = MAKEINTRESOURCEA(IDD_ROM_PREVIEW);
        file.lpstrInitialDir = SaveStateDirectory;
        file.hwndOwner = window;
        if (GetOpenFileNameA(&file))
        {
            SetStatusText(0, "Loading state...");
            CPU_ActionFlags |= 0x2000;
        }
        return 1;
    case ID_SAVE_STATE:
        memset(&file, 0, sizeof(file));
        StateFileName[0] = 0;
        file.lStructSize = sizeof(file);
        file.nMaxFile = 256;
        file.lpstrTitle = "Save state...";
        file.lpstrFilter = "Nemu save state (*.nss)\0*.nss\0All files\0*.*\0\0";
        file.lpstrFile = StateFileName;
        file.lpstrDefExt = "nss";
        file.lpstrInitialDir = SaveStateDirectory;
        file.hwndOwner = window;
        if (GetSaveFileNameA(&file))
            CaptureStatePreview();
        return 1;
    case ID_START_NETPLAY:
        {
            KailleraInfos infos;
            infos.AppName = "Nemu64 0.8";
            char* games = (char*)malloc(8192);
            games[0] = 0;
            games[1] = 0;
            int position = 0;
            unsigned int capacity = 8192;
            for (int index = 0; index < ListView_GetItemCount(BrowserWindow); ++index)
            {
                LVITEMA item;
                char name[512];
                item.mask = LVIF_TEXT | LVIF_IMAGE;
                item.iItem = index;
                item.iSubItem = 0;
                item.pszText = name;
                item.cchTextMax = 511;
                SendMessageA(BrowserWindow, LVM_GETITEMA, 0, (LPARAM)&item);
                if (item.iImage == 6)
                {
                    while (capacity < position + strlen(name) + 4)
                    {
                        capacity *= 2;
                        games = (char*)realloc(games, capacity);
                    }
                    strcpy(games + position, name);
                    games[position + strlen(games + position) + 1] = 0;
                    position += strlen(games + position) + 1;
                }
            }
            infos.GameList = games;
            infos.GameCallback = NetplayStartGame;
            infos.ChatReceivedCallback = (void (__stdcall *)(char*, char*))NetplayClientDropped;
            infos.ClientDroppedCallback = NetplayClientDropped;
            infos.MoreInfosCallback = NetplayMessage;
            kailleraSetInfos(&infos);
            kailleraSelectServerDialog(NULL);
            free(games);
        }
        return 1;
    case ID_ABOUT_VIDEO_PLUGIN:
        if (SelectedGraphicsPlugin->DllAbout)
            SelectedGraphicsPlugin->DllAbout(MainWindow);
        return 1;
    case ID_SHOW_ERRORS:
        ShowErrors = !ShowErrors;
        CheckMenuItem(GetMenu(MainWindow), ID_SHOW_ERRORS, ShowErrors ? MF_CHECKED : MF_UNCHECKED);
        WriteIniInteger("Configuration", "ShowErrors", ShowErrors, ConfigurationIniPath);
        return 1;
    case ID_USE_VIRTUAL_SAVE_FILES:
        if (MessageBoxA(MainWindow,
            "This will overwrite your own save files and replace them by the ones in memory. Proceed?",
            "Replace save files", MB_YESNO | MB_ICONQUESTION) == IDYES)
            FlushStateSaves();
        return 1;
    case ID_QUICK_SAVESTATE:
        strcpy(StateFileName, SaveStateDirectory);
        strcat(StateFileName, "_");
        strcat(StateFileName, CurrentRom.FileName);
        ReplaceExtension(StateFileName, ".nss");
        CaptureStatePreview();
        return 1;
    case ID_QUICK_LOADSTATE:
        strcpy(StateFileName, SaveStateDirectory);
        strcat(StateFileName, "_");
        strcat(StateFileName, CurrentRom.FileName);
        ReplaceExtension(StateFileName, ".nss");
        SetStatusText(0, "Loading state...");
        CPU_ActionFlags |= 0x2000;
        return 1;
    case ID_DISABLE_EXCEPTION:
        CPU_DisableExceptions = !CPU_DisableExceptions;
        CheckMenuItem(GetMenu(MainWindow), ID_DISABLE_EXCEPTION, CPU_DisableExceptions ? MF_CHECKED : MF_UNCHECKED);
        return 1;
    case ID_TEST_PRE_MNI:
        CPU_ActionFlags |= 0x8000000;
        EnablePIFSuccess();
        return 1;
    case ID_DISABLE_VIDEO_HLE_1:
        CPU_DisableVideoHLE = !CPU_DisableVideoHLE;
        CheckMenuItem(GetMenu(MainWindow), ID_DISABLE_VIDEO_HLE_1, CPU_DisableVideoHLE ? MF_CHECKED : MF_UNCHECKED);
        return 1;
    case ID_DISABLE_AUDIO_HLE_2:
        CPU_DisableAudioHLE = !CPU_DisableAudioHLE;
        CheckMenuItem(GetMenu(MainWindow), ID_DISABLE_AUDIO_HLE_2, CPU_DisableAudioHLE ? MF_CHECKED : MF_UNCHECKED);
        return 1;
    case ID_FORCE_CFB:
        CPU_ForceCFB = !CPU_ForceCFB;
        CheckMenuItem(GetMenu(MainWindow), ID_FORCE_CFB, CPU_ForceCFB ? MF_CHECKED : MF_UNCHECKED);
        return 1;
    case ID_ACTIVE_PAUSE:
        if (!(CPU_ActionFlags & 0x200000))
        {
            ActivePause = 1;
            CPU_ActionFlags |= 0x200000;
            SendMessageA(ToolbarWindow, TB_SETSTATE, ID_PAUSE, TBSTATE_ENABLED | TBSTATE_CHECKED);
        }
        else
        {
            CPU_ActionFlags &= ~0x200000;
            ResumeThread(CPU_Thread);
            SendMessageA(ToolbarWindow, TB_SETSTATE, ID_PAUSE, TBSTATE_ENABLED);
        }
        return 1;
    case ID_GAMESHARK_CHEATS:
        GSConfigure();
        return 1;
    case ID_NEMU_MOVIE_START_RECORD:
        memset(&file, 0, sizeof(file));
        StateFileName[0] = 0;
        file.lStructSize = sizeof(file);
        file.nMaxFile = 256;
        file.lpstrTitle = "Select movie file...";
        file.lpstrFilter = "Nemu movie file (*.nmf)\0*.nmf\0All files\0*.*\0\0";
        file.lpstrFile = StateFileName;
        file.lpstrDefExt = "nmf";
        file.lpstrInitialDir = SaveStateDirectory;
        file.hwndOwner = window;
        if (GetSaveFileNameA(&file))
        {
            CaptureStatePreview();
            MovieRecord = 1;
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_START_RECORD, MF_GRAYED);
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_STOP_RECORD, MF_ENABLED);
        }
        return 1;
    case ID_NEMU_MOVIE_STOP_RECORD:
        if (MovieRecord == 1)
        {
            StopMovie();
            MovieRecord = 0;
            SaveMovieState();
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_START_RECORD, MF_ENABLED);
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_STOP_RECORD, MF_GRAYED);
        }
        return 1;
    case ID_NEMU_MOVIE_PLAY_START:
        memset(&file, 0, sizeof(file));
        StateFileName[0] = 0;
        file.lStructSize = sizeof(file);
        file.nMaxFile = 256;
        file.lpstrTitle = "Select movie file...";
        file.lpstrFilter = "Nemu movie file (*.nmf)\0*.nmf\0All files\0*.*\0\0";
        file.lpstrFile = StateFileName;
        file.lpstrDefExt = "nmf";
        file.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
        file.lpfnHook = StatePreviewHook;
        file.hInstance = ApplicationInstance;
        file.lpTemplateName = MAKEINTRESOURCEA(IDD_ROM_PREVIEW);
        file.lpstrInitialDir = SaveStateDirectory;
        file.hwndOwner = window;
        if (GetOpenFileNameA(&file))
        {
            SetStatusText(0, "Loading movie...");
            CPU_ActionFlags |= 0x2000;
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_START_RECORD, MF_GRAYED);
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_PLAY_START, MF_GRAYED);
            EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_PLAY_STOP, MF_ENABLED);
        }
        return 1;
    case ID_NEMU_MOVIE_PLAY_STOP:
        MoviePlay = 0;
        StopMovie();
        EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_START_RECORD, MF_ENABLED);
        EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_PLAY_START, MF_ENABLED);
        EnableMenuItem(GetMenu(MainWindow), ID_NEMU_MOVIE_PLAY_STOP, MF_GRAYED);
        return 1;
    default:
        return DefWindowProcA(window, message, wParam, lParam);
    }
}
