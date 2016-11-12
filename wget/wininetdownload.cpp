#include "stdafx.h"
#include <Windows.h>
#include <WinInet.h>
#include <Shlwapi.h>
#include "console.h"

#pragma comment(lib, "WinInet.lib")
//https://msdn.microsoft.com/en-us/library/windows/desktop/aa385328(v=vs.85).aspx
//INTERNET_OPTION_ENABLE_HTTP_PROTOCOL
//HTTP_PROTOCOL_FLAG_HTTP2
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
		return false;
	}
	DWORD  dwOption = HTTP_PROTOCOL_FLAG_HTTP2;
	InternetSetOptionW(hInet, INTERNET_OPTION_ENABLE_HTTP_PROTOCOL,&dwOption,sizeof(dwOption));
	WinINetObject hConnect = InternetConnectW(hInet, zurl.host.c_str(),
		(INTERNET_PORT)zurl.nPort, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, NULL);
	if (!hConnect) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"InternetConnectW(): %s", err.message());
		return false;
	}
	WinINetObject hRequest = HttpOpenRequestW(hConnect, L"GET", zurl.path.c_str(),
		nullptr, L"", nullptr, INTERNET_FLAG_RELOAD, 0);
	if (!hRequest) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"HttpOpenRequestW(): %s", err.message());
		return false;
	}
	// lpszVersion ->nullptr ,use config
	std::wstring tmp = localFile + L".part";
	HANDLE hFile =
		CreateFileW(tmp.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	///
	BYTE fixedsizebuf[16384];
	DWORD dwReadSize = 0;
	DWORD dwWriteSize = 0;
	while (1)
	{
		if (InternetReadFile(hInet, fixedsizebuf, sizeof(fixedsizebuf), &dwReadSize)) {
			if (dwReadSize == 0) {
				break;
			}
			WriteFile(hFile, fixedsizebuf, dwReadSize, &dwWriteSize, NULL);
		}
	}

	std::wstring npath = localFile;
	int i = 1;
	while (PathFileExistsW(npath.c_str())) {
		auto n = localFile.find_last_of('.');
		if (n != std::wstring::npos) {
			npath = localFile.substr(0, n) + L"(";
			npath += std::to_wstring(i);
			npath += L")";
			npath += localFile.substr(n);
		}
		else {
			npath = localFile + L"(" + std::to_wstring(i) + L")";
		}
		i++;
	}
	CloseHandle(hFile);
	MoveFileExW(tmp.c_str(), npath.c_str(), MOVEFILE_COPY_ALLOWED);
	return true;
}