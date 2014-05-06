// 4 may 2014
#include "winiconview.h"

void panic(TCHAR *fmt, ...)
{
	TCHAR *msg;
	TCHAR *lerrmsg;
	TCHAR *fullmsg;
	va_list arg;
	DWORD lasterr;
	DWORD lerrsuccess;

	lasterr = GetLastError();
	va_start(arg, fmt);
	msg = ourvawsprintf(fmt, arg);
	if (msg == NULL) {
		fprintf(stderr, "critical error: ourvawsprintf() failed in panic() preparing panic message; fmt = \"%S\"\n", fmt);
		abort();
	}
	// according to http://msdn.microsoft.com/en-us/library/windows/desktop/ms680582%28v=vs.85%29.aspx
	lerrsuccess = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, lasterr,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lerrmsg, 0, NULL);
	if (lerrsuccess == 0) {
		fprintf(stderr, "critical error: FormatMessage() failed in panic() preparing GetLastError() string; panic message = \"%S\", last error in panic(): %ld, last error from FormatMessage(): %ld\n", msg, lasterr, GetLastError());
		abort();
	}
	fullmsg = ourawsprintf(L"panic: %s\nlast error: %s\n", msg, lerrmsg);
	if (fullmsg == NULL) {
		fprintf(stderr, "critical error: ourawsprintf() failed in panic() preparing full report; panic message = \"%S\", last error message: \"%S\"\n", msg, lerrmsg);
		abort();
	}
	fprintf(stderr, "%S\n", fullmsg);
	MessageBox(NULL,
		fullmsg,
		L"PANIC",			// TODO
		MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
	// TODO find a reasonable way to handle failures in the MessageBox() call?
	va_end(arg);
	exit(1);
}

TCHAR *ourawsprintf(TCHAR *fmt, ...)
{
	TCHAR *out;
	va_list arg;

	va_start(arg, fmt);
	out = ourvawsprintf(fmt, arg);
	va_end(arg);
	return out;
}

// don't call panic() here because panic() calls this!
TCHAR *ourvawsprintf(TCHAR *fmt, va_list arg)
{
	int n;
	TCHAR *out;
	va_list carg;

	va_copy(carg, arg);
	n = _vscwprintf(fmt, carg);
	va_end(carg);
	if (n == -1)
		return NULL;
	out = (TCHAR *) malloc((n + 1) * sizeof (TCHAR));
	if (out == NULL)
		return NULL;
	// BIG TODO: if Application Verifier is patrolling this process, the _vsnwprintf() call will CRASH and I have no idea why :S
	if (_vsnwprintf(out, (size_t) n, fmt, arg) == -1)
		return NULL;
	// TODO apparently the terminating null ISN'T written by the above?!
	out[n] = L'\0';
	return out;
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
			panic(L"error getting number of bytes to convert \"%S\" to UTF-16", what);
		buf = (TCHAR *) malloc((n + 1) * sizeof (TCHAR));
		if (buf == NULL)
			goto mallocfail;
		if (MultiByteToWideChar(CP_UTF8, 0, what, -1, buf, n) == 0)
			panic(L"error converting \"%S\" to UTF-16", what);
	}
	return buf;
mallocfail:
	panic(L"error allocating memory for UTF-16 version of \"%S\"", what);
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
			panic(L"IsWow64Process() failed during Wow64DisableWow64FsRedirection()/Wow64RevertWow64FsRedirection() dynamic loading");
		if (b) {		// on 32-bit, so try
			HMODULE kernel32;

			kernel32 = GetModuleHandle(L"kernel32.dll");
			if (kernel32 == NULL)
				panic(L"error loading kernel32.dll for Wow64DisableWow64FsRedirection()/Wow64RevertWow64FsRedirection() dynamic loading");
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
		panic(L"error disabling WOW64 pathname redirection");
}

void ourWow64RevertWow64FsRedirection(PVOID token)
{
	if (wow64enable == NULL)
		return;
	if ((*wow64enable)(token) == FALSE)
		panic(L"error re-enabling WOW64 pathname redirection");
}
