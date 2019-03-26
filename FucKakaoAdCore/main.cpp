#include <Windows.h>

#include <string>

#include "main.h"
#include "debug.h"

DWORD g_pid = NULL;
HINSTANCE g_hInst = NULL;

HWND g_hwndApp  = NULL;
HWND g_hwndAd   = NULL;
HWND g_hwndMain = NULL;
HWND g_hwndLock = NULL;

WNDPROC g_prevWpApp  = NULL;
WNDPROC g_prevWpAd   = NULL;
WNDPROC g_prevWpMain = NULL;
WNDPROC g_prevWpLock = NULL;

bool setHeight(HWND hwnd)
{
    // 계산된 높이랑 다를 경우 다시 설정
    RECT rectApp;
    RECT rectView;
    if (GetWindowRect(g_hwndApp, &rectApp) == TRUE &&
        GetWindowRect(hwnd, &rectView) == TRUE)
    {
        int heightCur  = rectView.bottom - rectView.top;
        int heightNoAd = (rectApp.bottom - rectApp.top) - (rectView.top - rectApp.top) - (rectView.left - rectApp.left) * 2;

        if (heightNoAd != heightCur)
        {
            DebugLog(L"App  (%d , %d) - (%d , %d)", rectApp .left, rectApp .top, rectApp .right, rectApp .bottom);
            DebugLog(L"View (%d , %d) - (%d , %d)", rectView.left, rectView.top, rectView.right, rectView.top + heightNoAd);

            SetWindowPos(
                hwnd,
                NULL,
                0,
                0,
                rectView.right - rectView.left,
                heightNoAd,
                SWP_NOMOVE);

            return true;
        }
    }

    return false;
}
bool wndProcMainLock(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_SIZE)
    {
        // 크기조절
        if (wParam == SIZE_RESTORED)
            if (setHeight(hwnd))
                return true;
    }
    else if (uMsg == WM_ENABLE || uMsg == WM_SHOWWINDOW)
    {
        DebugLog(L"hwnd = %p / wParam = %d", hwnd, wParam);

        // 창이 활성화 될 때 광고 다시 숨기기
        if (wParam == TRUE)
            hideKakaoAd();

        // Lock 페이지가 숨겨질 때 = Main 보이기 = Main Proc 새로 설정
        else if (wParam == FALSE && hwnd == g_hwndLock)
            hookMain();
    }

    return false;
}

LRESULT CALLBACK WndProcApp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // 최소크기
    if (uMsg == WM_GETMINMAXINFO)
    {
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = 200;
        return 0;
    }

    return CallWindowProcW(g_prevWpApp, hwnd, uMsg, wParam, lParam);
}
LRESULT CALLBACK WndProcAd(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
    case WM_NCPAINT:
        // 드로잉 안함
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    return CallWindowProcW(g_prevWpAd, hwnd, uMsg, wParam, lParam);
}
LRESULT CALLBACK WndProcMain(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (wndProcMainLock(hwnd, uMsg, wParam, lParam))
        return 0;

    return CallWindowProcW(g_prevWpMain, hwnd, uMsg, wParam, lParam);
}
LRESULT CALLBACK WndProcLock(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (wndProcMainLock(hwnd, uMsg, wParam, lParam))
        return 0;

    return CallWindowProcW(g_prevWpLock, hwnd, uMsg, wParam, lParam);
}

void hookApp()
{
    if (g_hwndApp == NULL) return;
    if (GetWindowLongW(g_hwndApp, GWL_WNDPROC) != (LONG)WndProcApp)
    {
        DebugLog(L"hookApp");
        g_prevWpApp = (WNDPROC)SetWindowLongW(g_hwndApp, GWL_WNDPROC, (LONG)WndProcApp);
    }
}
void hookAd()
{
    if (g_hwndAd == NULL) return;
    if (GetWindowLongW(g_hwndAd, GWL_WNDPROC) != (LONG)WndProcAd)
    {
        DebugLog(L"hookAd");
        g_prevWpAd = (WNDPROC)SetWindowLongW(g_hwndAd, GWL_WNDPROC, (LONG)WndProcAd);
        ShowWindow(g_hwndAd, SW_HIDE);
    }
}
void hookMain()
{
    if (g_hwndMain == NULL) return;
    if (GetWindowLongW(g_hwndMain, GWL_WNDPROC) != (LONG)WndProcMain)
    {
        DebugLog(L"hookMain");
        g_prevWpMain = (WNDPROC)SetWindowLongW(g_hwndMain, GWL_WNDPROC, (LONG)WndProcMain);
        setHeight(g_hwndMain);
    }
}
void hookLock()
{
    if (g_hwndLock == NULL) return;
    if (GetWindowLongW(g_hwndLock, GWL_WNDPROC) != (LONG)WndProcLock)
    {
        DebugLog(L"hookLock");
        g_prevWpLock = (WNDPROC)SetWindowLongW(g_hwndLock, GWL_WNDPROC, (LONG)WndProcLock);
        setHeight(g_hwndLock);
    }
}

void unhookMain()
{
    if (GetWindowLongW(g_hwndMain, GWL_WNDPROC) == (LONG)WndProcMain)
    {
        DebugLog(L"unhookMain");
        SetWindowLongW(g_hwndMain, GWL_WNDPROC, (LONG)g_prevWpMain);
    }
}
void unhookLock()
{
    if (GetWindowLongW(g_hwndLock, GWL_WNDPROC) == (LONG)WndProcLock)
    {
        DebugLog(L"unhookLock");
        SetWindowLongW(g_hwndLock, GWL_WNDPROC, (LONG)g_prevWpLock);
    }
}

BOOL CALLBACK FindAnyKakaoWindow(HWND hwnd, LPARAM lParam)
{
    DWORD pid;
    if (GetWindowThreadProcessId(hwnd, &pid) != 0 && pid == g_pid)
    {
        WCHAR className[MAX_PATH];
        GetClassNameW(hwnd, className, MAX_PATH);

        if (std::wcscmp(className, L"EVA_Window_Dblclk") == 0)
        {
            g_hwndApp = hwnd;
            return FALSE;
        }
    }

    return TRUE;
}
BOOL CALLBACK FindKakaoHwndProc(HWND hwnd, LPARAM lParam)
{
    WCHAR windowName[MAX_PATH];
    WCHAR className[MAX_PATH];

    GetWindowTextW(hwnd, windowName, MAX_PATH);
    GetClassNameW(hwnd, className, MAX_PATH);

    if (std::wcscmp(className, L"EVA_ChildWindow") == 0)
    {
        if (std::wcsncmp(windowName, L"OnlineMainView_", 15) == 0) g_hwndMain = hwnd;
        else if (std::wcsncmp(windowName, L"LockModeView_", 13) == 0) g_hwndLock = hwnd;
    }
    else if (std::wcscmp(className, L"EVA_Window") == 0 &&
        std::wcslen(windowName) == 0 &&
        GetWindow(hwnd, GW_CHILD) == NULL)
    {
        g_hwndAd = hwnd;
    }

    return TRUE;
}
BOOL CALLBACK FindKakaoAd(HWND hwnd, LPARAM lParam)
{
    WCHAR windowName[MAX_PATH];
    WCHAR className[MAX_PATH];

    GetWindowTextW(hwnd, windowName, MAX_PATH);
    GetClassNameW(hwnd, className, MAX_PATH);
    if (std::wcscmp(className, L"EVA_Window") == 0 &&
        std::wcslen(windowName) == 0 &&
        GetWindow(hwnd, GW_CHILD) == NULL)
    {
        g_hwndAd = hwnd;
        return FALSE;
    }

    return TRUE;
}

void hideKakaoAd()
{
    if (g_hwndAd == NULL || IsWindow(g_hwndAd) == FALSE)
    {
        EnumChildWindows(g_hwndApp, FindKakaoAd, NULL);
        hookAd();
    }

    ShowWindow(g_hwndAd, SW_HIDE);
}
bool Attach()
{
    EnumWindows(FindAnyKakaoWindow, NULL);
    DebugLog(L"g_hwndApp : %p", g_hwndApp);

    if (g_hwndApp != NULL)
    {
        EnumChildWindows(g_hwndApp, FindKakaoHwndProc, NULL);
        DebugLog(L"g_hwndApp  = %p", g_hwndApp );
        DebugLog(L"g_hwndAd   = %p", g_hwndAd  );
        DebugLog(L"g_hwndLock = %p", g_hwndLock);
        DebugLog(L"g_hwndMain = %p", g_hwndMain);

        hookApp();
        hookAd();
        hookMain();
        hookLock();

        hideKakaoAd();
    }

    return 0;
}