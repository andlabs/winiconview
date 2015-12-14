// pietro gagliardi 1 may 2014
// scratch Windows program by pietro gagliardi 17 april 2014
// fixed typos 1 may 2014
// borrows code from the scratch GTK+ program (16-17 april 2014) and from code written 31 march 2014 and 11-12 april 2014
#include "winiconview.h"

// TODO prune the DLLs

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
	} else {}
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

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	HWND mainwin;
	MSG msg;

	parseArgs();
	initSharedWindowsStuff(hInstance, nCmdShow);
	initGetIcons();
	registerMainWindowClass();

	mainwin = makeMainWindow(dirname);

}
