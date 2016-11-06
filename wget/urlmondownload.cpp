#include "stdafx.h"
#include <string>
#include <Urlmon.h>
#include <Wininet.h>
#include <functional>
#pragma comment(lib, "WinInet.lib")
#pragma comment(lib,"Urlmon")


class DownloadStatus :public IBindStatusCallback {
public:
	ULONG presize{ 0 };
	STDMETHOD(OnStartBinding)(
		/* [in] */ DWORD dwReserved,
		/* [in] */ IBinding __RPC_FAR *pib)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(GetPriority)(
		/* [out] */ LONG __RPC_FAR *pnPriority)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(OnLowResource)(
		/* [in] */ DWORD reserved)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(OnProgress)(
		/* [in] */ ULONG ulProgress,
		/* [in] */ ULONG ulProgressMax,
		/* [in] */ ULONG ulStatusCode,
		/* [in] */ LPCWSTR wszStatusText);

	STDMETHOD(OnStopBinding)(
		/* [in] */ HRESULT hresult,
		/* [unique][in] */ LPCWSTR szError)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(GetBindInfo)(
		/* [out] */ DWORD __RPC_FAR *grfBINDF,
		/* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(OnDataAvailable)(
		/* [in] */ DWORD grfBSCF,
		/* [in] */ DWORD dwSize,
		/* [in] */ FORMATETC __RPC_FAR *pformatetc,
		/* [in] */ STGMEDIUM __RPC_FAR *pstgmed)
	{
		return E_NOTIMPL;
	}

	STDMETHOD(OnObjectAvailable)(
		/* [in] */ REFIID riid,
		/* [iid_is][in] */ IUnknown __RPC_FAR *punk)
	{
		return E_NOTIMPL;
	}

	// IUnknown methods.  Note that IE never calls any of these methods, since
	// the caller owns the IBindStatusCallback interface, so the methods all
	// return zero/E_NOTIMPL.

	STDMETHOD_(ULONG, AddRef)()
	{
		return 0;
	}

	STDMETHOD_(ULONG, Release)()
	{
		return 0;
	}

	STDMETHOD(QueryInterface)(
		/* [in] */ REFIID riid,
		/* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *ppvObject)
	{
		return E_NOTIMPL;
	}
};

HRESULT DownloadStatus::OnProgress(ULONG ulProgress, ULONG ulProgressMax,
	ULONG ulStatusCode, LPCWSTR wszStatusText)
{
	float ps = 0;
	if (ulProgressMax != 0) {
		ps = (float)ulProgress * 100 / ulProgressMax;
	}

	fprintf(stderr, "\rDownload %2.2f%%", ps);
	return S_OK;
}


bool DownloadFileWarp(const std::wstring &remoteFile, const std::wstring &localFile) {
	DownloadStatus ds;
#ifndef ENABLE_CACHE
	DeleteUrlCacheEntryW(remoteFile.c_str());
#endif
	return URLDownloadToFileW(nullptr, remoteFile.c_str(), localFile.c_str(), 0, &ds) == S_OK;
}