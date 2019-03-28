#pragma once

#if _DEBUG
void DebugLog(const wchar_t* fmt, ...);
#define DebugLog(x, ...)    DebugLog(L"FucKakaoAd - " x, __VA_ARGS__)
#else
#define DebugLog
#endif
