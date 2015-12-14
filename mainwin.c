// 13 december 2015
#include "winiconview.h"

#define mainwinClass L"winiconview_mainwin"

struct mainwinData {
	HWND hwnd;
	HMENU menu;
	HWND treeview;
	HWND listview;
	int currentSize;		// 0 = large, 1 = small
	IProgressDialog *pd;
};

static void relayoutControls(struct mainwinData *d)
{
	RECT r;
	LONG treewidth;

	if (GetClientRect(d->hwnd, &r) == 0)
		panic(L"Error getting client rect of main window for relayout");
	treewidth = (r.right - r.left) / 3;
	if (SetWindowPos(d->treeview, NULL,
		0, 0, treewidth, r.bottom - r.top,
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER) == 0)
		panic(L"Error repositioning main window treeview for relayout");
	if (SetWindowPos(d->listview, NULL,
		treewidth, 0, r.right - r.left - treewidth, r.bottom - r.top,
		SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER) == 0)
		panic(L"Error repositioning main window listview for relayout");
}

static void onCreate(HWND hwnd)
{
	struct mainwinData *d;

	d = (struct mainwinData *) malloc(sizeof (struct mainwinData));
	if (d == NULL)
		panic(L"Error allocating internal data structures for the main window");
	ZeroMemory(d, sizeof (struct mainwinData));
	SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) d);

	d->hwnd = hwnd;

	d->menu = GetMenu(d->hwnd);
	if (d->menu == NULL)
		panic(L"Error getting main window menu for later use");

	d->treeview = CreateWindowExW(WS_EX_CLIENTEDGE,
		WC_TREEVIEWW, L"",
		TVS_DISABLEDRAGDROP | TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
		0, 0, 100, 100,
		d->hwnd, (HMENU) 100, hInstance, NULL);
	if (d->treeview == NULL)
		panic(L"Error creating main window treeview");

	d->listview = CreateWindowExW(WS_EX_CLIENTEDGE,
		WC_LISTVIEWW, L"",
		LVS_ICON | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
		0, 0, 100, 100,
		d->hwnd, (HMENU) 101, hInstance, NULL);
	if (d->listview == NULL)
		panic(L"Error creating main window listview");

	relayoutControls(d);
}

static void addIcons(struct mainwinData *d, WCHAR *dir)
{
	struct getIconsParams p;
	HRESULT hr;

	ZeroMemory(&p, sizeof (struct getIconsParams));
	p.parent = d->hwnd;
	p.dir = dir;
	hr = getIcons(&p);
	if (hr == S_FALSE)		// user cancelled; do nothing
		return;
	if (hr != S_OK) {
		// TODO
		MessageBoxW(d->hwnd, p.errmsg, L"It failed", 0);
		return;
	}
	if (p.entries == NULL) {
		// TODO check error
		MessageBoxW(d->hwnd,
			L"No files with icons were found in the chosen directory.",
			programName,
			MB_OK | MB_ICONINFORMATION);
		return;
	}
	appendFolder(d->treeview, dir, p.entries);
}

static void chooseFolder(struct mainwinData *d)
{
	BROWSEINFOW bi;
	PIDLIST_ABSOLUTE pidl;
	// THIS WILL CUT OFF. BIG TODO.
	WCHAR path[(4 * MAX_PATH) + 1];

	ZeroMemory(&bi, sizeof (BROWSEINFOW));
	bi.hwndOwner = d->hwnd;
	bi.lpszTitle = L"Select a folder below. Note that " programName L" does not search subdirectories of the chosen folder.";
	bi.ulFlags = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;
	pidl = SHBrowseForFolderW(&bi);
	if (pidl == NULL)		// cancelled
		return;

	path[0] = L'\0';			// TODO I forget why this was needed
	// TODO resolve shortcuts
	if (SHGetPathFromIDListW(pidl, path) == FALSE)
		panic(L"Error extracting folder path from PIDL from folder dialog");
	CoTaskMemFree(pidl);
	addIcons(d, path);
}

static void changeCurrentSize(struct mainwinData *d, int which)
{
	d->currentSize = which;

	// TODO

	if (CheckMenuRadioItem(d->menu,
		rcMenuLargeIcons, rcMenuSmallIcons,
		rcMenuLargeIcons + d->currentSize,
		MF_BYCOMMAND) == 0)
		panic(L"Error adjusting View menu radio buttons after changing icon size");
}

static LRESULT CALLBACK mainwinWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	struct mainwinData *d;
	WINDOWPOS *wp = (WINDOWPOS *) lParam;

	d = (struct mainwinData *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (d == NULL) {
		if (uMsg == WM_CREATE)
			onCreate(hwnd);
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
	case msgAddIcons:
		addIcons(d, (WCHAR *) wParam);
		return 0;
	case WM_COMMAND:
		if (HIWORD(wParam) != 0)
			break;
		switch (LOWORD(wParam)) {
		case rcMenuOpen:
			chooseFolder(d);
			break;
		case rcMenuQuit:
			PostQuitMessage(0);
			break;
		case rcMenuLargeIcons:
			changeCurrentSize(d, 0);
			break;
		case rcMenuSmallIcons:
			changeCurrentSize(d, 1);
			break;
		case rcMenuAbout:
			// TODO
			break;
		}
		break;
	case WM_WINDOWPOSCHANGED:
		if ((wp->flags & SWP_NOSIZE) != 0)
			break;
		relayoutControls(d);
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		// don't fall through to WM_DESTROY; let main() do that
		return 0;
	case WM_DESTROY:
		free(d);
		// prevent any future messages from getting bad state
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
		break;
	}
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

HWND initMainWindow(void)
{
	WNDCLASSW wc;
	RECT r;
	HWND mainwin;

	ZeroMemory(&wc, sizeof (WNDCLASSW));
	wc.lpszClassName = mainwinClass;
	wc.lpfnWndProc = mainwinWndProc;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIconW(NULL, IDI_APPLICATION);
	if (wc.hIcon == NULL)
		panic(L"Error loading application icon during main window creation");
	wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
	if (wc.hCursor == NULL)
		panic(L"Error loading application cursor during main window creation");
	wc.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	wc.lpszMenuName = MAKEINTRESOURCE(rcMenu);
	if (RegisterClassW(&wc) == 0)
		panic(L"Error registering main window window class");

	r.left = 0;
	r.top = 0;
	r.right = 320;
	r.bottom = 240;
	if (AdjustWindowRectEx(&r, WS_OVERLAPPEDWINDOW, TRUE, 0) == 0)
		panic(L"Error getting the appropriate size for the main window");
	mainwin = CreateWindowExW(0,
		mainwinClass, programName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		r.right - r.left, r.bottom - r.top,
		NULL, NULL, hInstance, NULL);
	if (mainwin == NULL)
		panic(L"Error creating main window");

	ShowWindow(mainwin, nCmdShow);
	if (UpdateWindow(mainwin) == 0)
		panic(L"Error showing the main window for the first time");

	panicParent = mainwin;
	return mainwin;
}

// TODO report errors?
void uninitMainWindow(HWND mainwin)
{
	BOOL destroyRet;

	destroyRet = DestroyWindow(mainwin);
	if (destroyRet == 0)
		panic(L"Error destroying main winow");
	if (UnregisterClassW(mainwinClass, hInstance) == 0)
		panic(L"Error unregistering the main window class");
}
