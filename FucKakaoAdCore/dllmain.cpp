#include "dllmain.h"

#include <windows.h>

#include <shared_mutex>

#include "main.h"
#include "debug.h"

DWORD     g_pid       = NULL;
HINSTANCE g_hInst     = NULL;

HWND g_kakaoLogin    = NULL;
HWND g_kakaoTalk     = NULL;
HWND g_kakaoTalkLock = NULL;
HWND g_kakaoTalkMain = NULL;
HWND g_kakaoTalkAd   = NULL;

int g_kakaoAdHeight = DEFAULT_AD_HEIGHT;

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);

        DebugLog(L"DLL_PROCESS_ATTACH");

        g_hInst = hinstDLL;
        g_pid   = GetCurrentProcessId();

        Attach();
    }

    return TRUE;
}
