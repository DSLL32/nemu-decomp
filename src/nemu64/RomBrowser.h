#ifndef NEMU64_ROM_BROWSER_H
#define NEMU64_ROM_BROWSER_H

#include <windows.h>

// This group is cleared as one 0x2c0-byte object when scanning a directory.
struct RomInformation
{
    char FileName[260];
    char InternalName[32];
    char DisplayName[100];
    char FullPath[256];
    char IniSection[20];
    unsigned long CRC1;
    unsigned long CRC2;
    unsigned long Reserved[3];
    int VideoHLE;
    int AudioHLE;
    unsigned long FileSize;
    unsigned long CompressedSize;
};
typedef char RomInformationSizeCheck[sizeof(RomInformation) == 0x2c0 ? 1 : -1];

extern RomInformation CurrentRom;
extern char RomDirectory[260];
extern int BrowserSortMode;
extern unsigned long BrowserRomCount;
extern HMENU FavouritesMenu;
extern HMENU DriveMenu;

int WindowHeight(HWND window);
void SetStatusText(unsigned int part, const char* text);
void RebuildFavouritesMenu();
void ShowFavouritesMenu();
void ShowDriveMenu();
void RefreshRomBrowser();
int CALLBACK CompareBrowserItems(LPARAM first, LPARAM second, LPARAM sortMode);
BOOL CALLBACK RemoveFavouritesDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AboutDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK DisclaimerDialogProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
void __stdcall NetplayMessage(const char* text);

#endif
