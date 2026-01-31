#pragma once

#include "owner.h"

struct SDL_Window;
union SDL_Event;
class IProgram;

#include <cstdint>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

class [[nodiscard]] DX11App
{
public:
    DX11App(const char* title = "DX11 App", int desired_width = 1280, int desired_height = 720);
    ~DX11App();

    DX11App(const DX11App&) = delete;
    DX11App& operator=(const DX11App&) = delete;
    DX11App(DX11App&&) noexcept = delete;
    DX11App& operator=(DX11App&&) noexcept = delete;

    void Update();
    bool IsDone() const noexcept;

    // Accessors
    ID3D11Device& GetDevice() const noexcept;
    ID3D11DeviceContext& GetContext() const noexcept;
    IDXGISwapChain& GetSwapChain() const noexcept;

    ID3D11RenderTargetView& GetRTV() const noexcept;
    ID3D11DepthStencilView& GetDSV() const noexcept;

    int  GetWidth() const noexcept;
    int  GetHeight() const noexcept;


    void SetClearColor(float r, float g, float b, float a = 1.0f) noexcept;

private:
    void InitSDLWindow(const char* title, int desired_width, int desired_height);
    void InitD3D11();
    void CreateBackBufferResources(int width, int height);
    void ReleaseBackBufferResources();
    void HandleSDLEvent(const SDL_Event& e);

private:
    util::owner<IProgram*>   ptr_program = nullptr;
    util::owner<SDL_Window*> ptr_window = nullptr;
    bool                     is_done = false;

    int viewport_width = 0;
    int viewport_height = 0;

    float clear_color[4] = { 0.08f, 0.08f, 0.12f, 1.0f };

    // D3D11 core
    util::owner<ID3D11Device*>           ptr_device = nullptr;
    util::owner<ID3D11DeviceContext*>    ptr_context = nullptr;
    util::owner<IDXGISwapChain*>         ptr_swapchain = nullptr;

    // Backbuffer-dependent
    util::owner<ID3D11RenderTargetView*> ptr_rtv = nullptr;
    util::owner<ID3D11DepthStencilView*> ptr_dsv = nullptr;
};