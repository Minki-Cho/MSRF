#include "Input.h"
#include "Engine.h"

#include <string>

#if defined(_WIN32)
#include <Windows.h>
#include <windowsx.h>
#endif

namespace
{
#if defined(_WIN32)
    InputKey::Keyboard VKToKeyboard(std::uintptr_t vk)
    {
        switch (vk)
        {
        case VK_RETURN: return InputKey::Keyboard::Enter;
        case VK_ESCAPE: return InputKey::Keyboard::Escape;
        case VK_SPACE: return InputKey::Keyboard::Space;
        case VK_BACK: return InputKey::Keyboard::BackSpace;
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT: return InputKey::Keyboard::Shift;
        case VK_LEFT: return InputKey::Keyboard::Left;
        case VK_RIGHT: return InputKey::Keyboard::Right;
        case VK_UP: return InputKey::Keyboard::Up;
        case VK_DOWN: return InputKey::Keyboard::Down;
        case VK_F1: return InputKey::Keyboard::F1;
        case VK_F3: return InputKey::Keyboard::F3;
        case VK_OEM_3: return InputKey::Keyboard::Tilde;
        case '1': return InputKey::Keyboard::Num1;
        case '2': return InputKey::Keyboard::Num2;
        case 'H': return InputKey::Keyboard::H;
        default:
            break;
        }

        if (vk >= 'A' && vk <= 'Z')
        {
            const int offset = static_cast<int>(vk - 'A');
            return static_cast<InputKey::Keyboard>(static_cast<int>(InputKey::Keyboard::A) + offset);
        }

        return InputKey::Keyboard::None;
    }
#endif
}

void Input::OnWin32Message(std::uint32_t msg, std::uintptr_t wParam, std::intptr_t lParam)
{
#if !defined(_WIN32)
    (void)msg;
    (void)wParam;
    (void)lParam;
    return;
#else
    switch (msg)
    {
    case WM_QUIT:
        isDone = true;
        return;

    case WM_CLOSE:
        isDone = true;
        return;

    case WM_KILLFOCUS:
        pause = true;
        return;

    case WM_SETFOCUS:
        pause = false;
        return;

    case WM_MOUSEMOVE:
    {
        const LPARAM lp = static_cast<LPARAM>(lParam);
        const int x = GET_X_LPARAM(lp);
        const int y = GET_Y_LPARAM(lp);
        mousePos.x() = static_cast<float>(x);
        mousePos.y() = static_cast<float>(y);
        return;
    }

    case WM_LBUTTONDOWN:
        isMouseDown = true;
        return;

    case WM_LBUTTONUP:
        isMouseUp = true;
        return;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        const bool wasDownRepeat = (lParam & (1LL << 30)) != 0;
        if (wasDownRepeat)
            return;

        const InputKey::Keyboard pressed = VKToKeyboard(wParam);
        if (pressed == InputKey::Keyboard::None)
            return;

        if (pressed == InputKey::Keyboard::F1)
        {
            ToggleKeyLogging();
            Engine::GetLogger().LogDebug(std::string("[Input] Key logging: ") + (keyLogEnabled ? "ON" : "OFF"));
            return;
        }

        if (pressed != InputKey::Keyboard::Enter)
            pause = false;

        const int idx = static_cast<int>(pressed);
        isKeyDownList[idx] = true;
        keyPressedThisFrame[idx] = true;

        if (keyLogEnabled)
            Engine::GetLogger().LogEvent(std::string("[Input] DOWN: ") + KeyboardToString(pressed));

        return;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        const InputKey::Keyboard released = VKToKeyboard(wParam);
        if (released == InputKey::Keyboard::None)
            return;

        const int idx = static_cast<int>(released);
        isKeyDownList[idx] = false;
        keyReleasedThisFrame[idx] = true;

        if (keyLogEnabled)
            Engine::GetLogger().LogEvent(std::string("[Input] UP  : ") + KeyboardToString(released));

        return;
    }

    case WM_SIZE:
        pause = (wParam == SIZE_MINIMIZED);
        return;

    case WM_MOVE:
    case WM_SHOWWINDOW:
        return;

    default:
        return;
    }
#endif
}
