#include "hook.h"

#include <Windows.h>

#include <MinHook.h>

#include "dllmain.h"
#include "debug.h"

typedef BOOL(WINAPI *DefSetWindowPos)(HWND hwnd, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags);

DefSetWindowPos SetWindowPosOrig;

BOOL WINAPI NewSetWindowPos(HWND hwnd, HWND hWndInsertAfter, int x, int y, int cx, int cy, UINT uFlags)
{
    if ((hwnd == g_kakaoMain || hwnd == g_kakaoLock) &&
        hWndInsertAfter == 0 &&
        uFlags == SWP_NOZORDER)
    {
        cy += AD_HEIGHT;
    }

    return SetWindowPosOrig(hwnd, hWndInsertAfter, x, y, cx, cy, uFlags);
}

void initApiHook()
{
    if (MH_Initialize() == MH_OK)
    {
        DebugLog(L"MH_Initialize");
        auto hWin32u = LoadLibraryW(L"win32u.dll");
        auto hProc = GetProcAddress(hWin32u, "NtUserSetWindowPos");

        auto res = MH_CreateHook(hProc, &NewSetWindowPos, (LPVOID*)&SetWindowPosOrig);
        DebugLog(L"MH_CreateHook : %d", res);

        res = MH_EnableHook(hProc);
        DebugLog(L"MH_EnableHook : %d", res);
    }
}
