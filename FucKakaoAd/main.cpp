#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>

#include <string>

#include "defer.h"
#include "resource.h"

#define MUTEX_NAME  L"FucKakaoAd"
#define DLL_NAME    L"FucKakaoAdCore.dll"

HINSTANCE g_hInstance;

bool RunAsDesktopUser(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, PCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
void ShowMessageBox(UINT strId);
bool isInjected(DWORD pid, LPCTSTR moduleName);
void injectDll(HANDLE hProcess);

#define ShowMessageBoxAndReturn(id) { ShowMessageBox(id); return; }

void ShowMessageBox(UINT strId)
{
    WCHAR msgTitle[MAX_PATH];
    WCHAR msgText[MAX_PATH];
    LoadStringW(g_hInstance, IDS_STRING_TITLE, msgTitle, MAX_PATH);
    LoadStringW(g_hInstance, strId, msgText, MAX_PATH);

    MessageBoxW(NULL, msgText, msgTitle, 0);
}

void wWinMainBody()
{
    WCHAR kakaoTitle[MAX_PATH];
    LoadStringW(g_hInstance, IDS_STRING_CAPTIONS, kakaoTitle, MAX_PATH);

    HWND hwndApp = FindWindowW(L"EVA_Window_Dblclk", kakaoTitle);
    if (hwndApp == NULL)
    {
        hwndApp = FindWindowW(L"EVA_Window", kakaoTitle);
        if (hwndApp == NULL)
        {
            WCHAR kakaoPath[MAX_PATH];

            // 카카오톡을 찾고
            HKEY hkey;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\KakaoTalk", 0, KEY_READ, &hkey) != ERROR_SUCCESS)
                ShowMessageBoxAndReturn(IDS_STRING_ERROR_NOT_FOUND);
            defer(RegCloseKey(hkey));

            DWORD dwType;
            DWORD dwBytes = MAX_PATH;
            if (RegQueryValueExW(hkey, L"DisplayIcon", 0, &dwType, (LPBYTE)kakaoPath, &dwBytes) != ERROR_SUCCESS)
                ShowMessageBoxAndReturn(IDS_STRING_ERROR_NOT_FOUND)

                std::wstring kakaoPathStd(kakaoPath);
            kakaoPathStd = kakaoPathStd.substr(0, kakaoPathStd.find_last_of(L'\\') + 1);
            kakaoPathStd.append(L"KakaoTalk.exe");

            STARTUPINFOW si = { 0 };
            si.cb = sizeof(si);

            PROCESS_INFORMATION pi = { 0, };

            if (!RunAsDesktopUser(kakaoPath, NULL, NULL, 0, FALSE, 0, NULL, NULL, &si, &pi) || pi.hProcess == NULL)
            {
                if (!CreateProcessW(kakaoPath, NULL, NULL, 0, FALSE, 0, NULL, NULL, &si, &pi) || pi.hProcess == NULL)
                    ShowMessageBoxAndReturn(IDS_STRING_FAIL)
            }

            // 프로세스가 종료되거나 메인 폼이
            DWORD exitCode;
            do
            {
                hwndApp = FindWindowW(L"EVA_Window_Dblclk", kakaoTitle);
                if (hwndApp == NULL)
                    hwndApp = FindWindowW(L"EVA_Window", kakaoTitle);

                Sleep(250);
            } while ((GetExitCodeProcess(pi.hProcess, &exitCode) == TRUE && exitCode == STILL_ACTIVE) && hwndApp == NULL);
        }
    }

    if (hwndApp != NULL)
    {
        // 프로세스 ID 얻어오고
        DWORD pid;
        if (GetWindowThreadProcessId(hwndApp, &pid) == 0 || pid == 0)
            ShowMessageBoxAndReturn(IDS_STRING_ERROR_PERMISION)

        if (isInjected(pid, DLL_NAME))
            ShowMessageBoxAndReturn(IDS_STRING_SUCCESS)
        else
        {
            auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
            if (hProcess == NULL)
                ShowMessageBoxAndReturn(IDS_STRING_FAIL)
            defer(CloseHandle(hProcess));

            injectDll(hProcess);
        }
    }
}

bool isInjected(DWORD pid, LPCTSTR moduleName)
{
    MODULEENTRY32W snapEntry = { 0 };
    snapEntry.dwSize = sizeof(MODULEENTRY32W);

    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    if (hSnapshot == NULL)
        return false;
    defer(CloseHandle(hSnapshot));

    if (Module32FirstW(hSnapshot, &snapEntry))
    {
        do
        {
            if (lstrcmpW(snapEntry.szModule, moduleName) == 0)
            {
                return true;
            }
        } while (Module32NextW(hSnapshot, &snapEntry));
    }

    return false;
}

bool RunAsDesktopUser(
    LPCWSTR lpApplicationName,
    LPWSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    PCWSTR lpCurrentDirectory,
    LPSTARTUPINFOW lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation)
{
    // To start process as shell user you will need to carry out these steps:
    // 1. Enable the SeIncreaseQuotaPrivilege in your current token
    // 2. Get an HWND representing the desktop shell (GetShellWindow)
    // 3. Get the Process ID(PID) of the process associated with that window(GetWindowThreadProcessId)
    // 4. Open that process(OpenProcess)
    // 5. Get the access token from that process (OpenProcessToken)
    // 6. Make a primary token with that token(DuplicateTokenEx)
    // 7. Start the new process with that primary token(CreateProcessWithTokenW)

    HANDLE process = GetCurrentProcess();

    HANDLE hProcessToken;
    if (!OpenProcessToken(process, 0x0020, &hProcessToken))
        return false;
    defer(CloseHandle(hProcessToken));

    TOKEN_PRIVILEGES tkp = { 0, };
    tkp.PrivilegeCount = 1;

    if (!LookupPrivilegeValueW(NULL, L"SeIncreaseQuotaPrivilege", &tkp.Privileges[0].Luid))
        return false;

    tkp.Privileges[0].Attributes = 0x00000002;

    if (AdjustTokenPrivileges(hProcessToken, false, &tkp, 0, NULL, NULL) == FALSE)
        return false;

    HWND hwnd = GetShellWindow();
    if (hwnd == NULL)
        return false;

    // Get the PID of the desktop shell process.
    DWORD dwPID;
    if (GetWindowThreadProcessId(hwnd, &dwPID) == 0)
        return false;

    // Open the desktop shell process in order to query it (get the token)
    HANDLE hShellProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, dwPID);
    if (hShellProcess == NULL)
        return false;
    defer(CloseHandle(hShellProcess));

    // Get the process token of the desktop shell.
    HANDLE hShellProcessToken;
    if (!OpenProcessToken(hShellProcess, 0x0002, &hShellProcessToken))
        return false;
    defer(CloseHandle(hShellProcessToken));

    // Duplicate the shell's process token to get a primary token.
    // Based on experimentation, this is the minimal set of rights required for CreateProcessWithTokenW (contrary to current documentation).
    HANDLE hPrimaryToken;
    if (!DuplicateTokenEx(hShellProcessToken, 395U, NULL, SecurityImpersonation, TokenPrimary, &hPrimaryToken))
        return false;
    defer(CloseHandle(hPrimaryToken));

    return CreateProcessWithTokenW(hPrimaryToken, 0, lpApplicationName, lpCommandLine, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation) == TRUE;
}

void injectDll(HANDLE hProcess)
{
    // EXE 위치를 알아내기 위한 사전 작업
    HMODULE hModule = GetModuleHandleW(NULL);
    if (hModule == NULL)
        ShowMessageBoxAndReturn(IDS_STRING_ERROR_PERMISION)

    // EXE 위치 불러오고
    TCHAR path[MAX_PATH] = { 0, };
    GetModuleFileNameW(hModule, path, sizeof(path));

    // DLL 이름 쓰고
    std::wstring pathString(path);
    pathString = pathString.substr(0, pathString.find_last_of(L'\\') + 1);
    pathString.append(DLL_NAME);

    auto hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (hKernel32 == NULL)
        ShowMessageBoxAndReturn(IDS_STRING_FAIL)

    auto lpLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
    if (lpLoadLibrary == NULL)
        ShowMessageBoxAndReturn(IDS_STRING_FAIL)

    auto pBuffSize = (pathString.size() + 1) * sizeof(TCHAR);
    auto pBuff = VirtualAllocEx(hProcess, NULL, pBuffSize, MEM_COMMIT, PAGE_READWRITE);
    if (pBuff != NULL)
    {
        defer(VirtualFreeEx(hProcess, pBuff, 0, MEM_RELEASE));

        if (WriteProcessMemory(hProcess, pBuff, (LPVOID)pathString.c_str(), pBuffSize, NULL))
        {
            auto hThread = CreateRemoteThread(hProcess, NULL, 0, lpLoadLibrary, pBuff, 0, NULL);
            if (hThread != NULL)
            {
                defer(CloseHandle(hThread));
                WaitForSingleObject(hThread, INFINITE);

                DWORD exitCode;
                if (GetExitCodeThread(hThread, &exitCode) == FALSE || exitCode == 0)
                    ShowMessageBoxAndReturn(IDS_STRING_FAIL)

            }
        }
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, message, wParam, lParam);
}
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int cmdShow)
{
    if (CreateMutexW(NULL, FALSE, MUTEX_NAME) == NULL || GetLastError() == ERROR_ALIAS_EXISTS)
        return 0;

    g_hInstance = hInstance;

    wWinMainBody();

    return 0;
}
