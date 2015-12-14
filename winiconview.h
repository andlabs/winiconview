// 13 december 2015
#include "winapi.h"
#include "resources.h"

#define programName L"Windows Icon Viewer"

// main.c
extern HINSTANCE hInstance;
extern int nCmdShow;

// mainwin.c
extern HWND initMainWindow(void);
extern void uninitMainWindow(HWND mainwin);

// panic.c
extern HWND panicParent;
extern void panic(const WCHAR *msg);
extern void panichr(const WCHAR *msg, HRESULT hr);
