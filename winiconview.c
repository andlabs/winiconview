// pietro gagliardi 1 may 2014
// scratch Windows program by pietro gagliardi 17 april 2014
// fixed typos and added toWideString() 1 may 2014
// borrows code from the scratch GTK+ program (16-17 april 2014) and from code written 31 march 2014 and 11-12 april 2014
#define _UNICODE
#define UNICODE
#define STRICT
#define STRICT_TYPED_ITEMIDS
#define _GNU_SOURCE		// needed to declare asprintf()/vasprintf()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <windows.h>
#include <commctrl.h>		// needed for InitCommonControlsEx() (thanks Xeek in irc.freenode.net/#winapi for confirming)
#include <shlobj.h>
#include <shlwapi.h>

#ifdef  _MSC_VER
#error sorry! the scratch windows program relies on mingw-only functionality! (specifically: asprintf())
#endif

HMODULE hInstance;
HICON hDefaultIcon;
HCURSOR hDefaultCursor;
HFONT controlfont;

void panic(char *fmt, ...);
TCHAR *toWideString(char *what);

TCHAR *dirname = NULL;

void init(void)
{
	int usageret = EXIT_FAILURE;
	int argc;
	TCHAR **argv;

	argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (argv == NULL)
		panic("error splitting command line into argc/argv form");
	if (argc > 2)
		goto usage;
	if (argc == 2) {
		if (wcscmp(argv[1], L"--help") == 0) {
			usageret = 0;
			goto usage;
		}
//	dirname = argv[optind];
BOOL b;
if (IsWow64Process(GetCurrentProcess(), &b) == 0)
	panic("IsWow64Process() failed");
dirname = L"C:\\Windows\\System32";
if (b)
dirname = L"C:\\Windows\\SysWow64";
	} else {
		HRESULT res;
		BROWSEINFO bi;
		PIDLIST_ABSOLUTE pidl;

		// the browse for folders dialog uses COM
		res = CoInitialize(NULL);
		if (res != S_OK && res != S_FALSE)
			panic("error initializing COM for browse for folders dialog");
		ZeroMemory(&bi, sizeof (BROWSEINFO));
		// TODO dialog properties
		pidl = SHBrowseForFolder(&bi);
		if (pidl != NULL) {
			// THIS WILL CUT OFF. BIG TODO.
			TCHAR path[(4 * MAX_PATH) + 1];

			path[0] = L'\0';
			// TODO resolve shortcuts
			if (SHGetPathFromIDList(pidl, path) == FALSE)
				panic("error extracting folder from PIDL from folder dialog");
			printf("user selected %ws\n", path);
			CoTaskMemFree(pidl);
		} else
			printf("user aborted selection\n");
		CoUninitialize();
		exit(0);
	}
	return;

usage:
	fprintf(stderr,  "usage: %S\n\t%S pathname\n\t%S --help\n",
		argv[0], argv[0], argv[0]);
	exit(usageret);
}

HWND listview = NULL;

LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg) {
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		if (listview != NULL) {
			RECT r;

			if (GetClientRect(hwnd, &r) == 0)
				panic("error getting new list view size");
			if (MoveWindow(listview, 0, 0, r.right - r.left, r.bottom - r.top, TRUE) == 0)
				panic("error resizing list view");
		}
		return 0;
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	panic("oops: message %ud does not return anything; bug in wndproc()", msg);
}

HWND makeMainWindow(void)
{
	WNDCLASS cls;
	HWND hwnd;

	ZeroMemory(&cls, sizeof (WNDCLASS));
	cls.lpszClassName = L"mainwin";
	cls.lpfnWndProc = wndproc;
	cls.hInstance = hInstance;
	cls.hIcon = hDefaultIcon;
	cls.hCursor = hDefaultCursor;
	cls.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	if (RegisterClass(&cls) == 0)
		panic("error registering window class");
	hwnd = CreateWindowEx(0,
		L"mainwin", L"Main Window",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);
	if (hwnd == NULL)
		panic("opening main window failed");
	return hwnd;
}

TCHAR **groupnames = NULL;		// to store group names for sorting
int ngroupnames = 0;

void addGroup(HWND listview, TCHAR *name, int id)
{
	LVGROUP g;
	LRESULT n;
	TCHAR *wname;

	ZeroMemory(&g, sizeof (LVGROUP));
	g.cbSize = sizeof (LVGROUP);
	g.mask = LVGF_HEADER | LVGF_GROUPID;
	g.pszHeader = name;
	// for some reason the group ID and the index returned by LVM_INSERTGROUP are separate concepts... so we have to provide an ID.
	// (thanks to roxfan in irc.efnet.net/#winprog for confirming)
	g.iGroupId = id;
	n = SendMessage(listview, LVM_INSERTGROUP,
		(WPARAM) -1, (LPARAM) &g);
	if (n == (LRESULT) -1)
		panic("error adding list view group \"%s\"", name);
	// save the name so we can sort
	if (ngroupnames < id + 1)
		ngroupnames = id + 1;
	groupnames = (TCHAR **) realloc(groupnames, ngroupnames * sizeof (TCHAR *));
	if (groupnames == NULL)
		panic("error expanding groupnames list to fit new group name \"%s\"", name);
	groupnames[id] = _wcsdup(name);
}

// MSDN is bugged; http://msdn.microsoft.com/en-us/library/windows/desktop/bb775142%28v=vs.85%29.aspx is missing the CALLBACK, which led to mysterious crashes in wine
// the CALLBACK is verified from Microsoft's headers instead
// the headers also say int/void instead of INT/VOID but eh
INT CALLBACK groupLess(INT gn1, INT gn2, VOID *data)
{
	if (gn1 < 0 || gn1 >= ngroupnames)
		panic("group ID %d out of range in compare (max %d)", gn1, ngroupnames - 1);
	if (gn2 < 0 || gn2 >= ngroupnames)
		panic("group ID %d out of range in compare (max %d)", gn2, ngroupnames - 1);
	// ignore case; these are filenames
	// Microsoft says to use the _-prefixed functions (http://msdn.microsoft.com/en-us/library/ms235365.aspx); I wonder why these would be in the C++ standard... (TODO)
	return _wcsicmp(groupnames[gn1], groupnames[gn2]);
	// why not just get the group info each time? because we can't get the header length later, at least not as far as I know
}

void buildUI(HWND mainwin)
{
#define CSTYLE (WS_CHILD | WS_VISIBLE)
#define CXSTYLE (0)
#define SETFONT(hwnd) SendMessage(hwnd, WM_SETFONT, (WPARAM) controlfont, (LPARAM) TRUE);

	listview = CreateWindowEx(CXSTYLE,
		WC_LISTVIEW, L"",
		LVS_ICON | WS_VSCROLL | CSTYLE,
		0, 0, 100, 100,
		mainwin, NULL, hInstance, NULL);
	if (listview == NULL)
		panic("error creating list view");

	int itemid = 0;

	// we need to have an item to be able to add a group
	LVITEM dummy;

	ZeroMemory(&dummy, sizeof (LVITEM));
	dummy.mask = LVIF_TEXT;
	dummy.pszText = L"dummy";
	dummy.iItem = itemid++;
	if (SendMessage(listview, LVM_INSERTITEM,
		0, (LPARAM) &dummy) == (LRESULT) -1)
		panic("error adding dummy item to list view");
	// the dummy item has index 0

	HIMAGELIST largeicons;

	largeicons = ImageList_Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
		ILC_COLOR32, 100, 100);
	if (largeicons == NULL)
		panic("error creating icon list for list view");
	if (SendMessage(listview, LVM_SETIMAGELIST,
		LVSIL_NORMAL, (LPARAM) largeicons) == (LRESULT) NULL)
;//		panic("error giving icon list to list view");

	if (SendMessage(listview, LVM_ENABLEGROUPVIEW,
		(WPARAM) TRUE, (LPARAM) NULL) == (LRESULT) -1)
		panic("error enabling groups in list view");

	HANDLE dir;
	WIN32_FIND_DATA entry;
	PVOID wow64token;
	TCHAR finddir[MAX_PATH + 1];
	int groupid = 0;

	if (Wow64DisableWow64FsRedirection(&wow64token) == 0)
		panic("error disabling WOW64 pathname redirection");
	if (PathCombine(finddir, dirname, L"*") == NULL)		// TODO MSDN is unclear; see if this is documented as working correctly
		panic("error producing version of \"%S\" for FindFirstFile()", dirname);
	dir = FindFirstFile(finddir, &entry);
	if (dir == INVALID_HANDLE_VALUE) {
		DWORD e;

		e = GetLastError();
		if (e == ERROR_FILE_NOT_FOUND) {
			// TODO report to user
			printf("no files\n");
			exit(0);
		}
		SetLastError(e);		// for panic()
		panic("error opening \"%S\"", dirname);
	}
	for (;;) {
		TCHAR filename[MAX_PATH + 1];

		if (PathCombine(filename, dirname, entry.cFileName) == NULL)
			panic("error allocating combined filename \"%S\\%S\"",
				dirname, entry.cFileName);

		UINT nIcons;

		nIcons = (UINT) ExtractIcon(hInstance, filename, -1);
		if (nIcons != 0) {
			UINT i;
			HICON *large, *small;

			addGroup(listview, entry.cFileName, groupid);

			large = (HICON *) malloc(nIcons * sizeof (HICON));
			if (large == NULL)
				panic("error allocating array of large icon handles for \"%S\"", filename);
			if (ExtractIconEx(filename, 0, large, NULL, nIcons) != nIcons)
				panic("error extracting large icons from \"%S\"", filename);
			for (i = 0; i < nIcons; i++) {
				int index;
				LVITEM item;

				index = ImageList_AddIcon(largeicons, large[i]);
				if (index == -1)
					panic("error adding icon %u from \"%S\" to large icon list", i, filename);
				ZeroMemory(&item, sizeof (LVITEM));
				item.mask = LVIF_IMAGE | LVIF_GROUPID | LVIF_TEXT;
				item.iImage = index;
				item.iGroupId = groupid;
				char *q;
				asprintf(&q, "%d", itemid);
				item.pszText = toWideString(q);
				free(q);
				item.iItem = itemid++;
				if (SendMessage(listview, LVM_INSERTITEM,
					(WPARAM) -1, (LPARAM) &item) == (LRESULT) -1)
					panic("error adding icon %u from %S to list view", i, entry.cFileName);
			}
			free(large);

			groupid++;
		}

		if (FindNextFile(dir, &entry) == 0) {
			DWORD e;

			e = GetLastError();
			if (e == ERROR_NO_MORE_FILES)		// we're done
				break;
			SetLastError(e);		// for panic()
			panic("error getting next filename in \"%S\"", dirname);
		}
	}
	if (FindClose(dir) == 0)
		panic("error closing \"%S\"", dirname);
	if (Wow64RevertWow64FsRedirection(wow64token) == FALSE)
		panic("error re-enabling WOW64 pathname redirection");

	// and we're done with the dummy item
	if (SendMessage(listview, LVM_DELETEITEM, 0, 0) == FALSE)
		panic("error removing dummy item from list view");

	// now sort the groups in alphabetical order
	if (SendMessage(listview, LVM_SORTGROUPS,
		(WPARAM) groupLess, (LPARAM) NULL) == 0)
		panic("error sorting icon groups by filename");

	// and now some extended styles
	// the mask (WPARAM) defines which bits of the value (LPARAM) are to be changed
	// all other bits are ignored
	// so to set just the ones we specify, keeping any other styles intact, set both to the same value
	// MSDN says this returns the previous styles, with no mention of an error condition
#define xstyle 0	// TODO LVS_EX_BORDERSELECT?
	if (xstyle != 0)
		SendMessage(listview, LVM_SETEXTENDEDLISTVIEWSTYLE,
			xstyle, xstyle);
}

void firstShowWindow(HWND hwnd);
void initwin(void);

int CALLBACK WinMain(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND mainwin;
	MSG msg;

	init();
	hInstance = _hInstance;
	initwin();

	mainwin = makeMainWindow();
	buildUI(mainwin);
	ShowWindow(mainwin, nCmdShow);
	if (UpdateWindow(mainwin) == 0)
		panic("UpdateWindow(mainwin) failed in first show");

	for (;;) {
		BOOL gmret;

		gmret = GetMessage(&msg, NULL, 0, 0);
		if (gmret == -1)
			panic("error getting message");
		if (gmret == 0)
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

DWORD iccFlags =
//	ICC_ANIMATE_CLASS |			// animation control
//	ICC_BAR_CLASSES |				// toolbar, statusbar, trackbar, tooltip
//	ICC_COOL_CLASSES |			// rebar
//	ICC_DATE_CLASSES |			// date and time picker
//	ICC_HOTKEY_CLASS |			// hot key
//	ICC_INTERNET_CLASSES |		// IP address entry field
//	ICC_LINK_CLASS |				// hyperlink
	ICC_LISTVIEW_CLASSES |			// list-view, header
//	ICC_NATIVEFNTCTL_CLASS |		// native font
//	ICC_PAGESCROLLER_CLASS |		// pager
//	ICC_PROGRESS_CLASS |			// progress bar
//	ICC_STANDARD_CLASSES |		// "one of the intrinsic User32 control classes"
//	ICC_TAB_CLASSES |				// tab, tooltip
//	ICC_TREEVIEW_CLASSES |		// tree-view, tooltip
//	ICC_UPDOWN_CLASS |			// up-down
//	ICC_USEREX_CLASSES |			// ComboBoxEx
//	ICC_WIN95_CLASSES |			// some of the above
	0;

void initwin(void)
{
	INITCOMMONCONTROLSEX icc;
	NONCLIENTMETRICS ncm;

	hDefaultIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
	if (hDefaultIcon == NULL)
		panic("error getting default window class icon");
	hDefaultCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW));
	if (hDefaultCursor == NULL)
		panic("error getting default window cursor");
	icc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	icc.dwICC = iccFlags;
	if (InitCommonControlsEx(&icc) == FALSE)
		panic("error initializing Common Controls");
	ncm.cbSize = sizeof (NONCLIENTMETRICS);
	if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
		sizeof (NONCLIENTMETRICS), &ncm, 0) == 0)
		panic("error getting non-client metrics for getting control font");
	controlfont = CreateFontIndirect(&ncm.lfMessageFont);
	if (controlfont == NULL)
		panic("error getting control font");
}

void panic(char *fmt, ...)
{
	char *msg;
	TCHAR *lerrmsg;
	char *fullmsg;
	va_list arg;
	DWORD lasterr;
	DWORD lerrsuccess;

	lasterr = GetLastError();
	va_start(arg, fmt);
	if (vasprintf(&msg, fmt, arg) == -1) {
		fprintf(stderr, "critical error: vasprintf() failed in panic() preparing panic message; fmt = \"%s\"\n", fmt);
		abort();
	}
	// according to http://msdn.microsoft.com/en-us/library/windows/desktop/ms680582%28v=vs.85%29.aspx
	lerrsuccess = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, lasterr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lerrmsg, 0, NULL);
	if (lerrsuccess == 0) {
		fprintf(stderr, "critical error: FormatMessage() failed in panic() preparing GetLastError() string; panic message = \"%s\", last error in panic(): %ld, last error from FormatMessage(): %ld\n", msg, lasterr, GetLastError());
		abort();
	}
	// note to self: use %ws instead of %S (thanks jon_y in irc.oftc.net/#mingw-w64)
	if (asprintf(&fullmsg, "panic: %s\nlast error: %ws\n", msg, lerrmsg) == -1) {
		fprintf(stderr, "critical error: asprintf() failed in panic() preparing full report; panic message = \"%s\", last error message: \"%ws\"\n", msg, lerrmsg);
		abort();
	}
	fprintf(stderr, "%s\n", fullmsg);
	MessageBoxA(NULL,		// TODO both the function name and the NULL
		fullmsg,
		"PANIC",			// TODO
		MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
	// TODO find a reasonable way to handle failures in the MessageBox() call?
	va_end(arg);
	exit(1);
}

TCHAR *toWideString(char *what)
{
	TCHAR *buf;
	int n;
	size_t len;

	len = strlen(what);
	if (len == 0) {
		buf = (TCHAR *) malloc(sizeof (TCHAR));
		if (buf == NULL)
			goto mallocfail;
		buf[0] = L'\0';
	} else {
		n = MultiByteToWideChar(CP_UTF8, 0, what, -1, NULL, 0);
		if (n == 0)
			panic("error getting number of bytes to convert \"%s\" to UTF-16", what);
		buf = (TCHAR *) malloc((n + 1) * sizeof (TCHAR));
		if (buf == NULL)
			goto mallocfail;
		if (MultiByteToWideChar(CP_UTF8, 0, what, -1, buf, n) == 0)
			panic("erorr converting \"%s\" to UTF-16", what);
	}
	return buf;
mallocfail:
	panic("error allocating memory for UTF-16 version of \"%s\"", what);
}
