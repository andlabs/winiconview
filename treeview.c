// 14 december 2015
#include "winiconview.h"

// the lParam of a root node is a pointer to the first struct entry
// the lParam of a child node is a pointer to its struct entry

void appendFolder(HWND tv, WCHAR *dir, struct entry *entries)
{
	TVINSERTSTRUCTW ti;
	HTREEITEM parent;

	ZeroMemory(&ti, sizeof (TVINSERTSTRUCTW));
	ti.hParent = TVI_ROOT;
	// TODO TVI_ROOT or TVI_LAST
	ti.hInsertAfter = TVI_ROOT;
	ti.itemex.mask = TVIF_PARAM | TVIF_TEXT;
	ti.itemex.pszText = dir;
	ti.itemex.lParam = (LPARAM) entries;
	parent = (HTREEITEM) SendMessageW(tv, TVM_INSERTITEM, 0, (LPARAM) (&ti));
	if (parent == NULL)
		panic(L"Error adding new folder to tree view");
}
