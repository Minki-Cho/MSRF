// Engine.cpp
#include "Engine.h"

#include <random>
#include <string>
#include <thread> // for std::this_thread::sleep_for (optional)

//Engine::Engine()
//{
//    // Member default initializers in Engine.h handle most initialization.
//}

void Engine::Init(const char* windowName)
{
    logger.LogEvent("Engine Init");

    // Init window (and inside it, you can init DX11/renderer later)
    window.Init(windowName, 1280, 720);

    // Reset timing / telemetry
    lastTick = Clock::now();
    fpsCalcTime = lastTick;
    frameCount = 0;

    gameFinish = false;
    initialized = true;

    // If you need a persistent RNG, store it as a member.
    // Creating it here as a local does nothing after Init returns.
    // std::mt19937 rng(std::random_device{}());
}

void Engine::Shutdown()
{
    if (!initialized)
        return;

    logger.LogEvent("Engine Shutdown");

    // Shutdown subsystems in a safe order (examples)
    // textureManager.Shutdown();
    // gameStateManager.Shutdown();
    // window.Shutdown();

    initialized = false;
}

void Engine::Update()
{
    if (!initialized)
        return;

    const double dt = ComputeDeltaSeconds();

    // Throttle to TargetFPS (simple approach).
    // NOTE: In a more advanced loop, you'd use a fixed timestep and sleep_until.
    const double targetStep = 1.0 / TargetFPS;

    if (dt < targetStep)
    {
        // Optional: reduce CPU usage. This is a very rough throttle.
        // A more accurate approach would be sleep_until(nextFrameTime).
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return;
    }

    logger.LogVerbose("Engine Update");

    // FPS telemetry (prints about every FPSIntervalSec seconds)
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

    // Update order (typical):
    // 1) Window messages
    // 2) Input
    // 3) Game/States
    //
    // If your Window::Update() pumps OS messages, call it first.
    window.Update();
    input.Update();

    // If your game state manager uses dt, pass it through.
    UpdateGameObjects(dt);

    // You can also set gameFinish based on states
    // gameFinish = gameStateManager.HasGameEnded();
}

void Engine::Draw()
{
}

//bool Engine::HasGameEnded()
//{
//    return gameStateManager.HasGameEnded();
//}

void Engine::AddSpriteFont(const std::filesystem::path& fileName)
{
    // Hook this up to your font system later.
    // Example:
    // spriteFontManager.Load(fileName);
    (void)fileName;
}

double Engine::ComputeDeltaSeconds()
{
    const auto now = Clock::now();
    std::chrono::duration<double> delta = now - lastTick;
    lastTick = now;

    double dt = delta.count();

    // Clamp to avoid massive dt spikes (debugger breaks, window drag, etc.)
    if (dt > 0.25) dt = 0.25;

    return dt;
}

void Engine::UpdateGameObjects(double dt)
{
    // Keep this function as the ¡°simulation¡± update path.
    // Later (Week 9) this is what you move to a simulation thread.
    gameStateManager.Update(dt);
}
