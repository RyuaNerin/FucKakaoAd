#pragma once

#include <Windows.h>

extern HHOOK g_hookWndProc;
extern HHOOK g_hookWndProcRet;

LRESULT CALLBACK wndProcHook   (int code, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK wndProcHookRet(int code, WPARAM wParam, LPARAM lParam);
