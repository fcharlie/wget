#include "stdafx.h"
#include <string>
#include <bits.h>
#include <bits5_0.h>

#pragma comment(lib,"Bits")

// IBackgroundCopyJob4 IBackgroundCopyJob5  

bool BITSDownloadFile(const std::wstring &remoteFile, const std::wstring &localFile) {
	HRESULT hr;
	IBackgroundCopyManager* pbcm = NULL;
	IBackgroundCopyJob* pJob = NULL;
	IBackgroundCopyJob5* pJob5 = NULL;
	GUID JobId;
	hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (hr != S_OK) {

	}
	hr = CoCreateInstance(CLSID_BackgroundCopyManager5_0, NULL,
		CLSCTX_LOCAL_SERVER,
		IID_IBackgroundCopyManager,
		(void**)&pbcm);
	if (SUCCEEDED(hr))
	{
		//BITS 5.0 is installed.
	}
	hr = pbcm->CreateJob(L"WindowsGet", BG_JOB_TYPE_DOWNLOAD, &JobId, &pJob);
	if (SUCCEEDED(hr))
	{
		//Save the JobId for later reference. 
		//Modify the job's property values.
		//Add files to the job.
		//Activate (resume) the job in the transfer queue.
	}

	hr = pJob->QueryInterface(__uuidof(IBackgroundCopyJob4), (void**)&pJob5);
	pJob->Release();
	if (FAILED(hr))
	{
		wprintf(L"pJob->QueryInterface failed with 0x%x.\n", hr);
	}
	hr = pJob5->AddFile(remoteFile.c_str(),localFile.c_str());
	if (SUCCEEDED(hr))
	{
		//Do something.
	}
	if (pbcm)
	{
		pbcm->Release();
		pbcm = NULL;
	}
	CoUninitialize();
	return true;
}