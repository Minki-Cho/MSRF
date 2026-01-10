#include "Window.h"
#include <stdexcept>
#include <cstring>

Window::~Window()
{
    Shutdown();
}

void Window::Init(const char* title, int width, int height)
{
    if (open)
        return;

    hInstance = GetModuleHandle(nullptr);
    clientWidth = width;
    clientHeight = height;

    const wchar_t* CLASS_NAME = L"PulseWin32WindowClass";

    WNDCLASS wc{};
    wc.lpfnWndProc = &Window::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);

    std::wstring wtitle(title, title + std::strlen(title));

    hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        wtitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, hInstance, this
    );

    if (!hWnd)
        throw std::runtime_error("Failed to create Win32 window.");

    ShowWindow(hWnd, SW_SHOW);

    open = true;
}

void Window::Shutdown()
{
    if (!open && !hWnd)
        return;

    if (hWnd)
    {
        DestroyWindow(hWnd);
        hWnd = nullptr;
    }

    open = false;
    resizePending = false;
}

bool Window::Update()
{
    MSG msg{};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            open = false;
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return open;
}

bool Window::ConsumeResize(int& outW, int& outH)
{
    if (!resizePending)
        return false;

    resizePending = false;
    outW = pendingW;
    outH = pendingH;
    return true;
}

LRESULT CALLBACK Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Window* window = nullptr;

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        window = reinterpret_cast<Window*>(cs->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    else
    {
        window = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    }

    if (window)
        return window->HandleMessage(hWnd, msg, wParam, lParam);

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT Window::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        open = false;
        return 0;

    case WM_SIZE:
    {
        const int newW = LOWORD(lParam);
        const int newH = HIWORD(lParam);

        // minimized => 0,0
        if (newW > 0 && newH > 0)
        {
            clientWidth = newW;
            clientHeight = newH;

            resizePending = true;
            pendingW = newW;
            pendingH = newH;
        }
        return 0;
    }

    default:
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}
