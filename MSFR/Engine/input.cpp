#include "Input.h"
#include "Engine.h"
#include <algorithm>

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
