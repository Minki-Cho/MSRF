#include "Engine.h"

#include <memory>
#include <string>
#include <algorithm>

#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#include "Window.h"

struct Engine::DX11State
{
    Microsoft::WRL::ComPtr<ID3D11Device> device;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> context;
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain;
};

Engine::Engine()
    : window(std::make_unique<Window>()),
    dx11(std::make_unique<DX11State>())
{
}

Engine::~Engine() = default;

ID3D11Device* Engine::GetDXDevice()
{
    return Instance().dx11 ? Instance().dx11->device.Get() : nullptr;
}

ID3D11DeviceContext* Engine::GetDXContext()
{
    return Instance().dx11 ? Instance().dx11->context.Get() : nullptr;
}

IDXGISwapChain* Engine::GetDXSwapChain()
{
    return Instance().dx11 ? Instance().dx11->swapChain.Get() : nullptr;
}

void Engine::SetDX11(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain)
{
    if (!Instance().dx11)
    {
        Instance().dx11 = std::make_unique<DX11State>();
    }

    Instance().dx11->device = device;
    Instance().dx11->context = context;
    Instance().dx11->swapChain = swapChain;
}

void Engine::InitCore()
{
    logger.LogEvent("Engine InitCore");

    lastTick = Clock::now();
    fpsCalcTime = lastTick;
    frameCount = 0;
    lastFrameDt = 1.0 / TargetFPS;
    fixedStepAccumulator = 0.0;
    renderInterpolationAlpha = 0.0;
    bulletPoolDebugStats = BulletPoolDebugStats{};
    gameplayHudStats = GameplayHudStats{};
    lastRunSummary = LastRunSummary{};

    gameFinish = false;
    initialized = true;

    jobSystem.Init();
    logger.LogEvent("Job workers (after init) = " + std::to_string(jobSystem.GetWorkerCount()));

    if (!audioSystem.Init(logger))
    {
        logger.LogWarning("Audio disabled: miniaudio init failed.");
    }
}

void Engine::InitWindow(const char* windowName, int w, int h)
{
    logger.LogEvent("Engine InitWindow");

    if (window)
        window->Init(windowName, w, h);
    usesInternalWindow = true;

    jobSystem.Init();
}

void Engine::Shutdown()
{
    if (!initialized)
        return;

    logger.LogEvent("Engine Shutdown");

    jobSystem.WaitIdle();
    jobSystem.Shutdown();

    audioSystem.Shutdown();

    eventBus.Clear();

    textureManager.Unload();
    bulletPoolDebugStats = BulletPoolDebugStats{};
    gameplayHudStats = GameplayHudStats{};
    lastRunSummary = LastRunSummary{};

    if (dx11 && dx11->context)
        dx11->context->ClearState();

    if (dx11)
    {
        dx11->swapChain.Reset();
        dx11->context.Reset();
        dx11->device.Reset();
    }

    if (window)
        window->Shutdown();

    initialized = false;
    usesInternalWindow = false;
}

void Engine::Update()
{
    if (!initialized)
        return;

    double dt = ComputeDeltaSeconds();

    frameCount++;
    const auto now = Clock::now();
    const double elapsed = std::chrono::duration<double>(now - fpsCalcTime).count();
    if (elapsed >= FPSIntervalSec)
    {
        const double avgFps = static_cast<double>(frameCount) / elapsed;
        logger.LogEvent("FPS: " + std::to_string(avgFps));
        frameCount = 0;
        fpsCalcTime = now;
    }
    lastFrameDt = dt;

    if (usesInternalWindow && window)
    {
        window->Update();
    }
    if (input.IsKeyPressed(InputKey::Keyboard::Tilde))
    {
        ToggleCollisionDebugDraw();
        logger.LogEvent(std::string("Collision debug draw: ") + (collisionDebugDrawEnabled ? "ON" : "OFF"));
    }

    fixedStepAccumulator += dt;
    const double maxAccumulator = FixedSimulationStepSec * static_cast<double>(MaxFixedStepsPerFrame);
    if (fixedStepAccumulator > maxAccumulator)
    {
        fixedStepAccumulator = maxAccumulator;
    }

    eventBus.DispatchQueued();

    int steps = 0;
    while (fixedStepAccumulator >= FixedSimulationStepSec && steps < MaxFixedStepsPerFrame)
    {
        UpdateGameObjects(FixedSimulationStepSec);
        fixedStepAccumulator -= FixedSimulationStepSec;
        ++steps;
        eventBus.DispatchQueued();
    }

    renderInterpolationAlpha = fixedStepAccumulator / FixedSimulationStepSec;
    renderInterpolationAlpha = std::clamp(renderInterpolationAlpha, 0.0, 1.0);
}

void Engine::Draw()
{
    if (!initialized)
        return;

    gameStateManager.Draw();
}

void Engine::AddSpriteFont(const std::filesystem::path& fileName)
{
    (void)fileName;
}

double Engine::ComputeDeltaSeconds()
{
    const auto now = Clock::now();
    std::chrono::duration<double> delta = now - lastTick;
    lastTick = now;

    double dt = delta.count();
    if (dt > 0.25) dt = 0.25;
    return dt;
}

void Engine::UpdateGameObjects(double dt)
{
    gameStateManager.Update(dt);
}
