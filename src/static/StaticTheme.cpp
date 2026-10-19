/* Runtime theme adapter for the static user build. Modern APIs are optional. */
#define _WIN32_WINNT 0x0501
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include "StaticTheme.h"

#ifndef BS_TYPEMASK
#define BS_TYPEMASK 0x0000000fL
#endif

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
static UINT nonClientMessage;
static DWORD uiThread, windowsBuild;
static bool started, updating, dark, vclReady;
extern "C" void __cdecl NemuStaticVclThemeRefresh(void);
typedef BOOL (WINAPI *SubclassFunction)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);
typedef BOOL (WINAPI *RemoveSubclassFunction)(HWND, SUBCLASSPROC, UINT_PTR);
typedef LRESULT (WINAPI *DefaultSubclassFunction)(HWND, UINT, WPARAM, LPARAM);
typedef HRESULT (WINAPI *WindowThemeFunction)(HWND, LPCWSTR, LPCWSTR);
static SubclassFunction subclassWindow;
static RemoveSubclassFunction removeSubclass;
static DefaultSubclassFunction defaultSubclass;
static WindowThemeFunction setWindowTheme;
static HBRUSH colorBrushes[31];
typedef BOOL (WINAPI *ScrollBarInfoFunction)(HWND, LONG, LPSCROLLBARINFO);
static ScrollBarInfoFunction scrollBarInfo;

extern "C" BOOL __cdecl NemuStaticThemeIsDark(void) { return dark; }
extern "C" DWORD WINAPI NemuStaticGetSysColor(int index)
{
    if (dark) switch (index) {
    case COLOR_WINDOW: return RGB(25,25,25);
    case COLOR_BTNFACE: case COLOR_MENU: case COLOR_SCROLLBAR: return RGB(40,40,40);
    case COLOR_APPWORKSPACE: case COLOR_BACKGROUND: return RGB(32,32,32);
    case COLOR_WINDOWTEXT: case COLOR_BTNTEXT: case COLOR_MENUTEXT:
    case COLOR_CAPTIONTEXT: case COLOR_INFOTEXT: return RGB(232,232,232);
    case COLOR_GRAYTEXT: return RGB(150,150,150);
    case COLOR_HIGHLIGHT: case COLOR_HOTLIGHT: return RGB(45,85,125);
    case COLOR_HIGHLIGHTTEXT: return RGB(255,255,255);
    case COLOR_BTNHIGHLIGHT: case COLOR_3DLIGHT: return RGB(80,80,80);
    case COLOR_BTNSHADOW: case COLOR_3DDKSHADOW: case COLOR_WINDOWFRAME: return RGB(60,60,60);
    case COLOR_INFOBK: return RGB(48,48,48);
    }
    return GetSysColor(index);
}
extern "C" HBRUSH WINAPI NemuStaticGetSysColorBrush(int index)
{
    if (!dark || index < 0 || index >= 31) return GetSysColorBrush(index);
    if (!colorBrushes[index]) colorBrushes[index] = CreateSolidBrush(NemuStaticGetSysColor(index));
    return colorBrushes[index];
}

enum ControlKind { Panel, List, Tree, Toolbar, Status, Header, Button, Tabs, Combo, Edit, Other };
struct ThemedWindow {
    HWND window;
    ControlKind kind;
    COLORREF background, textBackground, text;
    HIMAGELIST originalImages, darkImages, originalDisabledImages, darkDisabledImages;
    DWORD originalStyle;
    bool themed;
    ThemedWindow *next;
};
static ThemedWindow *windows;
static void toolbarImages(ThemedWindow *state);
static const UINT_PTR subclassId = 0x4e454d55;
struct ThemedMenu {
    HMENU menu;
    UINT index, type;
    ULONG_PTR data;
    ThemedMenu *next;
};
static ThemedMenu *menuItems;
static ThemedMenu *findMenuItem(ULONG_PTR data)
{
    for (ThemedMenu *item = menuItems; item; item = item->next)
        if ((ULONG_PTR)item == data) return item;
    return NULL;
}
static void menuColors(HWND window)
{
    HMENU menu = GetMenu(window);
    if (!menu) return;
    for (int i = 0; i < GetMenuItemCount(menu); ++i) {
        MENUITEMINFOA info; ZeroMemory(&info, sizeof(info)); info.cbSize = sizeof(info);
        info.fMask = MIIM_FTYPE | MIIM_DATA;
        if (!GetMenuItemInfoA(menu, i, TRUE, &info)) continue;
        if (dark) {
            if (info.fType & (MFT_OWNERDRAW | MFT_BITMAP | MFT_SEPARATOR)) continue;
            ThemedMenu *item = (ThemedMenu *)calloc(1, sizeof(*item));
            if (!item) continue;
            item->menu = menu; item->index = i; item->type = info.fType; item->data = info.dwItemData;
            item->next = menuItems; menuItems = item;
            info.fType |= MFT_OWNERDRAW; info.dwItemData = (ULONG_PTR)item;
            SetMenuItemInfoA(menu, i, TRUE, &info);
        } else {
            ThemedMenu *item = findMenuItem(info.dwItemData);
            if (!item) continue;
            info.fType = item->type; info.dwItemData = item->data;
            SetMenuItemInfoA(menu, i, TRUE, &info);
            ThemedMenu **link = &menuItems;
            while (*link && *link != item) link = &(*link)->next;
            if (*link) *link = item->next;
            free(item);
        }
    }
    MENUINFO info; ZeroMemory(&info, sizeof(info)); info.cbSize = sizeof(info);
    info.fMask = MIM_BACKGROUND;
    info.hbrBack = dark ? NemuStaticGetSysColorBrush(COLOR_MENU) : GetSysColorBrush(COLOR_MENU);
    SetMenuInfo(menu, &info);
}
static HFONT menuFont(void)
{
    NONCLIENTMETRICSA metrics; ZeroMemory(&metrics, sizeof(metrics)); metrics.cbSize = sizeof(metrics);
    if (SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0))
        return CreateFontIndirectA(&metrics.lfMenuFont);
    return NULL;
}
static void menuText(ThemedMenu *item, WCHAR *text, int size)
{
    MENUITEMINFOW info; ZeroMemory(&info, sizeof(info)); info.cbSize = sizeof(info);
    info.fMask = MIIM_STRING; info.dwTypeData = text; info.cch = size;
    text[0] = 0; GetMenuItemInfoW(item->menu, item->index, TRUE, &info);
}
static LRESULT CALLBACK themedProc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

static ControlKind controlKind(HWND window)
{
    char name[128]; GetClassNameA(window, name, sizeof(name));
    if (!lstrcmpiA(name, WC_LISTVIEWA) || !lstrcmpiA(name, "TListView")) return List;
    if (!lstrcmpiA(name, WC_TREEVIEWA) || !lstrcmpiA(name, "TTreeView")) return Tree;
    if (!lstrcmpiA(name, TOOLBARCLASSNAMEA) || !lstrcmpiA(name, "TToolBar")) return Toolbar;
    if (!lstrcmpiA(name, STATUSCLASSNAMEA) || !lstrcmpiA(name, "TStatusBar")) return Status;
    if (!lstrcmpiA(name, WC_HEADERA) || !lstrcmpiA(name, "THeaderControl")) return Header;
    if (!lstrcmpiA(name, "BUTTON") || !lstrcmpiA(name, "TButton") ||
        !lstrcmpiA(name, "TCheckBox") || !lstrcmpiA(name, "TGroupButton") ||
        !lstrcmpiA(name, "TRadioButton")) return Button;
    if (!lstrcmpiA(name, WC_TABCONTROLA) || !lstrcmpiA(name, "TPageControl") ||
        !lstrcmpiA(name, "TTabControl")) return Tabs;
    if (!lstrcmpiA(name, "COMBOBOX") || !lstrcmpiA(name, "TComboBox")) return Combo;
    if (!lstrcmpiA(name, "EDIT") || !lstrcmpiA(name, "TEdit") ||
        !lstrcmpiA(name, "TMemo") || !lstrcmpiA(name, "LISTBOX") ||
        !lstrcmpiA(name, "TListBox")) return Edit;
    if (!lstrcmpiA(name, "nemu64") || !lstrcmpiA(name, "#32770")) return Panel;
    return Other;
}

static void controlColors(ThemedWindow *state)
{
    HWND window = state->window;
    if (state->themed == dark) return;
    if (dark) {
        if (state->kind == List) {
            state->background = ListView_GetBkColor(window);
            state->textBackground = ListView_GetTextBkColor(window);
            state->text = ListView_GetTextColor(window);
        } else if (state->kind == Tree) {
            state->background = TreeView_GetBkColor(window);
            state->text = TreeView_GetTextColor(window);
        }
    }
    if (state->kind == Toolbar) {
        DWORD style = GetWindowLong(window, GWL_STYLE);
        if (dark) state->originalStyle = style;
        SetWindowLong(window, GWL_STYLE, dark ? style | CCS_NODIVIDER :
            (style & ~CCS_NODIVIDER) | (state->originalStyle & CCS_NODIVIDER));
        toolbarImages(state);
    }
    state->themed = dark; // Set before theme messages reenter the hook.
    if (allowWindow) allowWindow(window, dark);
    if (setWindowTheme && state->kind != Other && state->kind != Panel)
        setWindowTheme(window, dark ? ((state->kind == List || state->kind == Tree ||
            state->kind == Edit || state->kind == Combo) ? L"DarkMode_Explorer" : L"") : NULL,
            dark ? L"" : NULL);
    if (state->kind == List) {
        ListView_SetBkColor(window, dark ? NemuStaticGetSysColor(COLOR_WINDOW) : state->background);
        ListView_SetTextBkColor(window, dark ? NemuStaticGetSysColor(COLOR_WINDOW) : state->textBackground);
        ListView_SetTextColor(window, dark ? NemuStaticGetSysColor(COLOR_WINDOWTEXT) : state->text);
    } else if (state->kind == Tree) {
        TreeView_SetBkColor(window, dark ? NemuStaticGetSysColor(COLOR_WINDOW) : state->background);
        TreeView_SetTextColor(window, dark ? NemuStaticGetSysColor(COLOR_WINDOWTEXT) : state->text);
    }
    InvalidateRect(window, NULL, TRUE);
}

static void textColors(HDC dc, int background)
{
    SetTextColor(dc, NemuStaticGetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(dc, NemuStaticGetSysColor(background));
}
static void drawText(HWND window, HDC dc, RECT rect, UINT format)
{
    WCHAR text[512]; GetWindowTextW(window, text, 512);
    HFONT font = (HFONT)SendMessage(window, WM_GETFONT, 0, 0);
    HGDIOBJ old = font ? SelectObject(dc, font) : NULL;
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, NemuStaticGetSysColor(IsWindowEnabled(window) ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
    if (SendMessage(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEACCEL) format |= DT_HIDEPREFIX;
    DrawTextW(dc, text, -1, &rect, format);
    if (old) SelectObject(dc, old);
}
#include "StaticThemeControls.inc"

static void paintButton(HWND window, HDC dc)
{
    RECT rect; GetClientRect(window, &rect);
    FillRect(dc, &rect, NemuStaticGetSysColorBrush(COLOR_BTNFACE));
    DWORD style = GetWindowLong(window, GWL_STYLE);
    UINT type = style & BS_TYPEMASK;
    UINT state = (UINT)SendMessage(window, BM_GETSTATE, 0, 0);
    bool check = type == BS_CHECKBOX || type == BS_AUTOCHECKBOX || type == BS_3STATE ||
                 type == BS_AUTO3STATE || type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON;
    if (check) {
        RECT mark = rect;
        mark.top = (rect.bottom - 13) / 2; mark.bottom = mark.top + 13;
        if (style & BS_LEFTTEXT) { mark.left = rect.right - 13; rect.right -= 18; }
        else { mark.right = 13; rect.left += 18; }
        if (type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON) {
            HPEN outline = CreatePen(PS_SOLID, 1, NemuStaticGetSysColor(COLOR_BTNHIGHLIGHT));
            HGDIOBJ oldPen = SelectObject(dc, outline);
            HGDIOBJ oldBrush = SelectObject(dc, NemuStaticGetSysColorBrush(COLOR_WINDOW));
            Ellipse(dc, mark.left, mark.top, mark.right, mark.bottom);
            SelectObject(dc, oldBrush); SelectObject(dc, oldPen); DeleteObject(outline);
        } else {
            FillRect(dc, &mark, NemuStaticGetSysColorBrush(COLOR_WINDOW));
            FrameRect(dc, &mark, NemuStaticGetSysColorBrush(COLOR_BTNHIGHLIGHT));
        }
        UINT checked = (UINT)SendMessage(window, BM_GETCHECK, 0, 0);
        if (checked) {
            HPEN pen = CreatePen(PS_SOLID, 2, NemuStaticGetSysColor(COLOR_BTNTEXT));
            HGDIOBJ old = SelectObject(dc, pen);
            if (type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON) {
                HBRUSH brush = CreateSolidBrush(NemuStaticGetSysColor(COLOR_BTNTEXT));
                HGDIOBJ oldBrush = SelectObject(dc, brush);
                Ellipse(dc, mark.left+4, mark.top+4, mark.right-3, mark.bottom-3);
                SelectObject(dc, oldBrush); DeleteObject(brush);
            } else {
                MoveToEx(dc, mark.left+3, mark.top+6, NULL); LineTo(dc, mark.left+6, mark.top+9);
                LineTo(dc, mark.left+11, mark.top+3);
            }
            SelectObject(dc, old); DeleteObject(pen);
        }
        drawText(window, dc, rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else if (type == BS_GROUPBOX) {
        RECT frame = rect; frame.top += 7;
        FrameRect(dc, &frame, NemuStaticGetSysColorBrush(COLOR_BTNHIGHLIGHT));
        WCHAR text[512]; GetWindowTextW(window, text, 512);
        HFONT font = (HFONT)SendMessage(window, WM_GETFONT, 0, 0);
        HGDIOBJ old = font ? SelectObject(dc, font) : NULL;
        SIZE size; GetTextExtentPoint32W(dc, text, lstrlenW(text), &size);
        RECT caption = {8, 0, 12+size.cx, size.cy};
        FillRect(dc, &caption, NemuStaticGetSysColorBrush(COLOR_BTNFACE));
        drawText(window, dc, caption, DT_LEFT | DT_SINGLELINE);
        if (old) SelectObject(dc, old);
    } else {
        if (state & BST_PUSHED) FillRect(dc, &rect, NemuStaticGetSysColorBrush(COLOR_HIGHLIGHT));
        FrameRect(dc, &rect, NemuStaticGetSysColorBrush(type == BS_DEFPUSHBUTTON ? COLOR_HIGHLIGHT : COLOR_BTNHIGHLIGHT));
        if (state & BST_PUSHED) OffsetRect(&rect, 1, 1);
        drawText(window, dc, rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    if ((state & BST_FOCUS) && type != BS_GROUPBOX &&
        !(SendMessage(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS)) {
        InflateRect(&rect, -3, -3); DrawFocusRect(dc, &rect);
    }
}

static void paintStatus(HWND window, HDC dc)
{
    RECT client; GetClientRect(window, &client);
    FillRect(dc, &client, NemuStaticGetSysColorBrush(COLOR_BTNFACE));
    HFONT font = (HFONT)SendMessage(window, WM_GETFONT, 0, 0);
    HGDIOBJ old = font ? SelectObject(dc, font) : NULL;
    SetBkMode(dc, TRANSPARENT); textColors(dc, COLOR_BTNFACE);
    int parts = (int)SendMessage(window, SB_GETPARTS, 0, 0);
    if (SendMessage(window, SB_ISSIMPLE, 0, 0)) parts = 1;
    for (int i = 0; i < parts; ++i) {
        RECT rect = client;
        if (!SendMessage(window, SB_ISSIMPLE, 0, 0)) SendMessage(window, SB_GETRECT, i, (LPARAM)&rect);
        WCHAR text[1024];
        int index = SendMessage(window, SB_ISSIMPLE, 0, 0) ? SB_SIMPLEID : i;
        LRESULT length = SendMessage(window, SB_GETTEXTLENGTHW, index, 0);
        // Owner-drawn panels belong to the framework; dispatch its draw message.
        if (HIWORD(length) & SBT_OWNERDRAW) {
            DRAWITEMSTRUCT item; ZeroMemory(&item, sizeof(item));
            item.CtlType = ODT_STATIC; item.CtlID = GetDlgCtrlID(window); item.itemID = i;
            item.hwndItem = window; item.hDC = dc; item.rcItem = rect;
            item.itemData = SendMessage(window, SB_GETTEXTW, index, 0);
            SendMessage(GetParent(window), WM_DRAWITEM, item.CtlID, (LPARAM)&item);
        } else if (LOWORD(length) < 1024) {
            SendMessageW(window, SB_GETTEXTW, index, (LPARAM)text);
            rect.left += 4;
            HICON icon = (HICON)SendMessage(window, SB_GETICON, index, 0);
            if (icon) { DrawIconEx(dc, rect.left, rect.top+2, icon, 16,16,0,NULL,DI_NORMAL); rect.left += 20; }
            DrawTextW(dc, text, -1, &rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        }
        if (i+1 < parts) {
            RECT line = {rect.right-1, rect.top+3, rect.right, rect.bottom-3};
            FillRect(dc, &line, NemuStaticGetSysColorBrush(COLOR_BTNSHADOW));
        }
    }
    if (GetWindowLong(window, GWL_STYLE) & SBARS_SIZEGRIP) {
        HPEN pen = CreatePen(PS_SOLID, 1, NemuStaticGetSysColor(COLOR_BTNHIGHLIGHT));
        HGDIOBJ saved = SelectObject(dc, pen);
        for (int j = 3; j <= 11; j += 4) {
            MoveToEx(dc, client.right-j, client.bottom-2, NULL);
            LineTo(dc, client.right-2, client.bottom-j);
        }
        SelectObject(dc, saved); DeleteObject(pen);
    }
    if (old) SelectObject(dc, old);
}

static void paintTabs(HWND window, HDC dc)
{
    RECT client; GetClientRect(window, &client);
    FillRect(dc, &client, NemuStaticGetSysColorBrush(COLOR_BTNFACE));
    HFONT font = (HFONT)SendMessage(window, WM_GETFONT, 0, 0);
    HGDIOBJ old = font ? SelectObject(dc, font) : NULL;
    int selected = TabCtrl_GetCurSel(window), count = TabCtrl_GetItemCount(window);
    SetBkMode(dc, TRANSPARENT); textColors(dc, COLOR_BTNFACE);
    for (int i = 0; i < count; ++i) {
        RECT rect; TabCtrl_GetItemRect(window, i, &rect);
        FillRect(dc, &rect, NemuStaticGetSysColorBrush(i == selected ? COLOR_WINDOW : COLOR_BTNFACE));
        FrameRect(dc, &rect, NemuStaticGetSysColorBrush(COLOR_BTNSHADOW));
        WCHAR text[512] = {0}; TCITEMW item; ZeroMemory(&item, sizeof(item));
        item.mask = TCIF_TEXT | TCIF_IMAGE; item.pszText = text; item.cchTextMax = 512;
        SendMessageW(window, TCM_GETITEMW, i, (LPARAM)&item);
        HIMAGELIST images = TabCtrl_GetImageList(window);
        if (images && item.iImage >= 0) {
            int width, height; ImageList_GetIconSize(images, &width, &height);
            ImageList_Draw(images, item.iImage, dc, rect.left+5,
                (rect.top+rect.bottom-height)/2, ILD_TRANSPARENT);
            rect.left += width+6;
        }
        DrawTextW(dc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (i == selected && GetFocus() == window &&
            !(SendMessage(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS)) {
            InflateRect(&rect, -3, -3); DrawFocusRect(dc, &rect);
        }
    }
    if (old) SelectObject(dc, old);
}

static void paintCombo(HWND window, HDC dc)
{
    RECT rect; GetClientRect(window, &rect);
    FillRect(dc, &rect, NemuStaticGetSysColorBrush(COLOR_WINDOW));
    FrameRect(dc, &rect, NemuStaticGetSysColorBrush(COLOR_BTNHIGHLIGHT));
    RECT arrow = rect; arrow.left = arrow.right - GetSystemMetrics(SM_CXVSCROLL);
    InflateRect(&arrow, -1, -1);
    FillRect(dc, &arrow, NemuStaticGetSysColorBrush(COLOR_BTNFACE));
    int x = (arrow.left+arrow.right)/2, y = (arrow.top+arrow.bottom)/2;
    HPEN pen = CreatePen(PS_SOLID, 1, NemuStaticGetSysColor(IsWindowEnabled(window) ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
    HGDIOBJ old = SelectObject(dc, pen);
    for (int i = 0; i < 4; ++i) {
        MoveToEx(dc, x-3+i, y-1+i, NULL); LineTo(dc, x+4-i, y-1+i);
    }
    SelectObject(dc, old); DeleteObject(pen);
    rect.left += 4; rect.right = arrow.left - 2;
    drawText(window, dc, rect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    if (GetFocus() == window && !(SendMessage(window, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS)) {
        InflateRect(&rect, -1, -2); DrawFocusRect(dc, &rect);
    }
}

static LRESULT customDraw(NMHDR *header)
{
    NMCUSTOMDRAW *draw = (NMCUSTOMDRAW *)header;
    ControlKind kind = controlKind(header->hwndFrom);
    if (kind != Toolbar && kind != Header) return CDRF_DODEFAULT;
    if (draw->dwDrawStage == CDDS_PREPAINT) {
        FillRect(draw->hdc, &draw->rc, NemuStaticGetSysColorBrush(COLOR_BTNFACE));
        return CDRF_NOTIFYITEMDRAW;
    }
    if (kind == Toolbar && draw->dwDrawStage == CDDS_ITEMPOSTPAINT) {
        toolbarDropArrow(draw);
        return CDRF_DODEFAULT;
    }
    if (draw->dwDrawStage == CDDS_ITEMPREPAINT) {
        if (kind == Toolbar) {
            NMTBCUSTOMDRAW *bar = (NMTBCUSTOMDRAW *)draw;
            bar->clrText = NemuStaticGetSysColor(COLOR_BTNTEXT);
            bar->clrBtnFace = NemuStaticGetSysColor(COLOR_BTNFACE);
            bar->clrBtnHighlight = NemuStaticGetSysColor(COLOR_BTNHIGHLIGHT);
            bar->clrHighlightHotTrack = NemuStaticGetSysColor(COLOR_HIGHLIGHT);
            bar->nStringBkMode = bar->nHLStringBkMode = TRANSPARENT;
            return TBCDRF_HILITEHOTTRACK | TBCDRF_NOETCHEDEFFECT | CDRF_NOTIFYPOSTPAINT;
        }
        RECT rect = draw->rc;
        FillRect(draw->hdc, &rect, NemuStaticGetSysColorBrush(COLOR_BTNFACE));
        WCHAR text[512] = {0}; HDITEMW item; ZeroMemory(&item, sizeof(item));
        item.mask = HDI_TEXT | HDI_FORMAT; item.pszText = text; item.cchTextMax = 512;
        SendMessageW(header->hwndFrom, HDM_GETITEMW, draw->dwItemSpec, (LPARAM)&item);
        SetBkMode(draw->hdc, TRANSPARENT); textColors(draw->hdc, COLOR_BTNFACE);
        rect.left += 5; rect.right -= 5;
        DrawTextW(draw->hdc, text, -1, &rect, DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS |
            ((item.fmt & HDF_CENTER) ? DT_CENTER : (item.fmt & HDF_RIGHT) ? DT_RIGHT : DT_LEFT));
        return CDRF_SKIPDEFAULT;
    }
    return CDRF_DODEFAULT;
}

static LRESULT CALLBACK themedProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                                  UINT_PTR, DWORD_PTR data)
{
    ThemedWindow *state = (ThemedWindow *)data;
    if (message == WM_NCDESTROY) {
        removeSubclass(window, themedProc, subclassId);
        ThemedWindow **item = &windows;
        while (*item && *item != state) item = &(*item)->next;
        if (*item) *item = state->next;
        HIMAGELIST images = state->darkImages, disabledImages = state->darkDisabledImages;
        if (disabledImages) SendMessage(window, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)state->originalDisabledImages);
        if (images && (HIMAGELIST)SendMessage(window, TB_GETIMAGELIST, 0, 0) == images)
            SendMessage(window, TB_SETIMAGELIST, 0, (LPARAM)state->originalImages);
        free(state);
        LRESULT result = defaultSubclass(window, message, wParam, lParam);
        if (images) ImageList_Destroy(images);
        if (disabledImages) ImageList_Destroy(disabledImages);
        return result;
    }
    if (dark) {
        if (nonClientMessage && message == nonClientMessage) { paintNonClient(state); return 0; }
        if (message == WM_TIMER && wParam == subclassId && state->kind == List) {
            paintNonClient(state); return 0;
        }
        if (state->kind == Toolbar && message == WM_PAINT)
            toolbarImages(state);
        if (message == WM_MEASUREITEM && ((MEASUREITEMSTRUCT *)lParam)->CtlType == ODT_MENU) {
            MEASUREITEMSTRUCT *measure = (MEASUREITEMSTRUCT *)lParam;
            ThemedMenu *item = findMenuItem(measure->itemData);
            if (item) {
                WCHAR text[256]; menuText(item, text, 256);
                HDC dc = GetDC(window); HFONT font = menuFont();
                HGDIOBJ old = font ? SelectObject(dc, font) : NULL;
                SIZE size; GetTextExtentPoint32W(dc, text, lstrlenW(text), &size);
                measure->itemWidth = size.cx+8; measure->itemHeight = GetSystemMetrics(SM_CYMENU)-2;
                if (old) SelectObject(dc, old);
                if (font) DeleteObject(font);
                ReleaseDC(window, dc); return TRUE;
            }
        }
        if (message == WM_DRAWITEM && ((DRAWITEMSTRUCT *)lParam)->CtlType == ODT_MENU) {
            DRAWITEMSTRUCT *draw = (DRAWITEMSTRUCT *)lParam;
            ThemedMenu *item = findMenuItem(draw->itemData);
            if (item) {
                FillRect(draw->hDC, &draw->rcItem, NemuStaticGetSysColorBrush(
                    (draw->itemState & (ODS_SELECTED|ODS_HOTLIGHT)) ? COLOR_HIGHLIGHT : COLOR_MENU));
                SetBkMode(draw->hDC, TRANSPARENT);
                SetTextColor(draw->hDC, NemuStaticGetSysColor(
                    (draw->itemState & (ODS_DISABLED|ODS_GRAYED)) ? COLOR_GRAYTEXT : COLOR_MENUTEXT));
                WCHAR text[256]; menuText(item, text, 256);
                HFONT font = menuFont(); HGDIOBJ old = font ? SelectObject(draw->hDC, font) : NULL;
                DrawTextW(draw->hDC, text, -1, &draw->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE |
                    ((draw->itemState & ODS_NOACCEL) ? DT_HIDEPREFIX : 0));
                if (old) SelectObject(draw->hDC, old);
                if (font) DeleteObject(font);
                return TRUE;
            }
        }
        if (message >= WM_CTLCOLORMSGBOX && message <= WM_CTLCOLORSTATIC) {
            int bg = message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX ? COLOR_WINDOW : COLOR_BTNFACE;
            textColors((HDC)wParam, bg);
            return (LRESULT)NemuStaticGetSysColorBrush(bg);
        }
        if (message == WM_NOTIFY && ((NMHDR *)lParam)->code == NM_CUSTOMDRAW) {
            ControlKind kind = controlKind(((NMHDR *)lParam)->hwndFrom);
            if (kind == Toolbar || kind == Header) return customDraw((NMHDR *)lParam);
        }
        if (message == WM_ERASEBKGND && (state->kind == Panel || state->kind == Toolbar ||
            state->kind == Status || state->kind == Tabs || state->kind == Button)) {
            RECT rect; GetClientRect(window, &rect);
            FillRect((HDC)wParam, &rect, NemuStaticGetSysColorBrush(COLOR_BTNFACE)); return 1;
        }
        if ((message == WM_PAINT || message == WM_PRINTCLIENT) &&
            (state->kind == Status ||
             (state->kind == Tabs && !(GetWindowLong(window, GWL_STYLE) & TCS_OWNERDRAWFIXED)) ||
             (state->kind == Combo && (GetWindowLong(window, GWL_STYLE) & 3) == CBS_DROPDOWNLIST &&
              !(GetWindowLong(window, GWL_STYLE) & (CBS_OWNERDRAWFIXED|CBS_OWNERDRAWVARIABLE))) ||
             (state->kind == Button &&
             !(GetWindowLong(window, GWL_STYLE) & (BS_BITMAP|BS_ICON)) &&
             (GetWindowLong(window, GWL_STYLE) & BS_TYPEMASK) != BS_OWNERDRAW))) {
            PAINTSTRUCT paint;
            HDC dc = message == WM_PAINT ? BeginPaint(window, &paint) : (HDC)wParam;
            if (state->kind == Status) paintStatus(window, dc);
            else if (state->kind == Tabs) paintTabs(window, dc);
            else if (state->kind == Combo) paintCombo(window, dc);
            else paintButton(window, dc);
            if (message == WM_PAINT) EndPaint(window, &paint);
            return 0;
        }
    }
    bool tracking = dark && state->kind == List && message == WM_NCLBUTTONDOWN &&
        (wParam == HTHSCROLL || wParam == HTVSCROLL);
    if (tracking) SetTimer(window, subclassId, 30, NULL);
    LRESULT result = defaultSubclass(window, message, wParam, lParam);
    if (tracking && IsWindow(window)) {
        KillTimer(window, subclassId); paintNonClient(state);
    }
    if (dark && (message == WM_NCPAINT || message == WM_PAINT || message == WM_SIZE ||
        message == WM_HSCROLL || message == WM_VSCROLL || message == WM_NCMOUSEMOVE ||
        message == WM_NCLBUTTONUP || message == WM_NCMOUSELEAVE || message == LVM_SCROLL || message == LVM_ENSUREVISIBLE))
        paintNonClient(state);
    // Common controls can repaint their native scrollbars after returning from
    // a scroll/hover notification. Reapply the skin after that native work.
    if (dark && nonClientMessage && (message == WM_HSCROLL || message == WM_VSCROLL ||
        message == WM_NCMOUSEMOVE || message == WM_NCLBUTTONUP || message == WM_NCMOUSELEAVE || message == LVM_SCROLL ||
        message == LVM_ENSUREVISIBLE)) PostMessage(window, nonClientMessage, 0, 0);
    // Some menu implementations ignore MIM_BACKGROUND in the unused band.
    // Paint only the area after the last item; owner drawing handles the items.
    if (dark && (message == WM_NCPAINT || message == WM_NCACTIVATE) && GetMenu(window)) {
        MENUBARINFO bar; ZeroMemory(&bar, sizeof(bar)); bar.cbSize = sizeof(bar);
        HMENU menu = GetMenu(window); RECT last;
        if (GetMenuBarInfo(window, OBJID_MENU, 0, &bar) &&
            GetMenuItemRect(window, menu, GetMenuItemCount(menu)-1, &last)) {
            RECT rect = bar.rcBar, outer; GetWindowRect(window, &outer);
            // Include the bottom menu rule, which Windows otherwise draws light.
            RECT rule = rect; rule.top = rule.bottom-1;
            OffsetRect(&rule, -outer.left, -outer.top);
            HDC lineDC = GetWindowDC(window);
            FillRect(lineDC, &rule, NemuStaticGetSysColorBrush(COLOR_MENU)); ReleaseDC(window, lineDC);
            rect.left = last.right; OffsetRect(&rect, -outer.left, -outer.top);
            HDC dc = GetWindowDC(window);
            FillRect(dc, &rect, NemuStaticGetSysColorBrush(COLOR_MENU)); ReleaseDC(window, dc);
        }
    }
    return result;
}


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
    if (!(GetWindowLongA(window, GWL_STYLE) & WS_CHILD)) {
        if (allowWindow) allowWindow(window, dark);
        if (setAttribute) {
            BOOL enabled = dark ? TRUE : FALSE;
            HRESULT result = setAttribute(window, 20, &enabled, sizeof(enabled));
            if (FAILED(result) && windowsBuild < 19041)
                setAttribute(window, 19, &enabled, sizeof(enabled));
        }
    }
    if (!subclassWindow || !defaultSubclass || !removeSubclass) return;
    ThemedWindow *state;
    for (state = windows; state && state->window != window; state = state->next) {}
    if (!state) {
        state = (ThemedWindow *)calloc(1, sizeof(*state));
        if (!state) return;
        state->window = window; state->kind = controlKind(window);
        if (!subclassWindow(window, themedProc, subclassId, (DWORD_PTR)state)) { free(state); return; }
        state->next = windows; windows = state;
    }
    controlColors(state);
    if (!(GetWindowLong(window, GWL_STYLE) & WS_CHILD)) menuColors(window);
}

static BOOL CALLBACK updateChild(HWND window, LPARAM)
{
    applyWindow(window); return TRUE;
}
static BOOL CALLBACK updateWindow(HWND window, LPARAM)
{
    applyWindow(window);
    EnumChildWindows(window, updateChild, 0);
    DrawMenuBar(window);
    RedrawWindow(window, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
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
    if (vclReady) NemuStaticVclThemeRefresh();
    EnumThreadWindows(uiThread, updateWindow, 0);
    updating = false;
}

static bool formWindow(HWND window)
{
    DWORD style = GetWindowLong(window, GWL_STYLE);
    // Delphi's ParentWindow forms retain WS_CHILD despite being modal dialogs.
    // Wait for the form's show, rather than touching partially loaded controls.
    return !(style & WS_CHILD) || (style & WS_CAPTION) == WS_CAPTION;
}

static LRESULT CALLBACK windowMessages(int code, WPARAM wParam, LPARAM lParam)
{
    if (code >= 0 && !updating) {
        const CWPRETSTRUCT *message = (const CWPRETSTRUCT *)lParam;
        if (message->message == WM_SHOWWINDOW && message->wParam && vclReady &&
            formWindow(message->hwnd)) {
            updating = true;
            NemuStaticVclThemeRefresh();
            updateWindow(message->hwnd, 0);
            updating = false;
        } else if (message->message == WM_CREATE || message->message == WM_INITDIALOG)
            applyWindow(message->hwnd);
        else if ((message->message == WM_SETTINGCHANGE || message->message == WM_THEMECHANGED ||
                  message->message == WM_SYSCOLORCHANGE) &&
                 formWindow(message->hwnd))
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
    nonClientMessage = RegisterWindowMessageA("Nemu64.StaticTheme.NonClient");
    dwmModule = loadSystemLibrary("dwmapi.dll");
    if (dwmModule) setAttribute = (DwmAttributeFunction)GetProcAddress(dwmModule, "DwmSetWindowAttribute");
    themeModule = loadSystemLibrary("uxtheme.dll");
    HMODULE common = GetModuleHandleA("comctl32.dll");
    if (common) {
        subclassWindow = (SubclassFunction)GetProcAddress(common, "SetWindowSubclass");
        removeSubclass = (RemoveSubclassFunction)GetProcAddress(common, "RemoveWindowSubclass");
        defaultSubclass = (DefaultSubclassFunction)GetProcAddress(common, "DefSubclassProc");
    }
    HMODULE user = GetModuleHandleA("user32.dll");
    if (user) scrollBarInfo = (ScrollBarInfoFunction)GetProcAddress(user, "GetScrollBarInfo");
    if (themeModule) {
        setWindowTheme = (WindowThemeFunction)GetProcAddress(themeModule, "SetWindowTheme");
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
    // Thread-local hook also catches controls created later by VCL/MFC.
    windowHook = SetWindowsHookExA(WH_CALLWNDPROCRET, windowMessages, NULL, uiThread);
}

extern "C" void __cdecl NemuStaticThemeVclReady(BOOL ready)
{
    vclReady = ready != FALSE;
    if (vclReady) NemuStaticVclThemeRefresh();
}

extern "C" void __cdecl NemuStaticThemeShutdown(void)
{
    if (windowHook) UnhookWindowsHookEx(windowHook);
    windowHook = NULL;
    updating = true; dark = false;
    if (vclReady) NemuStaticVclThemeRefresh();
    vclReady = false;
    for (ThemedWindow *restore = windows; restore; restore = restore->next)
        if (IsWindow(restore->window)) menuColors(restore->window);
    while (menuItems) {
        ThemedMenu *item = menuItems; menuItems = item->next; free(item);
    }
    while (windows) {
        ThemedWindow *state = windows; windows = state->next;
        if (IsWindow(state->window)) {
            controlColors(state);
            removeSubclass(state->window, themedProc, subclassId);
        }
        if (state->darkImages) ImageList_Destroy(state->darkImages);
        if (state->darkDisabledImages) ImageList_Destroy(state->darkDisabledImages);
        free(state);
    }
    for (int i = 0; i < 31; ++i) {
        if (colorBrushes[i]) DeleteObject(colorBrushes[i]);
        colorBrushes[i] = NULL;
    }
    if (preferredApp) preferredApp(0);
    else if (allowApp) allowApp(false);
    if (themeModule) FreeLibrary(themeModule);
    if (dwmModule) FreeLibrary(dwmModule);
    themeModule = dwmModule = NULL;
    setAttribute = NULL; shouldUseDark = NULL; allowWindow = NULL;
    allowApp = NULL; preferredApp = NULL; refreshPolicy = flushMenus = NULL;
    subclassWindow = NULL; removeSubclass = NULL; defaultSubclass = NULL; setWindowTheme = NULL;
    scrollBarInfo = NULL; nonClientMessage = 0;
    started = updating = dark = false;
    uiThread = windowsBuild = 0;
}
