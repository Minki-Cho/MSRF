#pragma once
#include <windows.h>
#include <string>

class Window
{
public:
    Window() = default;
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void Init(const char* title, int width, int height);
    void Shutdown();

    // Pump OS messages. Returns false if WM_QUIT received.
    bool Update();

    bool IsOpen() const { return open; }
    HWND GetHWND() const { return hWnd; }
    int  GetClientWidth() const { return clientWidth; }
    int  GetClientHeight() const { return clientHeight; }
    float  GetClientWidth() { return static_cast<float>(clientWidth); }
    float  GetClientHeight() { return static_cast<float>(clientHeight); }

    // Resize signaling (Renderer reads these)
    bool ConsumeResize(int& outW, int& outH); // returns true if a resize was pending

private:
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    HWND hWnd = nullptr;
    HINSTANCE hInstance = nullptr;

    int clientWidth = 0;
    int clientHeight = 0;
    bool open = false;

    // Resize event state
    bool resizePending = false;
    int pendingW = 0;
    int pendingH = 0;
};
