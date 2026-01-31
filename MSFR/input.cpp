#include <windowsx.h>

#include "Input.h"
#include "Engine.h"

InputKey::InputKey(Keyboard button) : button(button) {}

bool InputKey::IsKeyDown() const
{
    return Engine::GetInput().IsKeyDown(button);
}

bool InputKey::IsKeyReleased() const
{
    return Engine::GetInput().IsKeyReleased(button);
}

Input::Input()
{
    const int n = static_cast<int>(InputKey::Keyboard::Count);
    isKeyDownList.assign(n, false);
    keyReleasedThisFrame.assign(n, false);
    keyPressedThisFrame.assign(n, false);
}

void Input::Update()
{
    std::fill(keyReleasedThisFrame.begin(), keyReleasedThisFrame.end(), false);
    std::fill(keyPressedThisFrame.begin(), keyPressedThisFrame.end(), false);

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
    return keyReleasedThisFrame[static_cast<int>(key)];
}

void Input::OnKeyDown(InputKey::Keyboard k)
{
    if (k == InputKey::Keyboard::None) return;
    const int idx = static_cast<int>(k);
    isKeyDownList[idx] = true;
    keyPressedThisFrame[idx] = true;
}

void Input::OnKeyUp(InputKey::Keyboard k)
{
    if (k == InputKey::Keyboard::None) return;
    const int idx = static_cast<int>(k);
    isKeyDownList[idx] = false;
    keyReleasedThisFrame[idx] = true;
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
    case VK_F1:      return InputKey::Keyboard::F1;
    default:
        break;
    }

    if (vk >= 'A' && vk <= 'Z')
    {
        int offset = static_cast<int>(vk - 'A');
        return static_cast<InputKey::Keyboard>(static_cast<int>(InputKey::Keyboard::A) + offset);
    }

    return InputKey::Keyboard::None;
}

const char* Input::KeyboardToString(InputKey::Keyboard key)
{
    switch (key)
    {
    case InputKey::Keyboard::None: return "None";
    case InputKey::Keyboard::Enter: return "Enter";
    case InputKey::Keyboard::Escape: return "Escape";
    case InputKey::Keyboard::Space: return "Space";
    case InputKey::Keyboard::BackSpace: return "BackSpace";
    case InputKey::Keyboard::Shift: return "Shift";
    case InputKey::Keyboard::Left: return "Left";
    case InputKey::Keyboard::Right: return "Right";
    case InputKey::Keyboard::Up: return "Up";
    case InputKey::Keyboard::Down: return "Down";
    case InputKey::Keyboard::F1: return "F1";
    default:
        break;
    }

    const int a0 = static_cast<int>(InputKey::Keyboard::A);
    const int z0 = static_cast<int>(InputKey::Keyboard::Z);
    const int k = static_cast<int>(key);

    if (k >= a0 && k <= z0)
    {
        static char buf[2] = {};
        buf[0] = static_cast<char>('A' + (k - a0));
        buf[1] = '\0';
        return buf;
    }

    return "Unknown";
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

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        const bool wasDownRepeat = (lParam & (1 << 30)) != 0;
        if (wasDownRepeat)
            return;

        InputKey::Keyboard pressed = VKToKeyboard(wParam);
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
            Engine::GetLogger().LogDebug(std::string("[Input] DOWN: ") + KeyboardToString(pressed));

        return;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        InputKey::Keyboard released = VKToKeyboard(wParam);
        if (released == InputKey::Keyboard::None)
            return;

        const int idx = static_cast<int>(released);
        isKeyDownList[idx] = false;
        keyReleasedThisFrame[idx] = true;

        if (keyLogEnabled)
            Engine::GetLogger().LogDebug(std::string("[Input] UP  : ") + KeyboardToString(released));

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
