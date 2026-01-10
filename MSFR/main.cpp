#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

static ID3D11Device* gDevice = nullptr;
static ID3D11DeviceContext* gContext = nullptr;
static IDXGISwapChain* gSwapChain = nullptr;
static ID3D11RenderTargetView* gRTV = nullptr;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static void CleanupD3D()
{
    if (gContext) gContext->ClearState();
    if (gRTV) gRTV->Release();
    if (gSwapChain) gSwapChain->Release();
    if (gContext) gContext->Release();
    if (gDevice) gDevice->Release();
}

static void CreateRenderTarget()
{
    ID3D11Texture2D* backBuffer = nullptr;
    gSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    gDevice->CreateRenderTargetView(backBuffer, nullptr, &gRTV);
    backBuffer->Release();
}

static bool InitD3D(HWND hWnd, int width, int height)
{
    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferCount = 2; // double buffering
    scd.BufferDesc.Width = width;
    scd.BufferDesc.Height = height;
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    scd.Flags = 0;

    UINT createFlags = 0;
#if defined(_DEBUG)

    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL chosenLevel{};

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,                    
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createFlags,
        featureLevels,
        (UINT)_countof(featureLevels),
        D3D11_SDK_VERSION,
        &scd,
        &gSwapChain,
        &gDevice,
        &chosenLevel,
        &gContext
    );

    if (FAILED(hr))
        return false;

    CreateRenderTarget();
    return true;
}

static void RenderFrame()
{
    float clearColor[4] = { 0.1f, 0.2f, 0.4f, 1.0f };

    gContext->OMSetRenderTargets(1, &gRTV, nullptr);
    gContext->ClearRenderTargetView(gRTV, clearColor);

    gSwapChain->Present(1, 0);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    const wchar_t* CLASS_NAME = L"DX11WindowClass";

    WNDCLASS wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    RegisterClass(&wc);

    int width = 1280;
    int height = 720;

    HWND hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        L"MSFR Engine ver 1.0",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hWnd) return 0;

    ShowWindow(hWnd, SW_SHOW);

    if (!InitD3D(hWnd, width, height))
    {
        MessageBox(hWnd, L"Failed to init D3D11", L"Error", MB_OK);
        return 0;
    }

    MSG msg{};
    while (true)
    {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                CleanupD3D();
                return 0;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        RenderFrame();
    }
}
