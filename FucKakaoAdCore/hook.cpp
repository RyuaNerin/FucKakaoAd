#include "hook.h"

#include <Windows.h>

#include <map>

#include "dllmain.h"
#include "debug.h"
#include "adblock.h"

#define CHAT_MIN_WIDTH  230
#define CHAT_MIN_HEIGHT 230

#define MAIN_MIN_WIDTH  180
#define MAIN_MIN_HEIGHT 400

std::map<HWND, WNDPROC> g_wndProc;

void hookCustomWndProc(HWND hwnd, WNDPROC proc)
{
    if (GetWindowLongW(hwnd, GWL_WNDPROC) != (LONG)proc)
    {
        WNDPROC prevProc = (WNDPROC)SetWindowLongW(hwnd, GWL_WNDPROC, (LONG)proc);
        DebugLog("hookCustomWndProc : %p (%p -> %p)", hwnd, prevProc, proc);
        g_wndProc[hwnd] = prevProc;
    }
}
void unhookCustomWndProc(HWND hwnd)
{
    auto f = g_wndProc.find(hwnd);
    if (f != g_wndProc.end())
    {
        SetWindowLongW(hwnd, GWL_WNDPROC, (LONG)f->second);
        g_wndProc.erase(hwnd);
    }
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

    return CallWindowProcW(g_wndProc[hwnd], hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK wndProcApp(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

    return CallWindowProcW(g_wndProc[hwnd], hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK wndProcAd(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_PAINT:
    case WM_NCPAINT:
        // 드로잉 안함
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);

    case WM_SHOWWINDOW:
        if (wParam == TRUE)
            ShowWindow(hwnd, SW_HIDE);
        return 0;
    }

    return CallWindowProcW(g_wndProc[hwnd], hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK wndProcMainLock(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_SIZE:
        if (wParam == SIZE_RESTORED)
        {
            if (adblock(hwnd))
                return 0;
        }
        break;
    case WM_SHOWWINDOW:
        adblock(hwnd);
        break;
    }

    return CallWindowProcW(g_wndProc[hwnd], hwnd, uMsg, wParam, lParam);
}
