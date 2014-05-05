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
		dirname = argv[1];
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
			// static so we can save it without doing /another/ string copy
			static TCHAR path[(4 * MAX_PATH) + 1];

			path[0] = L'\0';			// TODO I forget why this was needed
			// TODO resolve shortcuts
			if (SHGetPathFromIDList(pidl, path) == FALSE)
				panic("error extracting folder from PIDL from folder dialog");
			dirname = path;
			CoTaskMemFree(pidl);
		} else
			printf("user aborted selection\n");
		CoUninitialize();
		if (dirname == NULL)		// don't quit if a directory was selected
			exit(0);
	}
	return;

usage:
	fprintf(stderr,  "usage: %S\n\t%S pathname\n\t%S --help\n",
		argv[0], argv[0], argv[0]);
	exit(usageret);
}

HWND mainwin;

#define ID_LISTVIEW 100

HCURSOR currentCursor;

HWND progressbar;

LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	NMHDR *nmhdr;

	switch (msg) {
	case msgBegin:
		currentCursor = LoadCursor(NULL, IDC_WAIT);
		if (currentCursor == NULL)
			panic("error loading busy cursor");
		progressbar = CreateWindowEx(0,
			PROGRESS_CLASS, L"",
			PBS_SMOOTH | WS_CHILD | WS_VISIBLE,
			20, 20, 200, 40,
			mainwin, NULL, hInstance, NULL);
		if (progressbar == NULL)
			panic("error making progressbar");
		SendMessage(progressbar, PBM_SETRANGE32,
			0, lparam);
		SendMessage(progressbar, PBM_SETSTEP, 1, 0);
		return 0;
	case msgStep:
		SendMessage(progressbar, PBM_STEPIT, 0, 0);
		return 0;
	case msgEnd:
		// kill redraw because this is a heavy operation
		SendMessage(mainwin, WM_SETREDRAW, (WPARAM) FALSE, 0);
		if (DestroyWindow(progressbar) == 0)
			panic("error removing progressbar");
		makeListView(mainwin, (HMENU) ID_LISTVIEW);
		currentCursor = hDefaultCursor;
		resizeListView(mainwin);
		SendMessage(mainwin, WM_SETREDRAW, (WPARAM) TRUE, 0);
		RedrawWindow(mainwin, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);		// MSDN says to
		// TODO set focus on the listview so I can use the scroll wheel
		// while I'm here, TODO figure out why the icons now have a black border around them
		return 0;
	case WM_SETCURSOR:
		SetCursor(currentCursor);
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
		resizeListView(mainwin);
		return 0;
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	panic("oops: message %ud does not return anything; bug in wndproc()", msg);
}

void makeMainWindow(void)
{
	WNDCLASS cls;

	ZeroMemory(&cls, sizeof (WNDCLASS));
	cls.lpszClassName = L"mainwin";
	cls.lpfnWndProc = wndproc;
	cls.hInstance = hInstance;
	cls.hIcon = hDefaultIcon;
	cls.hCursor = hDefaultCursor;
	cls.hbrBackground = (HBRUSH) (COLOR_BTNFACE + 1);
	if (RegisterClass(&cls) == 0)
		panic("error registering window class");
	mainwin = CreateWindowEx(0,
		L"mainwin", L"Main Window",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);
	if (mainwin == NULL)
		panic("opening main window failed");
}

void initwin(void);

int CALLBACK WinMain(HINSTANCE _hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	struct giThreadData data;

	init();
	hInstance = _hInstance;
	initwin();
	currentCursor = hDefaultCursor;

	makeMainWindow();
	data.mainwin = mainwin;
	data.dirname = dirname;
	if (CreateThread(NULL, 0, getIcons, &data, 0, NULL) == NULL)
		panic("error creating worker thread to get icons");
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
	ICC_PROGRESS_CLASS |			// progress bar
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
	hDefaultCursor = LoadCursor(NULL, IDC_ARROW);
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
