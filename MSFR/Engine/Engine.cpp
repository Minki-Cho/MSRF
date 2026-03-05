#include "Engine.h"

#include <memory>
#include <string>
#include <thread>

#include "Window.h"

Engine::Engine() : window(std::make_unique<Window>()) {}
Engine::~Engine() = default;

void Engine::InitCore()
{
    logger.LogEvent("Engine InitCore");

    lastTick = Clock::now();
    fpsCalcTime = lastTick;
    frameCount = 0;
    lastFrameDt = 1.0 / TargetFPS;

    gameFinish = false;
    initialized = true;

    jobSystem.Init();
    logger.LogEvent("Job workers (after init) = " + std::to_string(jobSystem.GetWorkerCount()));
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

    eventBus.Clear();

    textureManager.Unload();

    if (dxContext)
        dxContext->ClearState();

    dxSwapChain.Reset();
    dxContext.Reset();
    dxDevice.Reset();

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

    const double targetStep = 1.0 / TargetFPS;
    if (dt < targetStep)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        dt = targetStep;
    }

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

    eventBus.DispatchQueued();
    UpdateGameObjects(dt);
    eventBus.DispatchQueued();
}

void Engine::Draw()
{
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
