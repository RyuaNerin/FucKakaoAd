#pragma once

#include <Windows.h>

#include <set>
#include <shared_mutex>

#define DEFAULT_AD_HEIGHT 101

extern DWORD g_pid;
extern HINSTANCE g_hInst;
extern HWND g_kakaoTalk;
extern HWND g_kakaoLock;
extern HWND g_kakaoMain;
extern HWND g_kakaoAd;

extern int g_kakaoAdHeight;

extern std::set<HWND> g_kakaoChat;
extern std::shared_mutex g_kakaoChatMut;

