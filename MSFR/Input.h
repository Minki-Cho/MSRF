#pragma once
#include <windows.h>
#include <vector>
#include <cstdint>

#include "vec2.h"

class InputKey
{
public:
    enum class Keyboard
    {
        None, Enter, Escape, Space, BackSpace, Shift, Left, Up, Right, Down,
        F1,
        A, B, C, D, E, F, G, H, I, J,
        K, L, M, N, O, P, Q, R, S, T,
        U, V, W, X, Y, Z,
        Count
    };

    explicit InputKey(Keyboard button);
    bool IsKeyDown() const;
    bool IsKeyReleased() const;

private:
    Keyboard button;
};

class Input
{
public:
    Input();

    void Update();
    void OnWin32Message(UINT msg, WPARAM wParam, LPARAM lParam);

    bool IsKeyDown(InputKey::Keyboard key) const;
    bool IsKeyReleased(InputKey::Keyboard key) const;

    void OnKeyDown(InputKey::Keyboard k);
    void OnKeyUp(InputKey::Keyboard k);

    bool getIsdone() const { return isDone; }
    bool getPause()  const { return pause; }
    void setPause(bool b) { pause = b; }

    vec2 GetMousePos() const { return mousePos; }
    bool GetMousePressed() const { return isMousePressed; }
    bool GetMouseDown() const { return isMouseDown; }
    bool GetMouseUp() const { return isMouseUp; }

    void toggleFullScreen() { fullScreen = !fullScreen; }
    bool GetFullScrean() const { return fullScreen; }

    void EnableKeyLogging(bool enable) { keyLogEnabled = enable; }
    bool IsKeyLoggingEnabled() const { return keyLogEnabled; }
    void ToggleKeyLogging() { keyLogEnabled = !keyLogEnabled; }

private:
    static InputKey::Keyboard VKToKeyboard(WPARAM vk);
    static const char* KeyboardToString(InputKey::Keyboard key);

private:
    std::vector<bool> isKeyDownList;
    std::vector<bool> keyReleasedThisFrame;
    std::vector<bool> keyPressedThisFrame;

    bool isDone = false;
    bool pause = false;

    vec2 mousePos = { 0,0 };

    bool isMouseDown = false;
    bool isMouseUp = false;
    bool isMousePressed = false;

    bool fullScreen = false;

    bool keyLogEnabled = false;
};
