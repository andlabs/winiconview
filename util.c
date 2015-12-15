// 15 december 2015
#include "winiconview.h"

HRESULT lasterrToHRESULT(DWORD lasterr)
{
	if (lasterr == 0)
		return E_FAIL;
	return HRESULT_FROM_WIN32(lasterr);
}

HRESULT pathJoin(WCHAR *a, WCHAR *b, WCHAR **ret)
{
	WCHAR *joined;
	DWORD lasterr;

	*ret = NULL;
	joined = (WCHAR *) malloc((MAX_PATH + 1) * sizeof (WCHAR));
	if (joined == NULL)
		return E_OUTOFMEMORY;
	result = PathCombineW(joined, a, b);
	if (result == NULL) {
		lasterr = GetLastError();
		free(joined);
		return lasterrToHRESULT(lasterr);
	}
	*ret = joined;
	return S_OK;
}

// this function is present in the event of me choosing to support long pathnames on Windows 8 (by dynamically loading PathAllocCombineW() and calling it)
void pathFree(WCHAR *path)
{
	free(path);
}
