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

    // Call once per frame (after pumping messages)
    void Update();

    // Window WndProc에서 메시지 받을 때마다 호출
    void OnWin32Message(UINT msg, WPARAM wParam, LPARAM lParam);

    bool IsKeyDown(InputKey::Keyboard key) const;
    bool IsKeyReleased(InputKey::Keyboard key) const;
    void SetKeyDown(InputKey::Keyboard key, bool value);

    // SDL의 getIsdone / pause 느낌 유지
    bool getIsdone() const { return isDone; }
    bool getPause()  const { return pause; }
    void setPause(bool b) { pause = b; }

    math::vec2 GetMousePos() const { return mousePos; }
    bool GetMousePressed() const { return isMousePressed; }
    bool GetMouseDown() const { return isMouseDown; }
    bool GetMouseUp() const { return isMouseUp; }

    // (옵션) 풀스크린 토글은 Window가 담당하는 게 맞음.
    // 여기서는 플래그만 유지해도 됨.
    void toggleFullScreen() { fullScreen = !fullScreen; }
    bool GetFullScrean() const { return fullScreen; }

private:
    static InputKey::Keyboard VKToKeyboard(WPARAM vk);

private:
    std::vector<bool> isKeyDownList;
    std::vector<bool> wasKeyDownList;

    bool isDone = false;
    bool pause = false;

    math::vec2 mousePos = { 0,0 };

    bool isMouseDown = false;
    bool isMouseUp = false;
    bool isMousePressed = false;

    bool fullScreen = false;
};
