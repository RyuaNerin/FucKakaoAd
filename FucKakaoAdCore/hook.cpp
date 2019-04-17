#include "hook.h"

#include <Windows.h>

#include <MinHook.h>

#include <map>
#include <shared_mutex>

#include "dllmain.h"
#include "debug.h"
#include "defer.h"

#define CHAT_MIN_WIDTH  230
#define CHAT_MIN_HEIGHT 230

#define MAIN_MIN_WIDTH  180
#define MAIN_MIN_HEIGHT 400

std::map<HWND, WNDPROC> g_wndProc;
std::shared_mutex       g_wndProcMut;

typedef BOOL(WINAPI *DefNtUserSetWindowPos)(HWND hWnd, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags);
typedef LONG(WINAPI *DefNtUserSetWindowLong)(HWND hWnd, int nIndex, LONG dwNewLong, BOOL ansi);
typedef BOOL(WINAPI *DefNtUserShowWindow)(HWND hWnd, int nCmdShow);

DefNtUserSetWindowPos     NativeNtUserSetWindowPos;
DefNtUserSetWindowLong    NativeNtUserSetWindowLong;
DefNtUserShowWindow       NativeNtUserShowWindow;

BOOL WINAPI NewNtUserSetWindowPos(HWND hWnd, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags)
{
    if ((hWnd == g_kakaoMain || hWnd == g_kakaoLock) &&
        hWndInsertAfter == 0 &&
        uFlags == SWP_NOZORDER)
    {
        cy += g_kakaoAdHeight;
    }

    return NativeNtUserSetWindowPos(hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
}

LONG WINAPI NewNtUserSetWindowLong(HWND hWnd, int nIndex, LONG dwNewLong, BOOL ansi)
{
    if (nIndex == GWL_WNDPROC)
    {
        g_wndProcMut.lock();
        defer(g_wndProcMut.unlock());

        auto found = g_wndProc.find(hWnd);
        if (found != g_wndProc.end())
        {
            auto old = found->second;
            g_wndProc[hWnd] = (WNDPROC)dwNewLong;

            return (LONG)old;
        }
    }

    return NativeNtUserSetWindowLong(hWnd, nIndex, dwNewLong, ansi);
}

BOOL WINAPI NewNtUserShowWindow(HWND hWnd, int nCmdShow)
{
    if (hWnd == g_kakaoAd && nCmdShow == SW_SHOW)
        return TRUE;

    return NativeNtUserShowWindow(hWnd, nCmdShow);
}

void initApiHook()
{
    MH_STATUS st;

    if (MH_Initialize() == MH_OK)
    {
        DebugLog(L"MH_Initialize");
        auto hWin32u = LoadLibraryW(L"win32u.dll");
        auto ntUserSetWindowPos  = GetProcAddress(hWin32u, "NtUserSetWindowPos");
        auto ntUserSetWindowLong = GetProcAddress(hWin32u, "NtUserSetWindowLong");
        auto ntUSerShowWindow    = GetProcAddress(hWin32u, "NtUserShowWindow");

        st = MH_CreateHook(ntUserSetWindowPos , &NewNtUserSetWindowPos , (LPVOID*)&NativeNtUserSetWindowPos ); DebugLog(L"MH_CreateHook / NtUserSetWindowPos  : %d", st);
        st = MH_CreateHook(ntUserSetWindowLong, &NewNtUserSetWindowLong, (LPVOID*)&NativeNtUserSetWindowLong); DebugLog(L"MH_CreateHook / NtUserSetWindowLong : %d", st);
        st = MH_CreateHook(ntUSerShowWindow   , &NewNtUserShowWindow   , (LPVOID*)&NativeNtUserShowWindow   ); DebugLog(L"MH_CreateHook / NtUserShowWindow    : %d", st);

        ////////////////////////////////////////////////////////////////////////////////////////////////////

        st = MH_EnableHook(ntUserSetWindowPos ); DebugLog(L"MH_EnableHook / NtUserSetWindowPos  : %d", st);
        st = MH_EnableHook(ntUserSetWindowLong); DebugLog(L"MH_EnableHook / NtUserSetWindowLong : %d", st);
        st = MH_EnableHook(ntUSerShowWindow   ); DebugLog(L"MH_EnableHook / NtUSerShowWindow    : %d", st);
    }
}

void hookCustomWndProc(HWND hwnd, WNDPROC proc)
{
    if (GetWindowLongW(hwnd, GWL_WNDPROC) != (LONG)proc)
    {
        WNDPROC prevProc = (WNDPROC)SetWindowLongW(hwnd, GWL_WNDPROC, (LONG)proc);

        g_wndProcMut.lock();
        g_wndProc[hwnd] = prevProc;
        g_wndProcMut.unlock();
    }
}

void unhookCustomWndProc(HWND hwnd)
{
    g_wndProcMut.lock();
    defer(g_wndProcMut.unlock());

    g_wndProc.erase(hwnd);
}

LRESULT CALLBACK wndProcAd(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void hookKakaoAd(HWND hwnd)
{
    hookCustomWndProc(hwnd, wndProcAd);
}

LRESULT CALLBACK wndProcTalk(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void hookKakaoTalk(HWND hwnd)
{
    hookCustomWndProc(hwnd, wndProcTalk);
}

LRESULT CALLBACK wndProcChat(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void hookKakaoChat(HWND hwnd)
{
    hookCustomWndProc(hwnd, wndProcChat);
}

LRESULT CALLBACK wndProcAd(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
    case WM_NCPAINT:
        // 드로잉 안함
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    g_wndProcMut.lock_shared();
    auto wndProc = g_wndProc[hwnd];
    g_wndProcMut.unlock_shared();

    return CallWindowProcW(wndProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK wndProcTalk(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_GETMINMAXINFO:
    {
        // 최소크기
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = MAIN_MIN_WIDTH;
        lpMMI->ptMinTrackSize.y = MAIN_MIN_HEIGHT;
        return 0;
    }
    }

    g_wndProcMut.lock_shared();
    auto wndProc = g_wndProc[hwnd];
    g_wndProcMut.unlock_shared();

    return CallWindowProcW(wndProc, hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK wndProcChat(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_GETMINMAXINFO:
    {
        // 최소크기
        LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
        lpMMI->ptMinTrackSize.x = CHAT_MIN_WIDTH;
        lpMMI->ptMinTrackSize.y = CHAT_MIN_HEIGHT;
        return 0;
    }
    }

    g_wndProcMut.lock_shared();
    auto wndProc = g_wndProc[hwnd];
    g_wndProcMut.unlock_shared();

    return CallWindowProcW(wndProc, hwnd, uMsg, wParam, lParam);
}
