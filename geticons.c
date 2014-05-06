// 4 may 2014
#include "winiconview.h"

struct giThreadData {
	struct giThreadOutput *o;			// pointer so it can be allocated on the process heap
	size_t nGroupsAlloc;
	size_t nItemsAlloc;
};

static UINT systemDependentGroupFlags = 0;

void initGetIcons(void)
{
	// TODO I feel uneasy about using an OS version check for this, but I don't know how to test for the existence of a style flag at runtime...
	OSVERSIONINFOEX ver;
	DWORDLONG cond = 0;

	ZeroMemory(&ver, sizeof (OSVERSIONINFOEX));
	ver.dwOSVersionInfoSize = sizeof (OSVERSIONINFOEX);
	ver.dwMajorVersion = 6;		// Windows Vista and Windows Server 2008
	ver.dwMinorVersion = 0;
	ver.wServicePackMajor = 0;
	ver.wServicePackMinor = 0;
	VER_SET_CONDITION(cond, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(cond, VER_MINORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(cond, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
	VER_SET_CONDITION(cond, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);
	if (VerifyVersionInfo(&ver,
		VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR,
		cond) == 0) {
		DWORD e;

		e = GetLastError();
		if (e == ERROR_OLD_WIN_VERSION)
			return;
		SetLastError(e);		// for panic()
		panic(L"error checking for Windows Vista or newer to enable collapsible listview groups");
	}
	// otherwise we're good
	systemDependentGroupFlags = LVGS_COLLAPSIBLE;
}

static void addGroup(struct giThreadData *d, TCHAR *name, int id)
{
	LVGROUP *g;

	if (d->o->nGroups >= d->nGroupsAlloc) {		// need more memory
		d->nGroupsAlloc += 200;		// will handle first run too
		d->o->groups = (LVGROUP *) realloc(d->o->groups, d->nGroupsAlloc * sizeof (LVGROUP));
		if (d->o->groups == NULL)
			panic(L"error allocating room for list view groups");
	}
	g = &d->o->groups[d->o->nGroups++];
	ZeroMemory(g, sizeof (LVGROUP));
	g->cbSize = sizeof (LVGROUP);
	g->mask = LVGF_HEADER | LVGF_GROUPID;
	if (systemDependentGroupFlags != 0) {
		g->mask |= LVGF_STATE;
		g->state = systemDependentGroupFlags;
	}
	g->pszHeader = _wcsdup(name);
	if (g->pszHeader == NULL)
		panic(L"error making copy of filename %s for grouping/sorting", name);
	// for some reason the group ID and the index returned by LVM_INSERTGROUP are separate concepts... so we have to provide an ID.
	// (thanks to roxfan in irc.efnet.net/#winprog for confirming)
	g->iGroupId = id;

	// save the name so we can sort
	if (d->o->ngroupnames < id + 1)
		d->o->ngroupnames = id + 1;
	d->o->groupnames = (TCHAR **) realloc(d->o->groupnames, d->o->ngroupnames * sizeof (TCHAR *));
	if (d->o->groupnames == NULL)
		panic(L"error expanding groupnames list to fit new group name \"%s\"", name);
	d->o->groupnames[id] = g->pszHeader;
}

// MSDN is bugged; http://msdn.microsoft.com/en-us/library/windows/desktop/bb775142%28v=vs.85%29.aspx is missing the CALLBACK, which led to mysterious crashes in wine
// the CALLBACK is verified from Microsoft's headers instead
// the headers also say int/void instead of INT/VOID but eh
INT CALLBACK groupLess(INT gn1, INT gn2, VOID *data)
{
	struct giThreadOutput *o = (struct giThreadOutput *) data;

	if (gn1 < 0 || gn1 >= o->ngroupnames)
		panic(L"group ID %d out of range in compare (max %d)", gn1, o->ngroupnames - 1);
	if (gn2 < 0 || gn2 >= o->ngroupnames)
		panic(L"group ID %d out of range in compare (max %d)", gn2, o->ngroupnames - 1);
	// ignore case; these are filenames
	// Microsoft says to use the _-prefixed functions (http://msdn.microsoft.com/en-us/library/ms235365.aspx); I wonder why these would be in the C++ standard... (TODO)
	return _wcsicmp(o->groupnames[gn1], o->groupnames[gn2]);
	// why not just get the group info each time? because we can't get the header length later, at least not as far as I know
}

static LPARAM filecount(TCHAR *, TCHAR *);
static void addIcons(struct giThreadData *, UINT, HICON *, HICON *, int, int *, TCHAR *);
static void addInvalidIcon(struct giThreadData *, int, int *, TCHAR *);

DWORD WINAPI getIcons(LPVOID vinput)
{
	struct giThreadInput *input = (struct giThreadInput *) vinput;
	HWND mainwin = input->mainwin;
	TCHAR *dirname = input->dirname;

	struct giThreadData d;

	ZeroMemory(&d, sizeof (struct giThreadData));
	d.o = (struct giThreadOutput *) malloc(sizeof (struct giThreadOutput));
	if (d.o == NULL)
		panic(L"error allocating getIcons() thread output area");
	ZeroMemory(d.o, sizeof (struct giThreadOutput));

	PVOID wow64token;

	ourWow64DisableWow64FsRedirection(&wow64token);

	TCHAR finddir[MAX_PATH + 1];

	if (PathCombine(finddir, dirname, L"*") == NULL)		// TODO MSDN is unclear; see if this is documented as working correctly
		panic(L"error producing version of \"%s\" for FindFirstFile()", dirname);

	if (PostMessage(mainwin, msgBegin, 0, filecount(dirname, finddir)) == 0)
		panic(L"error notifying main window that processing has begun");

	int itemid = 1;		// 0 is the dummy item

	// ILC_MASK for icons that use a transparency mask; ILC_COLOR32 for those that don't so we can support newer icons
	d.o->largeicons = ImageList_Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
		ILC_COLOR32 | ILC_MASK, 100, 100);
	if (d.o->largeicons == NULL)
		panic(L"error creating large icon list for list view");
	d.o->smallicons = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
		ILC_COLOR32 | ILC_MASK, 100, 100);
	if (d.o->smallicons == NULL)
		panic(L"error creating small icon list for list view");

	HANDLE dir;
	WIN32_FIND_DATA entry;
	int groupid = 0;

	dir = FindFirstFile(finddir, &entry);
	if (dir == INVALID_HANDLE_VALUE)			// don't handle file not found here; if that comes up then something bad happened between the file count and this
		panic(L"error opening \"%s\"", dirname);
	for (;;) {
		TCHAR filename[MAX_PATH + 1];

		if (PostMessage(mainwin, msgStep, 0, 0) == 0)
			panic(L"error notifying main window that processing has moved to the next file");

		if (PathCombine(filename, dirname, entry.cFileName) == NULL)
			panic(L"error allocating combined filename \"%s\\%s\"",
				dirname, entry.cFileName);

		UINT nIcons;

		nIcons = (UINT) ExtractIcon(hInstance, filename, -1);
		if (nIcons != 0) {
			HICON *large, *small;
			UINT nlarge, nsmall;
			UINT nStart = 0;

			addGroup(&d, entry.cFileName, groupid);

			large = (HICON *) malloc(nIcons * sizeof (HICON));
			if (large == NULL)
				panic(L"error allocating array of large icon handles for \"%s\"", filename);
			small = (HICON *) malloc(nIcons * sizeof (HICON));
			if (small == NULL)
				panic(L"error allocating array of small icon handles for \"%s\"", filename);

			while (nStart < nIcons) {
				nlarge = ExtractIconEx(filename, nStart, large, NULL, nIcons);
				nsmall = ExtractIconEx(filename, nStart, NULL, small, nIcons);
				// don't check returns from ExtractIconEx() here; a 0 return indicates an invalid icon
				if (nlarge != nsmall)
					panic(L"internal inconsisitency reading icons from \"%s\": differing number of large and small icons read (large: %u, small: %u)", filename, nlarge, nsmall);
				if (nlarge == 0)	{		// no icon; mark this as invalid
					addInvalidIcon(&d, groupid, &itemid, filename);
					nStart++;
				} else {				// we have icons!
					addIcons(&d, nlarge, large, small, groupid, &itemid, filename);
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
			panic(L"error getting next filename in \"%s\"", dirname);
		}
	}
	if (FindClose(dir) == 0)
		panic(L"error closing \"%s\"", dirname);

	ourWow64RevertWow64FsRedirection(wow64token);

	if (PostMessage(mainwin, msgEnd, 0, (LPARAM) d.o) == 0)
		panic(L"error notifying main window that processing has ended");
	return 0;
}

static LVITEM *addItem(struct giThreadData *d)
{
	if (d->o->nItems >= d->nItemsAlloc) {		// need more memory
		d->nItemsAlloc += 200;			// will handle first run too
		d->o->items = (LVITEM *) realloc(d->o->items, d->nItemsAlloc * sizeof (LVITEM));
		if (d->o->items == NULL)
			panic(L"error allocating room for list view items");
	}
	return &d->o->items[d->o->nItems++];
}

static void addIcons(struct giThreadData *d, UINT nIcons, HICON *large, HICON *small, int groupid, int *itemid, TCHAR *filename)
{
	UINT i;
	int index, i2;
	LVITEM *item;

	for (i = 0; i < nIcons; i++) {
		index = ImageList_AddIcon(d->o->largeicons, large[i]);
		if (index == -1)
			panic(L"error adding icon %u from \"%s\" to large icon list", i, filename);
		DestroyIcon(large[i]);		// the image list seems to keep a copy of the icon; destroy the original to avoid running up against memory limits (TODO verify against documentation; it confused me at first)
		i2 = ImageList_AddIcon(d->o->smallicons, small[i]);
		if (i2 == -1)
			panic(L"error adding icon %u from \"%s\" to small icon list (%p)", i, filename, small[i]);
		DestroyIcon(small[i]);
		if (index != i2)
			panic(L"internal inconsistency: indices of icon %u from \"%s\" in image lists do not match (large %d, small %d)", i, filename, index, i2);

		item = addItem(d);
		ZeroMemory(item, sizeof (LVITEM));
		item->mask = LVIF_IMAGE | LVIF_GROUPID | LVIF_TEXT;
		item->iImage = index;
		item->iGroupId = groupid;
		item->pszText = ourawsprintf(L"%d", *itemid);
		if (item->pszText == NULL)
			panic(L"error giving list view item %u from \"%s\" text", i, filename);
		item->iItem = (*itemid)++;
		// TODO above errors (note plural) and the one below needs to be changed to represent the correct icon count
	}
}

static void addInvalidIcon(struct giThreadData *d, int groupid, int *itemid, TCHAR *filename)
{
	LVITEM *item;

	// TODO prevent an icon from being shown for these?
	item = addItem(d);
	ZeroMemory(item, sizeof (LVITEM));
	item->mask = LVIF_GROUPID | LVIF_TEXT;
	item->iGroupId = groupid;
	item->pszText = ourawsprintf(L"%d (invalid)", *itemid);
	if (item->pszText == NULL)
		panic(L"error giving list view invalid item from \"%s\" text", filename);
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
		panic(L"error opening \"%s\" for counting files", dirname);
	}
	for (;;) {
		count++;
		if (FindNextFile(dir, &entry) == 0) {
			DWORD e;

			e = GetLastError();
			if (e == ERROR_NO_MORE_FILES)		// we're done
				break;
			SetLastError(e);						// for panic()
			panic(L"error getting next filename in \"%s\" for counting files", dirname);
		}
	}
	if (FindClose(dir) == 0)
		panic(L"error closing \"%s\" for counting files", dirname);
	return count;
}
