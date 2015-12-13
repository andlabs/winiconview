// 13 december 2015
#include "winapi.h"

// main.c
extern HINSTANCE hInstance;
extern int nCmdShow;

// mainwin.c
extern HWND mainwin;
extern void initMainWindow(void);
extern void uninitMainWindow(void);

// panic.c
extern void panic(const WCHAR *msg);
extern void panichr(const WCHAR *msg, HRESULT hr);
