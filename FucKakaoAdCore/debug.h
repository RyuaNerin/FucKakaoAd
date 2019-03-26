#pragma once

#if _DEBUG
void DebugLog(const wchar_t* fmt, ...);
#else
#define DebugLog
#endif

#define DebugLog(x, ...)    DebugLog(L"FucKakaoAd - " x, __VA_ARGS__)