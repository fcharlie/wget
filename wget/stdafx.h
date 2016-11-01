// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
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
	ErrorMessage(DWORD errid):release_(false){
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
private:
	LPWSTR buf;
	bool release_;
};
int BaseErrorWriteConhost(const wchar_t *buf, size_t len);
int BaseErrorMessagePrint(const wchar_t *format, ...);

// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�
struct ProgressCallback {
	void(*impl)(size_t rate, void *userdata);
	void *userdata;
};

bool WinRTDownloadFile(const std::wstring &remoteFile, const std::wstring &localFile);
bool WinHTTPDownloadDriver(const std::wstring &url, const std::wstring &localFile, ProgressCallback *callback);