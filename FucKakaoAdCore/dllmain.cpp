#include <windows.h>

#include "main.h"
#include "debug.h"

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DebugLog(L"DLL_PROCESS_ATTACH");

        g_hInst = hinstDLL;
        g_pid = GetCurrentProcessId();

        if (Attach())
        {
            DisableThreadLibraryCalls(hinstDLL);
        }
    }

    return TRUE;
}
