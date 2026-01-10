#include "RendererDX11.h"
#include <stdexcept>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

RendererDX11::~RendererDX11()
{
    Shutdown();
}

bool RendererDX11::Init(HWND hWnd, int w, int h)
{
    width = w;
    height = h;

    DXGI_SWAP_CHAIN_DESC scd{};
    scd.BufferCount = 2;
    scd.BufferDesc.Width = w;
    scd.BufferDesc.Height = h;
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
        &swapChain,
        &device,
        &chosenLevel,
        &context
    );

    if (FAILED(hr))
        return false;

    CreateRenderTarget();
    return true;
}

void RendererDX11::Shutdown()
{
    CleanupD3D();
}

void RendererDX11::BeginFrame(float r, float g, float b, float a)
{
    if (!context || !rtv)
        return;

    float clearColor[4] = { r, g, b, a };
    context->OMSetRenderTargets(1, &rtv, nullptr);
    context->ClearRenderTargetView(rtv, clearColor);
}

void RendererDX11::EndFrame(bool vsync)
{
    if (!swapChain)
        return;

    swapChain->Present(vsync ? 1 : 0, 0);
}

void RendererDX11::Resize(int w, int h)
{
    if (!swapChain || !context)
        return;

    if (w <= 0 || h <= 0)
        return;

    width = w;
    height = h;

    // Release old RTV before resizing
    CleanupRenderTarget();

    HRESULT hr = swapChain->ResizeBuffers(
        0,
        (UINT)w,
        (UINT)h,
        DXGI_FORMAT_UNKNOWN,
        0
    );

    if (FAILED(hr))
        throw std::runtime_error("RendererDX11::ResizeBuffers failed.");

    CreateRenderTarget();
}

void RendererDX11::CreateRenderTarget()
{
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr) || !backBuffer)
        throw std::runtime_error("Failed to get swap chain back buffer.");

    hr = device->CreateRenderTargetView(backBuffer, nullptr, &rtv);
    backBuffer->Release();

    if (FAILED(hr))
        throw std::runtime_error("Failed to create render target view.");
}

void RendererDX11::CleanupRenderTarget()
{
    if (rtv)
    {
        rtv->Release();
        rtv = nullptr;
    }
}

void RendererDX11::CleanupD3D()
{
    if (context) context->ClearState();

    CleanupRenderTarget();

    if (swapChain) { swapChain->Release(); swapChain = nullptr; }
    if (context) { context->Release();   context = nullptr; }
    if (device) { device->Release();    device = nullptr; }
}
