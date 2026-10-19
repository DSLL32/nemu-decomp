/* Static-build drawing bridges. Do not change Windows' global system colors. */
#include "../nemu64/resource.h"

extern "C" BOOL WINAPI NemuStaticDrawFrameControl(HDC dc, LPRECT rect, UINT type, UINT state)
{
    if (!dark || type != DFC_BUTTON || (state & 0xff) != DFCS_BUTTONPUSH)
        return DrawFrameControl(dc, rect, type, state);
    FillRect(dc, rect, NemuStaticGetSysColorBrush(COLOR_BTNFACE));
    FrameRect(dc, rect, NemuStaticGetSysColorBrush(
        state & DFCS_PUSHED ? COLOR_BTNSHADOW : COLOR_BTNHIGHLIGHT));
    if (state & DFCS_ADJUSTRECT) InflateRect(rect, -2, -2);
    return TRUE;
}
extern "C" BOOL WINAPI NemuStaticDrawEdge(HDC dc, LPRECT rect, UINT edge, UINT flags)
{
    if (!dark || (flags & BF_DIAGONAL)) return DrawEdge(dc, rect, edge, flags);
    RECT inner = *rect;
    int width = (edge & BDR_INNER) && (edge & BDR_OUTER) ? 2 : 1;
    HBRUSH border = NemuStaticGetSysColorBrush(
        edge & (BDR_SUNKENOUTER | BDR_SUNKENINNER) ? COLOR_BTNSHADOW : COLOR_BTNHIGHLIGHT);
    if (flags & BF_TOP) { RECT line = inner; line.bottom = line.top+width; FillRect(dc, &line, border); inner.top += width; }
    if (flags & BF_BOTTOM) { RECT line = inner; line.top = line.bottom-width; FillRect(dc, &line, border); inner.bottom -= width; }
    if (flags & BF_LEFT) { RECT line = inner; line.right = line.left+width; FillRect(dc, &line, border); inner.left += width; }
    if (flags & BF_RIGHT) { RECT line = inner; line.left = line.right-width; FillRect(dc, &line, border); inner.right -= width; }
    if (flags & BF_MIDDLE) FillRect(dc, &inner, NemuStaticGetSysColorBrush(COLOR_BTNFACE));
    if (flags & BF_ADJUST) *rect = inner;
    return TRUE;
}

static void toolbarImage(HIMAGELIST source, HIMAGELIST target, int index, int width, int height, bool disabled)
{
    // Render the original mask to a color key first. A 24-bit image avoids
    // treating old masked bitmap icons as transparent ARGB after inversion.
    BITMAPINFO info; ZeroMemory(&info, sizeof(info)); info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = width; info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1; info.bmiHeader.biBitCount = 24;
    BYTE *pixels;
    HBITMAP image = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, (void **)&pixels, NULL, 0);
    HBITMAP mask = CreateBitmap(width, height, 1, 1, NULL);
    HDC dc = CreateCompatibleDC(NULL), maskDC = CreateCompatibleDC(NULL);
    HGDIOBJ oldImage = SelectObject(dc, image), oldMask = SelectObject(maskDC, mask);
    if (image && mask) {
        RECT rect = {0,0,width,height}; HBRUSH key = CreateSolidBrush(RGB(255,0,255));
        FillRect(dc, &rect, key); DeleteObject(key);
        if (ImageList_Draw(source, index, dc, 0, 0, ILD_TRANSPARENT)) {
            GdiFlush(); // Complete GDI writes before reading the DIB memory.
            int stride = (width*3+3)&~3;
            for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
                BYTE *pixel = pixels + y*stride+x*3;
                bool transparent = pixel[0]==255 && pixel[1]==0 && pixel[2]==255;
                SetPixelV(maskDC, x, y, transparent ? RGB(255,255,255) : RGB(0,0,0));
                if (!transparent) {
                    if (disabled) pixel[0]=pixel[1]=pixel[2]=150;
                    else { pixel[0]^=255; pixel[1]^=255; pixel[2]^=255; }
                }
            }
            GdiFlush();
            SelectObject(dc, oldImage); SelectObject(maskDC, oldMask);
            ImageList_Replace(target, index, image, mask);
        }
    }
    SelectObject(dc, oldImage); SelectObject(maskDC, oldMask);
    DeleteDC(dc); DeleteDC(maskDC);
    if (image) DeleteObject(image);
    if (mask) DeleteObject(mask);
}
static void toolbarImages(ThemedWindow *state)
{
    HWND window = state->window;
    HIMAGELIST current = (HIMAGELIST)SendMessage(window, TB_GETIMAGELIST, 0, 0);
    if (!dark) {
        if (state->darkDisabledImages)
            SendMessage(window, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)state->originalDisabledImages);
        if (state->darkImages && current == state->darkImages)
            SendMessage(window, TB_SETIMAGELIST, 0, (LPARAM)state->originalImages);
        return;
    }
    if (!current || current == state->darkImages) return;
    char name[128]; GetClassNameA(GetParent(window), name, sizeof(name));
    if (lstrcmpiA(name, "Nemu64")) return;
    if (!state->originalImages)
        state->originalDisabledImages = (HIMAGELIST)SendMessage(window, TB_GETDISABLEDIMAGELIST, 0, 0);
    if (state->darkImages) { ImageList_Destroy(state->darkImages); state->darkImages = NULL; }
    if (state->darkDisabledImages) {
        SendMessage(window, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)state->originalDisabledImages);
        ImageList_Destroy(state->darkDisabledImages); state->darkDisabledImages = NULL;
    }
    int width, height, count = ImageList_GetImageCount(current);
    if (!ImageList_GetIconSize(current, &width, &height)) return;
    HIMAGELIST copy = ImageList_Create(width, height, ILC_COLOR24 | ILC_MASK, count, 1);
    if (!copy) return;
    HIMAGELIST disabled = ImageList_Create(width, height, ILC_COLOR24 | ILC_MASK, count, 1);
    if (!disabled) { ImageList_Destroy(copy); return; }
    for (int i = 0; i < count; ++i) {
        HICON icon = ImageList_GetIcon(current, i, ILD_NORMAL);
        int index = icon ? ImageList_AddIcon(copy, icon) : -1;
        int disabledIndex = icon ? ImageList_AddIcon(disabled, icon) : -1;
        if (icon) DestroyIcon(icon);
        if (index != i || disabledIndex != i) { ImageList_Destroy(copy); ImageList_Destroy(disabled); return; }
        toolbarImage(current, disabled, i, width, height, true);
    }
    int buttons = (int)SendMessage(window, TB_BUTTONCOUNT, 0, 0);
    for (int i = 0; i < buttons; ++i) {
        TBBUTTON button;
        if (!SendMessage(window, TB_GETBUTTON, i, (LPARAM)&button)) continue;
        if (button.idCommand != ID_PAUSE && button.idCommand != ID_CLOSE_ROM && button.idCommand != ID_RESET) continue;
        toolbarImage(current, copy, button.iBitmap, width, height, false);
    }
    state->originalImages = current; state->darkImages = copy; state->darkDisabledImages = disabled;
    SendMessage(window, TB_SETIMAGELIST, 0, (LPARAM)copy);
    SendMessage(window, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)disabled);
}

static void scrollArrow(HDC dc, RECT rect, bool vertical, bool forward, bool enabled)
{
    FillRect(dc, &rect, NemuStaticGetSysColorBrush(COLOR_BTNFACE));
    HPEN pen = CreatePen(PS_SOLID, 1, NemuStaticGetSysColor(enabled ? COLOR_BTNTEXT : COLOR_GRAYTEXT));
    HGDIOBJ old = SelectObject(dc, pen);
    int x = (rect.left+rect.right)/2, y = (rect.top+rect.bottom)/2;
    for (int i = 0; i < 4; ++i) {
        int direction = forward ? i : -i;
        if (vertical) { MoveToEx(dc, x-3+i, y-1+direction, NULL); LineTo(dc, x+4-i, y-1+direction); }
        else { MoveToEx(dc, x-1+direction, y-3+i, NULL); LineTo(dc, x-1+direction, y+4-i); }
    }
    SelectObject(dc, old); DeleteObject(pen);
}
static void paintScrollBar(HWND window, HDC dc, LONG object, const RECT &outer)
{
    SCROLLBARINFO info; ZeroMemory(&info, sizeof(info)); info.cbSize = sizeof(info);
    if (!scrollBarInfo || !scrollBarInfo(window, object, &info) ||
        (info.rgstate[0] & (STATE_SYSTEM_INVISIBLE | STATE_SYSTEM_OFFSCREEN))) return;
    RECT rect = info.rcScrollBar; OffsetRect(&rect, -outer.left, -outer.top);
    if (IsRectEmpty(&rect)) return;
    bool vertical = object == OBJID_VSCROLL;
    FillRect(dc, &rect, NemuStaticGetSysColorBrush(COLOR_WINDOW));
    RECT first = rect, last = rect, thumb = rect;
    if (vertical) {
        first.bottom = first.top + info.dxyLineButton; last.top = last.bottom-info.dxyLineButton;
        thumb.top = rect.top+info.xyThumbTop; thumb.bottom = rect.top+info.xyThumbBottom;
    } else {
        first.right = first.left+info.dxyLineButton; last.left = last.right-info.dxyLineButton;
        thumb.left = rect.left+info.xyThumbTop; thumb.right = rect.left+info.xyThumbBottom;
    }
    scrollArrow(dc, first, vertical, false, !(info.rgstate[1] & STATE_SYSTEM_UNAVAILABLE));
    scrollArrow(dc, last, vertical, true, !(info.rgstate[5] & STATE_SYSTEM_UNAVAILABLE));
    if (info.xyThumbBottom > info.xyThumbTop && !(info.rgstate[3] & STATE_SYSTEM_INVISIBLE)) {
        InflateRect(&thumb, -2, -2);
        HBRUSH brush = CreateSolidBrush(info.rgstate[3] & STATE_SYSTEM_PRESSED ? RGB(140,140,140) : RGB(100,100,100));
        FillRect(dc, &thumb, brush); DeleteObject(brush);
    }
}
static void paintNonClient(ThemedWindow *state)
{
    HWND window = state->window;
    if (state->kind != List && state->kind != Edit && state->kind != Toolbar) return;
    HDC dc = GetWindowDC(window);
    RECT outer; GetWindowRect(window, &outer);
    if (state->kind == Toolbar) {
        RECT client; GetClientRect(window, &client);
        POINT origin = {0,0}; ClientToScreen(window, &origin);
        RECT divider = {0,0,outer.right-outer.left,origin.y-outer.top};
        FillRect(dc, &divider, NemuStaticGetSysColorBrush(COLOR_BTNFACE));
    } else {
        paintScrollBar(window, dc, OBJID_HSCROLL, outer);
        paintScrollBar(window, dc, OBJID_VSCROLL, outer);
        if ((GetWindowLong(window, GWL_STYLE) & (WS_HSCROLL|WS_VSCROLL)) == (WS_HSCROLL|WS_VSCROLL) && scrollBarInfo) {
            SCROLLBARINFO horizontal, vertical; ZeroMemory(&horizontal,sizeof(horizontal));
            ZeroMemory(&vertical,sizeof(vertical)); horizontal.cbSize=sizeof(horizontal); vertical.cbSize=sizeof(vertical);
            if (scrollBarInfo(window,OBJID_HSCROLL,&horizontal) && scrollBarInfo(window,OBJID_VSCROLL,&vertical)) {
                RECT corner={vertical.rcScrollBar.left,horizontal.rcScrollBar.top,vertical.rcScrollBar.right,horizontal.rcScrollBar.bottom};
                OffsetRect(&corner,-outer.left,-outer.top); FillRect(dc,&corner,NemuStaticGetSysColorBrush(COLOR_BTNFACE));
            }
        }
        if (GetWindowLong(window, GWL_EXSTYLE) & WS_EX_CLIENTEDGE) {
            RECT edge = {0,0,outer.right-outer.left,outer.bottom-outer.top};
            FrameRect(dc, &edge, NemuStaticGetSysColorBrush(COLOR_BTNSHADOW));
            InflateRect(&edge,-1,-1); FrameRect(dc, &edge, NemuStaticGetSysColorBrush(COLOR_BTNSHADOW));
        }
    }
    ReleaseDC(window, dc);
}
