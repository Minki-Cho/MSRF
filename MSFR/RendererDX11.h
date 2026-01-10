#pragma once
#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>

class RendererDX11
{
public:
    RendererDX11() = default;
    ~RendererDX11();

    RendererDX11(const RendererDX11&) = delete;
    RendererDX11& operator=(const RendererDX11&) = delete;

    bool Init(HWND hWnd, int width, int height);
    void Shutdown();

    void BeginFrame(float r, float g, float b, float a = 1.0f);
    void EndFrame(bool vsync = true);

    void Resize(int width, int height);

    ID3D11Device* GetDevice() const { return device; }
    ID3D11DeviceContext* GetContext() const { return context; }

private:
    void CreateRenderTarget();
    void CleanupRenderTarget();
    void CleanupD3D();

private:
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* rtv = nullptr;

    int width = 0;
    int height = 0;
};
