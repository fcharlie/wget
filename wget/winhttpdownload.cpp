#include "stdafx.h"
#include <Shlwapi.h>
#include <windows.h>
#include <winhttp.h>
#include <string>
#include <algorithm>
#include <unordered_map>
#include "console.h"

#pragma comment(lib,"WinHTTP")

#define MinWarp(a,b) ((b<a)?b:a)


struct RequestURL {
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

		if (!WinHttpCrackUrl(url.data(), dwUrlLen, 0, &urlComp)) {
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


#define DEFAULT_USERAGENT L"WindowsGet/1.0"

static bool
ParseHeader(wchar_t *begin, size_t length,
	std::unordered_map<std::wstring, std::wstring> &headers) {
	std::wstring header(begin, length);
	auto np = header.find(L"\r\n");
	size_t offset = 0;
	while (np != std::wstring::npos) {
		auto sp = header.find(L':', offset);
		if (sp != std::wstring::npos && sp < np && sp > offset) {
			headers[header.substr(offset, sp - offset)] =
				header.substr(sp + 2, np - sp - 2);
		}
		offset = np + 2;
		np = header.find(L"\r\n", offset);
	}
	return true;
}

class InternetObject {
public:
	InternetObject(HINTERNET hInternet):hInternet_(hInternet) {
	}
	operator HINTERNET() {
		return hInternet_;
	}
	operator bool() {
		return hInternet_ != nullptr;
	}
	InternetObject() {
		if (hInternet_) {
			WinHttpCloseHandle(hInternet_);
		}
	}
private:
	HINTERNET hInternet_;
};

bool WinHTTPDownloadDriver(const std::wstring &url, const std::wstring &localFile,ProgressCallback *callback) {
	RequestURL zurl;
	if (!zurl.Parse(url)) {
		BaseErrorMessagePrint(L"Wrong URL: %s\n",url.c_str());
		return false;
	}
	InternetObject hInternet =
		WinHttpOpen(DEFAULT_USERAGENT, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
			WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hInternet) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"WinHttpOpen(): %s", err.message());
		return false;
	}
	//	WinHttpSetOption(hInternet, WINHTTP_OPTION_REDIRECT_POLICY, &dwOption,sizeof(DWORD));
	/// https://msdn.microsoft.com/en-us/library/windows/desktop/aa384066(v=vs.85).aspx
	/// WINHTTP_PROTOCOL_FLAG_HTTP2
	DWORD dwOption = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
	if (!WinHttpSetOption(hInternet, WINHTTP_OPTION_REDIRECT_POLICY,
		&dwOption, sizeof(DWORD))) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"WINHTTP_OPTION_REDIRECT_POLICY: %s", err.message());
	}
	dwOption = WINHTTP_PROTOCOL_FLAG_HTTP2;
	if (!WinHttpSetOption(hInternet, WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL, &dwOption, sizeof(dwOption))) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"WINHTTP_OPTION_ENABLE_HTTP_PROTOCOL: %s", err.message());
	}
	InternetObject hConnect = WinHttpConnect(hInternet, zurl.host.c_str(),
		(INTERNET_PORT)zurl.nPort, 0);
	if (!hConnect) {
		BaseErrorMessagePrint(L"Server unable to connect: %s", zurl.host.c_str());
		return false;
	}
	DWORD dwOpenRequestFlag =
		(zurl.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0;
	InternetObject hRequest = WinHttpOpenRequest(
		hConnect, L"GET", zurl.path.c_str(), nullptr, WINHTTP_NO_REFERER,
		WINHTTP_DEFAULT_ACCEPT_TYPES, dwOpenRequestFlag);
	if (!hRequest) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"Open Request failed: %s", err.message());
		return false;
	}
	if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		WINHTTP_NO_REQUEST_DATA, 0, 0, 0) == FALSE) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"Send Request failed: %s", err.message());
		return false;
	}
	if (WinHttpReceiveResponse(hRequest, NULL) == FALSE) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"Receive Response failed: %s", err.message());
		return false;
	}
	DWORD dwHeader = 0;
	BOOL bResult = FALSE;
	bResult = ::WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
		WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwHeader,
		WINHTTP_NO_HEADER_INDEX);
	std::wstring m_header;
	m_header.reserve(dwHeader + 1);
	uint64_t contentLength = 0;
	::WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
		WINHTTP_HEADER_NAME_BY_INDEX, &m_header[0], &dwHeader,
		WINHTTP_NO_HEADER_INDEX);
	std::unordered_map<std::wstring, std::wstring> headers;
	ParseHeader(&m_header[0], dwHeader, headers);
	contentLength =
		static_cast<uint64_t>(_wtoll(headers[L"Content-Length"].c_str()));
	std::wstring tmp = localFile + L".part";
	HANDLE hFile =
		CreateFileW(tmp.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ,
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	///
	uint64_t total = 0;
	DWORD dwSize = 0;
	char fixedsizebuf[16384];
	///
	if (callback) {
		callback->impl(0, callback->userdata);
	}
	do {
		// Check for available data.
		if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
			break;
		}
		total += dwSize;
		if (contentLength > 0) {
			if (callback) {
				callback->impl(total * 100 / contentLength, callback->userdata);
			}
		}
		auto dwSizeN = dwSize;
		while (dwSizeN > 0) {
			DWORD dwDownloaded = 0;
			if (!WinHttpReadData(hRequest, (LPVOID)fixedsizebuf, MinWarp(sizeof(fixedsizebuf), dwSizeN), &dwDownloaded)) {
				break;
			}
			dwSizeN = dwSizeN - dwDownloaded;
			DWORD wmWritten;
			WriteFile(hFile, fixedsizebuf, dwSize, &wmWritten, NULL);
		}
	} while (dwSize > 0);
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