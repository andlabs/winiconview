// 5 may 2014
#include "winiconview.h"

static HIMAGELIST iconlists[2];

HWND makeListView(HWND parent, HMENU controlID)
{
	HWND listview;

	listview = CreateWindowEx(0,
		WC_LISTVIEW, L"",
		LVS_ICON | WS_VSCROLL | WS_CHILD | WS_VISIBLE,
		0, 0, 100, 100,
		parent, controlID, hInstance, NULL);
	if (listview == NULL)
		panic("error creating list view");

	if (SendMessage(listview, LVM_SETIMAGELIST,
		LVSIL_NORMAL, (LPARAM) largeicons) == (LRESULT) NULL)
;//		panic("error giving large icon list to list view");
	iconlists[0] = largeicons;
	iconlists[1] = smallicons;

	// we need to have an item to be able to add a group
	LVITEM dummy;

	ZeroMemory(&dummy, sizeof (LVITEM));
	dummy.mask = LVIF_TEXT;
	dummy.pszText = L"dummy";
	dummy.iItem = 0;
	if (SendMessage(listview, LVM_INSERTITEM,
		(WPARAM) 0, (LPARAM) &dummy) == (LRESULT) -1)
		panic("error adding dummy item to list view");
	// the dummy item has index 0

	if (SendMessage(listview, LVM_ENABLEGROUPVIEW,
		(WPARAM) TRUE, (LPARAM) NULL) == (LRESULT) -1)
		panic("error enabling groups in list view");

	size_t i;

	for (i = 0; i < nGroups; i++)
		if (SendMessage(listview, LVM_INSERTGROUP,
			(WPARAM) -1, (LPARAM) &groups[i]) == (LRESULT) -1)
			panic("error adding group \"%S\" to list view", groups[i].pszHeader);
	for (i = 0; i < nItems; i++)
		if (SendMessage(listview, LVM_INSERTITEM,
			(WPARAM) 0, (LPARAM) &items[i]) == (LRESULT) -1)
			panic("error adding item \"%S\" to list view", items[i].pszText);

	// and we're done with the dummy item
	if (SendMessage(listview, LVM_DELETEITEM, 0, 0) == FALSE)
		panic("error removing dummy item from list view");

	// now sort the groups in alphabetical order
	if (SendMessage(listview, LVM_SORTGROUPS,
		(WPARAM) groupLess, (LPARAM) NULL) == 0)
		panic("error sorting icon groups by filename");

	// and now some extended styles
	// the mask (WPARAM) defines which bits of the value (LPARAM) are to be changed
	// all other bits are ignored
	// so to set just the ones we specify, keeping any other styles intact, set both to the same value
	// MSDN says this returns the previous styles, with no mention of an error condition
#define xstyle 0	// TODO LVS_EX_BORDERSELECT?
	if (xstyle != 0)
		SendMessage(listview, LVM_SETEXTENDEDLISTVIEWSTYLE,
			xstyle, xstyle);

	return listview;
}

void resizeListView(HWND listview, HWND parent)
{
	if (listview != NULL) {
		RECT r;

		if (GetClientRect(parent, &r) == 0)
			panic("error getting new list view size");
		if (MoveWindow(listview, 0, 0, r.right - r.left, r.bottom - r.top, TRUE) == 0)
			panic("error resizing list view");
	}
}

LRESULT handleListViewRightClick(HWND listview, NMHDR *nmhdr)
{
	int times = 1;

	// TODO needed?
	if (nmhdr->code == NM_RDBLCLK) {	// turn double-clicks into two single-clicks
		times = 2;
		nmhdr->code = NM_CLICK;
	}
	if (nmhdr->code == NM_RCLICK) {
		for (; times != 0; times--) {
			HIMAGELIST temp;

			printf("right click %d\n", times);		// TODO
			temp = iconlists[0];
			iconlists[0] = iconlists[1];
			iconlists[1] = temp;
			if (SendMessage(listview, LVM_SETIMAGELIST,
				LVSIL_NORMAL, (LPARAM) iconlists[0]) == (LRESULT) NULL)
				panic("error swapping list view icon lists");
		}
		return 1;
	}
	return 0;
}
