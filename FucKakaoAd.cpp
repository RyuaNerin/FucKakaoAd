#include <Windows.h>

#include <iostream>
#include <string>

#include <cstring>

#include "resource.h"

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam);
bool calcClientSize();
void setClientSize();
DWORD WINAPI WaitForTerminate(void *lpVoid);
VOID CALLBACK winHook(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime);

HWND hwndWindow = NULL;
HWND hwndAd    = NULL;
HWND hwndMain  = NULL;
HWND hwndLock  = NULL;

int diff = 0;

int APIENTRY wWinMain(
    _In_     HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_     LPWSTR    lpCmdLine,
    _In_     int       nCmdShow)
{
    if (CreateMutexW(NULL, false, L"FucKakaoAd") == NULL)
        return 0;
    if (GetLastError() == ERROR_ALREADY_EXISTS)
        return 0;

    hwndWindow = FindWindowW(L"EVA_Window_Dblclk", L"카카오톡");
    if (hwndWindow != NULL)
    {
        DWORD kakaoPid;
        DWORD kakaoThreadId = GetWindowThreadProcessId(hwndWindow, &kakaoPid);
        if (kakaoThreadId == 0)
            return 0;

        EnumChildWindows(hwndWindow, EnumChildProc, 0);
        if (!calcClientSize())
            return 0;

        setClientSize();

        SetWinEventHook(EVENT_MIN, EVENT_MAX, NULL, winHook, kakaoPid, kakaoThreadId, NULL);

        CreateThread(NULL, 0, WaitForTerminate, &kakaoPid, 0, NULL);

        MSG msg;
        while (true)
        {
            if (GetMessageW(&msg, NULL, 0, 0) > 0)
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
        }
    }
    return 0;
}

DWORD WINAPI WaitForTerminate(void *lpVoid)
{
    DWORD pid = *(DWORD*)lpVoid;

    HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
    auto r = WaitForSingleObject(hProcess, INFINITE);
    auto err = GetLastError();
    CloseHandle(hProcess);

    ExitProcess(0);

}

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
    WCHAR windowName[MAX_PATH];
    WCHAR className [MAX_PATH];

    GetWindowTextW(hwnd, windowName, MAX_PATH);
    GetClassNameW(hwnd, className, MAX_PATH);

    if (std::wcscmp(className, L"EVA_Window") == 0 &&
        std::wcslen(windowName) == 0 &&
        GetWindow(hwnd, GW_CHILD) == NULL)
    {
        hwndAd = hwnd;
    }
    else if (std::wcscmp(className, L"EVA_ChildWindow") == 0)
    {
             if (std::wcsncmp(windowName, L"OnlineMainView_", 15) == 0) hwndMain = hwnd;
        else if (std::wcsncmp(windowName, L"LockModeView_"  , 13) == 0) hwndLock = hwnd;
    }

    return TRUE;
}

bool calcClientSize()
{
    if (hwndAd == NULL || hwndMain == NULL || hwndLock == NULL)
        return false;

    HWND hwndClient;
    if (IsWindowVisible(hwndMain))
        hwndClient = hwndMain;
    else
        hwndClient = hwndLock;

    RECT rectKakao;
    RECT rectClient;
    RECT rectAd;
    if (GetWindowRect(hwndWindow , &rectKakao ) == FALSE) return false;
    if (GetWindowRect(hwndClient, &rectClient) == FALSE) return false;
    if (GetWindowRect(hwndAd    , &rectAd    ) == FALSE) return false;

    diff  = (rectKakao.right - rectKakao.left) - (rectClient.right - rectClient.left);

    // 광고 숨기는 부분
    ShowWindow(hwndAd, SW_HIDE);
    LONG attr = GetWindowLongW(hwndAd, GWL_EXSTYLE);
    if ((attr & WS_EX_LAYERED) != WS_EX_LAYERED)
    {
        // 광고창 투명하게 만들어버리기
        SetWindowLongW(hwndAd, GWL_EXSTYLE, attr | WS_EX_LAYERED);
        SetLayeredWindowAttributes(hwndAd,0, 0, LWA_COLORKEY);
    }

    return true;
}

void resizeWindow(HWND hwnd, RECT recTKakao);
void setClientSize()
{
    ShowWindow(hwndAd, SW_HIDE);

    RECT rectWindow;
    if (GetWindowRect(hwndWindow, &rectWindow) == FALSE)
        return;

    resizeWindow(hwndMain, rectWindow);
    resizeWindow(hwndLock, rectWindow);
}
void resizeWindow(HWND hwnd, RECT recTKakao)
{
    RECT rect;
    if (GetWindowRect(hwnd, &rect) == FALSE)
        return;

    SetWindowPos(
        hwnd,
        NULL,
        rect.left,
        rect.top,
        recTKakao.right  - recTKakao.left - diff,
        recTKakao.bottom - recTKakao.top  - diff - (rect.top - recTKakao.top),
        SWP_NOMOVE);
}

bool isMoveSize = false;
VOID CALLBACK winHook(
    HWINEVENTHOOK hWinEventHook,
    DWORD         event,
    HWND          hwnd,
    LONG          idObject,
    LONG          idChild,
    DWORD         idEventThread,
    DWORD         dwmsEventTime)
{
    switch (event)
    {
    case EVENT_SYSTEM_MOVESIZESTART:
        isMoveSize = true;
        break;

    case EVENT_SYSTEM_MOVESIZEEND:
        isMoveSize = false;
        if (hwnd == hwndWindow)
            setClientSize();
        break;

    case EVENT_OBJECT_HIDE:
    case EVENT_OBJECT_REORDER:
    case EVENT_OBJECT_LOCATIONCHANGE:
        if (isMoveSize)
        {
            if (hwnd == hwndWindow)
                setClientSize();
        }
        break;

    case EVENT_OBJECT_SHOW:
        if (hwnd == hwndAd || hwnd == hwndMain || hwnd == hwndLock)
            setClientSize();
        break;

    default:
        if (isMoveSize)
        {
            char buf[24];
            _ltoa_s(event, buf, 24, 10);
            strcat_s(buf, "\n");
            OutputDebugStringA(buf);
        }
        break;
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, message, wParam, lParam);
}
