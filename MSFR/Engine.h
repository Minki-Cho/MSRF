#pragma once
#include <chrono>
#include <filesystem>

#include <wrl/client.h>
#include <d3d11.h>
#include <dxgi.h>

#include "GameStateManager.h"
#include "Input.h"
#include "Window.h"
#include "Logger.h"
#include "TextureManager.h"
#include "JobSystem.h"
#include "ActionSystem.h"
#include "CommandPool.h"
#include "Command.h"

class Engine
{
public:
    enum class AnimationSpeed
    {
        Slow = 1,
        Normal = 2,
        Fast = 3,
    };
    Engine();
    ~Engine() = default;

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) = delete;
    Engine& operator=(Engine&&) = delete;

    static int GetViewportWidth() { return Instance().viewportWidth; }
    static int GetViewportHeight() { return Instance().viewportHeight; }
    static void SetViewportSize(int w, int h)
    {
        Instance().viewportWidth = (w > 0) ? w : 1;
        Instance().viewportHeight = (h > 0) ? h : 1;
    }
    static Engine& Instance() { static Engine instance; return instance; }

    static Logger& GetLogger() { return Instance().logger; }
    static Input& GetInput() { return Instance().input; }
    static Window& GetWindow() { return Instance().window; }
    static GameStateManager& GetGameStateManager() { return Instance().gameStateManager; }
    static TextureManager& GetTextureManager() { return Instance().textureManager; }
    static JobSystem& GetJobSystem() { return Instance().jobSystem; }
    static ActionSystem& GetActionSystem() { return Instance().actionSystem; }
    static auto& GetCommandPool() { return Instance().commandPool; }
    static AnimationSpeed GetAnimationSpeedLevel() { return Instance().animationSpeedLevel; }
    static bool IsCollisionDebugDrawEnabled() { return Instance().collisionDebugDrawEnabled; }
    static void ToggleCollisionDebugDraw()
    {
        Instance().collisionDebugDrawEnabled = !Instance().collisionDebugDrawEnabled;
    }
    static void SetAnimationSpeedLevel(AnimationSpeed level)
    {
        Instance().animationSpeedLevel = level;
    }
    static double GetAnimationSpeedMultiplier()
    {
        switch (Instance().animationSpeedLevel)
        {
        case AnimationSpeed::Slow:   return 0.25;
        case AnimationSpeed::Normal: return 0.5; // current default
        case AnimationSpeed::Fast:   return 1.0;
        default:                     return 0.5;
        }
    }

    template<typename T>
    static T* GetGSComponent() { return GetGameStateManager().GetGSComponent<T>(); }

    // DX11 Access
    static ID3D11Device* GetDXDevice() { return Instance().dxDevice.Get(); }
    static ID3D11DeviceContext* GetDXContext() { return Instance().dxContext.Get(); }
    static IDXGISwapChain* GetDXSwapChain() { return Instance().dxSwapChain.Get(); }

    static void SetDX11(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain)
    {
        Instance().dxDevice = device;
        Instance().dxContext = context;
        Instance().dxSwapChain = swapChain;
    }

    void InitCore();
    void InitWindow(const char* windowName, int w, int h); // lgacy

    void Shutdown();

    void Update();
    void Draw();

    bool IsGameFinished() const { return gameFinish; }
    void AddSpriteFont(const std::filesystem::path& fileName);

private:
    using Clock = std::chrono::steady_clock;

    double ComputeDeltaSeconds();
    void   UpdateGameObjects(double dt);

private:
    Clock::time_point lastTick = Clock::now();
    Clock::time_point fpsCalcTime = Clock::now();
    int frameCount = 0;

    bool gameFinish = false;
    bool initialized = false;

    bool usesInternalWindow = false;

    Logger logger;
    GameStateManager gameStateManager;
    Input input;
    Window window;
    TextureManager textureManager;
    JobSystem jobSystem;
    ActionSystem actionSystem;
    CommandPool<2048, 64> commandPool;

    // DX11 members
    Microsoft::WRL::ComPtr<ID3D11Device>        dxDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> dxContext;
    Microsoft::WRL::ComPtr<IDXGISwapChain>      dxSwapChain;

    static constexpr double TargetFPS = 60.0;
    static constexpr int FPSIntervalSec = 5;
    int viewportWidth = 1280;
    int viewportHeight = 720;
    AnimationSpeed animationSpeedLevel = AnimationSpeed::Normal;
    bool collisionDebugDrawEnabled = false;
};