#include <Windows.h>

#include <memory>

#include <stdarg.h>

void DebugLog(const wchar_t* fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    int len = vswprintf(NULL, 0, fmt, va) + 2;
    std::unique_ptr<wchar_t[]> buff(new wchar_t[len]);

    wvsprintfW(buff.get(), fmt, va);

    va_end(va);

    OutputDebugStringW(buff.get());
}