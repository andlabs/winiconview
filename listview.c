// 14 december 2015
#include "winiconview.h"

void loadListview(HWND lv, struct entry *entry)
{
	UINT i;
	LVITEMW li;
	WCHAR label[256];

	if (SendMessageW(lv, LVM_DELETEALLITEMS, 0, 0) == (LRESULT) FALSE)
		panic(L"Error clearing list view for showing new icons");
	if (entry == NULL)
		return;

	// TODO load image lists

	for (i = 0; i <entry->n; i++) {
		ZeroMemory(&li, sizeof (LVITEMW));
		li.mask = /*LVIF_IMAGE | */LVIF_TEXT;
		li.iItem = (int) i;
		_snwprintf(label, 256, L"%I32u", i + 1);
		li.pszText = label;
		li.iImage = (int) i;
		if (SendMessageW(lv, LVM_INSERTITEMW, 0, (LPARAM) (&li)) == (LRESULT) (-1))
			panic(L"Error adding icon entry to list view");
	}
}
