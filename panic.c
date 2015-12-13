// 13 december 2015
#include "winiconview.h"

static WCHAR *panicformat(WCHAR *format, ...)
{
	va_list ap;
	WCHAR *msg;

	va_start(ap, format);
	if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING, format, 0, 0, (LPWSTR) (&msg), 0, &ap) == 0)
		abort();
	va_end(ap);
	return msg;
}

static void realpanic(const WCHAR *msg, DWORD code, const WCHAR *codestr)
{
	WCHAR *sysmsg;
	int hasSysMsg;
	WCHAR *errmsg;

	hasSysMsg = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, code, 0, (LPWSTR) (&sysmsg), 0, NULL) != 0;
	if (hasSysMsg)
		errmsg = panicformat(L"%1!ws!: %2!ws! (%3!ws!)%0", errmsg, sysmsg, codestr);
	else
		errmsg = panicformat(L"%1!ws! (%2!ws!)%0", errmsg, codestr);
	if (MessageBoxW(mainwin, errmsg, programName, MB_OK | MB_ICONERROR) == 0)
		abort();
	if (LocalFree(errmsg) == NULL)
		abort();
	if (hasSysMsg)
		if (LocalFree(sysmsg) == NULL)
			abort();
	exit(EXIT_FAILURE);
}

void panic(const WCHAR *msg)
{
	DWORD lasterr;
	WCHAR errcode[64];

	lasterr = GetLastError();
	swnprintf(errcode, 64, L"last error %I32u", lasterr);
}

void panichr(const WCHAR *msg, HRESULT hr)
{
	WCHAR errcode[64];

	swprintf(errcode, 64, L"HRESULT 0x%08I32X", hr);
	realpanic(msg, (DWORD) hr, errcode);
}
