#include "main.h"

#include <Windows.h>

#include <string>
#include <shared_mutex>
#include <map>

#include "defer.h"
#include "debug.h"
#include "dllmain.h"
#include "hook.h"
#include "signal_wait.h"

enum WindowType
{
    WindowType_Login,
    WindowType_Talk,
    WindowType_TalkMain,
    WindowType_TalkLock,
    WindowType_TalkAd,
    WindowType_Chat,
    WindowType_Etc
};

std::map<HWND, WindowType>  g_hwndCache;
std::mutex                  g_hwndCacheMut;

signal_wait g_exitWait;

void expandMainLock(HWND hwnd);

WindowType detectWindow(HWND hwnd)
{
    g_hwndCacheMut.lock();
    defer(g_hwndCacheMut.unlock());

    auto found = g_hwndCache.find(hwnd);
    if (found != g_hwndCache.end())
        return found->second;

    WCHAR className[MAX_PATH];
    WCHAR windowName[MAX_PATH];

    GetClassNameW(hwnd, className, MAX_PATH);
    GetWindowTextW(hwnd, windowName, MAX_PATH);

    if (std::wcscmp(className, L"EVA_Window_Dblclk") == 0 && GetParent(hwnd) == NULL)
    {
        auto style = GetWindowLongW(hwnd, GWL_EXSTYLE);
        if ((style & WS_EX_APPWINDOW) == WS_EX_APPWINDOW)
        {
            // App
            g_kakaoTalk = hwnd;
            g_hwndCache[hwnd] = WindowType_Talk;
            return WindowType_Talk;
        }
    }
    else if (std::wcscmp(className, L"EVA_ChildWindow") == 0)
    {
        if (std::wcsncmp(windowName, L"OnlineMainView_", 15) == 0)
        {
            // Main / Lock
            g_kakaoTalkMain = hwnd;
            g_hwndCache[hwnd] = WindowType_TalkMain;
            return WindowType_TalkMain;
        }
        else if (std::wcsncmp(windowName, L"LockModeView_", 13) == 0)
        {
            // Lock
            g_kakaoTalkLock = hwnd;
            g_hwndCache[hwnd] = WindowType_TalkLock;
            return WindowType_TalkLock;
        }
    }
    else if (std::wcscmp(className, L"EVA_Window") == 0)
    {
        if (GetParent(hwnd) == NULL &&
            std::wcslen(windowName) == 0)
        {
            // 광고
            g_kakaoLogin = hwnd;
            g_hwndCache[hwnd] = WindowType_Login;

            return WindowType_Login;
        }
        else if (
            std::wcslen(windowName) == 0 &&
            GetWindow(hwnd, GW_CHILD) == NULL &&
            GetParent(hwnd) == g_kakaoTalk)
        {
            // 광고
            g_kakaoTalkAd = hwnd;
            g_hwndCache[hwnd] = WindowType_TalkAd;

            // 사이즈 측정 후 기록
            {
                RECT rect;
                if (GetWindowRect(hwnd, &rect) == TRUE)
                {
                    g_kakaoAdHeight = rect.bottom - rect.top + 1;
                    DebugLog(L"g_kakaoAdHeight : %d", g_kakaoAdHeight);
                }
            }
            return WindowType_TalkAd;
        }
    }
    else if (std::wcsncmp(className, L"#32770", 6) == 0)
    {
        // 채팅
        g_kakaoChatMut.lock();
        g_kakaoChat.insert(hwnd);
        g_kakaoChatMut.unlock();

        g_hwndCache[hwnd] = WindowType_Chat;
        return WindowType_Chat;
    }

    g_hwndCache[hwnd] = WindowType_Etc;
    return WindowType_Etc;
}
void newWindow(HWND hwnd)
{
    switch (detectWindow(hwnd))
    {
    case WindowType_Talk:
        DebugLog(L"newWindow > app");
        hookKakaoTalk(hwnd);

        ShowWindow(g_kakaoTalkAd, SW_HIDE);
        break;

    case WindowType_TalkMain:
    case WindowType_TalkLock:
        DebugLog(L"newWindow > main/lock");
        //expandMainLock(hwnd);
        break;

    case WindowType_TalkAd:
        DebugLog(L"newWindow > ad");
        hookKakaoAd(hwnd);

        // 숨기기
        ShowWindow(hwnd, SW_HIDE);
        break;

    case WindowType_Chat:
        DebugLog(L"newWindow > chat");
        hookKakaoChat(hwnd);
        break;

    case WindowType_Etc:
        break;
    }
}

void delWindow(HWND hwnd)
{
    g_hwndCacheMut.lock();
    defer(g_hwndCacheMut.unlock());

    if (g_hwndCache.erase(hwnd) == 0)
        return;

    unhookCustomWndProc(hwnd);

    if (hwnd == g_kakaoTalkMain || hwnd == g_kakaoLogin)
    {
        DebugLog(L"EXIT");
        g_exitWait.set();
    }
    else
    {
        g_kakaoChatMut.lock();
        g_kakaoChat.erase(hwnd);
        g_kakaoChatMut.unlock();
    }
}

VOID CALLBACK ChatWindowHookProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime)
{
    if (hwnd == NULL)
        return;

    switch (event)
    {
    case EVENT_OBJECT_CREATE:       // 0x 80 00
    case EVENT_OBJECT_SHOW:         // 0x 80 02
        newWindow(hwnd);
        break;

    case EVENT_OBJECT_DESTROY:      // 0x 80 01
        delWindow(hwnd);
        break;
    }
}

BOOL CALLBACK findKakaoTalk(HWND hwnd, LPARAM lParam)
{
    DWORD pid;
    if (GetWindowThreadProcessId(hwnd, &pid) != 0 && pid == g_pid)
    {
        WCHAR className[MAX_PATH];
        GetClassNameW(hwnd, className, MAX_PATH);

        if (std::wcscmp(className, L"EVA_Window_Dblclk") == 0)
        {
            g_kakaoTalk = hwnd;
            return FALSE;
        }
    }

    return TRUE;
}
BOOL CALLBACK findKakaoLogin(HWND hwnd, LPARAM lParam)
{
    DWORD pid;
    if (GetWindowThreadProcessId(hwnd, &pid) != 0 && pid == g_pid)
    {
        WCHAR className[MAX_PATH];
        GetClassNameW(hwnd, className, MAX_PATH);

        if (std::wcscmp(className, L"EVA_Window") == 0)
        {
            g_kakaoLogin = hwnd;
            return FALSE;
        }
    }

    return TRUE;
}
BOOL CALLBACK findKakaoWindows(HWND hwnd, LPARAM lParam)
{
    newWindow(hwnd);

    return TRUE;
}

DWORD CALLBACK hookThread(PVOID param)
{
    // 앞으로 새로운 창에 후킹
    auto hHook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW, NULL, ChatWindowHookProc, g_pid, 0, WINEVENT_OUTOFCONTEXT);
    DebugLog(L"hookThread - SetWinEventHook : %p", hHook);
    if (hHook == NULL)
        return 0;
    defer(UnhookWinEvent(hHook));

    DebugLog(L"hookThread - GetMessageW");
    MSG msg;
    BOOL r;
    while ((r = GetMessageW(&msg, NULL, 0, 0)) != 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DebugLog(L"hookThread - return");

    return 0;
}

DWORD CALLBACK AttachThread(PVOID param)
{
    EnumWindows(findKakaoTalk, NULL);
    DebugLog(L"kakaoMain : %p", g_kakaoTalk);

    if (g_kakaoTalk != NULL)
    {
        EnumWindows(findKakaoLogin, NULL);

        newWindow(g_kakaoTalk);

        // 기존 창 인식
        EnumChildWindows(g_kakaoTalk, findKakaoWindows, NULL);
        
        // 기존 창 키우기
        if (g_kakaoTalkAd != NULL)
        {
            expandMainLock(g_kakaoTalkLock);
            expandMainLock(g_kakaoTalkMain);
        }
    }

    initApiHook();

    DebugLog(L"AttachThread - CreateThread");
    auto hThread = CreateThread(NULL, 0, hookThread, NULL, 0, NULL);

    DebugLog(L"AttachThread - wait");
    g_exitWait.wait();

    DebugLog(L"AttachThread - TerminateThread");
    TerminateThread(hThread, 0);

    CloseHandle(hThread);

    return 0;
}

void Attach()
{
    auto hThread = CreateThread(NULL, 0, AttachThread, NULL, 0, NULL);
    if (hThread != NULL)
        CloseHandle(hThread);
}

bool m_expanededMain = false;
bool m_expanededLock = false;
void expandMainLock(HWND hwnd)
{
    DebugLog(L"expandMainLock");

    if ((!m_expanededMain && hwnd == g_kakaoTalkMain) ||
        (!m_expanededLock && hwnd == g_kakaoTalkLock))
    {
        DebugLog(L"GetWindowRect");
        RECT rectView;
        if (GetWindowRect(hwnd, &rectView) == TRUE)
        {
            DebugLog(L"RECT / x = %d / y = %d / w = %d / h = %d", rectView.left, rectView.top, rectView.right - rectView.left, rectView.bottom - rectView.top);

            SetWindowPos(
                hwnd,
                0,
                0,
                0,
                rectView.right - rectView.left,
                rectView.bottom - rectView.top + g_kakaoAdHeight,
                SWP_NOMOVE | SWP_NOZORDER
            );
        }

        if (hwnd == g_kakaoTalkMain)
            m_expanededMain = true;
        else if (hwnd == g_kakaoTalkLock)
            m_expanededLock = true;
    }
}
