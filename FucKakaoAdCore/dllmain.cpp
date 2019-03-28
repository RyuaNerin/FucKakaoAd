#include <windows.h>

#include "main.h"
#include "debug.h"
#include "chat.h"

DWORD g_pid = NULL;
HINSTANCE g_hInst = NULL;

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinstDLL);

        DebugLog(L"DLL_PROCESS_ATTACH");

        g_hInst = hinstDLL;
        g_pid = GetCurrentProcessId();

        AttachMainWindow();
        AttachChatHook();
    }

    return TRUE;
}
