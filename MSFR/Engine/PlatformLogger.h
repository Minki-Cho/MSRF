#pragma once

namespace platform
{
    void DebugOutput(const char* text) noexcept;
    void RestoreWindowFocus(void* nativeWindowHandle) noexcept;
}

