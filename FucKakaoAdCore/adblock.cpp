#include "adblock.h"

#include <Windows.h>

#include "dllmain.h"

bool adblock(HWND hwnd)
{
    // 계산된 높이랑 다를 경우 다시 설정
    RECT rectApp;
    RECT rectView;
    if (GetWindowRect(g_kakaoMain, &rectApp ) == TRUE &&
        GetWindowRect(hwnd,        &rectView) == TRUE)
    {
        int heightCur  = rectView.bottom - rectView.top;
        int heightNoAd = (rectApp.bottom - rectApp.top) - (rectView.top - rectApp.top) - (rectView.left - rectApp.left);

        if (heightNoAd != heightCur)
        {
            SetWindowPos(
                hwnd,
                NULL,
                0,
                0,
                rectView.right - rectView.left,
                heightNoAd,
                SWP_NOMOVE);

            return true;
        }
    }

    return false;
}
