// 13 december 2015
#include "winapi.h"
#include "resources.h"

#define programName L"Windows Icon Viewer"

// main.c
extern HINSTANCE hInstance;
extern int nCmdShow;

// mainwin.c
enum {
	// Sent to begin a directory search.
	// wParam - pointer to wide string with directory name
	// lParam - 0
	// lResult - 0
	msgAddIcons = WM_USER + 40,
	// Sent by collector thread to indicate progress.
	// wParam - ULONGLONG pointer with number of items completed
	// lParam - ULONGLONG pointer with number of items total
	// lResult - nonzero if user cancelled
	msgProgress,
	// Sent by collector thread when done. Not sent if user cancelled.
	// wParam - if successful, TODO
	// lParam - if failure, TODO
	// lResult - 0
	msgFinished,
};
extern HWND initMainWindow(void);
extern void uninitMainWindow(HWND mainwin);

// panic.c
extern HWND panicParent;
extern void panic(const WCHAR *msg);
extern void panichr(const WCHAR *msg, HRESULT hr);
