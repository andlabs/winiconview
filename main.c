// 13 december 2015
#include "winiconview.h"

HINSTANCE hInstance;
int nCmdShow;

static void init(HINSTANCE winmainhInstance, int winmainnCmdShow)
{
	HRESULT hr;

	hInstance = winmainhInstance;
	nCmdShow = winmainnCmdShow;

	hr = CoInitialize(NULL);
	if (hr != S_OK && hr != S_FALSE)
		panichr(L"Error initializing COM", hr);
}

static void uninit(void)
{
	CoUninitialize();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	BOOL gmret;

	init(hInstance, nCmdShow);
	initMainWindow();

	for (;;) {
		gmret = GetMessageW(&msg, NULL, 0, 0);
		if (gmret == -1)
			panic(L"error getting message");
		if (gmret == 0)
			break;
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	uninitMainWindow();
	uninit();
	return 0;
}
