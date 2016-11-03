#include "stdafx.h"
#include <Shlwapi.h>
#include <cppwinrt/base.h>
#include <cppwinrt/Windows.Web.h>
#include <cppwinrt/Windows.Web.Http.h>
#include <cppwinrt/Windows.Web.Http.Filters.h>
#include <cppwinrt/Windows.Storage.Streams.h>

#pragma comment(lib, "windowsapp")
#pragma comment(lib,"Shlwapi")

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Web::Http;
using namespace Windows::Web::Http::Filters;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;


//https://msdn.microsoft.com/en-us/library/windows/apps/xaml/windows.web.http.httpclient.aspx


IAsyncAction DownloadFileInternal(hstring_ref remoteFile,hstring_ref localFile) {
	Uri uri(remoteFile);
	HttpBaseProtocolFilter baseFilter;
	baseFilter.AllowAutoRedirect(true);
	HttpClient client(baseFilter);
	StorageFolder folder=KnownFolders::DocumentsLibrary();
	{
		auto file = co_await folder.CreateFileAsync(localFile);
		auto stream = co_await file.OpenAsync(FileAccessMode::ReadWrite);
		auto result = co_await client.GetAsync(uri);
		auto content = co_await result.Content().WriteToStreamAsync(stream);
	}
}

class RoInitializeWarp {
public:
	RoInitializeWarp() {
		initialize();
	}
	~RoInitializeWarp() {
		uninitialize();
	}
};

bool WinRTDownloadFile(const std::wstring &remoteFile, const std::wstring &localFile) {
	initialize();
	try {
		DownloadFileInternal(remoteFile.c_str(), localFile.c_str()).get();
	}
	catch (hresult_error const  &e) {
		printf("Error: %ls\n", e.message().c_str());
		uninitialize();
		return false;
	}
	catch (...) {
		return false;
	}
	uninitialize();
	return true;
}