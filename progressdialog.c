// 13 december 2015
#include "winiconview.h"

/*
tl;dr IProgressDialog cannot be used from C as-is
explanation:

First, Microsoft simply forgot to provide the COBJMACROS equivalent macros. That's not much of an issue; we can just write them ourselves, right?

There are some macros for defining COM objects in a form that is both C and C++ agnostic. A description can be found at https://blogs.msdn.microsoft.com/oldnewthing/20041005-00/?p=37653/.

One of the rules for defining interfaces this way is that you have to include the methods of your derived interfaces before your own. So if you derive directly from IUnknown (like IProgressDialog does), you have to start your interface definition with

	STDMETHOD(QueryInterface)(THIS_ REFIID riid, void **ppv) PURE;
	STDMETHOD_(ULONG, AddRef)(THIS) PURE;
	STDMETHOD_(ULONG, Release)(THIS) PURE;

Microsoft forgot to do this, only specifying that IUnknown is the base interface in the DECLARE_INTERFACE_IID_() macro.

This means that as far as the C compiler is concerned, pd->lpVtbl->StartProgressDialog is the first entry in the vtable, not the fourth.

Result: if we write a fake IProgressDialog_StartProgressDialog(), which is the third method in IProgressDialog, it'll actually call IProgressDialog::QueryInterface() (with the wrong parameters!) because the methods aren't in the right place!

MinGW-w64 preserves both errors, which is probably a good thing as far as code portability goes, but means switching SDKs will do us no good.

I could just do

	struct RealIProgressDialog {
		IUnknownVtbl unk;
		IProgressDialogVtbl pd;
	};

but I don't want to deal with potential alignment issues with the pd field.

I could just do this from C++ instead.

But since this interface is old, let's just fake out the vtable.
*/
#define xptr(name) (STDMETHODCALLTYPE *name)
struct realIProgressDialogVtbl {
	// IUnknown
	HRESULT xptr(QueryInterface)(IProgressDialog *, REFIID, void **);
	ULONG xptr(AddRef)(IProgressDialog *);
	ULONG xptr(Release)(IProgressDialog *);

	// IProgressDialog
	HRESULT xptr(StartProgressDialog)(IProgressDialog *,
		HWND, IUnknown *, DWORD, LPCVOID);
	HRESULT xptr(StopProgressDialog)(IProgressDialog *);
	HRESULT xptr(SetTitle)(IProgressDialog *, PCWSTR);
	HRESULT xptr(SetAnimation)(IProgressDialog *,
		HINSTANCE, UINT);
	BOOL xptr(HasUserCancelled)(IProgressDialog *);
	HRESULT xptr(SetProgress)(IProgressDialog *, DWORD, DWORD);
	HRESULT xptr(SetProgress64)(IProgressDialog *, ULONGLONG, ULONGLONG);
	HRESULT xptr(SetLine)(IProgressDialog *,
		DWORD, PCWSTR, BOOL, LPCVOID);
	HRESULT xptr(SetCancelMsg)(IProgressDialog *, PCWSTR, LPCVOID);
	HRESULT xptr(Timer)(IProgressDialog *, DWORD, LPCVOID);
};

// use an intermediate void * cast to keep msvc happy
#define transmogrify(pd) ((struct realIProgressDialogVtbl *) ((void *) ((pd)->lpVtbl)))

HRESULT IProgressDialog_QueryInterface(IProgressDialog *pd, REFIID a, void **b)
{
	return transmogrify(pd)->QueryInterface(pd, a, b);
}

ULONG IProgressDialog_AddRef(IProgressDialog *pd)
{
	return transmogrify(pd)->AddRef(pd);
}

ULONG IProgressDialog_Release(IProgressDialog *pd)
{
	return transmogrify(pd)->Release(pd);
}

HRESULT IProgressDialog_StartProgressDialog(IProgressDialog *pd, HWND a, IUnknown *b, DWORD c, LPCVOID d)
{
	return transmogrify(pd)->StartProgressDialog(pd, a, b, c, d);
}

HRESULT IProgressDialog_StopProgressDialog(IProgressDialog *pd)
{
	return transmogrify(pd)->StopProgressDialog(pd);
}

HRESULT IProgressDialog_SetTitle(IProgressDialog *pd, PCWSTR a)
{
	return transmogrify(pd)->SetTitle(pd, a);
}

HRESULT IProgressDialog_SetAnimation(IProgressDialog *pd, HINSTANCE a, UINT b)
{
	return transmogrify(pd)->SetAnimation(pd, a, b);
}

BOOL IProgressDialog_HasUserCancelled(IProgressDialog *pd)
{
	return transmogrify(pd)->HasUserCancelled(pd);
}

HRESULT IProgressDialog_SetProgress(IProgressDialog *pd, DWORD a, DWORD b)
{
	return transmogrify(pd)->SetProgress(pd, a, b);
}

HRESULT IProgressDialog_SetProgress64(IProgressDialog *pd, ULONGLONG a, ULONGLONG b)
{
	return transmogrify(pd)->SetProgress64(pd, a, b);
}

HRESULT IProgressDialog_SetLine(IProgressDialog *pd, DWORD a, PCWSTR b, BOOL c, LPCVOID d)
{
	return transmogrify(pd)->SetLine(pd, a, b, c, d);
}

HRESULT IProgressDialog_SetCancelMsg(IProgressDialog *pd, PCWSTR a, LPCVOID b)
{
	return transmogrify(pd)->SetCancelMsg(pd, a, b);
}

HRESULT IProgressDialog_Timer(IProgressDialog *pd, DWORD a, LPCVOID b)
{
	return transmogrify(pd)->Timer(pd, a, b);
}

// And now, some helper functions.

IProgressDialog *newProgressDialog(void)
{
	IProgressDialog *pd;
	HRESULT hr;

	hr = CoCreateInstance(&CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER,
		&IID_IProgressDialog, (LPVOID *) (&(d->pd)));
	if (hr != S_OK)
		panichr(L"Error creating progress dialog", hr);
	return pd;
}

void progdlgSetTexts(IProgressDialog *pd, WCHAR *title)
{
	HRESULT;

	hr = IProgressDialog_SetTitle(pd, title);
	if (hr != S_OK)
		panichr(L"Error setting progress dialog title", hr);
}

void progdlgStart(IProgressDialog *pd, HWND owner, DWORD flags)
{
	HRESULT hr;

	hr = IProgressDialog_StartProgressDialog(pd, owner, NULL, flags, NULL);
	if (hr != S_OK)
		panichr(L"Error starting progress dialog", hr);
}

void progdlgResetTimer(IProgressDialog *pd)
{
	HRESULT hr;

	hr = IProgressDIalog_Timer(pd, PDTIMER_RESET, NULL);
	if (hr != S_OK)
		panichr(L"Error resetting progress dialog timer", hr);
}

void progdlgSetProgress(IProgressDialog *pd, ULONGLONG current, ULONGLONG total)
{
	HRESULT hr;

	hr = IProgressDialog_SetProgress64(pd, completed, total);
	if (hr != S_OK)
		panichr(L"Error updating progress dialog", hr);
}

void progdlgDestroy(IProgressDialog *pd)
{
	HRESULT hr;

	hr = IProgressDialog_StopProgressDialog(pd);
	if (hr != S_OK)
		panichr(L"Error stopping progress dialog", hr);
	IProgressDialog_Release(pd);
}
