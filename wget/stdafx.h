// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include <string>


// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�
struct ProgressCallback {
	void(*impl)(size_t rate, void *userdata);
	void *userdata;
};

bool WinRTDownloadFile(const std::wstring &remoteFile, const std::wstring &localFile);
bool WinHTTPDownloadDriver(const std::wstring &url, const std::wstring &localFile, ProgressCallback *callback);