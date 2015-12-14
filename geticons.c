// 13 december 2015
#include "winiconview.h"

// TODO restructure this to not need threads
// turns out because we're using IProgressDialog we don't need a background thread :D

struct entry *allocEntry(struct entry *prev, WCHAR *filename)
{
	struct entry *e;

	e = (struct entry *) malloc(sizeof (struct entry));
	if (e == NULL)
		return NULL;
	ZeroMemory(e, sizeof (struct entry));
	e->filename = _wcsdup(filename);
	if (e->filename == NULL) {
		free(e);
		return NULL;
	}
	if (prev != NULL)
		prev->next = e;
	return e;
}

void freeEntries(struct entry *cur)
{
	struct entry *next;

	while (cur != NULL) {
		next = cur->next;
		free(cur->filename);
		if (cur->largeIcons != NULL)
			if (ImageList_Destroy(cur->largeIcons) == 0)
				panic(L"Error destroying large icon image list");
		if (cur->smallIcons != NULL)
			if (ImageList_Destroy(cur->smallIcons) == 0)
				panic(L"Error destroying small icon image list");
		free(cur);
		cur = next;
	}
}

HRESULT collectFiles(WCHAR *dir, struct entry **out, ULONGLONG *count)
{
	struct entry *first;
	struct entry *current;
	BOOL isFirst;
	struct findFile *ff;
	WCHAR filename[MAX_PATH + 1];
	UINT n;
	ULONGLONG cnt;
	HRESULT hr;

	ff = startFindFile(dir);
	if (ff == NULL)
		return E_OUTOFMEMORY;
	current = NULL;
	isFirst = TRUE;
	cnt = 0;
	while (findFileNext(ff)) {
		// TODO check error
		PathCombineW(filename, dir, findFileName(ff));
		n = (UINT) ExtractIconW(hInstance, filename, (UINT) (-1));
		if (n != 0) {
			// this file has icons! queue it up
			current = allocEntry(current, findFileName(ff));
			if (current == NULL) {
				findFileEnd(ff);		// ignore error; it's too late
				return E_OUTOFMEMORY;
			}
			current->n = n;
			cnt++;
		}
		if (isFirst) {
			first = current;
			isFirst = FALSE;
		}
	}
	hr = findFileError(ff);
	if (hr != S_OK)
		return hr;
	hr = findFileEnd(ff);
	if (hr != S_OK)
		return hr;
	*out = first;
	*count = cnt;
	return S_OK;
}
