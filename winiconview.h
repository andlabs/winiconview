// 4 may 2014
#define _UNICODE
#define UNICODE
#define STRICT
#define STRICT_TYPED_ITEMIDS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>
#include <commctrl.h>		// needed for InitCommonControlsEx() (thanks Xeek in irc.freenode.net/#winapi for confirming)
#include <shlobj.h>
#include <shlwapi.h>

#define PROGNAME (L"Windows Icon Viewer")

// winiconview.c
extern HINSTANCE hInstance;
extern int nCmdShow;

void setControlFont(HWND);
HFONT selectControlFont(HDC);

// mainwin.c
void registerMainWindowClass(void);
HWND makeMainWindow(TCHAR *);

// util.c
void panic(TCHAR *fmt, ...);
TCHAR *ourawsprintf(TCHAR *fmt, ...);
TCHAR *ourvawsprintf(TCHAR *fmt, va_list arg);
TCHAR *toWideString(char *what);
void ourWow64DisableWow64FsRedirection(PVOID *);
void ourWow64RevertWow64FsRedirection(PVOID);

// geticons.c
enum {
	msgBegin = WM_APP + 1,
	msgStep,
	msgEnd,
};

struct giThreadInput {
	HWND mainwin;
	TCHAR *dirname;
};

struct giThreadOutput {
	HIMAGELIST largeicons;
	HIMAGELIST smallicons;
	LVGROUP *groups;
	size_t nGroups;
	LVITEM *items;
	size_t nItems;
	TCHAR **groupnames;		// to store group names for sorting
	int ngroupnames;
};

DWORD WINAPI getIcons(LPVOID);
INT CALLBACK groupLess(INT gn1, INT gn2, VOID *data);

// listview.c
HWND makeListView(HWND, HMENU, struct giThreadOutput *);
void resizeListView(HWND, HWND);
LRESULT handleListViewRightClick(HWND, NMHDR *);
