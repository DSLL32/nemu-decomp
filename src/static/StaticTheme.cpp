/* Native dark-mode opt-in for the static user build. No modern DLL/API imports.
   The VCL's explicit client colors and custom drawing are deliberately retained. */
#include <windows.h>
#include "StaticTheme.h"

#ifndef WM_THEMECHANGED
#define WM_THEMECHANGED 0x031A
#endif

typedef LONG (WINAPI *VersionFunction)(OSVERSIONINFOW *);
typedef HRESULT (WINAPI *DwmAttributeFunction)(HWND, DWORD, LPCVOID, DWORD);
typedef bool (WINAPI *ShouldUseDarkFunction)(void);
typedef bool (WINAPI *AllowWindowFunction)(HWND, bool);
typedef bool (WINAPI *AllowAppFunction)(bool);
typedef int (WINAPI *PreferredAppFunction)(int);
typedef void (WINAPI *ThemeNotificationFunction)(void);

static HMODULE dwmModule, themeModule;
static DwmAttributeFunction setAttribute;
static ShouldUseDarkFunction shouldUseDark;
static AllowWindowFunction allowWindow;
static AllowAppFunction allowApp;
static PreferredAppFunction preferredApp;
static ThemeNotificationFunction refreshPolicy, flushMenus;
static HHOOK windowHook;
static DWORD uiThread, windowsBuild;
static bool started, updating, dark;

static HMODULE loadSystemLibrary(const char *name)
{
    // GetSystemDirectory works on the old targets too; no dependency on the
    // newer LOAD_LIBRARY_SEARCH_SYSTEM32 flag, nor an application-directory DLL.
    char path[MAX_PATH];
    UINT size = GetSystemDirectoryA(path, sizeof(path));
    if (!size || size + 1 + lstrlenA(name) >= sizeof(path)) return NULL;
    lstrcatA(path, "\\"); lstrcatA(path, name);
    return LoadLibraryA(path);
}

static bool applicationPrefersDark(void)
{
    HIGHCONTRASTA contrast;
    ZeroMemory(&contrast, sizeof(contrast)); contrast.cbSize = sizeof(contrast);
    if (!SystemParametersInfoA(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0) ||
        (contrast.dwFlags & HCF_HIGHCONTRASTON)) return false;
    if (shouldUseDark) return shouldUseDark();
    HKEY key;
    if (RegOpenKeyExA(HKEY_CURRENT_USER,
        "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
        0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) return false;
    DWORD value = 1, type = 0, size = sizeof(value);
    LONG result = RegQueryValueExA(key, "AppsUseLightTheme", NULL, &type, (LPBYTE)&value, &size);
    RegCloseKey(key);
    return result == ERROR_SUCCESS && type == REG_DWORD && size == sizeof(value) && value == 0;
}

static void applyWindow(HWND window)
{
    // Covers native dialogs, MFC windows and VCL forms, including recreated
    // handles. Leave child controls' palettes/theming to their own frameworks.
    if (GetWindowLongA(window, GWL_STYLE) & WS_CHILD) return;
    if (allowWindow) allowWindow(window, dark);
    if (setAttribute) {
        BOOL enabled = dark ? TRUE : FALSE;
        HRESULT result = setAttribute(window, 20, &enabled, sizeof(enabled));
        // Windows 10's earlier implementation used attribute 19. Never try
        // that undocumented value on older OSes or newer attribute layouts.
        if (FAILED(result) && windowsBuild < 19041)
            setAttribute(window, 19, &enabled, sizeof(enabled));
    }
}

static BOOL CALLBACK updateWindow(HWND window, LPARAM)
{
    applyWindow(window);
    DrawMenuBar(window);
    RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN);
    return TRUE;
}

static void refreshTheme(void)
{
    if (updating) return;
    updating = true;
    if (refreshPolicy) refreshPolicy();
    dark = applicationPrefersDark();
    // Ordinal 135 changed ABI between 1809 and 1903. Resolve just the signature
    // appropriate for the actual NT build; presence alone is insufficient.
    if (preferredApp) preferredApp(dark ? 1 /* AllowDark */ : 0 /* Default */);
    else if (allowApp) allowApp(dark);
    if (flushMenus) flushMenus();
    EnumThreadWindows(uiThread, updateWindow, 0);
    updating = false;
}

static LRESULT CALLBACK windowMessages(int code, WPARAM wParam, LPARAM lParam)
{
    if (code >= 0 && !updating) {
        const CWPRETSTRUCT *message = (const CWPRETSTRUCT *)lParam;
        if (message->message == WM_CREATE || message->message == WM_INITDIALOG)
            applyWindow(message->hwnd);
        else if ((message->message == WM_SETTINGCHANGE || message->message == WM_THEMECHANGED ||
                  message->message == WM_SYSCOLORCHANGE) &&
                 !(GetWindowLongA(message->hwnd, GWL_STYLE) & WS_CHILD))
            refreshTheme();
    }
    return CallNextHookEx(windowHook, code, wParam, lParam);
}

extern "C" void __cdecl NemuStaticThemeInitialize(void)
{
    if (started) return;
    started = true;
    OSVERSIONINFOW version;
    ZeroMemory(&version, sizeof(version)); version.dwOSVersionInfoSize = sizeof(version);
    HMODULE nt = GetModuleHandleA("ntdll.dll");
    VersionFunction getVersion = nt ? (VersionFunction)GetProcAddress(nt, "RtlGetVersion") : NULL;
    // Ordinal exports on XP/Vista/7 can mean entirely different functions.
    // RtlGetVersion also avoids the application-manifest version lie on 8.1+.
    if (!getVersion || getVersion(&version) < 0 || version.dwMajorVersion != 10 ||
        version.dwBuildNumber < 17763) return;
    windowsBuild = version.dwBuildNumber;
    uiThread = GetCurrentThreadId();
    dwmModule = loadSystemLibrary("dwmapi.dll");
    if (dwmModule) setAttribute = (DwmAttributeFunction)GetProcAddress(dwmModule, "DwmSetWindowAttribute");
    themeModule = loadSystemLibrary("uxtheme.dll");
    if (themeModule) {
        shouldUseDark = (ShouldUseDarkFunction)GetProcAddress(themeModule, (LPCSTR)132);
        allowWindow = (AllowWindowFunction)GetProcAddress(themeModule, (LPCSTR)133);
        if (windowsBuild >= 18362)
            preferredApp = (PreferredAppFunction)GetProcAddress(themeModule, (LPCSTR)135);
        else
            allowApp = (AllowAppFunction)GetProcAddress(themeModule, (LPCSTR)135);
        refreshPolicy = (ThemeNotificationFunction)GetProcAddress(themeModule, (LPCSTR)104);
        flushMenus = (ThemeNotificationFunction)GetProcAddress(themeModule, (LPCSTR)136);
    }
    if (!setAttribute && !preferredApp && !allowApp && !allowWindow) return;
    refreshTheme();
    // Thread-local, in-process hook: observes all GUI frameworks without
    // replacing their window procedures or touching external processes.
    windowHook = SetWindowsHookExA(WH_CALLWNDPROCRET, windowMessages, NULL, uiThread);
}

extern "C" void __cdecl NemuStaticThemeShutdown(void)
{
    if (windowHook) UnhookWindowsHookEx(windowHook);
    windowHook = NULL;
    if (preferredApp) preferredApp(0);
    else if (allowApp) allowApp(false);
    if (themeModule) FreeLibrary(themeModule);
    if (dwmModule) FreeLibrary(dwmModule);
    themeModule = dwmModule = NULL;
    setAttribute = NULL; shouldUseDark = NULL; allowWindow = NULL;
    allowApp = NULL; preferredApp = NULL; refreshPolicy = flushMenus = NULL;
    started = updating = dark = false;
    uiThread = windowsBuild = 0;
}
