// 13 december 2015
#include "winiconview.h"

struct findFile {
	HANDLE dir;
	WIN32_FIND_DATAW entry;
	BOOL first;
	BOOL done;
	HRESULT hr;
};

static BOOL ffReallyNoMoreFiles(struct findFile *ff, DWORD nmferr)
{
	DWORD lasterr;

	lasterr = GetLastError();
	if (lasterr == nmferr)
		return TRUE;
	// nope, an actual error occurred
	ff->hr = HRESULT_FROM_WIN32(lasterr);
	if (ff->hr == S_OK)		// in case lasterr was 0
		ff->hr = E_FAIL;
	return FALSE;
}

struct findFile *startFindFile(WCHAR *path)
{
	struct findFile *ff;
	WCHAR finddir[MAX_PATH + 1];

	if (PathCombineW(finddir, path, L"*") == NULL)		// TODO MSDN is unclear; see if this is documented as working correctly
		// TODO return actual error
		return NULL;
	ff = (struct findFile *) malloc(sizeof (struct findFile));
	if (ff == NULL)
		return NULL;
	ZeroMemory(ff, sizeof (struct findFile));
	// get the first file now
	// findFileNext() will see that ff->first is TRUE and return immediately; we get that first file's info then
	ff->first = TRUE;
	ff->dir = FindFirstFileW(finddir, &(ff->entry));
	if (ff->dir == INVALID_HANDLE_VALUE)
		if (ffReallyNoMoreFiles(ff, ERROR_FILE_NOT_FOUND))
			ff->done = TRUE;
	return ff;
}

BOOL findFileNext(struct findFile *ff)
{
	if (ff->hr != S_OK)		// don't continue on error
		return FALSE;
	if (ff->done)			// no more files
		return FALSE;
	if (ff->first) {			// consume first file
		ff->first = FALSE;
		return TRUE;
	}
	if (FindNextFileW(ff->dir, &(ff->entry)) == 0) {
		if (ffReallyNoMoreFiles(ff, ERROR_NO_MORE_FILES))
			ff->done = TRUE;
		return FALSE;
	}
	return TRUE;
}

WCHAR *findFileName(struct findFile *ff)
{
	return ff->entry.cFileName;
}

HRESULT findFileError(struct findFile *ff)
{
	return ff->hr;
}

HRESULT findFileEnd(struct findFile *ff)
{
	DWORD lasterr;
	HRESULT hr;

	hr = S_OK;
	if (FindClose(ff->dir) == 0) {
		lasterr = GetLastError();
		hr = HRESULT_FROM_WIN32(lasterr);
		if (hr == S_OK)
			hr = E_FAIL;
	}
	// always free ff, even on error
	// sure this leaks ff->dir but the cause was already lost
	free(ff);
	return hr;
}
