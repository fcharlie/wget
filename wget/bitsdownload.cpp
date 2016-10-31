#include "stdafx.h"
#include <string>
#include <bits.h>
#include <bits5_0.h>

// IBackgroundCopyJob4 IBackgroundCopyJob5  

bool BITSDownloadFile(const std::wstring &remoteFile, const std::wstring &localFile) {
	HRESULT hr;
	GUID guidJob;
	IBackgroundCopyJob5* pBackgroundCopyJob5;
	IBackgroundCopyJob* pBackgroundCopyJob;
	IBackgroundCopyManager* pQueueMgr;
	BITS_JOB_PROPERTY_VALUE propval;

	return true;
}