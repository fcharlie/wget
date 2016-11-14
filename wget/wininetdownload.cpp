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
	int nPort=0;
	int nScheme=0;
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

bool DownloadFileUseWininet(const std::wstring &url, const std::wstring &localFile, ProgressCallback *callback) {
	WinINetRequestURL zurl;
	if (!zurl.Parse(url)) {
		BaseErrorMessagePrint(L"Wrong URL: %s\n", url.c_str());
		return false;
	}
	DeleteUrlCacheEntryW(url.c_str());
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
	DWORD dwOpenRequestFlags = INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP |
		INTERNET_FLAG_KEEP_CONNECTION |
		INTERNET_FLAG_NO_AUTH |
		INTERNET_FLAG_NO_COOKIES |
		INTERNET_FLAG_NO_UI |
		INTERNET_FLAG_SECURE |
		INTERNET_FLAG_IGNORE_CERT_CN_INVALID |
		INTERNET_FLAG_RELOAD;

	DWORD64 dwContentLength=1;
	WinINetObject hRequest = InternetOpenUrlW(hInet, url.c_str(), nullptr, 0,
		dwOpenRequestFlags, 0);
	if (zurl.nScheme == INTERNET_SCHEME_HTTP
		|| zurl.nScheme == INTERNET_SCHEME_HTTPS) {
		DWORD dwStatusCode = 0;
		DWORD dwSizeLength = sizeof(dwStatusCode);
		if (HttpQueryInfoW(hRequest,
			HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE,
			&dwStatusCode, &dwSizeLength, nullptr)) {
			return false;
		}
		wchar_t szbuf[20];
		dwSizeLength = sizeof(szbuf);
		HttpQueryInfoW(hRequest,HTTP_QUERY_CONTENT_LENGTH, 
			szbuf, &dwSizeLength, nullptr);
		wchar_t *cx;
		dwContentLength = wcstoull(szbuf, &cx, 10);
		fprintf(stderr, "Content-Length: %llu\n", dwContentLength);
	}
	else if(zurl.nScheme==URL_SCHEME_FTP) {
		DWORD highSize=0;
		auto loSize=FtpGetFileSize(hRequest, &highSize);
		dwContentLength = ((DWORD64)highSize << 32)+loSize ;
	}
	//InternetQueryDataAvailable
	if (!hRequest) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"InternetOpenUrlW(): %s", err.message());
		return false;
	}

	// lpszVersion ->nullptr ,use config
	std::wstring tmp = localFile + L".part";
	HANDLE hFile =
		CreateFileW(tmp.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	///
	BYTE fixedsizebuf[16384];
	//DWORD64 rdsize = 0;
	DWORD dwReadSize = 0;
	DWORD dwWriteSize = 0;
	uint64_t rdsize = 0;
	if (callback) {
		callback->impl(0, callback->userdata);
	}
	BOOL result=true;
	do {
		result=InternetReadFile(hRequest, fixedsizebuf, sizeof(fixedsizebuf), &dwReadSize);
		if (!result) {
			ErrorMessage err(GetLastError());
			BaseErrorMessagePrint(L"HttpOpenRequestW(): %s", err.message());
		}
		WriteFile(hFile, fixedsizebuf, dwReadSize, &dwWriteSize, nullptr);
		rdsize += dwReadSize;
		if (callback) {
			callback->impl(rdsize *100/ dwContentLength, callback->userdata);
		}
	} while (result&&dwReadSize);
	if (callback) {
		callback->impl(100, callback->userdata);
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