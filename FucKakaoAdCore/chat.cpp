#include "chat.h"

#include <windows.h>

#include <shared_mutex>
#include <map>
#include <set>
#include <string>

#include "debug.h"
#include "defer.h"

std::shared_mutex   g_hookedCacheSync;
std::set<HWND>      g_hookedCache;
std::map<HWND, WNDPROC> g_wndProc;

void hookCustomWndProc(HWND hwnd);
void unHookChatWndProc(HWND hwnd);

LRESULT CALLBACK ChatWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_GETMINMAXINFO:
    {
        // 최소크기
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = 100;
        return 0;
    }

    case WM_CLOSE:
        unHookChatWndProc(hwnd);
    }

    return CallWindowProcW(g_wndProc[hwnd], hwnd, uMsg, wParam, lParam);
}

void hookCustomWndProc(HWND hwnd)
{
    if (GetWindowLongW(hwnd, GWL_WNDPROC) != (LONG)ChatWndProc)
    {
        WNDPROC prevProc = (WNDPROC)SetWindowLongW(hwnd, GWL_WNDPROC, (LONG)ChatWndProc);
        g_wndProc[hwnd] = prevProc;
    }
}
void unHookChatWndProc(HWND hwnd)
{
    if (GetWindowLongW(hwnd, GWL_WNDPROC) == (LONG)ChatWndProc)
    {
        (WNDPROC)SetWindowLongW(hwnd, GWL_WNDPROC, (LONG)g_wndProc[hwnd]);
        g_wndProc.erase(hwnd);
    }
}

VOID CALLBACK ChatWindowHookProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD idEventThread, DWORD dwmsEventTime)
{
    g_hookedCacheSync.lock();
    defer(g_hookedCacheSync.unlock());

    if (g_hookedCache.insert(hwnd).second)
        return;

    DebugLog(L"ChatWindowHookProc : event = %d / hwnd = %p / idObject = %d / idChild = %d", event, hwnd, idObject, idChild);

    WCHAR className[MAX_PATH];
    if (GetClassNameW(hwnd, className, MAX_PATH) == 0)
    {
        g_hookedCache.erase(hwnd);
        return;
    }

    DebugLog(L"ChatWindowHookProc : className = %ws", className);

    if (std::wcsncmp(className, L"#32770", 6) == 0)
        hookCustomWndProc(hwnd);
}

DWORD CALLBACK AttachChatHookThread(PVOID param)
{
    auto hHook = SetWinEventHook(EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW, NULL, ChatWindowHookProc, GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
    DebugLog(L"SetWinEventHook : %p", hHook);
    if (hHook == NULL)
        return 0;

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (msg.message > 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    UnhookWinEvent(hHook);

    return 0;
}

void AttachChatHook()
{
    auto hThread = CreateThread(NULL, 0, AttachChatHookThread, NULL, 0, NULL);
    if (hThread != NULL)
        CloseHandle(hThread);
}
