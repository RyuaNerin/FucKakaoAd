#include "latest_release.h"

#include <winhttp.h>
#pragma comment(lib, "Winhttp.lib")
#include <Windows.h>
#pragma comment(lib, "Mincore.lib") // VerQueryValueW

#include <string>
#include <sstream>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "defer.h"

#define USER_AGENT      L"FucKakaoAd"

std::string getCurrentVersion(HINSTANCE hInstance);
bool getHttp(LPCWSTR host, LPCWSTR path, std::string &body);

bool needToUpdate(HINSTANCE hInstance)
{
    std::string currentVersion = getCurrentVersion(hInstance);

    std::string body;
    if (getHttp(L"api.github.com", L"/repos/RyuaNerin/FucKakaoAd/releases/latest", body))
    {
        auto js = json::parse(body);

        auto tag_name = js["tag_name"].get<std::string>();

        return tag_name != currentVersion;
    }

    return false;
}

std::string getCurrentVersion(HINSTANCE hInstance)
{
    auto hResource  = FindResourceW(hInstance, MAKEINTRESOURCE(VS_VERSION_INFO), RT_VERSION);
    auto dwSize     = SizeofResource(hInstance, hResource);
    auto hResData   = LoadResource(hInstance, hResource);
    defer(FreeResource(hResData));
    auto pRes       = LockResource(hResData);

    auto pResCopy   = LocalAlloc(LMEM_FIXED, dwSize);
    defer(LocalFree(pResCopy));

    memcpy(pResCopy, pRes, dwSize);

    VS_FIXEDFILEINFO *lpFfi;
    UINT uLen;
    VerQueryValueW(pResCopy, L"\\", (LPVOID*)&lpFfi, &uLen);
    DWORD dwFileVersionMS = lpFfi->dwFileVersionMS;
    DWORD dwFileVersionLS = lpFfi->dwFileVersionLS;

    DWORD dwLeftMost = HIWORD(dwFileVersionMS);
    DWORD dwSecondLeft = LOWORD(dwFileVersionMS);
    DWORD dwSecondRight = HIWORD(dwFileVersionLS);
    DWORD dwRightMost = LOWORD(dwFileVersionLS);

    std::ostringstream oss;
    oss << dwLeftMost << "." << dwSecondLeft << "." << dwSecondRight << "." << dwRightMost;

    return oss.str();
}

bool getHttp(LPCWSTR host, LPCWSTR path, std::string &body)
{
    bool res = false;

    BOOL      bResults = FALSE;
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;

    DWORD dwStatusCode;
    DWORD dwSize;
    DWORD dwRead;

    hSession = WinHttpOpen(USER_AGENT, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession)
        bResults = WinHttpSetTimeouts(hSession, 5000, 5000, 5000, 5000);
    if (bResults)
        hConnect = WinHttpConnect(hSession, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (hConnect)
        hRequest = WinHttpOpenRequest(hConnect, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (hRequest)
        bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, NULL);
    if (bResults)
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    if (bResults)
    {
        dwSize = sizeof(dwStatusCode);
        bResults = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
    }
    if (bResults)
        bResults = dwStatusCode == 200;
    if (bResults)
    {
        size_t dwOffset;
        do
        {
            dwSize = 0;
            bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);
            if (!bResults || dwSize == 0)
                break;

            while (dwSize > 0)
            {
                dwOffset = body.size();
                body.resize(dwOffset + dwSize);

                bResults = WinHttpReadData(hRequest, &body[dwOffset], dwSize, &dwRead);
                if (!bResults || dwRead == 0)
                    break;

                body.resize(dwOffset + dwRead);

                dwSize -= dwRead;
            }
        } while (true);

        res = true;
    }
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return res;
}
