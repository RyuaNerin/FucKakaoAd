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

HHOOK g_hookWndProc;
HHOOK g_hookWndProcRet;

LRESULT CALLBACK wndProcHook(int code, WPARAM wParam, LPARAM lParam)
{
    auto cpw = (PCWPSTRUCT)lParam;

    if (cpw->hwnd == g_kakaoMain ||
        cpw->hwnd == g_kakaoLock)
    {
        switch (cpw->message)
        {
        case WM_SHOWWINDOW:
            adblock(cpw->hwnd);
            return 0;
        }
    }
    else if (cpw->hwnd == g_kakaoAd)
    {
        switch (cpw->message)
        {
        case WM_PAINT:
        case WM_NCPAINT:
            // 드로잉 안함
            return DefWindowProcW(cpw->hwnd, cpw->message, cpw->wParam, cpw->lParam);

        case WM_SHOWWINDOW:
            if (cpw->wParam == TRUE)
                ShowWindow(cpw->hwnd, SW_HIDE);
            return 0;
        }
    }

    return CallNextHookEx(g_hookWndProc, code, wParam, lParam);
}
LRESULT CALLBACK wndProcHookRet(int code, WPARAM wParam, LPARAM lParam)
{
    auto cpw = (PCWPSTRUCT)lParam;
    DebugLog(L"Hwnd : %p / msg = %d / lparam = %p / wParam = %p", cpw->hwnd, cpw->message, cpw->lParam, cpw->wParam);

    if (cpw->hwnd == g_kakaoMain ||
        cpw->hwnd == g_kakaoLock)
    {
        switch (cpw->message)
        {
        case WM_SIZE:
            if (cpw->wParam == SIZE_RESTORED)
            {
                if (adblock(cpw->hwnd))
                    return 0;
            }
            break;
        }
    }
    else if (cpw->hwnd == g_kakaoTalk)
    {
        switch (cpw->message)
        {
        case WM_GETMINMAXINFO:
        {
            // 최소크기
            auto lpMMI = (LPMINMAXINFO)cpw->lParam;
            lpMMI->ptMinTrackSize.x = MAIN_MIN_WIDTH;
            lpMMI->ptMinTrackSize.y = MAIN_MIN_HEIGHT;
            return 0;
        }
        }
    }
    else
    {
        g_kakaoChatMut.lock_shared();
        bool isChatWindow = g_kakaoChat.find(cpw->hwnd) != g_kakaoChat.end();
        g_kakaoChatMut.unlock_shared();

        if (isChatWindow)
        {
            switch (cpw->message)
            {
            case WM_GETMINMAXINFO:
            {
                // 최소크기
                auto lpMMI = (LPMINMAXINFO)cpw->lParam;
                lpMMI->ptMinTrackSize.x = CHAT_MIN_WIDTH;
                lpMMI->ptMinTrackSize.y = CHAT_MIN_HEIGHT;
                return 0;
            }
            }
        }
    }

    return CallNextHookEx(g_hookWndProcRet, code, wParam, lParam);
}
