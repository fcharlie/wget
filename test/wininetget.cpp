
//code by MulinB, 2011-07-27  
  
#include <windows.h>  
#include <wininet.h>  
  
#include <string>  
#include <iostream>  
using namespace std;  
  
#pragma comment(lib, "wininet.lib")  
  
//下载  
#define  DOWNHELPER_AGENTNAME         "MyAppByMulinB"  
#define  LEN_OF_BUFFER_FOR_QUERYINFO  128  
#define  DOWNLOAD_BUF_SIZE            (10*1024)  //10KB  
#define  MAX_DOWNLOAD_REQUEST_TIME    10    
#define  MAX_DOWNLOAD_BYTESIZE        (1000*1024*1024) //1000MB  
  
  
BOOL _TryHttpSendRequest(LPVOID hRequest, int nMaxTryTimes); //多次发送请求函数  
  
//HTTP下载函数，通过先请求HEAD的方式然后GET，可以通过HEAD对下载的文件类型和大小做限制  
BOOL DownloadUrl(std::string strUrl, std::string strFileName)  
{  
    BOOL bRet = FALSE;  
    if (strUrl == "" || strFileName == "")  
        return FALSE;  
  
    //定义变量  
    HINTERNET hInet = NULL; //打开internet连接handle  
    HINTERNET hConnect = NULL; //HTTP连接  
    HINTERNET hRequestHead = NULL; //HTTP Request  
    HINTERNET hRequestGet = NULL; //HTTP Request  
    HANDLE hFileWrite = NULL; //写文件的句柄  
    char* pBuf = NULL; //缓冲区  
    DWORD dwRequestTryTimes = MAX_DOWNLOAD_REQUEST_TIME; //尝试请求的次数  
    DWORD dwDownBytes = 0; //每次下载的大小  
    DWORD dwDownFileTotalBytes = 0; //下载的文件总大小  
    DWORD dwWriteBytes = 0; //写入文件的大小  
    char bufQueryInfo[LEN_OF_BUFFER_FOR_QUERYINFO] = {0}; //用来查询信息的buffer  
    DWORD dwBufQueryInfoSize = sizeof(bufQueryInfo);  
    DWORD dwStatusCode = 0;  
    DWORD dwContentLen = 0;  
    DWORD dwSizeDW = sizeof(DWORD);  
  
    //分割URL  
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
      
    //打开一个internet连接  
    hInet = InternetOpenA(DOWNHELPER_AGENTNAME, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, NULL);  
    if (!hInet)  
    {  
        bRet = FALSE;  
        goto _END_OF_DOWNLOADURL;  
    }  
      
    //打开HTTP连接  
    hConnect = InternetConnectA(hInet, pszHostName, urlComponents.nPort, pszUserName, pszPassword, INTERNET_SERVICE_HTTP, 0, NULL);  
    if (!hConnect)  
    {  
        bRet = FALSE;  
        goto _END_OF_DOWNLOADURL;  
    }  
      
    //创建HTTP request句柄  
    if (urlComponents.dwUrlPathLength !=  0)  
        strcpy(szURL, urlComponents.lpszUrlPath);  
    else  
        strcpy(szURL, "/");  
      
    //请求HEAD，通过HEAD获得文件大小及类型进行校验  
    hRequestHead = HttpOpenRequestA(hConnect, "HEAD", szURL, "HTTP/1.1", "", NULL, INTERNET_FLAG_RELOAD, 0);  
    bRet = _TryHttpSendRequest(hRequestHead, dwRequestTryTimes);  
    if (!bRet)  
    {  
        goto _END_OF_DOWNLOADURL; //请求HEAD失败  
    }  
     
    //查询content-length大小  
    dwContentLen = 0;  
    dwSizeDW = sizeof(DWORD);  
    bRet = HttpQueryInfo(hRequestHead, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_CONTENT_LENGTH, &dwContentLen, &dwSizeDW, NULL);  
    if (bRet)  
    {  
        //检查是否文件过大  
        if (dwContentLen > MAX_DOWNLOAD_BYTESIZE)  
        {  
            bRet = FALSE;  
            goto _END_OF_DOWNLOADURL;  
        }  
    }  
  
    //校验完成后再请求GET，下载文件  
    hRequestGet = HttpOpenRequestA(hConnect, "GET", szURL, "HTTP/1.1", "", NULL, INTERNET_FLAG_RELOAD, 0);  
    bRet = _TryHttpSendRequest(hRequestGet, dwRequestTryTimes);  
    if (!bRet)  
    {  
        goto _END_OF_DOWNLOADURL; //请求HEAD失败  
    }  
  
    //创建文件  
    hFileWrite = CreateFileA(strFileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);  
    if (INVALID_HANDLE_VALUE == hFileWrite)  
    {  
        bRet = FALSE;  
        goto _END_OF_DOWNLOADURL;  
    }  
  
    //分配缓冲  
    pBuf = new char[DOWNLOAD_BUF_SIZE]; //分配内存  
    if (!pBuf)  
    {  
        bRet = FALSE;  
        goto _END_OF_DOWNLOADURL;  
    }  
  
    //多次尝试下载文件  
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
                bRet = WriteFile(hFileWrite, pBuf, dwDownBytes, &dwWriteBytes, NULL); //写入文件  
                if (!bRet)  
                {  
                    goto _END_OF_DOWNLOADURL;  
                }  
            }  
            else if (0 == dwDownBytes)  
            {  
                bRet = TRUE;  
                break; //下载成功完成  
            }  
        }  
    }  
      
    //清理  
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
  
//多次发送请求函数  
BOOL _TryHttpSendRequest(LPVOID hRequest, int nMaxTryTimes)  
{  
    BOOL bRet = FALSE;  
    DWORD dwStatusCode = 0;  
    DWORD dwSizeDW = sizeof(DWORD);  
    while (hRequest && (nMaxTryTimes-- > 0)) //多次尝试发送请求  
    {  
        //发送请求  
        bRet = HttpSendRequestA(hRequest, NULL, 0, NULL, 0);  
        if (!bRet)  
        {  
            continue;  
        }  
        else  
        {  
            //判断HTTP返回的状态码  
            dwStatusCode = 0;  
            dwSizeDW = sizeof(DWORD);  
            bRet = HttpQueryInfo(hRequest, HTTP_QUERY_FLAG_NUMBER | HTTP_QUERY_STATUS_CODE, &dwStatusCode, &dwSizeDW, NULL);  
            if (bRet)  
            {  
                //检查状态码  
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
    cout << "正在下载...";  
    BOOL bR = DownloadUrl("http://42.duote.com.cn/office2007.zip", "test.zip");  
    if (bR)  
        cout << "完成" << endl;  
    else  
        cout << "失败" << endl;  
    return 0;  
}  