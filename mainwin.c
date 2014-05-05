// 5 may 2014
#include "winiconview.h"

static HICON hDefaultIcon;
static HCURSOR arrowCursor, waitCursor;

#define ID_LISTVIEW 100

struct mainwinData {
	HCURSOR currentCursor;
	HWND label;
	HWND progressbar;
	HWND listview;
};

static LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	struct mainwinData *data;
	NMHDR *nmhdr;

	// we can assume the GetWindowLongPtr()/SetWindowLongPtr() calls will work; see comments of http://blogs.msdn.com/b/oldnewthing/archive/2014/02/03/10496248.aspx
	data = (struct mainwinData *) GetWindowLongPtr(hwnd, GWL_USERDATA);
	if (data == NULL) {
		data = (struct mainwinData *) malloc(sizeof (struct mainwinData));
		if (data == NULL)
			panic("error allocating main window data structure");
		ZeroMemory(data, sizeof (struct mainwinData));
		data->currentCursor = arrowCursor;
		SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR) data);
	}

	switch (msg) {
	case msgBegin:
		data->currentCursor = waitCursor;
		data->label = CreateWindowEx(0,
			L"STATIC", L"Gathering icons. Please wait.",
			SS_NOPREFIX | SS_LEFTNOWORDWRAP | WS_CHILD | WS_VISIBLE,
			20, 20, 200, 20,
			hwnd, NULL, hInstance, NULL);
		if (data->label == NULL)
			panic("error making \"please wait\" label");
		setControlFont(data->label);
		data->progressbar = CreateWindowEx(0,
			PROGRESS_CLASS, L"",
			PBS_SMOOTH | WS_CHILD | WS_VISIBLE,
			20, 45, 200, 40,
			hwnd, NULL, hInstance, NULL);
		if (data->progressbar == NULL)
			panic("error making progressbar");
		SendMessage(data->progressbar, PBM_SETRANGE32,
			0, lparam);
		SendMessage(data->progressbar, PBM_SETSTEP, 1, 0);
		return 0;
	case msgStep:
		SendMessage(data->progressbar, PBM_STEPIT, 0, 0);
		return 0;
	case msgEnd:
		// kill redraw because this is a heavy operation
		SendMessage(hwnd, WM_SETREDRAW, (WPARAM) FALSE, 0);
		if (DestroyWindow(data->label) == 0)
			panic("error removing \"please wait\" label");
		if (DestroyWindow(data->progressbar) == 0)
			panic("error removing progressbar");
		makeListView(hwnd, (HMENU) ID_LISTVIEW);
		data->currentCursor = arrowCursor;		// TODO move to end and make atomic
		resizeListView(hwnd);
		SendMessage(hwnd, WM_SETREDRAW, (WPARAM) TRUE, 0);
		RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);		// MSDN says to
		// TODO set focus on the listview so I can use the scroll wheel
		// while I'm here, TODO figure out why the icons now have a black border around them
		return 0;
	case WM_SETCURSOR:
		SetCursor(data->currentCursor);
		return TRUE;
	case WM_NOTIFY:
		nmhdr = (NMHDR *) lparam;
		if (nmhdr->idFrom == ID_LISTVIEW)
			return handleListViewRightClick(nmhdr);
		return 0;
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		resizeListView(hwnd);
		return 0;
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	panic("oops: message %ud does not return anything; bug in wndproc()", msg);
}

void registerMainWindowClass(void)
{
	WNDCLASS cls;

	hDefaultIcon = LoadIcon(NULL, MAKEINTRESOURCE(IDI_APPLICATION));
	if (hDefaultIcon == NULL)
		panic("error getting default window class icon");
	arrowCursor = LoadCursor(NULL, IDC_ARROW);
	if (arrowCursor == NULL)
		panic("error getting arrow cursor");
	waitCursor = LoadCursor(NULL, IDC_WAIT);
	if (waitCursor == NULL)
		panic("error getting wait cursor");
	ZeroMemory(&cls, sizeof (WNDCLASS));
	cls.lpszClassName = L"mainwin";
	cls.lpfnWndProc = wndproc;
	cls.hInstance = hInstance;
	cls.hIcon = hDefaultIcon;
	cls.hCursor = arrowCursor;
	cls.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	if (RegisterClass(&cls) == 0)
		panic("error registering window class");
}

HWND makeMainWindow(void)
{
	HWND mainwin;

	mainwin = CreateWindowEx(0,
		L"mainwin", L"Main Window",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);
	if (mainwin == NULL)
		panic("opening main window failed");
	return mainwin;
}
