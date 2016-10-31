#include "stdafx.h"
#include <Windows.h>
#include <WinInet.h>

#pragma comment(lib, "WinInet.lib")

bool WinINetDownloadDriver(const std::wstring &url, const std::wstring &localFile, ProgressCallback *callback) {
	return true;
}