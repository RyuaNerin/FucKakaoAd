#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

#include <string>
#include <experimental/filesystem>
using namespace std::experimental;

#include "defer.h"
#include "resource.h"

#define KAKAO_EXE   L"KakaoTalk.exe"
#define MUTEX_NAME  L"FucKakaoAd"
#define DLL_NAME    L"FucKakaoAdCore.dll"

HINSTANCE g_hInstance;

bool RunAsDesktopUser(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, PCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);
void ShowMessageBox(UINT strId);
bool isInjected(DWORD pid);
void injectDll(HANDLE hProcess);

#define ShowMessageBoxAndReturn(id) { ShowMessageBox(id); return; }

void ShowMessageBox(UINT strId)
{
    WCHAR msgTitle[MAX_PATH];
    WCHAR msgText [MAX_PATH];
    LoadStringW(g_hInstance, IDS_STRING_TITLE, msgTitle, MAX_PATH);
    LoadStringW(g_hInstance, strId           , msgText , MAX_PATH);

    MessageBoxW(NULL, msgText, msgTitle, 0);
}

BOOL CALLBACK FindKaKaoWindowProc(HWND hwnd, LPARAM lParam)
{
    if (!IsWindowVisible(hwnd))
        return TRUE;

    WINDOWPLACEMENT wndPlacement;
    if (GetWindowPlacement(hwnd, &wndPlacement) == FALSE)
        return TRUE;

    if (wndPlacement.showCmd == SW_HIDE)
        return TRUE;

    WCHAR className[MAX_PATH];
    if (GetClassNameW(hwnd, className, MAX_PATH) == 0)
        return TRUE;

    if (std::wcscmp(className, L"EVA_Window_Dblclk") != 0 &&
        std::wcscmp(className, L"EVA_Window"       ) != 0)
        return TRUE;

    DWORD pid;
    if (GetWindowThreadProcessId(hwnd, &pid) == 0)
        return TRUE;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (hProcess == NULL)
        return TRUE;
    defer(CloseHandle(hProcess));

    HMODULE hModule;
    DWORD needed;
    if (K32EnumProcessModules(hProcess, &hModule, sizeof(hModule), &needed) == FALSE)
        return TRUE;

    WCHAR szProcessName[MAX_PATH];
    if (K32GetModuleBaseNameW(hProcess, hModule, szProcessName, MAX_PATH) == 0)
        return TRUE;

    if (lstrcmpiW(szProcessName, KAKAO_EXE) != 0)
        return TRUE;

    *(HWND*)lParam = hwnd;
    return FALSE;
}
HWND FindKakaoWindow()
{
    HWND hwnd = NULL;
    EnumWindows(FindKaKaoWindowProc, (LPARAM)&hwnd);
    return hwnd;
}

void wWinMainBody()
{
    HWND hwndApp = FindKakaoWindow();
    if (hwndApp == NULL)
    {
        // 카카오톡을 찾고
        WCHAR kakaoPathBuffer[MAX_PATH];
        {
            HKEY hkey;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\KakaoTalk", 0, KEY_READ, &hkey) != ERROR_SUCCESS)
                ShowMessageBoxAndReturn(IDS_STRING_ERROR_NOT_FOUND);
            defer(RegCloseKey(hkey));

            DWORD dwType;
            DWORD dwBytes = MAX_PATH;
            if (RegQueryValueExW(hkey, L"DisplayIcon", 0, &dwType, (LPBYTE)kakaoPathBuffer, &dwBytes) != ERROR_SUCCESS)
                ShowMessageBoxAndReturn(IDS_STRING_ERROR_NOT_FOUND)
        }

        std::wstring kakaoPath = filesystem::path(kakaoPathBuffer).replace_filename(KAKAO_EXE).wstring();

        STARTUPINFOW si = { 0 };
        si.cb = sizeof(si);

        PROCESS_INFORMATION pi = { 0, };

        if (!RunAsDesktopUser(kakaoPath.c_str(), NULL, NULL, 0, FALSE, 0, NULL, NULL, &si, &pi) || pi.hProcess == NULL)
        {
            if (!CreateProcessW(kakaoPath.c_str(), NULL, NULL, 0, FALSE, 0, NULL, NULL, &si, &pi) || pi.hProcess == NULL)
                ShowMessageBoxAndReturn(IDS_STRING_FAIL)
        }

        // 프로세스가 종료되거나 메인 폼이
        DWORD exitCode;
        do
        {
            hwndApp = FindKakaoWindow();
            Sleep(250);
        } while ((GetExitCodeProcess(pi.hProcess, &exitCode) == TRUE && exitCode == STILL_ACTIVE) && hwndApp == NULL);
    }

    if (hwndApp != NULL)
    {
        // 프로세스 ID 얻어오고
        DWORD pid;
        if (GetWindowThreadProcessId(hwndApp, &pid) == 0 || pid == 0)
            ShowMessageBoxAndReturn(IDS_STRING_ERROR_PERMISION)

        if (!isInjected(pid))
        {
            auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
            if (hProcess == NULL)
                ShowMessageBoxAndReturn(IDS_STRING_FAIL)
            defer(CloseHandle(hProcess));

            injectDll(hProcess);
        }
    }
}

bool isInjected(DWORD pid)
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
            if (lstrcmpiW(snapEntry.szModule, DLL_NAME) == 0)
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
    WCHAR tempPathBuffer[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPathBuffer) == 0)
        ShowMessageBoxAndReturn(IDS_STRING_FAIL)
    std::wstring dllPath = filesystem::path(tempPathBuffer).append(DLL_NAME).wstring();

    {
        auto hModule   = GetModuleHandleW(NULL);
        auto hResource = FindResourceW(hModule, MAKEINTRESOURCEW(IDF_DLL), L"FILE");
        auto hMemory   = LoadResource(hModule, hResource);
        auto dwSize    = SizeofResource(hModule, hResource);
        auto lpAddress = LockResource(hMemory);

        auto hFile = CreateFileW(dllPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != NULL)
        {
            defer(CloseHandle(hFile));
            
            WriteFile(hFile, lpAddress, dwSize, NULL, NULL);
        }
    }

    if (!filesystem::exists(dllPath))
        ShowMessageBoxAndReturn(IDS_STRING_FAIL)

    auto hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (hKernel32 == NULL)
        ShowMessageBoxAndReturn(IDS_STRING_FAIL)

    auto lpLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryW");
    if (lpLoadLibrary == NULL)
        ShowMessageBoxAndReturn(IDS_STRING_FAIL)

    auto pBuffSize = (dllPath.size() + 1) * sizeof(TCHAR);
    auto pBuff = VirtualAllocEx(hProcess, NULL, pBuffSize, MEM_COMMIT, PAGE_READWRITE);
    if (pBuff == NULL)
        ShowMessageBoxAndReturn(IDS_STRING_FAIL)
    defer(VirtualFreeEx(hProcess, pBuff, 0, MEM_RELEASE));

    if (WriteProcessMemory(hProcess, pBuff, (LPVOID)dllPath.c_str(), pBuffSize, NULL) == FALSE)
        ShowMessageBoxAndReturn(IDS_STRING_FAIL)

    auto hThread = CreateRemoteThread(hProcess, NULL, 0, lpLoadLibrary, pBuff, 0, NULL);
    if (hThread == NULL)
        ShowMessageBoxAndReturn(IDS_STRING_FAIL)
    defer(CloseHandle(hThread));

    WaitForSingleObject(hThread, INFINITE);

    DWORD exitCode;
    if (GetExitCodeThread(hThread, &exitCode) == FALSE || exitCode == 0)
        ShowMessageBoxAndReturn(IDS_STRING_FAIL)
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
