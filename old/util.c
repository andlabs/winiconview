// 4 may 2014
#include "winiconview.h"

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
