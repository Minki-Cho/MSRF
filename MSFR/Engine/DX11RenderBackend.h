#pragma once

#include "RenderBackend.h"

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;

class DX11RenderBackend final : public IRenderBackend
{
public:
    DX11RenderBackend() = default;
    ~DX11RenderBackend() override;

    DX11RenderBackend(const DX11RenderBackend&) = delete;
    DX11RenderBackend& operator=(const DX11RenderBackend&) = delete;
    DX11RenderBackend(DX11RenderBackend&&) = delete;
    DX11RenderBackend& operator=(DX11RenderBackend&&) = delete;

    bool Initialize(void* nativeWindowHandle, int width, int height) override;
    void Shutdown() override;

    void BeginFrame(const float clearColor[4]) override;
    void EndFrame(bool vsync) override;
    void Resize(int width, int height) override;
    void BindMainRenderTarget() override;

    void* GetNativeDevice() const override { return ptrDevice; }
    void* GetNativeContext() const override { return ptrContext; }
    void* GetNativeSwapChain() const override { return ptrSwapChain; }
    void* GetNativeRTV() const override { return ptrRTV; }
    void* GetNativeDSV() const override { return ptrDSV; }

private:
    void CreateBackBufferResources(int width, int height);
    void ReleaseBackBufferResources();

private:
    ID3D11Device* ptrDevice = nullptr;
    ID3D11DeviceContext* ptrContext = nullptr;
    IDXGISwapChain* ptrSwapChain = nullptr;
    ID3D11RenderTargetView* ptrRTV = nullptr;
    ID3D11DepthStencilView* ptrDSV = nullptr;
};

