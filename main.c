// pietro gagliardi 1 may 2014
// scratch Windows program by pietro gagliardi 17 april 2014
// fixed typos 1 may 2014
// borrows code from the scratch GTK+ program (16-17 april 2014) and from code written 31 march 2014 and 11-12 april 2014
#include "winiconview.h"

// TODO prune the DLLs
// #qo LIBS: user32 kernel32 comctl32 comdlg32 gdi32 msimg32 shell32 advapi32 ole32 shlwapi

HINSTANCE hInstance;
int nCmdShow;

static HFONT controlfont;

static WCHAR *dirname = NULL;

static WCHAR *folderDialogHelpText =
	L"Choose a folder from the list below to see its icons. "
	PROGNAME L" won't enter subfolders; it'll only scan files in the chosen folder.";

static void parseArgs(void)
{
	int usageret = EXIT_FAILURE;
	int argc;
	WCHAR **argv;

	argv = CommandLineToArgvW(GetCommandLine(), &argc);
	if (argv == NULL)
		panic(L"error splitting command line into argc/argv form");
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
			panic(L"error initializing COM for browse for folders dialog");
		ZeroMemory(&bi, sizeof (BROWSEINFO));
		bi.lpszTitle = folderDialogHelpText;
		bi.ulFlags = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;
		pidl = SHBrowseForFolder(&bi);
		if (pidl != NULL) {
			// THIS WILL CUT OFF. BIG TODO.
			// static so we can save it without doing /another/ string copy
			static WCHAR path[(4 * MAX_PATH) + 1];

			path[0] = L'\0';			// TODO I forget why this was needed
			// TODO resolve shortcuts
			if (SHGetPathFromIDList(pidl, path) == FALSE)
				panic(L"error extracting folder from PIDL from folder dialog");
			dirname = path;
			CoTaskMemFree(pidl);
		}
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

static void initSharedWindowsStuff(HINSTANCE winmainhInstance, int winmainnCmdShow)
{
	INITCOMMONCONTROLSEX icc;
	NONCLIENTMETRICS ncm;

	hInstance = winmainhInstance;
	nCmdShow = winmainnCmdShow;
	icc.dwSize = sizeof (INITCOMMONCONTROLSEX);
	icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_PROGRESS_CLASS;
	if (InitCommonControlsEx(&icc) == FALSE)
		panic(L"error initializing Common Controls");
	ncm.cbSize = sizeof (NONCLIENTMETRICS);
	if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
		sizeof (NONCLIENTMETRICS), &ncm, 0) == 0)
		panic(L"error getting non-client metrics for getting control font");
	controlfont = CreateFontIndirect(&ncm.lfMessageFont);
	if (controlfont == NULL)
		panic(L"error getting control font");
}

void setControlFont(HWND hwnd)
{
	SendMessage(hwnd, WM_SETFONT, (WPARAM) controlfont, (LPARAM) TRUE);
}

HFONT selectControlFont(HDC dc)
{
	HFONT prev;

	prev = (HFONT) SelectObject(dc, controlfont);
	if (prev == NULL)
		panic(L"error selecting control font into DC");
	return prev;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND mainwin;
	MSG msg;

	parseArgs();
	initSharedWindowsStuff(hInstance, nCmdShow);
	initGetIcons();
	registerMainWindowClass();

	mainwin = makeMainWindow(dirname);

	for (;;) {
		BOOL gmret;

		gmret = GetMessage(&msg, NULL, 0, 0);
		if (gmret == -1)
			panic(L"error getting message");
		if (gmret == 0)
			break;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}
