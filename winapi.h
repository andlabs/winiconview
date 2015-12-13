// 13 december 2015
#define _UNICODE
#define UNICODE
#define STRICT
#define STRICT_TYPED_ITEMIDS
// get Windows version right; right now Windows XP
#define WINVER 0x0501
#define _WIN32_WINNT 0x0501
#define _WIN32_WINDOWS 0x0501		/* according to Microsoft's winperf.h */
#define _WIN32_IE 0x0600			/* according to Microsoft's sdkddkver.h */
#define NTDDI_VERSION 0x05010000	/* according to Microsoft's sdkddkver.h */
#include <windows.h>
// Microsoft's resource compiler will segfault if we feed it headers it was not designed to handle
#ifndef RC_INVOKED
#include <commctrl.h>			// needed for InitCommonControlsEx() (thanks Xeek in irc.freenode.net/#winapi for confirming)
#include <shlobj.h>
#include <shlwapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#endif
