#pragma once
#include <d3d11.h>
#include <dxgi.h>

class DX11Services
{
public:
    static void Init(ID3D11Device* dev, ID3D11DeviceContext* ctx, IDXGISwapChain* sc)
    {
        s_device = dev;
        s_context = ctx;
        s_swapChain = sc;
    }

    static ID3D11Device* Device() { return s_device; }
    static ID3D11DeviceContext* Context() { return s_context; }
    static IDXGISwapChain* SwapChain() { return s_swapChain; }

private:
    inline static ID3D11Device* s_device = nullptr;
    inline static ID3D11DeviceContext* s_context = nullptr;
    inline static IDXGISwapChain* s_swapChain = nullptr;
};
