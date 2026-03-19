#include "PlatformLogger.h"

#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#endif

namespace platform
{
    void DebugOutput(const char* text) noexcept
    {
#if defined(_WIN32)
        if (text)
            OutputDebugStringA(text);
#else
        (void)text;
#endif
    }

    void RestoreWindowFocus(void* nativeWindowHandle) noexcept
    {
#if defined(_WIN32)
        HWND hwnd = static_cast<HWND>(nativeWindowHandle);
        if (!hwnd)
            return;

        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
#else
        (void)nativeWindowHandle;
#endif
    }
}

