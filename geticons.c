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

static HRESULT collectFiles(WCHAR *dir, struct entry **out, ULONGLONG *count)
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

// TODO

struct giparams {
	HWND hwnd;
	WCHAR *dir;
};

static DWORD WINAPI getIconsThread(LPVOID vinput)
{
	struct giparams *p = (struct giparams *) vinput;
	struct entry *entries;
	ULONGLONG completed;
	ULONGLONG total;
	DWORD result;
	struct getIconsFailure err;
	HRESULT hr;

	hr = collectFiles(p->dir, &entries, &total);
	if (hr != S_OK) {
		err.msg = L"Error collecting files to get icons out of";
		err.hr = hr;
		goto fail;
	}

	for (completed = 0; completed < total; completed++) {
		// TODO make this asynchronous?
		// TODO see if IProgressDialog docs say anything
		SendMessageW(p->hwnd, msgProgress,
			(WPARAM) (&completed), (LPARAM) (&total));
		// TODO
		Sleep(500);
	}
	// send the completed progress just to be safe
	SendMessageW(p->hwnd, msgProgress,
		(WPARAM) (&completed), (LPARAM) (&total));

	// and we're done! give all this stuff back to the main window
	SendMessageW(p->hwnd, msgFinished, (WPARAM) entries, 0);
	result = 0;
	goto out;

fail:
	SendMessageW(p->hwnd, msgFinished, 0, (LPARAM) (&err));
	result = 1;
	// fall through to out

out:
	free(p->dir);
	free(p);
	return result;
}

void getIcons(HWND hwnd, WCHAR *dir)
{
	struct giparams *p;
	struct getIconsFailure err;
	DWORD lasterr;

	p = (struct giparams *) malloc (sizeof (struct giparams));
	if (p == NULL) {
		err.msg = L"Error allocating memory for icon collection thread";
		err.hr = E_OUTOFMEMORY;
		goto fail;
	}
	ZeroMemory(p, sizeof (struct giparams));
	p->hwnd = hwnd;
	p->dir = _wcsdup(dir);
	if (p->dir == NULL) {
		err.msg = L"Error copying directory name for icon collection thread";
		err.hr = E_OUTOFMEMORY;
		goto fail;
	}

	if (CreateThread(NULL, 0, getIconsThread, p, 0, NULL) == NULL) {
		lasterr = GetLastError();
		err.msg = L"Error creating icon collection thread";
		err.hr = HRESULT_FROM_WIN32(lasterr);
		if (err.hr == S_OK)		// if lasterr is 0
			err.hr = E_FAIL;
		goto fail;
	}

	// all done!
	return;

fail:
	if (p != NULL) {
		if (p->dir != NULL)
			free(p->dir);
		free(p);
	}
	SendMessageW(hwnd, msgFinished, 0, (LPARAM) (&err));
}
