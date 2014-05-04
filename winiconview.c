// pietro gagliardi 1 may 2014
// scratch Windows program by pietro gagliardi 17 april 2014
// fixed typos and added toWideString() 1 may 2014
// borrows code from the scratch GTK+ program (16-17 april 2014) and from code written 31 march 2014 and 11-12 april 2014
#include "winiconview.h"

#ifdef  _MSC_VER
#error sorry! the scratch windows program relies on mingw-only functionality! (specifically: asprintf())
#endif

HMODULE hInstance;
HICON hDefaultIcon;
HCURSOR hDefaultCursor;
HFONT controlfont;

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
#define ID_LISTVIEW 100
HIMAGELIST iconlists[2];

LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	NMHDR *nmhdr;

	switch (msg) {
	case WM_NOTIFY:
		nmhdr = (NMHDR *) lparam;
		if (nmhdr->idFrom == ID_LISTVIEW) {
			int times = 1;

			// TODO needed?
			if (nmhdr->code == NM_RDBLCLK) {	// turn double-clicks into two single-clicks
				times = 2;
				nmhdr->code = NM_CLICK;
			}
			if (nmhdr->code == NM_RCLICK) {
				for (; times != 0; times--) {
					HIMAGELIST temp;

					printf("right click %d\n", times);		// TODO
					temp = iconlists[0];
					iconlists[0] = iconlists[1];
					iconlists[1] = temp;
					if (SendMessage(listview, LVM_SETIMAGELIST,
						LVSIL_NORMAL, (LPARAM) iconlists[0]) == (LRESULT) NULL)
						panic("error swapping list view icon lists");
				}
				return 1;
			}
		}
		return 0;
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

void buildUI(HWND mainwin)
{
#define CSTYLE (WS_CHILD | WS_VISIBLE)
#define CXSTYLE (0)
#define SETFONT(hwnd) SendMessage(hwnd, WM_SETFONT, (WPARAM) controlfont, (LPARAM) TRUE);

	listview = CreateWindowEx(CXSTYLE,
		WC_LISTVIEW, L"",
		LVS_ICON | WS_VSCROLL | CSTYLE,
		0, 0, 100, 100,
		mainwin, (HMENU) ID_LISTVIEW, hInstance, NULL);
	if (listview == NULL)
		panic("error creating list view");

	// we need to have an item to be able to add a group
	LVITEM dummy;

	ZeroMemory(&dummy, sizeof (LVITEM));
	dummy.mask = LVIF_TEXT;
	dummy.pszText = L"dummy";
	dummy.iItem = 0;
	if (SendMessage(listview, LVM_INSERTITEM,
		0, (LPARAM) &dummy) == (LRESULT) -1)
		panic("error adding dummy item to list view");
	// the dummy item has index 0

	if (SendMessage(listview, LVM_ENABLEGROUPVIEW,
		(WPARAM) TRUE, (LPARAM) NULL) == (LRESULT) -1)
		panic("error enabling groups in list view");

	getIcons();

	if (SendMessage(listview, LVM_SETIMAGELIST,
		LVSIL_NORMAL, (LPARAM) largeicons) == (LRESULT) NULL)
;//		panic("error giving large icon list to list view");
	iconlists[0] = largeicons;
	iconlists[1] = smallicons;

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
