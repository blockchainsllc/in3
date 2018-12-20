#include <windows.h>
#include <stdio.h>
#include "winhttp.h"

BOOL WinHttpSamplePost(LPCWSTR szServerUrl, LPCWSTR szFile, LPCWSTR szProxyUrl);

int main(char *url, char *payload)
{
    LPSTR pszData = "WinHttpWriteData Example";
    DWORD dwBytesWritten = 0;
    BOOL bResults      = FALSE;
    HINTERNET hSession = NULL,
              hConnect = NULL,
              hRequest = NULL;

    // Use WinHttpOpen to obtain a session handle.
    hSession = WinHttpOpen(L"in3/1.0",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME,
                           WINHTTP_NO_PROXY_BYPASS, 0);

    // Specify an HTTP server.
    if (hSession)
        hConnect = WinHttpConnect(hSession,L url,
                                  INTERNET_DEFAULT_HTTP_PORT, 0);

    // Create an HTTP Request handle.
    if (hConnect)
        hRequest = WinHttpOpenRequest(hConnect, L"POST",
                                      L"/",
                                      NULL, WINHTTP_NO_REFERER,
                                      WINHTTP_DEFAULT_ACCEPT_TYPES,
                                      0);

    // Send a Request.
    if (hRequest)
        bResults = WinHttpSendRequest(hRequest,
                                      WINHTTP_NO_ADDITIONAL_HEADERS,
                                      0, WINHTTP_NO_REQUEST_DATA, 0,
                                      (DWORD)strlen(pszData), 0);

    // Write data to the server.
    if (bResults)
        bResults = WinHttpWriteData(hRequest, pszData,
                                    (DWORD)strlen(pszData),
                                    &amp;
                                    dwBytesWritten);

    // End the request.
    if (bResults)
        bResults = WinHttpReceiveResponse(hRequest, NULL);

    // Report any errors.
    if (!bResults)
        printf("Error %d has occurred.\n", GetLastError());

    // Close any open handles.
    if (hRequest)
        WinHttpCloseHandle(hRequest);
    if (hConnect)
        WinHttpCloseHandle(hConnect);
    if (hSession)
        WinHttpCloseHandle(hSession);
}