#pragma once

#include <Windows.h>

void initApiHook();

void hookKakaoAd(HWND hwnd);
void hookKakaoTalk(HWND hwnd);
void hookKakaoChat(HWND hwnd);

void unhookCustomWndProc(HWND hwnd);
