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
	ff->hr = lasterrToHRESULT(lasterr);
	return FALSE;
}

// returns:
// - S_OK if there are files
// - S_FALSE if there are none
// - an error code otherwise
HRESULT startFindFile(WCHAR *path, struct findFile **out)
{
	struct findFile *ff;
	WCHAR *finddir;
	HRESULT hr;

	// initialize output parameters
	*out = NULL;

	ff = (struct findFile *) malloc(sizeof (struct findFile));
	if (ff == NULL)
		return E_OUTOFMEMORY;
	ZeroMemory(ff, sizeof (struct findFile));

	hr = pathJoin(path, L"*", &finddir);
	if (hr != S_OK) {
		free(ff);
		return hr;
	}

	// get the first file now
	// findFileNext() will see that ff->first is TRUE and return immediately; we can get that first file's info then
	ff->first = TRUE;
	ff->dir = FindFirstFileW(finddir, &(ff->entry));
	if (ff->dir == INVALID_HANDLE_VALUE)
		if (ffReallyNoMoreFiles(ff, ERROR_FILE_NOT_FOUND)) {
			// no files
			pathFree(finddir);
			free(ff);
			return S_FALSE;
		}

	pathFree(finddir);
	*out = ff;
	return S_OK;
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

void findFileEnd(struct findFile *ff)
{
	// we have to keep going even if this fails
	// TODO log an error
	FindClose(ff->dir);
	free(ff);
}
