#include "stdafx.h"
#include <Windows.h>
#include <WinInet.h>
#include "console.h"

#pragma comment(lib, "WinInet.lib")

struct WinINetRequestURL {
	int nPort;
	int nScheme;
	std::wstring scheme;
	std::wstring host;
	std::wstring path;
	std::wstring username;
	std::wstring password;
	std::wstring extrainfo;
	bool Parse(const std::wstring &url) {
		URL_COMPONENTS urlComp;
		DWORD dwUrlLen = 0;
		ZeroMemory(&urlComp, sizeof(urlComp));
		urlComp.dwStructSize = sizeof(urlComp);
		urlComp.dwSchemeLength = (DWORD)-1;
		urlComp.dwHostNameLength = (DWORD)-1;
		urlComp.dwUrlPathLength = (DWORD)-1;
		urlComp.dwExtraInfoLength = (DWORD)-1;

		if (!InternetCrackUrlW(url.data(), dwUrlLen, 0, &urlComp)) {
			return false;
		}
		scheme.assign(urlComp.lpszScheme, urlComp.dwSchemeLength);
		host.assign(urlComp.lpszHostName, urlComp.dwHostNameLength);
		path.assign(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
		nPort = urlComp.nPort;
		nScheme = urlComp.nScheme;
		if (urlComp.lpszUserName) {
			username.assign(urlComp.lpszUserName, urlComp.dwUserNameLength);
		}
		if (urlComp.lpszPassword) {
			password.assign(urlComp.lpszPassword, urlComp.dwPasswordLength);
		}
		if (urlComp.lpszExtraInfo) {
			extrainfo.assign(urlComp.lpszExtraInfo,
				urlComp.dwExtraInfoLength);
		}
		return true;
	}
};


class WinINetObject {
public:
	WinINetObject(HINTERNET hInternet) :hInternet_(hInternet) {
	}
	operator HINTERNET() {
		return hInternet_;
	}
	operator bool() {
		return hInternet_ != nullptr;
	}
	WinINetObject() {
		if (hInternet_) {
			InternetCloseHandle(hInternet_);
		}
	}
private:
	HINTERNET hInternet_;
};

bool WinINetDownloadDriver(const std::wstring &url, const std::wstring &localFile, ProgressCallback *callback) {
	WinINetRequestURL zurl;
	if (!zurl.Parse(url)) {
		BaseErrorMessagePrint(L"Wrong URL: %s\n", url.c_str());
		return false;
	}
	WinINetObject hInet = InternetOpenW(L"WindowsGet", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
	if (!hInet) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"InternetOpenW(): %s", err.message());
	}
	return true;
}