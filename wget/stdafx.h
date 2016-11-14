// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <string>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

class ErrorMessage {
public:
	ErrorMessage(DWORD errid):lastError(errid),release_(false){
		if (FormatMessageW(
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_ALLOCATE_BUFFER , nullptr, errid,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
			(LPWSTR)&buf, 0, nullptr)==0) {
			buf = L"Unknown error";
		}
		else {
			release_ = true;
		}
	}
	~ErrorMessage() {
		if (release_) {
			LocalFree(buf);
		}
	}
	const wchar_t *message()const {
		return buf;
	}
	DWORD LastError()const { return lastError; }
private:
	DWORD lastError;
	LPWSTR buf;
	bool release_;
	char reserved[sizeof(intptr_t) - sizeof(bool)];
};

struct ProgressInvoke {
	void(*start)(uint64_t filesize,void *userdata);
	void(*progress)(uint64_t rdsize, float rate, void *userdata);
	void *userdata;
};

struct ProgressCallback {
	void(*impl)(uint64_t rate, void *userdata);
	void *userdata;
};



bool DownloadFileUseWinRT(const std::wstring &remoteFile, const std::wstring &localFile);
bool DownloadFileUseWinHTTP(const std::wstring &url, const std::wstring &localFile, ProgressCallback *callback);
bool DownloadFileUseWininet(const std::wstring &url, const std::wstring &localFile, ProgressCallback *callback);
bool DownloadFileUseBITS(const std::wstring &remoteFile, const std::wstring &localFile);
bool UseDownloadURLToFile(const std::wstring &remoteFile, const std::wstring &localFile);