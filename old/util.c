// 4 may 2014
#include "winiconview.h"

void panic(WCHAR *fmt, ...)
{
	WCHAR *msg;
	WCHAR *lerrmsg;
	WCHAR *fullmsg;
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
		PROGNAME,
		MB_OK | MB_ICONERROR | MB_SYSTEMMODAL);
	// TODO find a reasonable way to handle failures in the MessageBox() call?
	va_end(arg);
	exit(1);
}

WCHAR *ourawsprintf(WCHAR *fmt, ...)
{
	WCHAR *out;
	va_list arg;

	va_start(arg, fmt);
	out = ourvawsprintf(fmt, arg);
	va_end(arg);
	return out;
}

// HUGE TODO - VISUAL C++ 2010 DOESN'T PROVIDE VA_COPY AND THIS IS A **MAJOR HACK**
#ifndef va_copy
#define va_copy(d, s) ((d) = (s))
#endif

// don't call panic() here because panic() calls this!
WCHAR *ourvawsprintf(WCHAR *fmt, va_list arg)
{
	int n;
	WCHAR *out;
	va_list carg;

	va_copy(carg, arg);
	n = _vscwprintf(fmt, carg);
	va_end(carg);
	if (n == -1)
		return NULL;
	out = (WCHAR *) malloc((n + 1) * sizeof (WCHAR));
	if (out == NULL)
		return NULL;
	// BIG TODO: if Application Verifier is patrolling this process, the _vsnwprintf() call will CRASH and I have no idea why :S
	if (_vsnwprintf(out, (size_t) n, fmt, arg) == -1)
		return NULL;
	// TODO apparently the terminating null ISN'T written by the above?!
	out[n] = L'\0';
	return out;
}
