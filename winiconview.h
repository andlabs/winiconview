// 13 december 2015
#include "winapi.h"
#include "resources.h"

#define programName L"Windows Icon Viewer"

// main.c
extern HINSTANCE hInstance;
extern int nCmdShow;

// mainwin.c
enum {
	// Sent to begin a directory search.
	// wParam - pointer to wide string with directory name
	// lParam - 0
	// lResult - 0
	msgAddIcons = WM_USER + 40,
};
extern HWND initMainWindow(void);
extern void uninitMainWindow(HWND mainwin);

// geticons.c
struct entry {
	// TODO rename to basename
	WCHAR *filename;
	UINT n;
	HIMAGELIST largeIcons;
	HIMAGELIST smallIcons;
	struct entry *next;
};
extern struct entry *allocEntry(struct entry *prev, WCHAR *filename);
extern void freeEntries(struct entry *cur);
struct getIconsParams {
	// input
	HWND parent;
	WCHAR *dir;
	// output
	struct entry *entries;
	const WCHAR *errmsg;
};
extern HRESULT getIcons(struct getIconsParams *p);

// findfile.c
struct findFile;		// TODO find the real way to shut msvc up about this
extern HRESULT startFindFile(WCHAR *path, struct findFile **out);
extern BOOL findFileNext(struct findFile *ff);
extern WCHAR *findFileName(struct findFile *ff);
extern HRESULT findFileError(struct findFile *ff);
extern void findFileEnd(struct findFile *ff);

// treeview.c
extern void appendFolder(HWND tv, WCHAR *dir, struct entry *entries);

// listview.c
extern void loadListview(HWND lv, struct entry *entry);

// util.c
extern HRESULT lasterrToHRESULT(DWORD lasterr);
extern HRESULT pathJoin(WCHAR *a, WCHAR *b, WCHAR **ret);
extern void pathFree(WCHAR *path);

// panic.c
extern HWND panicParent;
extern void panic(const WCHAR *msg);
extern void panichr(const WCHAR *msg, HRESULT hr);

// progressdialog.c
extern HRESULT IProgressDialog_QueryInterface(IProgressDialog *pd, REFIID a, void **b);
extern ULONG IProgressDialog_AddRef(IProgressDialog *pd);
extern ULONG IProgressDialog_Release(IProgressDialog *pd);
extern HRESULT IProgressDialog_StartProgressDialog(IProgressDialog *pd, HWND a, IUnknown *b, DWORD c, LPCVOID d);
extern HRESULT IProgressDialog_StopProgressDialog(IProgressDialog *pd);
extern HRESULT IProgressDialog_SetTitle(IProgressDialog *pd, PCWSTR a);
extern HRESULT IProgressDialog_SetAnimation(IProgressDialog *pd, HINSTANCE a, UINT b);
extern BOOL IProgressDialog_HasUserCancelled(IProgressDialog *pd);
extern HRESULT IProgressDialog_SetProgress(IProgressDialog *pd, DWORD a, DWORD b);
extern HRESULT IProgressDialog_SetProgress64(IProgressDialog *pd, ULONGLONG a, ULONGLONG b);
extern HRESULT IProgressDialog_SetLine(IProgressDialog *pd, DWORD a, PCWSTR b, BOOL c, LPCVOID d);
extern HRESULT IProgressDialog_SetCancelMsg(IProgressDialog *pd, PCWSTR a, LPCVOID b);
extern HRESULT IProgressDialog_Timer(IProgressDialog *pd, DWORD a, LPCVOID b);
extern IProgressDialog *newProgressDialog(void);
extern void progdlgSetTexts(IProgressDialog *pd, WCHAR *title);
extern void progdlgStartModal(IProgressDialog *pd, HWND owner, DWORD flags);
extern void progdlgResetTimer(IProgressDialog *pd);
extern void progdlgSetProgress(IProgressDialog *pd, ULONGLONG completed, ULONGLONG total);
extern void progdlgDestroyModal(IProgressDialog *pd, HWND owner);
