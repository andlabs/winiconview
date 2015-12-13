// 4 may 2014
#include "winapi.h"

// the WINVER defines in winapi.h could have the consequence of leaving this undefined (and it's not defined by the MinGW headers so)
#ifndef LVGS_COLLAPSIBLE
#define LVGS_COLLAPSIBLE 0x00000008		// value from Microsoft's commctrl.h
#endif

// oh my fucking god you have got to be fucking kidding me THANKS RPCNDR.H
#undef small

// winiconview.c
extern HINSTANCE hInstance;
extern int nCmdShow;

void setControlFont(HWND);
HFONT selectControlFont(HDC);

// mainwin.c
void registerMainWindowClass(void);
HWND makeMainWindow(WCHAR *);

// util.c
void panic(WCHAR *fmt, ...);
WCHAR *ourawsprintf(WCHAR *fmt, ...);
WCHAR *ourvawsprintf(WCHAR *fmt, va_list arg);

// geticons.c
enum {
	msgBegin = WM_APP + 1,
	msgStep,
	msgEnd,
};

struct giThreadInput {
	HWND mainwin;
	WCHAR *dirname;
};

struct giThreadOutput {
	HIMAGELIST largeicons;
	HIMAGELIST smallicons;
	LVGROUP *groups;
	size_t nGroups;
	LVITEM *items;
	size_t nItems;
	WCHAR **groupnames;		// to store group names for sorting
	int ngroupnames;
};

void initGetIcons(void);
DWORD WINAPI getIcons(LPVOID);
INT CALLBACK groupLess(INT gn1, INT gn2, VOID *data);

// listview.c
HWND makeListView(HWND, HMENU, struct giThreadOutput *);
void resizeListView(HWND, HWND);
LRESULT handleListViewRightClick(HWND, NMHDR *);
