#ifndef NEMU64_WINMAIN_H
#define NEMU64_WINMAIN_H

#ifndef WINVER
#define WINVER 0x0500
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#include <windows.h>
#include <commctrl.h>

extern HWND MainWindow;
extern HINSTANCE ApplicationInstance;
extern char ApplicationDirectory[MAX_PATH];
extern HWND ToolbarWindow;
extern HWND StatusWindow;
extern HWND BrowserWindow;
extern int MainWindowX;
extern int MainWindowY;
extern int DisplayResolution;
extern unsigned short StatePreview[120][160];
extern char StateFileName[256];
extern char EasterText[10000];
extern int EasterTextLength;
extern HIMAGELIST BrowserImageList;
extern bool BrowserInitialized;
extern bool Closing;
extern bool FullScreen;
extern unsigned int EasterTextPosition;
extern int ActivePause;
extern bool FrontendInitialized;

void ResizeMainWindow(unsigned int width, int height);
void SetEmulationMenus(unsigned char running);
void DrawStatePreview(HWND dialog);
void CaptureStatePreview();
void AddToolbarButton(TBBUTTON* buttons, int* count, BYTE style, int bitmap,
                      BYTE state, int command);
LRESULT MainWindowCommand(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MainWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
UINT CALLBACK StatePreviewHook(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam);

#endif
