#include "stdafx.h"
#include <Shlwapi.h>
#include <winrt/base.h>
#include <winrt/Windows.Web.h>
#include <winrt/Windows.Web.Http.h>
#include <winrt/Windows.Web.Http.Filters.h>
#include <winrt/Windows.Storage.Streams.h>

#pragma comment(lib, "windowsapp")
#pragma comment(lib,"Shlwapi")

using namespace winrt;
using namespace Windows::Foundation;
using namespace Windows::Web::Http;
using namespace Windows::Web::Http::Filters;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;


//https://msdn.microsoft.com/en-us/library/windows/apps/xaml/windows.web.http.httpclient.aspx


IAsyncAction DownloadFileInternal(const wchar_t *remoteFile, const wchar_t* localdir,const std::wstring & name) {
	Uri uri(remoteFile);
	HttpBaseProtocolFilter baseFilter;
	baseFilter.AllowAutoRedirect(true);
	HttpClient client(baseFilter);
	StorageFolder folder = co_await StorageFolder::GetFolderFromPathAsync(localdir);
	auto file = co_await folder.CreateFileAsync(name.data());
	auto stream = co_await file.OpenAsync(FileAccessMode::ReadWrite);
	auto result = co_await client.GetAsync(uri);
	auto content = co_await result.Content().WriteToStreamAsync(stream);
}

bool DownloadFileUseWinRT(const std::wstring &remoteFile, const std::wstring &localFile) {
	

	return false;
}