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
	// Sent by collector thread to indicate progress.
	// wParam - ULONGLONG pointer with number of items completed
	// lParam - ULONGLONG pointer with number of items total
	// lResult - nonzero if user cancelled
	msgProgress,
	// Sent by collector thread when done. Not sent if user cancelled.
	// wParam - if successful, TODO
	// lParam - if failure, pointer to struct getIconsFailure
	// lResult - 0
	msgFinished,
};
extern HWND initMainWindow(void);
extern void uninitMainWindow(HWND mainwin);

// geticons.c
struct entry {
	WCHAR *filename;
	UINT n;
	HIMAGELIST largeIcons;
	HIMAGELIST smallIcons;
	struct entry *next;
};
extern struct entry *allocEntry(struct entry *prev, WCHAR *filename);
extern void freeEntries(struct entry *cur);
struct getIconsFailure {
	const WCHAR *msg;
	HRESULT hr;
};
extern void getIcons(HWND hwnd, WCHAR *dir);

// util.c
extern struct findFile *startFindFile(WCHAR *path);
extern BOOL findFileNext(struct findFile *ff);
extern WCHAR *findFileName(struct findFile *ff);
extern HRESULT findFileError(struct findFile *ff);
extern HRESULT findFileEnd(struct findFile *ff);

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
