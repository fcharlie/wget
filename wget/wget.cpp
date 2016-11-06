// wget.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>
#include "console.h"


void ProgressStatus(size_t rate, void *userdata) {
	(void)userdata;
	printf("\rDownload: %d%%", rate);
}

int wmain(int argc,wchar_t **argv)
{
	//BaseErrorMessagePrint(L"\xD83C\xDD9A\n");
	if (argc < 2) {
		fwprintf(stderr, L"usage: %s url localFile\n", argv[0]);
		return 1;
	}
	if (argc < 3) {
		return 1;
	}
	// Parse commandline
	//WinRTDownloadFile(argv[1], argv[2]);
	ProgressCallback callback = { ProgressStatus,nullptr };
	WinHTTPDownloadDriver(argv[1], argv[2], &callback);
    return 0;
}

