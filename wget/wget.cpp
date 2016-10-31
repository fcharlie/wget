// wget.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>

bool WinRTDownloadFile(const std::wstring &remoteFile, const std::wstring &localFile);
int wmain(int argc,wchar_t **argv)
{
	if (argc < 3) {
		fwprintf(stderr, L"usage: %s url localFile\n", argv[0]);
		return 1;
	}
	//WinRTDownloadFile(argv[1], argv[2]);
    return 0;
}

