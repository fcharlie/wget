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

class HttpHeader{
public:
	~HttpHeader() {
		if (buf) {
			delete[] buf;
		}
	}
	bool Query(HINTERNET hRequest) {
		DWORD dwHeader = 0;
		WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
			WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwHeader,
			WINHTTP_NO_HEADER_INDEX);
		buf = new wchar_t[dwHeader + 1];
		if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
			WINHTTP_HEADER_NAME_BY_INDEX, buf, &dwHeader,
			WINHTTP_NO_HEADER_INDEX)) {
			return false;
		}
		///
		return ParseHeader(buf,dwHeader);
	}
	DWORD StatusCode()const {
		return dwStatusCode;
	}
	uint64_t ContentLength()const {
		return contentLength;
	}
private:
	bool ParseHeader(const wchar_t *data, size_t len) {
		if (len < 16)
			return false; // invalid
		if (wmemcmp(data, L"HTTP/", sizeof("HTTP/") - 1) != 0)
			return false;
		auto iter = data + sizeof("HTTP/") - 1;
		auto end = data + len;
		if (*iter < '0' || *iter > '9' || iter[1] != '.' || iter[2] < '0' ||
			iter[2] > '9') {
			/// invalid http version
			return false;
		}
		//httpversion_ = ((iter[0] - '0') << 4) + iter[2] - '0';
		iter += 3;
		wchar_t *c;
		dwStatusCode = wcstol(iter, &c, 10);
		if (dwStatusCode != 0 && c) {
			iter = c;
			iter++;
			for (; iter < end; iter++) {
				if (*iter == '\r')
					continue;
				else if (*iter == '\n')
					break;
				else
					continue;
					//statusline_.push_back(*iter);
			}
		}
		else {
			return false;
		}
		std::pair<std::wstring, std::wstring> hl;
		bool left = false;
		for (; iter != end; iter++) {
			switch (*iter) {
			case ':':
				if (left) {
					hl.second.push_back(':');
					continue;
				}
				left = true;
				if (iter + 1 < end)
					iter++;
				break;
			case '\r':
				break;
			case '\n':
				if (hl.first.size() && hl.second.size()) {
					if (_wcsicmp(hl.first.c_str(), L"Content-Length") == 0) {
						wchar_t *cx;
						contentLength = wcstoull(hl.second.c_str(), &cx, 10);
						return true;
					}
					hl.first.clear();
					hl.second.clear();
				}
				else {
					hl.first.clear();
					hl.second.clear();
				}
				left = false;
				break;
			default:
				if (left) {
					hl.second.push_back(*iter);
				}
				else {
					hl.first.push_back(*iter);
				}
				break;
			}
		}
		return true;
	}
	wchar_t *buf{ nullptr };
	size_t textlen;
	size_t buflen;
	DWORD dwStatusCode;
	uint64_t contentLength{ 0 };
};


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
	HINTERNET Raw()const {
		return hInternet_;
	}
private:
	HINTERNET hInternet_;
};

bool DownloadFileUseWinHTTP(const std::wstring &url, const std::wstring &localFile,ProgressCallback *callback) {
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
	HttpHeader header;
	if (!header.Query(hRequest.Raw())) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"Query Header failed: %s", err.message());
		return false;
	}
	if (header.StatusCode() != 200 && header.StatusCode() != 201) {
		BaseErrorMessagePrint(L"StatusCode: %d",header.StatusCode());
		return false;
	}
	wprintf(L"File size: %lld\n", header.ContentLength());
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
		if (header.ContentLength() > 0) {
			if (callback) {
				callback->impl(total * 100 / header.ContentLength(), callback->userdata);
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