#include "Engine.h"

#include <thread>
#include <string>

Engine::Engine() = default;

void Engine::InitCore()
{
    logger.LogEvent("Engine InitCore");

    lastTick = Clock::now();
    fpsCalcTime = lastTick;
    frameCount = 0;

    gameFinish = false;
    initialized = true;
}

// for other graphic
void Engine::InitWindow(const char* windowName, int w, int h)
{
    logger.LogEvent("Engine InitWindow");

    window.Init(windowName, w, h);
    usesInternalWindow = true;
}

void Engine::Shutdown()
{
    if (!initialized)
        return;

    logger.LogEvent("Engine Shutdown");

    textureManager.Unload();

    if (dxContext)
        dxContext->ClearState();

    dxSwapChain.Reset();
    dxContext.Reset();
    dxDevice.Reset();

    window.Shutdown();

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
        //return;
    }

    // FPS telemetry
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
    if (usesInternalWindow)
    {
        window.Update();
    }

    UpdateGameObjects(dt);
}

void Engine::Draw()
{
    // gameStateManager.Draw();
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
