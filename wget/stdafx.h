// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#include <string>


// TODO:  在此处引用程序需要的其他头文件
struct ProgressCallback {
	void(*impl)(size_t rate, void *userdata);
	void *userdata;
};

bool WinRTDownloadFile(const std::wstring &remoteFile, const std::wstring &localFile);
bool WinHTTPDownloadDriver(const std::wstring &url, const std::wstring &localFile, ProgressCallback *callback);