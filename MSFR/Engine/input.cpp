#include <Windows.h>
#include <windowsx.h>
#include <array>

#include "Input.h"
#include "Engine.h"

namespace
{
    struct VkBinding
    {
        std::uintptr_t vk = 0;
        InputKey::Keyboard key = InputKey::Keyboard::None;
    };

    constexpr std::array<VkBinding, 17> kVkBindings{ {
        { VK_RETURN, InputKey::Keyboard::Enter },
        { VK_ESCAPE, InputKey::Keyboard::Escape },
        { VK_SPACE, InputKey::Keyboard::Space },
        { VK_BACK, InputKey::Keyboard::BackSpace },
        { VK_SHIFT, InputKey::Keyboard::Shift },
        { VK_LSHIFT, InputKey::Keyboard::Shift },
        { VK_RSHIFT, InputKey::Keyboard::Shift },
        { VK_LEFT, InputKey::Keyboard::Left },
        { VK_RIGHT, InputKey::Keyboard::Right },
        { VK_UP, InputKey::Keyboard::Up },
        { VK_DOWN, InputKey::Keyboard::Down },
        { VK_F1, InputKey::Keyboard::F1 },
        { VK_F3, InputKey::Keyboard::F3 },
        { VK_OEM_3, InputKey::Keyboard::Tilde },
        { static_cast<std::uintptr_t>('1'), InputKey::Keyboard::Num1 },
        { static_cast<std::uintptr_t>('2'), InputKey::Keyboard::Num2 },
        { static_cast<std::uintptr_t>('H'), InputKey::Keyboard::H },
    } };

    InputKey::Keyboard LookupMappedKey(std::uintptr_t vk)
    {
        for (const VkBinding& binding : kVkBindings)
        {
            if (binding.vk == vk)
                return binding.key;
        }

        return InputKey::Keyboard::None;
    }
}

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

    mousePressedThisFrame = false;
    mouseReleasedThisFrame = false;

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

bool Input::IsKeyPressed(InputKey::Keyboard key) const
{
    return keyPressedThisFrame[static_cast<int>(key)];
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

InputKey::Keyboard Input::VKToKeyboard(std::uintptr_t vk)
{
    const InputKey::Keyboard mapped = LookupMappedKey(vk);
    if (mapped != InputKey::Keyboard::None)
        return mapped;

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
    case InputKey::Keyboard::F3: return "F3";
    case InputKey::Keyboard::Tilde: return "Tilde";
    case InputKey::Keyboard::Num1: return "1";
    case InputKey::Keyboard::Num2: return "2";
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

void Input::OnMouseMove(float x, float y)
{
    mousePos.x() = x;
    mousePos.y() = y;
}

void Input::OnMouseDown(int button)
{
    if (button == 1)
    {
        isMouseDown = true;
        mousePressedThisFrame = true;
    }
}

void Input::OnMouseUp(int button)
{
    if (button == 1)
    {
        isMouseDown = false;
        mouseReleasedThisFrame = true;
    }
}

void Input::OnWin32Message(std::uint32_t msg, std::uintptr_t wParam, std::intptr_t lParam)
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
        const LPARAM lp = static_cast<LPARAM>(lParam);
        int x = GET_X_LPARAM(lp);
        int y = GET_Y_LPARAM(lp);
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
            Engine::GetLogger().LogEvent(std::string("[Input] DOWN: ") + KeyboardToString(pressed));

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
            Engine::GetLogger().LogEvent(std::string("[Input] UP  : ") + KeyboardToString(released));

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






