#include "main.h"

#include <Windows.h>

#include <string>
#include <shared_mutex>
#include <set>

#include "defer.h"
#include "debug.h"
#include "dllmain.h"
#include "hook.h"
#include "adblock.h"
#include "signal_wait.h"

std::shared_mutex   g_hookedCacheSync;
std::set<HWND>      g_hookedCache;
std::set<HWND>      g_mainlock;
HWND                g_hwndAd   = NULL; // 잡힐 때마다 다시 숨기기

signal_wait         g_exitWait;

void hookWindow(HWND hwnd)
{
    g_hookedCacheSync.lock();
    defer(g_hookedCacheSync.unlock());

    if (g_hwndAd != NULL && hwnd == g_hwndAd)
    {
        ShowWindow(hwnd, SW_HIDE);
        return;
    }

    auto mainlock = g_mainlock.find(hwnd) != g_mainlock.end();

    if (g_hookedCache.find(hwnd) != g_hookedCache.end() && !mainlock)
        return;

    if (mainlock)
    {
        adblock(hwnd);
        hookCustomWndProc(hwnd, &wndProcMainLock);
        return;
    }

    g_hookedCache.insert(hwnd);

    if (IsWindow(hwnd) == FALSE)
        return;

    WCHAR className [MAX_PATH];
    WCHAR windowName[MAX_PATH];

    GetClassNameW (hwnd, className , MAX_PATH);
    GetWindowTextW(hwnd, windowName, MAX_PATH);

    DebugLog(L"hookWindow [%p] (%ws, %ws)", hwnd, className, windowName);

    if (std::wcscmp(className, L"EVA_Window_Dblclk") == 0 && GetParent(hwnd) == NULL)
    {
        auto style = GetWindowLongW(hwnd, GWL_EXSTYLE);
        if ((style & WS_EX_APPWINDOW) == WS_EX_APPWINDOW)
        {
            // App
            DebugLog("------> App");
            hookCustomWndProc(hwnd, &wndProcApp);
        }
    }
    else if (std::wcscmp(className, L"EVA_ChildWindow") == 0)
    {
        if (std::wcsncmp(windowName, L"OnlineMainView_", 15) == 0)
        {
            // Main / Lock
            DebugLog("------> Main");
            adblock(hwnd);
            hookCustomWndProc(hwnd, &wndProcMainLock);
            g_mainlock.insert(hwnd);
        }
        else if (std::wcsncmp(windowName, L"LockModeView_", 13) == 0)
        {
            // Lock
            DebugLog("------> Lock");
            adblock(hwnd);
            hookCustomWndProc(hwnd, &wndProcMainLock);
            g_mainlock.insert(hwnd);
        }
    }
    else if (std::wcscmp(className, L"EVA_Window") == 0 &&
             std::wcslen(windowName) == 0 &&
             GetWindow(hwnd, GW_CHILD) == NULL)
    {
        // 광고
        DebugLog("------> Ad");

        // 숨기고
        ShowWindow(hwnd, SW_HIDE);

        // 투명하게
        auto exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
        if ((GWL_EXSTYLE & WS_EX_LAYERED) != WS_EX_LAYERED)
        {
            SetWindowLongW(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
            SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
        }

        hookCustomWndProc(hwnd, &wndProcAd);
    }
    else if (std::wcsncmp(className, L"#32770", 6) == 0)
    {
        // 채팅
        DebugLog("------> Chat");
        hookCustomWndProc(hwnd, &wndProcChat);
    }
}

void unhookWindow(HWND hwnd)
{
    if (hwnd == g_kakaoMain)
        g_exitWait.set();

    g_hookedCacheSync.lock();
    defer(g_hookedCacheSync.unlock());

    if (g_hookedCache.find(hwnd) == g_hookedCache.end())
        return;

    DebugLog(L"unhookWind [%p]", hwnd);

    unhookCustomWndProc(hwnd);

    g_hookedCache.erase(hwnd);
    g_mainlock .erase(hwnd);
}

VOID CALLBACK ChatWindowHookProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime)
{
    if (hwnd == NULL)
        return;

    switch (event)
    {
    case EVENT_OBJECT_CREATE:       // 0x 80 00
    case EVENT_OBJECT_SHOW:         // 0x 80 02
        hookWindow(hwnd);
        break;

    case EVENT_OBJECT_DESTROY:      // 0x 80 01
        unhookWindow(hwnd);
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
            g_kakaoMain = hwnd;
            return FALSE;
        }
    }

    return TRUE;
}
BOOL CALLBACK findKakaoWindows(HWND hwnd, LPARAM lParam)
{
    hookWindow(hwnd);

    return TRUE;
}

DWORD CALLBACK hookThread(PVOID param)
{
    // 앞으로 새로운 창에 후킹
    auto hHook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW, NULL, ChatWindowHookProc, g_pid, 0, WINEVENT_OUTOFCONTEXT);
    DebugLog(L"hookThread - SetWinEventHook : %p", hHook);
    if (hHook == NULL)
        return 0;

    DebugLog(L"hookThread - GetMessageW");

    MSG msg;
    BOOL r;
    while ((r = GetMessageW(&msg, NULL, 0, 0)) != 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DebugLog(L"hookThread - UnhookWinEvent");
    UnhookWinEvent(hHook);

    return 0;
}

DWORD CALLBACK AttachThread(PVOID param)
{
    EnumWindows(findKakaoTalk, NULL);
    DebugLog(L"kakaoMain : %p", g_kakaoMain);

    if (g_kakaoMain != NULL)
    {
        hookWindow(g_kakaoMain);

        // 기존 창에 후킹
        EnumChildWindows(g_kakaoMain, findKakaoWindows, NULL);
    }

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
