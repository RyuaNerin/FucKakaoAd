#pragma once

#include <Windows.h>

extern DWORD g_pid;
extern HINSTANCE g_hInst;

bool setHeight(HWND hwnd);
bool wndProcMainLock(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProcApp (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProcAd  (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProcMain(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProcLock(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void hookApp();
void hookAd();
void hookMain();
void hookLock();

BOOL CALLBACK FindAnyKakaoWindow(HWND hwnd, LPARAM lParam);
BOOL CALLBACK FindKakaoHwndProc(HWND hwnd, LPARAM lParam);
BOOL CALLBACK FindKakaoAd(HWND hwnd, LPARAM lParam);

void hideKakaoAd();
bool Attach();
