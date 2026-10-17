/* Reconstructed logging unit; original filename is not retained in audio.dll. */
#include "transform.h"
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

HINSTANCE LogInstance; /* 10169024 */
HWND LogEdit;         /* 10169028 */
HWND LogWindow;       /* 1016902c */
int LogHidden = 0;        /* 100e8fcc */
int DriverInfoVisible = 0; /* 100e8fd0 */
FILE *DebugOutput = 0;    /* 100e8fd4 */
extern HINSTANCE AudioInstance;

/* 10006610–1000664c */
void ToggleLogWindow(void)
{
    if (!LogHidden) {
        ShowWindow(LogWindow, SW_HIDE);
        LogHidden = 1;
    } else {
        ShowWindow(LogWindow, SW_SHOW);
        LogHidden = 0;
    }
}
/* 10006650–10006662 */
void ClearLogWindow(void) { SetWindowText(LogEdit, ""); }
/* 10006670–10006687 */
void DestroyLogWindow(void)
{
    DestroyWindow(LogWindow);
    LogWindow = NULL;
}
/* 10006690–100066e5 */
void CreateLogWindow(int hidden)
{
    LogHidden = hidden;
    if (!LogWindow)
        LogWindow = CreateWindowEx(0, "CONSOLE", "Console Window", WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT, CW_USEDEFAULT, 320, 260,
                                  NULL, NULL, LogInstance, NULL);
    ShowWindow(LogWindow, SW_SHOW);
}
/* 100066f0–10006734 */
void FilePrint(FILE *file, char *format, ...)
{
    char text[300];
    va_list args;
    if (file) {
        va_start(args, format);
        vsprintf(text, format, args);
        vfprintf(file, text, NULL);
    }
}
/* 10006740–10006792 */
void DebugPrint(char *format, ...)
{
    char text[300];
    va_list args;
    va_start(args, format);
    vsprintf(text, format, args);
    if (!DebugOutput) DebugOutput = fopen("dout.txt", "wb");
    fprintf(DebugOutput, text);
}
/* 100067a0–100067a1 */
void FlushAudioLog(void) {}
/* 100067b0–100067b5 */
void CloseAudioLog(void) { DestroyLogWindow(); }
/* 100067c0–10006994 (includes compiler switch tables) */
LRESULT CALLBACK LogWindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    RECT bounds, paintBounds;
    LOGFONT font;
    PAINTSTRUCT paint;
    HFONT fontHandle;
    switch (message) {
    case WM_PAINT:
        BeginPaint(window, &paint);
        GetClientRect(window, &paintBounds);
        EndPaint(window, &paint);
        break;
    case WM_CREATE:
        GetClientRect(LogWindow, &bounds);
        LogEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "", 0x40200a44,
                                bounds.left, bounds.top, bounds.left + 320,
                                bounds.top + 240, window, NULL, LogInstance, NULL);
        SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(font), &font, 0);
        fontHandle = CreateFontIndirect(&font);
        SendMessage(LogEdit, WM_SETFONT, (WPARAM)fontHandle, 0);
        ShowWindow(LogEdit, SW_SHOW);
        break;
    case WM_CLOSE:
        ToggleLogWindow();
        break;
    case WM_DESTROY:
        LogWindow = NULL;
        DestroyWindow(LogEdit);
        LogEdit = NULL;
        break;
    case WM_SIZE:
        GetClientRect(LogWindow, &bounds);
        SetWindowPos(LogEdit, NULL, bounds.top, bounds.left,
                     bounds.right - bounds.left, bounds.bottom - bounds.top, 0x204);
        break;
    default:
        return DefWindowProc(window, message, wparam, lparam);
    }
    return 0;
}
/* 100069a0–10006a36 */
ATOM RegisterLogWindow(HINSTANCE instance)
{
    WNDCLASSEX wc;
    LogInstance = instance;
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = LogWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = instance;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = "CONSOLE";
    wc.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    return RegisterClassEx(&wc);
}
/* 10006a40–10006a56 */
void OpenAudioLog(void)
{
    RegisterLogWindow(AudioInstance);
    CreateLogWindow(0);
}
