// 13 december 2015
#include "winiconview.h"

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

static HRESULT collectFIles(WCHAR *dir, struct entry **out)
{
	struct entry *first;
	struct entry *current;
	BOOL isFirst;
	struct findFIle *ff;
	HRESULT hr;

	ff = startFindFile(dir);
	if (ff == NULL)
		return E_OUTOFMEMORY;
	current = NULL;
	isFirst = TRUE;
	while (findFileNext(ff)) {
		// TODO
		if (isFirst) {
			first = current;
			isFirst = FALSE;
		}
	}
	hr = findFileError(ff);
	if (hr != S_OK)
		return hr;
	hr = findFileEnd(ff);
	*out = first;
	return S_OK;
}
