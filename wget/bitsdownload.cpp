#include "stdafx.h"
#include <string>
#include <bits.h>
#include <Shlwapi.h>
//#include <bits5_0.h>
#include "console.h"

#define ARRAY_LENGTH(x) (sizeof(x) / sizeof( *(x) ))


/**
* Definition of constants
*/
static const unsigned int HALF_SECOND_AS_MILLISECONDS = 500;
static const unsigned int TWO_SECOND_LOOP = 2000 / HALF_SECOND_AS_MILLISECONDS;


#pragma comment(lib,"Bits")

class MzCoInitilaize {
public:
	MzCoInitilaize() {
		CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	}
	~MzCoInitilaize()
	{
		CoUninitialize();
	}
};

/**
* Parses the provided string containing HTTP headers,
* splits them apart and displays them to the user.
*/
VOID
DisplayHeaders(_In_ LPWSTR Headers)
{
	printf("Headers: %ws\n", Headers);
}


VOID
DisplayError(_In_ IBackgroundCopyJob *Job)
{
	HRESULT hr;
	IBackgroundCopyError *Error;
	LPWSTR ErrorDescription;

	hr = Job->GetError(&Error);
	if (FAILED(hr))
	{
		printf("WARNING: Error details are not available.\n");
	}
	else
	{
		hr = Error->GetErrorDescription(LANGIDFROMLCID(GetThreadLocale()), &ErrorDescription);
		if (SUCCEEDED(hr))
		{
			printf("   Error details: %ws\n", ErrorDescription);
			CoTaskMemFree(ErrorDescription);
		}
		Error->Release();
	}
}


/**
* For each file in the job, obtains the (final) HTTP headers received from the
* remote server that hosts the files and then displays the HTTP headers.
*/
HRESULT
DisplayFileHeaders(_In_ IBackgroundCopyJob *Job)
{
	HRESULT hr;
	IEnumBackgroundCopyFiles *FileEnumerator;

	printf("Individual file information.\n");

	hr = Job->EnumFiles(&FileEnumerator);
	if (FAILED(hr))
	{
		printf("WARNING: Unable to obtain an IEnumBackgroundCopyFiles interface.\n");
		printf("         No further information can be provided about the files in the job.\n");
	}
	else
	{
		ULONG Count;

		hr = FileEnumerator->GetCount(&Count);
		if (FAILED(hr))
		{
			printf("WARNING: Unable to obtain a count of the number of files in the job.\n");
			printf("        No further information can be provided about the files in the job.\n");
		}
		else
		{
			for (ULONG i = 0; i < Count; ++i)
			{
				IBackgroundCopyFile *TempFile;

				hr = FileEnumerator->Next(1, &TempFile, NULL);
				if (FAILED(hr))
				{
					printf("WARNING: Unable to obtain an IBackgroundCopyFile interface for the next file in the job.\n");
					printf("        No further information can be provided about this file.\n");
				}
				else
				{
					IBackgroundCopyFile5 *File;
					hr = TempFile->QueryInterface(__uuidof(IBackgroundCopyFile5), (void **)&File);
					if (FAILED(hr))
					{
						printf("WARNING: Unable to obtain an IBackgroundCopyFile5 interface for the file.\n");
						printf("        No further information can be provided about this file.\n");
					}
					else
					{
						LPWSTR RemoteFileName;
						hr = File->GetRemoteName(&RemoteFileName);
						if (FAILED(hr))
						{
							printf("WARNING: Unable to obtain the remote file name for this file.\n");
						}
						else
						{
							printf("HTTP headers for remote file '%ws'\n", RemoteFileName);
							CoTaskMemFree(RemoteFileName);
							RemoteFileName = NULL;
						}

						BITS_FILE_PROPERTY_VALUE Value;
						hr = File->GetProperty(
							BITS_FILE_PROPERTY_ID_HTTP_RESPONSE_HEADERS,
							&Value
						);
						if (FAILED(hr))
						{
							printf("WARNING: Unable to obtain the HTTP headers for this file.\n");
						}
						else
						{
							if (Value.String)
							{
								DisplayHeaders(Value.String);
								CoTaskMemFree(Value.String);
								Value.String = NULL;
							}
						}

						File->Release();
						File = NULL;
					}

					TempFile->Release();
					TempFile = NULL;
				}
			}
		}

		FileEnumerator->Release();
		FileEnumerator = NULL;
	}

	return S_OK;
}


/**
* Displays the current progress of the job in terms of the amount of data
* and number of files transferred.
*/
VOID
DisplayProgress(_In_ IBackgroundCopyJob *Job)
{
	HRESULT hr;
	BG_JOB_PROGRESS Progress;

	hr = Job->GetProgress(&Progress);
	if (SUCCEEDED(hr))
	{
		printf(
			"%llu of %llu bytes transferred (%lu of %lu files).\n",
			Progress.BytesTransferred, Progress.BytesTotal,
			Progress.FilesTransferred, Progress.FilesTotal
		);
	}
	else
	{
		printf("ERROR: Unable to get job progress (error code %08X).\n", hr);
	}
}



HRESULT
MonitorJobProgress(_In_ IBackgroundCopyJob *Job)
{
	HRESULT hr;
	LPWSTR JobName;
	BG_JOB_STATE State;
	int PreviousState = -1;
	bool Exit = false;
	int ProgressCounter = 0;

	hr = Job->GetDisplayName(&JobName);
	printf("Progress report for download job '%ws'.\n", JobName);

	// display the download progress
	while (!Exit)
	{
		hr = Job->GetState(&State);

		if (State != PreviousState)
		{

			switch (State)
			{
			case BG_JOB_STATE_QUEUED:
				printf("Job is in the queue and waiting to run.\n");
				break;

			case BG_JOB_STATE_CONNECTING:
				printf("BITS is trying to connect to the remote server.\n");
				break;

			case BG_JOB_STATE_TRANSFERRING:
				printf("BITS has started downloading data.\n");
				DisplayProgress(Job);
				break;

			case BG_JOB_STATE_ERROR:
				printf("ERROR: BITS has encountered a non-recoverable error (error code %08X).\n", GetLastError());
				printf("       Exiting job.\n");
				Exit = true;
				break;

			case BG_JOB_STATE_TRANSIENT_ERROR:
				printf("ERROR: BITS has encountered a recoverable error.\n");
				DisplayError(Job);
				printf("       Continuing to retry.\n");
				break;

			case BG_JOB_STATE_TRANSFERRED:
				DisplayProgress(Job);
				printf("The job has been successfully completed.\n");
				printf("Finalizing local files.\n");
				Job->Complete();
				break;

			case BG_JOB_STATE_ACKNOWLEDGED:
				printf("Finalization complete.\n");
				Exit = true;
				break;

			case BG_JOB_STATE_CANCELLED:
				printf("WARNING: The job has been cancelled.\n");
				Exit = true;
				break;

			default:
				printf("WARNING: Unknown BITS state %d.\n", State);
				Exit = true;
			}

			PreviousState = State;
		}

		else if (State == BG_JOB_STATE_TRANSFERRING)
		{
			// display job progress every 2 seconds
			if (++ProgressCounter % TWO_SECOND_LOOP == 0)
			{
				DisplayProgress(Job);
			}
		}

		Sleep(HALF_SECOND_AS_MILLISECONDS);
	}

	printf("\n");

	if (SUCCEEDED(hr))
	{
		hr = DisplayFileHeaders(Job);
	}

	return hr;
}

bool DownloadFileUseBITS(const std::wstring &remoteFile, const std::wstring &localFile) {
	HRESULT hr=S_OK;
	IBackgroundCopyManager* manager = NULL;
	IBackgroundCopyJob* Job = NULL;
	//IBackgroundCopyJob5* pBackgroundCopyJob5=NULL;
	//BITS_JOB_PROPERTY_VALUE propval;
	bool result = true;
	GUID JobId;
	MzCoInitilaize mzCoInitialzie;
	hr = CoInitializeSecurity(NULL, -1, NULL, NULL,
		RPC_C_AUTHN_LEVEL_CONNECT,
		RPC_C_IMP_LEVEL_IMPERSONATE,
		NULL, EOAC_NONE, 0);
	if (FAILED(hr)) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"CoInitializeSecurity: %s", err.message());
		return false;
	}
	hr = CoCreateInstance(__uuidof(BackgroundCopyManager), NULL,
		CLSCTX_LOCAL_SERVER,
		__uuidof(IBackgroundCopyManager),
		(void**)&manager);
	if (!SUCCEEDED(hr))
	{
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"CoCreateInstance: %s", err.message());
		return false;
	}
	hr = manager->CreateJob(L"WindowsBitsGet", 
		BG_JOB_TYPE_DOWNLOAD, &JobId, &Job);
	if (!SUCCEEDED(hr)) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"CreateJob: %s", err.message());
		manager->Release();
		return false;
	}
	std::wstring _localFile(localFile);
	if (PathIsRelativeW(localFile.c_str())) {
		_localFile.reserve(0x8000);
		DWORD dwSize = 0;
		if ((dwSize=GetCurrentDirectoryW(32767, &_localFile[0]))==0) {
			ErrorMessage err(GetLastError());
			BaseErrorMessagePrint(L"GetCurrentDirectoryW: %s", err.message());
			result = false;
			goto Cleanup;
		}
		else if (dwSize > 0x8000 - 2 - localFile.size()) {
			BaseErrorMessagePrint(L"Over Length Path\n");
			result = false;
			goto Cleanup;
		}
		LPWSTR buf = &_localFile[0] + dwSize;
		buf[0] = '\\';
		wcscpy_s(buf+1, 0x8000 - dwSize - 1, localFile.c_str());
	}
	else {
		(void)_localFile.c_str();
	}
	hr = Job->AddFile(remoteFile.c_str(), &_localFile[0]);
	if (FAILED(hr)) {
		switch (hr) {
		case BG_E_TOO_MANY_FILES:
			fprintf(stderr, "BG_E_TOO_MANY_FILES");
			break;
		case BG_E_TOO_MANY_FILES_IN_JOB:
			fprintf(stderr, "BG_E_TOO_MANY_FILES_IN_JOB");
			break;
		case E_INVALIDARG:
			fprintf(stderr, "E_INVALIDARG");
			break;
		case E_ACCESSDENIED:
			fprintf(stderr, "E_ACCESSDENIED");
			break;
		}
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"LastError: %d, AddFile: %s URL: %s", err.LastError(),err.message(),remoteFile.c_str());
		result = false;
		goto Cleanup;
	}
	hr = Job->Resume();
	if (FAILED(hr)) {
		ErrorMessage err(GetLastError());
		BaseErrorMessagePrint(L"AddFile: %s", err.message());
		result = false;
		goto Cleanup;
	}
	MonitorJobProgress(Job);
Cleanup:
	Job->Release();
	manager->Release();
	return result;
}