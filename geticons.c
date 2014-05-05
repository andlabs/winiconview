// 4 may 2014
#include "winiconview.h"

HIMAGELIST largeicons, smallicons;
LVGROUP *groups = NULL;
size_t nGroups = 0;
LVITEM *items = NULL;
size_t nItems = 0;

static size_t nGroupsAlloc = 0, nItemsAlloc = 0;

static TCHAR **groupnames = NULL;		// to store group names for sorting
static int ngroupnames = 0;

static void addGroup(TCHAR *name, int id)
{
	LVGROUP *g;
	LRESULT n;
	TCHAR *wname;

	if (nGroups >= nGroupsAlloc) {		// need more memory
		nGroupsAlloc += 200;		// will handle first run too
		groups = (LVGROUP *) realloc(groups, nGroupsAlloc * sizeof(LVGROUP));
		if (groups == NULL)
			panic("error allocating room for list view groups");
	}
	g = &groups[nGroups++];
	ZeroMemory(g, sizeof (LVGROUP));
	g->cbSize = sizeof (LVGROUP);
	g->mask = LVGF_HEADER | LVGF_GROUPID;
	g->pszHeader = _wcsdup(name);
	if (g->pszHeader == NULL)
		panic("error making copy of filename %S for grouping/sorting", name);
	// for some reason the group ID and the index returned by LVM_INSERTGROUP are separate concepts... so we have to provide an ID.
	// (thanks to roxfan in irc.efnet.net/#winprog for confirming)
	g->iGroupId = id;

	// save the name so we can sort
	if (ngroupnames < id + 1)
		ngroupnames = id + 1;
	groupnames = (TCHAR **) realloc(groupnames, ngroupnames * sizeof (TCHAR *));
	if (groupnames == NULL)
		panic("error expanding groupnames list to fit new group name \"%s\"", name);
	groupnames[id] = g->pszHeader;
}

// MSDN is bugged; http://msdn.microsoft.com/en-us/library/windows/desktop/bb775142%28v=vs.85%29.aspx is missing the CALLBACK, which led to mysterious crashes in wine
// the CALLBACK is verified from Microsoft's headers instead
// the headers also say int/void instead of INT/VOID but eh
INT CALLBACK groupLess(INT gn1, INT gn2, VOID *data)
{
	if (gn1 < 0 || gn1 >= ngroupnames)
		panic("group ID %d out of range in compare (max %d)", gn1, ngroupnames - 1);
	if (gn2 < 0 || gn2 >= ngroupnames)
		panic("group ID %d out of range in compare (max %d)", gn2, ngroupnames - 1);
	// ignore case; these are filenames
	// Microsoft says to use the _-prefixed functions (http://msdn.microsoft.com/en-us/library/ms235365.aspx); I wonder why these would be in the C++ standard... (TODO)
	return _wcsicmp(groupnames[gn1], groupnames[gn2]);
	// why not just get the group info each time? because we can't get the header length later, at least not as far as I know
}

static LPARAM filecount(TCHAR *, TCHAR *);
static void addIcons(UINT, HICON *, HICON *, int, int *, TCHAR *);
static void addInvalidIcon(int, int *, TCHAR *);

DWORD WINAPI getIcons(LPVOID data)
{
	struct giThreadData *threadData = (struct giThreadData *) data;
	HWND mainwin = threadData->mainwin;
	TCHAR *dirname = threadData->dirname;

	PVOID wow64token;

	ourWow64DisableWow64FsRedirection(&wow64token);

	TCHAR finddir[MAX_PATH + 1];

	if (PathCombine(finddir, dirname, L"*") == NULL)		// TODO MSDN is unclear; see if this is documented as working correctly
		panic("error producing version of \"%S\" for FindFirstFile()", dirname);

	if (PostMessage(mainwin, msgBegin, 0, filecount(dirname, finddir)) == 0)
		panic("error notifying main window that processing has begun");

	int itemid = 1;		// 0 is the dummy item

	largeicons = ImageList_Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
		ILC_COLOR32, 100, 100);
	if (largeicons == NULL)
		panic("error creating large icon list for list view");
	smallicons = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
		ILC_COLOR32, 100, 100);
	if (smallicons == NULL)
		panic("error creating small icon list for list view");

	HANDLE dir;
	WIN32_FIND_DATA entry;
	int groupid = 0;

	dir = FindFirstFile(finddir, &entry);
	if (dir == INVALID_HANDLE_VALUE) {
		DWORD e;

		e = GetLastError();
		if (e == ERROR_FILE_NOT_FOUND) {
			// TODO report to user
			printf("no files\n");
			exit(0);
		}
		SetLastError(e);		// for panic()
		panic("error opening \"%S\"", dirname);
	}
	for (;;) {
		TCHAR filename[MAX_PATH + 1];

		if (PostMessage(mainwin, msgStep, 0, 0) == 0)
			panic("error notifying main window that processing has moved to the next file");

		if (PathCombine(filename, dirname, entry.cFileName) == NULL)
			panic("error allocating combined filename \"%S\\%S\"",
				dirname, entry.cFileName);

		UINT nIcons;

		nIcons = (UINT) ExtractIcon(hInstance, filename, -1);
		if (nIcons != 0) {
			HICON *large, *small;
			UINT nlarge, nsmall;
			UINT nStart = 0;

			addGroup(entry.cFileName, groupid);

			large = (HICON *) malloc(nIcons * sizeof (HICON));
			if (large == NULL)
				panic("error allocating array of large icon handles for \"%S\"", filename);
			small = (HICON *) malloc(nIcons * sizeof (HICON));
			if (small == NULL)
				panic("error allocating array of small icon handles for \"%S\"", filename);

			while (nStart < nIcons) {
				nlarge = ExtractIconEx(filename, nStart, large, NULL, nIcons);
				nsmall = ExtractIconEx(filename, nStart, NULL, small, nIcons);
				// don't check returns from ExtractIconEx() here; a 0 return indicates an invalid icon
				if (nlarge != nsmall)
					panic("internal inconsisitency reading icons from \"%S\": differing number of large and small icons read (large: %u, small: %u)", filename, nlarge, nsmall);
				if (nlarge == 0)	{		// no icon; mark this as invalid
					addInvalidIcon(groupid, &itemid, filename);
					nStart++;
				} else {				// we have icons!
					addIcons(nlarge, large, small, groupid, &itemid, filename);
					nStart += nlarge;
				}
			}

			free(large);
			free(small);

			groupid++;
		}

		if (FindNextFile(dir, &entry) == 0) {
			DWORD e;

			e = GetLastError();
			if (e == ERROR_NO_MORE_FILES)		// we're done
				break;
			SetLastError(e);		// for panic()
			panic("error getting next filename in \"%S\"", dirname);
		}
	}
	if (FindClose(dir) == 0)
		panic("error closing \"%S\"", dirname);

	ourWow64RevertWow64FsRedirection(wow64token);

	if (PostMessage(mainwin, msgEnd, 0, 0) == 0)
		panic("error notifying main window that processing has ended");
	return 0;
}

static LVITEM *addItem(void)
{
	if (nItems >= nItemsAlloc) {		// need more memory
		nItemsAlloc += 200;			// will handle first run too
		items = (LVITEM *) realloc(items, nItemsAlloc * sizeof (LVITEM));
		if (items == NULL)
			panic("error allocating room for list view items");
	}
	return &items[nItems++];
}

static void addIcons(UINT nIcons, HICON *large, HICON *small, int groupid, int *itemid, TCHAR *filename)
{
	UINT i;
	int index, i2;
	LVITEM *item;

	for (i = 0; i < nIcons; i++) {
		index = ImageList_AddIcon(largeicons, large[i]);
		if (index == -1)
			panic("error adding icon %u from \"%S\" to large icon list", i, filename);
		DestroyIcon(large[i]);		// the image list seems to keep a copy of the icon; destroy the original to avoid running up against memory limits (TODO verify against documentation; it confused me at first)
		i2 = ImageList_AddIcon(smallicons, small[i]);
		if (i2 == -1)
			panic("error adding icon %u from \"%S\" to small icon list (%p)", i, filename, small[i]);
		DestroyIcon(small[i]);
		if (index != i2)
			panic("internal inconsistency: indices of icon %u from \"%S\" in image lists do not match (large %d, small %d)", i, filename, index, i2);

		item = addItem();
		ZeroMemory(item, sizeof (LVITEM));
		item->mask = LVIF_IMAGE | LVIF_GROUPID | LVIF_TEXT;
		item->iImage = index;
		item->iGroupId = groupid;
		char *q;
		asprintf(&q, "%d", *itemid);
		item->pszText = toWideString(q);
		free(q);
		item->iItem = (*itemid)++;
		// TODO above errors (note plural) needs to be changed to represent the correct icon count
	}
}

static void addInvalidIcon(int groupid, int *itemid, TCHAR *filename)
{
	LVITEM *item;

	// TODO prevent an icon from being shown for these?
	item = addItem();
	ZeroMemory(item, sizeof (LVITEM));
	item->mask = LVIF_GROUPID | LVIF_TEXT;
	item->iGroupId = groupid;
	char *q;
	asprintf(&q, "%d (invalid)", *itemid);
	item->pszText = toWideString(q);
	free(q);
	item->iItem = (*itemid)++;
}

static LPARAM filecount(TCHAR *dirname, TCHAR *finddir)
{
	HANDLE dir;
	WIN32_FIND_DATA entry;
	LPARAM count = 0;

	dir = FindFirstFile(finddir, &entry);
	if (dir == INVALID_HANDLE_VALUE) {
		DWORD e;

		e = GetLastError();
		if (e == ERROR_FILE_NOT_FOUND)		// no files
			return 0;
		SetLastError(e);						// for panic()
		panic("error opening \"%S\" for counting files", dirname);
	}
	for (;;) {
		count++;
		if (FindNextFile(dir, &entry) == 0) {
			DWORD e;

			e = GetLastError();
			if (e == ERROR_NO_MORE_FILES)		// we're done
				break;
			SetLastError(e);						// for panic()
			panic("error getting next filename in \"%S\" for counting files", dirname);
		}
	}
	if (FindClose(dir) == 0)
		panic("error closing \"%S\" for counting files", dirname);
	return count;
}
