// A control handle with the original trivial global lifetime hooks.
#ifndef KAILLERA_CONTROL_H
#define KAILLERA_CONTROL_H
class TextControl {
public:
    TextControl() { window=0; }
    ~TextControl() {}
    operator HWND() { return window; }
    HWND operator=(HWND value) { return window=value; }
    void select(int first,int last) { SendMessage(window,EM_SETSEL,first,last); }
    void replace(char *text) { SendMessage(window,EM_REPLACESEL,0,(LPARAM)text); }
    HWND window;
};
#endif
