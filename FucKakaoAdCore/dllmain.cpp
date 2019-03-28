#include <windows.h>

#include "main.h"
#include "debug.h"

DWORD     g_pid       = NULL;
HINSTANCE g_hInst     = NULL;
HWND      g_kakaoMain = NULL;

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
