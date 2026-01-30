#define NOMINMAX
#include <Windows.h>

#include "DX11App.h"
#include "IProgram.h"
#include "DX11Services.h"
#include "Engine.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <d3d11.h>
#include <dxgi.h>

#include <stdexcept>
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>

 // Link libs (you can also put these in project settings)
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

    HWND GetHWNDFromSDL(SDL_Window* window)
    {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (SDL_GetWindowWMInfo(window, &wmInfo) == SDL_FALSE)
        {
            throw std::runtime_error("SDL_GetWindowWMInfo failed (cannot get HWND).");
        }
        return wmInfo.info.win.window;
    }
}

DX11App::DX11App(const char* title, int desired_width, int desired_height)
{
    InitSDLWindow(title, desired_width, desired_height);
    InitD3D11();
    DX11Services::Init(ptr_device, ptr_context, ptr_swapchain);

    Engine::SetDX11(ptr_device, ptr_context, ptr_swapchain);
    Engine::SetViewportSize(viewport_width, viewport_height);

    ptr_program = create_program(viewport_width, viewport_height);


    ptr_program = create_program(viewport_width, viewport_height);
    if (ptr_program == nullptr)
    {
        throw std::runtime_error("create_program returned nullptr.");
    }
}

DX11App::~DX11App()
{
    // Program first (in case it references device resources)
    if (ptr_program)
    {
        delete ptr_program;
        ptr_program = nullptr;
    }

    // Release backbuffer resources before swapchain/device
    ReleaseBackBufferResources();

    SafeRelease(ptr_swapchain);
    SafeRelease(ptr_context);
    SafeRelease(ptr_device);

    if (ptr_window)
    {
        SDL_DestroyWindow(ptr_window);
        ptr_window = nullptr;
    }

    SDL_Quit();
}

bool DX11App::IsDone() const noexcept
{
    return is_done;
}

int DX11App::GetWidth() const noexcept
{
    return viewport_width;
}

int DX11App::GetHeight() const noexcept
{
    return viewport_height;
}

void DX11App::SetClearColor(float r, float g, float b, float a) noexcept
{
    clear_color[0] = r;
    clear_color[1] = g;
    clear_color[2] = b;
    clear_color[3] = a;
}

ID3D11Device& DX11App::GetDevice() const noexcept
{
    return *ptr_device;
}

ID3D11DeviceContext& DX11App::GetContext() const noexcept
{
    return *ptr_context;
}

IDXGISwapChain& DX11App::GetSwapChain() const noexcept
{
    return *ptr_swapchain;
}

ID3D11RenderTargetView& DX11App::GetRTV() const noexcept
{
    return *ptr_rtv;
}

ID3D11DepthStencilView& DX11App::GetDSV() const noexcept
{
    return *ptr_dsv;
}

void DX11App::InitSDLWindow(const char* title, int desired_width, int desired_height)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    const Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

    ptr_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        desired_width,
        desired_height,
        flags);

    if (ptr_window == nullptr)
    {
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }

    viewport_width = desired_width;
    viewport_height = desired_height;
}

void DX11App::InitD3D11()
{
    // Create device/context
    UINT createFlags = 0;
#if defined(_DEBUG)
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, // may fail on some setups but DX will fallback
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL chosenLevel = D3D_FEATURE_LEVEL_11_0;

    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // default adapter
        D3D_DRIVER_TYPE_HARDWARE,   // hardware
        nullptr,
        createFlags,
        featureLevels,
        static_cast<UINT>(std::size(featureLevels)),
        D3D11_SDK_VERSION,
        &ptr_device,
        &chosenLevel,
        &ptr_context);

    if (FAILED(hr))
    {
        throw MakeError("D3D11CreateDevice", hr);
    }

    // Build swapchain via DXGI factory from device
    IDXGIDevice* dxgiDevice = nullptr;
    hr = ptr_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr) || dxgiDevice == nullptr)
    {
        throw MakeError("QueryInterface(IDXGIDevice)", hr);
    }

    IDXGIAdapter* adapter = nullptr;
    hr = dxgiDevice->GetAdapter(&adapter);
    SafeRelease(dxgiDevice);
    if (FAILED(hr) || adapter == nullptr)
    {
        throw MakeError("IDXGIDevice::GetAdapter", hr);
    }

    IDXGIFactory* factory = nullptr;
    hr = adapter->GetParent(__uuidof(IDXGIFactory), (void**)&factory);
    SafeRelease(adapter);
    if (FAILED(hr) || factory == nullptr)
    {
        throw MakeError("IDXGIAdapter::GetParent(IDXGIFactory)", hr);
    }

    HWND hwnd = GetHWNDFromSDL(ptr_window);

    DXGI_SWAP_CHAIN_DESC scDesc = {};
    scDesc.BufferDesc.Width = static_cast<UINT>(viewport_width);
    scDesc.BufferDesc.Height = static_cast<UINT>(viewport_height);
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferDesc.RefreshRate.Numerator = 0;
    scDesc.BufferDesc.RefreshRate.Denominator = 0;
    scDesc.SampleDesc.Count = 1;
    scDesc.SampleDesc.Quality = 0;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2; // double buffering
    scDesc.OutputWindow = hwnd;
    scDesc.Windowed = TRUE;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // broadly compatible
    scDesc.Flags = 0;

    hr = factory->CreateSwapChain(ptr_device, &scDesc, &ptr_swapchain);
    SafeRelease(factory);
    if (FAILED(hr))
    {
        throw MakeError("IDXGIFactory::CreateSwapChain", hr);
    }

    // Disable alt-enter fullscreen toggling (SDL handles windowing)
    // (If this fails, it¡¯s not fatal)
    // Note: need IDXGIFactory again to call MakeWindowAssociation; skip to keep minimal.

    CreateBackBufferResources(viewport_width, viewport_height);
}

void DX11App::ReleaseBackBufferResources()
{
    SafeRelease(ptr_dsv);
    SafeRelease(ptr_rtv);
}

void DX11App::CreateBackBufferResources(int width, int height)
{
    ReleaseBackBufferResources();

    // RTV from swapchain backbuffer
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = ptr_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr) || backBuffer == nullptr)
    {
        throw MakeError("IDXGISwapChain::GetBuffer(backbuffer)", hr);
    }

    hr = ptr_device->CreateRenderTargetView(backBuffer, nullptr, &ptr_rtv);
    SafeRelease(backBuffer);
    if (FAILED(hr))
    {
        throw MakeError("ID3D11Device::CreateRenderTargetView", hr);
    }

    // Depth buffer + DSV
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
    hr = ptr_device->CreateTexture2D(&depthDesc, nullptr, &depthTex);
    if (FAILED(hr) || depthTex == nullptr)
    {
        throw MakeError("ID3D11Device::CreateTexture2D(depth)", hr);
    }

    hr = ptr_device->CreateDepthStencilView(depthTex, nullptr, &ptr_dsv);
    SafeRelease(depthTex);
    if (FAILED(hr))
    {
        throw MakeError("ID3D11Device::CreateDepthStencilView", hr);
    }

    // Viewport
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = static_cast<float>(std::max(1, width));
    vp.Height = static_cast<float>(std::max(1, height));
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    ptr_context->RSSetViewports(1, &vp);
}

void DX11App::HandleSDLEvent(const SDL_Event& e)
{
    // Forward to program first (lets it react even if we also handle)
    if (ptr_program)
    {
        ptr_program->HandleEvent(*ptr_window, e);
    }

    switch (e.type)
    {
    case SDL_QUIT:
        Engine::GetLogger().LogEvent("SDL_QUIT received");
        is_done = true;
        break;

    case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_CLOSE)
        {
            is_done = true;
        }
        else if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            const int newW = std::max(1, static_cast<int>(e.window.data1));
            const int newH = std::max(1, static_cast<int>(e.window.data2));

            viewport_width = newW;
            viewport_height = newH;

            Engine::SetViewportSize(viewport_width, viewport_height);
            // Recreate backbuffer resources
            if (ptr_swapchain)
            {
                // Unbind targets before resizing
                ID3D11RenderTargetView* nullRTV[1] = { nullptr };
                ptr_context->OMSetRenderTargets(1, nullRTV, nullptr);

                ReleaseBackBufferResources();

                HRESULT hr = ptr_swapchain->ResizeBuffers(
                    0, // keep buffer count
                    static_cast<UINT>(viewport_width),
                    static_cast<UINT>(viewport_height),
                    DXGI_FORMAT_UNKNOWN,
                    0);

                if (FAILED(hr))
                {
                    throw MakeError("IDXGISwapChain::ResizeBuffers", hr);
                }

                CreateBackBufferResources(viewport_width, viewport_height);
            }
        }
        break;

    default:
        break;
    }
}

void DX11App::Update()
{
    // Events
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        HandleSDLEvent(e);
    }

    if (is_done || ptr_program == nullptr)
    {
        return;
    }

    // Bind + clear
    ptr_context->OMSetRenderTargets(1, &ptr_rtv, ptr_dsv);

    ptr_context->ClearRenderTargetView(ptr_rtv, clear_color);
    ptr_context->ClearDepthStencilView(ptr_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // User program
    ptr_program->Update();
    ptr_program->Draw();
    //ptr_program->ImGuiDraw();

    // Present
    // vsync=1 is nicer. If you want uncapped, change first arg to 0.
    ptr_swapchain->Present(1, 0);
}
