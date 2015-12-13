// 13 december 2015
#include "winiconview.h"

// explicitly initialize to NULL for panic()
HWND mainwin = NULL;

#define mainwinClass L"winiconview_mainwin"

struct mainwinData {
	int dummy;
};

static void onCreate(HWND hwnd)
{
	struct mainwinData *d;

	d = (struct mainwinData *) malloc(sizeof (struct mainwinData));
	if (d == NULL)
		panic(L"Error allocating internal data structures for the main window");
	ZeroMemory(d, sizeof (struct mainwinData));
	SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) d);
}

static LRESULT CALLBACK mainwinWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	struct mainwinData *d;

	d = (struct mainwinData *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	if (d == NULL) {
		if (uMsg == WM_CREATE)
			onCreate(hwnd);
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}

	switch (uMsg) {
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

void initMainWindow(void)
{
	WNDCLASSW wc;
	RECT r;

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
}

// TODO report errors?
void uninitMainWindow(void)
{
	BOOL destroyRet;

	destroyRet = DestroyWindow(mainwin);
	// this part is important; otherwise panic() will try to MessageBoxW() on a destroyed window
	mainwin = NULL;
	if (destroyRet == 0)
		panic(L"Error destroying main winow");
	if (UnregisterClassW(mainwinClass, hInstance) == 0)
		panic(L"Error unregistering the main window class");
}
