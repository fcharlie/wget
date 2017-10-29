
//code by MulinB, 2011-07-27  
  
#include <windows.h>  
#include <wininet.h>  
  
#include <string>  
#include <iostream>  
using namespace std;  
  
#pragma comment(lib, "wininet.lib")  
  
//����  
#define  DOWNHELPER_AGENTNAME         "MyAppByMulinB"  
#define  LEN_OF_BUFFER_FOR_QUERYINFO  128  
#define  DOWNLOAD_BUF_SIZE            (10*1024)  //10KB  
#define  MAX_DOWNLOAD_REQUEST_TIME    10    
#define  MAX_DOWNLOAD_BYTESIZE        (1000*1024*1024) //1000MB  
  
  
BOOL _TryHttpSendRequest(LPVOID hRequest, int nMaxTryTimes); //��η���������  
  
//HTTP���غ�����ͨ��������HEAD�ķ�ʽȻ��GET������ͨ��HEAD�����ص��ļ����ͺʹ�С������  
BOOL DownloadUrl(std::string strUrl, std::string strFileName)  
{  
    BOOL bRet = FALSE;  
    if (strUrl == "" || strFileName == "")  
        return FALSE;  
  
    //�������  
    HINTERNET hInet = NULL; //��internet����handle  
    HINTERNET hConnect = NULL; //HTTP����  
    HINTERNET hRequestHead = NULL; //HTTP Request  
    HINTERNET hRequestGet = NULL; //HTTP Request  
    HANDLE hFileWrite = NULL; //д�ļ��ľ��  
    char* pBuf = NULL; //������  
    DWORD dwRequestTryTimes = MAX_DOWNLOAD_REQUEST_TIME; //��������Ĵ���  
    DWORD dwDownBytes = 0; //ÿ�����صĴ�С  
    DWORD dwDownFileTotalBytes = 0; //���ص��ļ��ܴ�С  
    DWORD dwWriteBytes = 0; //д���ļ��Ĵ�С  
    char bufQueryInfo[LEN_OF_BUFFER_FOR_QUERYINFO] = {0}; //������ѯ��Ϣ��buffer  
    DWORD dwBufQueryInfoSize = sizeof(bufQueryInfo);  
    DWORD dwStatusCode = 0;  
    DWORD dwContentLen = 0;  
    DWORD dwSizeDW = sizeof(DWORD);  
  
    //�ָ�URL  
    CHAR pszHostName[INTERNET_MAX_HOST_NAME_LENGTH] = {0};  
    CHAR pszUserName[INTERNET_MAX_USER_NAME_LENGTH] = {0};  
    CHAR pszPassword[INTERNET_MAX_PASSWORD_LENGTH] = {0};  
    CHAR pszURLPath[INTERNET_MAX_URL_LENGTH] = {0};  
    CHAR szURL[INTERNET_MAX_URL_LENGTH] = {0};  
    URL_COMPONENTSA urlComponents = {0};  
    urlComponents.dwStructSize = sizeof(URL_COMPONENTSA);  
    urlComponents.lpszHostName = pszHostName;  
    urlComponents.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;  
    urlComponents.lpszUserName = pszUserName;  
    urlComponents.dwUserNameLength = INTERNET_MAX_USER_NAME_LENGTH;  
    urlComponents.lpszPassword = pszPassword;  
    urlComponents.dwPasswordLength = INTERNET_MAX_PASSWORD_LENGTH;  
    urlComponents.lpszUrlPath = pszURLPath;  
    urlComponents.dwUrlPathLength = INTERNET_MAX_URL_LENGTH;  
  
    bRet = InternetCrackUrlA(strUrl.c_str(), 0, NULL, &urlComponents);  
    bRet = (bRet && urlComponents.nScheme == INTERNET_SERVICE_HTTP);  
    if (!bRet)  
    {  
        goto _END_OF_DOWNLOADURL;  
    }  
      
    //��һ��internet����  
    hInet = InternetOpenA(DOWNHELPER_AGENTNAME, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, NULL);  
    if (!hInet)  
    {  
        bRet = FALSE;  
        goto _END_OF_DOWNLOADURL;  
    }  
      
    //��HTTP����  
    hConnect = InternetConnectA(hInet, pszHostName, urlComponents.nPort, pszUserName, pszPassword, INTERNET_SERVICE_HTTP, 0, NULL);  
    if (!hConnect)  
    {  
        bRet = FALSE;  
        goto _END_OF_DOWNLOADURL;  
    }  
      
    //����HTTP request���  
    if (urlComponents.dwUrlPathLength !=  0)  
        strcpy(szURL, urlComponents.lpszUrlPath);  
    else  
        strcpy(szURL, "/");  
      
    //����HEAD��ͨ��HEAD����ļ���С�����ͽ���У��  
    hRequestHead = HttpOpenRequestA(hConnect, "HEAD", szURL, "HTTP/1.1", "", NULL, INTERNET_FLAG_RELOAD, 0);  
    bRet = _TryHttpSendRequest(hRequestHead, dwRequestTryTimes);  
    if (!bRet)  
    {  
        goto _END_OF_DOWNLOADURL; //����HEADʧ��  
    }  
     
    //��ѯcontent-length��С  
    dwContentLen = 0;  
    dwSizeDW = sizeof(DWORD);  
    bRet = HttpQueryInfo(hRequestHead, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_CONTENT_LENGTH, &dwContentLen, &dwSizeDW, NULL);  
    if (bRet)  
    {  
        //����Ƿ��ļ�����  
        if (dwContentLen > MAX_DOWNLOAD_BYTESIZE)  
        {  
            bRet = FALSE;  
            goto _END_OF_DOWNLOADURL;  
        }  
    }  
  
    //У����ɺ�������GET�������ļ�  
    hRequestGet = HttpOpenRequestA(hConnect, "GET", szURL, "HTTP/1.1", "", NULL, INTERNET_FLAG_RELOAD, 0);  
    bRet = _TryHttpSendRequest(hRequestGet, dwRequestTryTimes);  
    if (!bRet)  
    {  
        goto _END_OF_DOWNLOADURL; //����HEADʧ��  
    }  
  
    //�����ļ�  
    hFileWrite = CreateFileA(strFileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);  
    if (INVALID_HANDLE_VALUE == hFileWrite)  
    {  
        bRet = FALSE;  
        goto _END_OF_DOWNLOADURL;  
    }  
  
    //���仺��  
    pBuf = new char[DOWNLOAD_BUF_SIZE]; //�����ڴ�  
    if (!pBuf)  
    {  
        bRet = FALSE;  
        goto _END_OF_DOWNLOADURL;  
    }  
  
    //��γ��������ļ�  
    dwDownFileTotalBytes = 0;  
    while (1)  
    {  
        dwDownBytes = 0;  
        memset(pBuf, 0, DOWNLOAD_BUF_SIZE*sizeof(char));  
        bRet = InternetReadFile(hRequestGet, pBuf, DOWNLOAD_BUF_SIZE, &dwDownBytes);  
        if (bRet)  
        {  
            if (dwDownBytes > 0)  
            {  
                dwDownFileTotalBytes += dwDownBytes;  
                bRet = WriteFile(hFileWrite, pBuf, dwDownBytes, &dwWriteBytes, NULL); //д���ļ�  
                if (!bRet)  
                {  
                    goto _END_OF_DOWNLOADURL;  
                }  
            }  
            else if (0 == dwDownBytes)  
            {  
                bRet = TRUE;  
                break; //���سɹ����  
            }  
        }  
    }  
      
    //����  
_END_OF_DOWNLOADURL:  
    if (INVALID_HANDLE_VALUE != hFileWrite)  
        CloseHandle(hFileWrite);  
    if (pBuf)  
        delete [] pBuf;  
    if (hRequestGet)  
        InternetCloseHandle(hRequestGet);  
    if (hRequestHead)  
        InternetCloseHandle(hRequestHead);  
    if (hConnect)  
        InternetCloseHandle(hConnect);  
    if (hInet)  
        InternetCloseHandle(hInet);  
      
    return bRet;  
}  
  
//��η���������  
BOOL _TryHttpSendRequest(LPVOID hRequest, int nMaxTryTimes)  
{  
    BOOL bRet = FALSE;  
    DWORD dwStatusCode = 0;  
    DWORD dwSizeDW = sizeof(DWORD);  
    while (hRequest && (nMaxTryTimes-- > 0)) //��γ��Է�������  
    {  
        //��������  
        bRet = HttpSendRequestA(hRequest, NULL, 0, NULL, 0);  
        if (!bRet)  
        {  
            continue;  
        }  
        else  
        {  
            //�ж�HTTP���ص�״̬��  
            dwStatusCode = 0;  
            dwSizeDW = sizeof(DWORD);  
            bRet = HttpQueryInfo(hRequest, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwStatusCode, &dwSizeDW, NULL);  
            if (bRet)  
            {  
                //���״̬��  
                if (HTTP_STATUS_OK == dwStatusCode) //200 OK  
                {  
                    break;  
                }  
                else  
                {  
                    bRet = FALSE;  
                    continue;  
                }  
            }  
        }  
    }  
  
    return bRet;  
}  
  
  
int main(int argc, char* argv[])  
{  
    cout << "��������...";  
    BOOL bR = DownloadUrl("http://42.duote.com.cn/office2007.zip", "test.zip");  
    if (bR)  
        cout << "���" << endl;  
    else  
        cout << "ʧ��" << endl;  
    return 0;  
}  