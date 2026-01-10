#include <windowsx.h>L

#include "Input.h"
#include "Engine.h" // Engine::GetInput(), Engine::GetLogger()

// ------------------------
// InputKey
// ------------------------
InputKey::InputKey(Keyboard button) : button(button) {}

bool InputKey::IsKeyDown() const
{
    return Engine::GetInput().IsKeyDown(button);
}

bool InputKey::IsKeyReleased() const
{
    return Engine::GetInput().IsKeyReleased(button);
}

// ------------------------
// Input
// ------------------------
Input::Input()
{
    isKeyDownList.resize(static_cast<int>(InputKey::Keyboard::Count), false);
    wasKeyDownList.resize(static_cast<int>(InputKey::Keyboard::Count), false);
}

void Input::Update()
{
    wasKeyDownList = isKeyDownList;

    if (isMouseDown && isMouseUp)
    {
        isMousePressed = true;
        isMouseDown = false;
        isMouseUp = false;
    }
    else
    {
        isMousePressed = false;
    }

}

bool Input::IsKeyDown(InputKey::Keyboard key) const
{
    return isKeyDownList[static_cast<int>(key)];
}

bool Input::IsKeyReleased(InputKey::Keyboard key) const
{
    const int idx = static_cast<int>(key);
    return (!isKeyDownList[idx] && wasKeyDownList[idx]);
}

void Input::SetKeyDown(InputKey::Keyboard key, bool value)
{
    isKeyDownList[static_cast<int>(key)] = value;
}

InputKey::Keyboard Input::VKToKeyboard(WPARAM vk)
{
    switch (vk)
    {
    case VK_RETURN:  return InputKey::Keyboard::Enter;
    case VK_ESCAPE:  return InputKey::Keyboard::Escape;
    case VK_SPACE:   return InputKey::Keyboard::Space;
    case VK_BACK:    return InputKey::Keyboard::BackSpace;
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:  return InputKey::Keyboard::Shift;
    case VK_LEFT:    return InputKey::Keyboard::Left;
    case VK_RIGHT:   return InputKey::Keyboard::Right;
    case VK_UP:      return InputKey::Keyboard::Up;
    case VK_DOWN:    return InputKey::Keyboard::Down;
    default:
        break;
    }

    // A~Z
    if (vk >= 'A' && vk <= 'Z')
    {
        int offset = static_cast<int>(vk - 'A');
        return static_cast<InputKey::Keyboard>(static_cast<int>(InputKey::Keyboard::A) + offset);
    }

    return InputKey::Keyboard::None;
}

void Input::OnWin32Message(UINT msg, WPARAM wParam, LPARAM lParam)
{
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

        // ------------------------
        // Mouse
        // ------------------------
    case WM_MOUSEMOVE:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        mousePos.x = static_cast<float>(x);
        mousePos.y = static_cast<float>(y);
        return;
    }

    case WM_LBUTTONDOWN:
        isMouseDown = true;
        return;

    case WM_LBUTTONUP:
        isMouseUp = true;
        return;

        // ------------------------
        // Keyboard
        // ------------------------
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        const bool wasDown = (lParam & (1 << 30)) != 0;
        if (wasDown)
            return;

        InputKey::Keyboard pressed = VKToKeyboard(wParam);
        if (pressed != InputKey::Keyboard::None)
        {
            if (pressed != InputKey::Keyboard::Enter)
                pause = false;

            SetKeyDown(pressed, true);
            Engine::GetLogger().LogDebug("on_key_pressed");
        }
        return;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        InputKey::Keyboard released = VKToKeyboard(wParam);
        if (released != InputKey::Keyboard::None)
        {
            SetKeyDown(released, false);
            Engine::GetLogger().LogDebug("on_key_released");
        }
        return;
    }

    case WM_SIZE:
    {

        if (wParam == SIZE_MINIMIZED)
            pause = true;
        else
            pause = false;

        return;
    }

    case WM_MOVE:
    case WM_SHOWWINDOW:
        return;

    default:
        return;
    }
}
