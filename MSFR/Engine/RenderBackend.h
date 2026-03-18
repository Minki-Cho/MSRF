#pragma once

class IRenderBackend
{
public:
    virtual ~IRenderBackend() = default;

    virtual bool Initialize(void* nativeWindowHandle, int width, int height) = 0;
    virtual void Shutdown() = 0;

    virtual void BeginFrame(const float clearColor[4]) = 0;
    virtual void EndFrame(bool vsync) = 0;
    virtual void Resize(int width, int height) = 0;
    virtual void BindMainRenderTarget() = 0;

    virtual void* GetNativeDevice() const = 0;
    virtual void* GetNativeContext() const = 0;
    virtual void* GetNativeSwapChain() const = 0;
    virtual void* GetNativeRTV() const = 0;
    virtual void* GetNativeDSV() const = 0;
};

