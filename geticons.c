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
		// TODO log if these failed
		if (cur->largeIcons != NULL)
			ImageList_Destroy(cur->largeIcons);
		if (cur->smallIcons != NULL)
			ImageList_Destroy(cur->smallIcons);
		free(cur);
		cur = next;
	}
}

static HRESULT collectFiles(WCHAR *dir, struct entry **out, ULONGLONG *countOut)
{
	struct entry *first;
	struct entry *current;
	struct findFile *ff;
	WCHAR *filename;
	UINT n;
	ULONGLONG count;
	HRESULT hr;

	// initialize output parameters
	*out = NULL;
	*countOut = 0;

	hr = startFindFile(dir, &ff);
	if (hr == S_FALSE)		// no files; return defaults
		return S_OK;
	if (hr != S_OK)
		return hr;

	first = NULL;			// this is important; see below
	current = NULL;
	count = 0;
	while (findFileNext(ff)) {
		hr = pathJoin(dir, findFileName(ff), &filename);
		if (hr != S_OK)
			goto fail;
		n = (UINT) ExtractIconW(hInstance, filename, (UINT) (-1));
		pathFree(filename);
		if (n == 0)			// no icons in this file; try the next one
			continue;
		// this file has icons! queue it up
		current = allocEntry(current, findFileName(ff));
		if (current == NULL) {
			hr = E_OUTOFMEMORY;
			goto fail;
		}
		current->n = n;
		count++;
		if (first == NULL)		// save the first one; we return it
			first = current;
	}
	hr = findFileError(ff);
	if (hr != S_OK)
		goto fail;
	findFileEnd(ff);

	// and we're good
	*out = first;
	*countOut = count;
	return S_OK;

fail:
	findFileEnd(ff);
	freeEntries(first);
	return hr;
}

static HRESULT getOne(struct entry *entry, WCHAR *dir)
{
	WCHAR *filename;
	HICON *largei, *smalli;
	UINT m;
	int i;
	DWORD lasterr;
	HRESULT hr;

	largei = (HICON *) malloc(entry->n * sizeof (HICON));
	if (largei == NULL)
		return E_OUTOFMEMORY;
	smalli = (HICON *) malloc(entry->n * sizeof (HICON));
	if (smalli == NULL) {
		free(largei);
		return E_OUTOFMEMORY;
	}

	hr = pathJoin(dir, entry->filename, &filename);
	if (hr != S_OK) {
		free(largei);
		free(smalli);
		return hr;
	}

	m = ExtractIconExW(filename, 0, largei, smalli, entry->n);
{DWORD lasterr;
lasterr = GetLastError();
WCHAR msg[2048];
_snwprintf(msg, 2048, L"%I32u %I32u", m, entry->n);
MessageBoxW(NULL, msg, entry->filename, 0);
SetLastError(lasterr);}
	if (m != entry->n * 2) {
		lasterr = GetLastError();
		hr = lasterrToHRESULT(lasterr);
//		goto out;
	}

	// ILC_MASK for icons that use a transparency mask; ILC_COLOR32 for those that don't so we can support newer icons
	entry->largeIcons = ImageList_Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
		ILC_COLOR32 | ILC_MASK, 100, 100);
	if (entry->largeIcons == NULL) {
		lasterr = GetLastError();
		hr = lasterrToHRESULT(lasterr);
		goto out;
	}
	entry->smallIcons = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
		ILC_COLOR32 | ILC_MASK, 100, 100);
	if (entry->smallIcons == NULL) {
		lasterr = GetLastError();
		hr = lasterrToHRESULT(lasterr);
		goto smallfail;
	}

	for (i = 0; i < (int) (entry->n); i++) {
		if (ImageList_AddIcon(entry->largeIcons, largei[i]) != i) {
			lasterr = GetLastError();
			hr = lasterrToHRESULT(lasterr);
			goto ilfail;
		}
		if (ImageList_AddIcon(entry->smallIcons, smalli[i]) != i) {
			lasterr = GetLastError();
			hr = lasterrToHRESULT(lasterr);
			goto ilfail;
		}
	}

	// and we're good!
	hr = S_OK;
	goto out;

ilfail:
	// TODO log if these failed
	ImageList_Destroy(entry->smallIcons);
	entry->smallIcons = NULL;
smallfail:
	ImageList_Destroy(entry->largeIcons);
	entry->smallIcons = NULL;

out:
	pathFree(filename);
	for (i = 0; i < (int) (entry->n); i++) {
		// TODO log failures
		DestroyIcon(largei[i]);
		DestroyIcon(smalli[i]);
	}
	free(largei);
	free(smalli);
	return hr;
}

// returns:
// - S_OK, p->entries != NULL - files
// - S_OK, p->entries == NULL - no files
// - S_FALSE - user cancelled
// - error code - error
HRESULT getIcons(struct getIconsParams *p)
{
	IProgressDialog *pd;
	struct entry *first;
	struct entry *current;
	ULONGLONG completed;
	ULONGLONG total;
	ULONGLONG i;
	HRESULT hr;

	pd = newProgressDialog();
	progdlgSetTexts(pd, L"Adding icons");
	progdlgStartModal(pd, p->parent,
		PROGDLG_NORMAL | PROGDLG_AUTOTIME | PROGDLG_NOMINIMIZE);

	hr = collectFiles(p->dir, &first, &total);
	if (hr != S_OK) {
		p->errmsg = L"Error collecting files in the chosen directory";
		first = NULL;
		goto out;
	}
	if (total == 0) {
		first = NULL;
		hr = S_OK;
		goto out;
	}

	progdlgResetTimer(pd);
	current = first;
	completed = 0;
	for (i = 0; i < total; i++) {
		if (IProgressDialog_HasUserCancelled(pd) != FALSE) {
			freeEntries(first);
			first = NULL;
			hr = S_FALSE;
			goto out;
		}

		hr = getOne(current, p->dir);
		if (hr != S_OK) {
			p->errmsg = L"Error extracting icons from file";
			freeEntries(first);
			first = NULL;
			goto out;
		}
		current = current->next;

		completed++;
//TODO		progdlgSetProgress(pd, completed, total);
	}

	hr = S_OK;
	// fall through to out

out:
	p->entries = first;
	progdlgDestroyModal(pd, p->parent);
	return hr;
}
