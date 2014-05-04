// 4 may 2014
#include "winiconview.h"

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

static BOOL wow64loaded = FALSE;
static BOOL (WINAPI *wow64disable)(PVOID *);
static BOOL (WINAPI *wow64enable)(PVOID);

void ourWow64DisableWow64FsRedirection(PVOID *token)
{
	if (!wow64loaded) {
		BOOL b;

		// the functions seem to fail with a last error ERROR_INVALID_FUNCTION when run as a 64-bit process, so don't load them then
		if (IsWow64Process(GetCurrentProcess(), &b) == 0)
			panic("IsWow64Process() failed during Wow64DisableWow64FsRedirection()/Wow64RevertWow64FsRedirection() dynamic loading");
		if (b) {		// on 32-bit, so try
			HMODULE kernel32;

			kernel32 = GetModuleHandle(L"kernel32.dll");
			if (kernel32 == NULL)
				panic("error loading kernel32.dll for Wow64DisableWow64FsRedirection()/Wow64RevertWow64FsRedirection() dynamic loading");
			// GetProcAddress() seems to always take non-Unicode strings... and the WINAPI is in the (*) parentheses... why for both? TODO
			wow64disable = (BOOL (WINAPI *)(PVOID *)) GetProcAddress(kernel32, "Wow64DisableWow64FsRedirection");
			wow64enable = (BOOL (WINAPI *)(PVOID)) GetProcAddress(kernel32, "Wow64RevertWow64FsRedirection");
		}
		// otherwise keep them no-ops so we can just skip them
		wow64loaded = TRUE;
	}
	if (wow64disable == NULL)
		return;
	if ((*wow64disable)(token) == 0)
		panic("error disabling WOW64 pathname redirection");
}

void ourWow64RevertWow64FsRedirection(PVOID token)
{
	if (wow64enable == NULL)
		return;
	if ((*wow64enable)(token) == FALSE)
		panic("error re-enabling WOW64 pathname redirection");
}
