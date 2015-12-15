// 14 december 2015
#include "winiconview.h"

// the lParam of a root node is a pointer to the first struct entry
// the lParam of a child node is a pointer to its struct entry

void appendFolder(HWND tv, WCHAR *dir, struct entry *entries)
{
	TVINSERTSTRUCTW ti;
	HTREEITEM parent, child;
	struct entry *current;

	ZeroMemory(&ti, sizeof (TVINSERTSTRUCTW));
	ti.hParent = TVI_ROOT;
	ti.hInsertAfter = TVI_LAST;
	ti.itemex.mask = TVIF_PARAM | TVIF_TEXT;
	ti.itemex.pszText = dir;
	ti.itemex.lParam = (LPARAM) entries;
	parent = (HTREEITEM) SendMessageW(tv, TVM_INSERTITEM, 0, (LPARAM) (&ti));
	if (parent == NULL)
		panic(L"Error adding new folder to tree view");

	// TODO insert the items in reverse order somehow to make it faster; keeping track of child isn't good enough (https://blogs.msdn.microsoft.com/oldnewthing/20111125-00/?p=9033/)
	child = TVI_FIRST;
	for (current = entries; current != NULL; current = current->next) {
		ZeroMemory(&ti, sizeof (TVINSERTSTRUCTW));
		ti.hParent = parent;
		ti.hInsertAfter = child;
		ti.itemex.mask = TVIF_PARAM | TVIF_TEXT;
		ti.itemex.pszText = current->filename;
		ti.itemex.lParam = (LPARAM) current;
		child = (HTREEITEM) SendMessageW(tv, TVM_INSERTITEM, 0, (LPARAM) (&ti));
		if (child == NULL)
			panic(L"Error adding folder entry to tree view");
	}
}
