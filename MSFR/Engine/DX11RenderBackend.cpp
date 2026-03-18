#define NOMINMAX
#include <Windows.h>

#include "DX11RenderBackend.h"

#include <array>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <sstream>

#include <d3d11.h>
#include <dxgi1_4.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace
{
    template <typename T>
    void SafeRelease(T*& p) noexcept
    {
        if (p)
        {
            p->Release();
            p = nullptr;
        }
    }

    std::runtime_error MakeError(const char* where, HRESULT hr)
    {
        std::ostringstream oss;
        oss << where << " failed. HRESULT=0x"
            << std::hex << std::uppercase << static_cast<unsigned>(hr);
        return std::runtime_error(oss.str());
    }

    void ThrowIfFailed(HRESULT hr, const char* where)
    {
        if (FAILED(hr))
            throw MakeError(where, hr);
    }
}

DX11RenderBackend::~DX11RenderBackend()
{
    Shutdown();
}

bool DX11RenderBackend::Initialize(void* nativeWindowHandle, int width, int height)
{
    HWND hwnd = static_cast<HWND>(nativeWindowHandle);
    if (!hwnd)
        return false;

    UINT createFlags = 0;
#if defined(_DEBUG)
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    std::array<D3D_FEATURE_LEVEL, 4> featureLevels{
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL chosenLevel = D3D_FEATURE_LEVEL_11_0;

    ThrowIfFailed(
        D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            createFlags,
            featureLevels.data(),
            static_cast<UINT>(featureLevels.size()),
            D3D11_SDK_VERSION,
            &ptrDevice,
            &chosenLevel,
            &ptrContext),
        "D3D11CreateDevice");

    IDXGIDevice* dxgiDevice = nullptr;
    ThrowIfFailed(ptrDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice)),
        "QueryInterface(IDXGIDevice)");

    IDXGIAdapter* adapter = nullptr;
    HRESULT hr = dxgiDevice->GetAdapter(&adapter);
    SafeRelease(dxgiDevice);
    ThrowIfFailed(hr, "IDXGIDevice::GetAdapter");

    IDXGIFactory2* factory2 = nullptr;
    hr = adapter->GetParent(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(&factory2));
    SafeRelease(adapter);
    ThrowIfFailed(hr, "IDXGIAdapter::GetParent(IDXGIFactory2)");

    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width = static_cast<UINT>(std::max(1, width));
    scDesc.Height = static_cast<UINT>(std::max(1, height));
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.Stereo = FALSE;
    scDesc.SampleDesc.Count = 1;
    scDesc.SampleDesc.Quality = 0;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2;
    scDesc.Scaling = DXGI_SCALING_STRETCH;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
    scDesc.Flags = 0;

    IDXGISwapChain1* swapChain1 = nullptr;
    hr = factory2->CreateSwapChainForHwnd(
        ptrDevice,
        hwnd,
        &scDesc,
        nullptr,
        nullptr,
        &swapChain1);
    if (FAILED(hr))
    {
        SafeRelease(factory2);
        ThrowIfFailed(hr, "IDXGIFactory2::CreateSwapChainForHwnd");
    }

    // SDL manages the window mode/state.
    factory2->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);
    SafeRelease(factory2);

    hr = swapChain1->QueryInterface(__uuidof(IDXGISwapChain), reinterpret_cast<void**>(&ptrSwapChain));
    SafeRelease(swapChain1);
    ThrowIfFailed(hr, "IDXGISwapChain1::QueryInterface(IDXGISwapChain)");

    CreateBackBufferResources(width, height);
    return true;
}

void DX11RenderBackend::Shutdown()
{
    ReleaseBackBufferResources();

    if (ptrContext)
        ptrContext->ClearState();

    SafeRelease(ptrSwapChain);
    SafeRelease(ptrContext);
    SafeRelease(ptrDevice);
}

void DX11RenderBackend::BeginFrame(const float clearColor[4])
{
    if (!ptrContext || !ptrRTV || !ptrDSV)
        return;

    ptrContext->OMSetRenderTargets(1, &ptrRTV, nullptr);
    ptrContext->ClearRenderTargetView(ptrRTV, clearColor);
    ptrContext->ClearDepthStencilView(ptrDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void DX11RenderBackend::EndFrame(bool vsync)
{
    if (!ptrSwapChain)
        return;

    ptrSwapChain->Present(vsync ? 1 : 0, 0);
}

void DX11RenderBackend::Resize(int width, int height)
{
    if (!ptrSwapChain || !ptrContext)
        return;

    ID3D11RenderTargetView* nullRTV[1] = { nullptr };
    ptrContext->OMSetRenderTargets(1, nullRTV, nullptr);

    ReleaseBackBufferResources();

    HRESULT hr = ptrSwapChain->ResizeBuffers(
        0,
        static_cast<UINT>(std::max(1, width)),
        static_cast<UINT>(std::max(1, height)),
        DXGI_FORMAT_UNKNOWN,
        0);
    ThrowIfFailed(hr, "IDXGISwapChain::ResizeBuffers");

    CreateBackBufferResources(width, height);
}

void DX11RenderBackend::BindMainRenderTarget()
{
    if (!ptrContext || !ptrRTV || !ptrDSV)
        return;

    ptrContext->OMSetRenderTargets(1, &ptrRTV, ptrDSV);
}

void DX11RenderBackend::CreateBackBufferResources(int width, int height)
{
    ReleaseBackBufferResources();

    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = ptrSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    ThrowIfFailed(hr, "IDXGISwapChain::GetBuffer(backbuffer)");

    hr = ptrDevice->CreateRenderTargetView(backBuffer, nullptr, &ptrRTV);
    SafeRelease(backBuffer);
    ThrowIfFailed(hr, "ID3D11Device::CreateRenderTargetView");

    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = static_cast<UINT>(std::max(1, width));
    depthDesc.Height = static_cast<UINT>(std::max(1, height));
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthTex = nullptr;
    hr = ptrDevice->CreateTexture2D(&depthDesc, nullptr, &depthTex);
    ThrowIfFailed(hr, "ID3D11Device::CreateTexture2D(depth)");

    hr = ptrDevice->CreateDepthStencilView(depthTex, nullptr, &ptrDSV);
    SafeRelease(depthTex);
    ThrowIfFailed(hr, "ID3D11Device::CreateDepthStencilView");

    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = static_cast<float>(std::max(1, width));
    vp.Height = static_cast<float>(std::max(1, height));
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    ptrContext->RSSetViewports(1, &vp);
}

void DX11RenderBackend::ReleaseBackBufferResources()
{
    SafeRelease(ptrDSV);
    SafeRelease(ptrRTV);
}

