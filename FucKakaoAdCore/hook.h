#pragma once

#include <Windows.h>

void hookCustomWndProc(HWND hwnd, WNDPROC proc);
void unhookCustomWndProc(HWND hwnd);

LRESULT CALLBACK wndProcChat    (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK wndProcApp     (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK wndProcAd      (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK wndProcMainLock(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
