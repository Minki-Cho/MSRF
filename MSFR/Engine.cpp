#include "Engine.h"

#include <random>
#include <string>
#include <thread>

Engine::Engine() = default;

void Engine::Init(const char* windowName)
{
    logger.LogEvent("Engine Init");

    // 1) Window 생성 (SDL2 + Win32 window handle 등이 여기서 준비된다고 가정)
    window.Init(windowName, 1280, 720);

    // 2) DX11 생성은 Engine에서 직접 안 함 (너는 RendererDX11 없앴으니까)
    //    -> DX11App 또는 Window 내부에서 device/context/swapchain 만든 후
    //       Engine::SetDX11(device, context, swapchain) 호출해서 주입해주면 됨.

    // Reset timing / telemetry
    lastTick = Clock::now();
    fpsCalcTime = lastTick;
    frameCount = 0;

    gameFinish = false;
    initialized = true;
}

void Engine::Shutdown()
{
    if (!initialized)
        return;

    logger.LogEvent("Engine Shutdown");

    // (선택) 텍스처 먼저 해제: GPU 리소스가 texture 안에 있으니까
    textureManager.Unload();

    // DX11 context state clear (안전)
    if (dxContext)
        dxContext->ClearState();

    // DX11 ComPtr reset
    dxSwapChain.Reset();
    dxContext.Reset();
    dxDevice.Reset();

    // Window shutdown (만약 구현되어 있으면)
    // window.Shutdown();

    initialized = false;
}

void Engine::Update()
{
    if (!initialized)
        return;

    const double dt = ComputeDeltaSeconds();

    const double targetStep = 1.0 / TargetFPS;
    if (dt < targetStep)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return;
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

    window.Update();
    input.Update();

    UpdateGameObjects(dt);
}

void Engine::Draw()
{
    // 여기서 실제 렌더링 호출.
    // DX11App가 BeginFrame/EndFrame을 하고 있다면, 그 안에서 Program::Draw를 호출하는 구조일 수도 있음.
    // 네 구조에 맞게:
    // 1) DX11App.Update() 안에서 Present까지 처리
    // 2) Engine::Draw()는 Program/GameState draw만 호출
    //
    // 예시(네가 나중에 연결할 자리):
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
