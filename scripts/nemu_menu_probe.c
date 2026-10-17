/* Read-only validation hook for Wine's process-local menu implementation.
 * The external fixture installs this through SetWindowsHookEx, requests a
 * snapshot, then removes the hook. This is instrumentation, not app source.
 */
#include <windows.h>
#include <stdio.h>

#define SNAPSHOT_MESSAGE (WM_APP + 0x317)
static int snapshot(HMENU menu, unsigned depth, FILE *file) {
    int count = GetMenuItemCount(menu), i;
    if (count < 0) return 0;
    for (i = 0; i < count; ++i) {
        char label[1024];
        const unsigned char *p;
        HMENU submenu = GetSubMenu(menu, i);
        UINT state = GetMenuState(menu, i, MF_BYPOSITION) & 0xff;
        state &= ~(MF_HILITE | MF_MOUSESELECT);
        GetMenuStringA(menu, i, label, sizeof(label), MF_BYPOSITION);
        fprintf(file, "Menu depth=%u index=%d id=%u state=%02X text=\"", depth, i,
                GetMenuItemID(menu, i), state);
        for (p = (const unsigned char *)label; *p; ++p) {
            if (*p == '\\' || *p == '"') fprintf(file, "\\%c", *p);
            else if (*p < 32 || *p >= 127) fprintf(file, "\\x%02X", *p);
            else fputc(*p, file);
        }
        fputs("\"\n", file);
        if (submenu && !snapshot(submenu, depth + 1, file)) return 0;
    }
    return 1;
}

LRESULT CALLBACK MenuProbeHook(int code, WPARAM wParam, LPARAM lParam) {
    if (code >= 0) {
        CWPSTRUCT *call = (CWPSTRUCT *)lParam;
        if (call->message == SNAPSHOT_MESSAGE) {
            FILE *file = fopen("nemu-menu.txt", "w");
            if (file) {
                if (snapshot(GetMenu(call->hwnd), 0, file)) fputs("Menu snapshot completed\n", file);
                fclose(file);
            }
        }
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}
