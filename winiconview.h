// 4 may 2014
#define _UNICODE
#define UNICODE
#define STRICT
#define STRICT_TYPED_ITEMIDS
#define WINVER 0x0501			// to get struct sizes right for Windows XP; see http://stackoverflow.com/questions/5803103/grid-control-run-time-error-when-run-at-windows-xp-in-visual-studio-2008
#define _WIN32_WINNT 0x0501
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>
#include <commctrl.h>			// needed for InitCommonControlsEx() (thanks Xeek in irc.freenode.net/#winapi for confirming)
#include <shlobj.h>
#include <shlwapi.h>

// the WINVER/_WIN32_WINNT defines above could have the consequence of leaving this undefined (and it's not defined by the MinGW headers so)
#ifndef LVGS_COLLAPSIBLE
#define LVGS_COLLAPSIBLE 0x00000008		// value from Microsoft's commctrl.h
#endif

// oh my fucking god you have got to be fucking kidding me THANKS RPCNDR.H
#undef small

// no parentheses so it can be concatenated with other string constants
#define PROGNAME L"Windows Icon Viewer"

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

void initGetIcons(void);
DWORD WINAPI getIcons(LPVOID);
INT CALLBACK groupLess(INT gn1, INT gn2, VOID *data);

// listview.c
HWND makeListView(HWND, HMENU, struct giThreadOutput *);
void resizeListView(HWND, HWND);
LRESULT handleListViewRightClick(HWND, NMHDR *);
