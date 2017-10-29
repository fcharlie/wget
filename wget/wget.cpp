// wget.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>
#include "console.h"

inline bool IsArg(const wchar_t *candidate, const wchar_t *longname) {
	return (wcscmp(candidate, longname) == 0);
}

inline bool IsArg(const wchar_t *candidate, const wchar_t *shortname,
	const wchar_t *longname) {
	if (wcscmp(candidate, shortname) == 0 ||
		(longname != nullptr && wcscmp(candidate, longname) == 0))
		return true;
	return false;
}

inline int SearchIndex(const wchar_t *str, int ch) {
	auto iter = str;
	for (; *iter; iter++) {
		if (*iter == ch) {
			return (iter - str);
		}
	}
	return -1;
}

inline bool IsArg(const wchar_t *candidate, const wchar_t *shortname,
	const wchar_t *longname, const wchar_t **av) {
	auto i = SearchIndex(candidate, '=');
	if (i == -1) {
		if (wcscmp(candidate, shortname) == 0 || wcscmp(candidate, longname) == 0) {
			*av = nullptr;
			return true;
		}
		return false;
	}
	if (i < 2)
		return false;
	//
	if (shortname) {
		auto sl = wcslen(shortname);
		if (sl == (decltype(sl))i && wcsncmp(candidate, shortname, sl) == 0) {
			*av = candidate + i + 1;
			return true;
		}
	}
	if (longname) {
		auto ll = wcslen(longname);
		if (ll == (decltype(ll))i && wcsncmp(candidate, longname, ll) == 0) {
			*av = candidate + i + 1;
			return true;
		}
	}
	return false;
}

bool RemoteFileSplitName(const std::wstring &url,std::wstring &file) {
	auto np = url.rfind('/');
	if (np == url.npos)
		return false;
	auto xnp = url.find('?', np);
	if (xnp == url.npos) {
		file.assign(url.substr(np + 1));
	}
	else {
		file.assign(url.substr(np + 1, xnp - np-1));
	}
	return true;
}

void ProgressStatus(uint64_t rate, void *userdata) {
	(void)userdata;
	printf("\rDownload: %lld%%", rate);
}
//#define FKG_FORCED_USAGE
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
	//DownloadFileUseWinRT(argv[1], argv[2]);
	ProgressCallback callback = { ProgressStatus,nullptr };
	//DownloadFileUseWininet(argv[1], argv[2], &callback);
	DownloadFileUseWinHTTP(argv[1], argv[2], &callback);
	//DownloadFileUseBITS(argv[1], argv[2]);
    return 0;
}

