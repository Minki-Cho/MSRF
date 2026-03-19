#pragma once
#include <cstddef>
#include <chrono>
#include <filesystem>
#include <memory>

#include "GameStateManager.h"
#include "Input.h"
#include "Logger.h"
#include "TextureManager.h"
#include "JobSystem.h"
#include "ActionSystem.h"
#include "AudioSystem.h"
#include "CommandPool.h"
#include "Command.h"
#include "EventBus.h"

class Window;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;

class Engine
{
public:
    struct LastRunSummary
    {
        double survivalSec = 0.0;
        int killCount = 0;
        int coresCollected = 0;
        int coresTotal = 0;
        bool cleared = false;
        bool valid = false;
    };

    enum class AnimationSpeed
    {
        Slow = 1,
        Normal = 2,
        Fast = 3,
    };

    Engine();
    ~Engine();

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
    static Window& GetWindow() { return *Instance().window; }
    static GameStateManager& GetGameStateManager() { return Instance().gameStateManager; }
    static TextureManager& GetTextureManager() { return Instance().textureManager; }
    static JobSystem& GetJobSystem() { return Instance().jobSystem; }
    static ActionSystem& GetActionSystem() { return Instance().actionSystem; }
    static auto& GetCommandPool() { return Instance().commandPool; }
    static EventBus& GetEventBus() { return Instance().eventBus; }
    static AudioSystem& GetAudioSystem() { return Instance().audioSystem; }
    static bool PlaySound(const std::filesystem::path& soundPath) { return Instance().audioSystem.PlayOneShot(soundPath); }
    static bool IsAutoPlayEnabled() { return Instance().autoPlayEnabled; }
    static void SetAutoPlayEnabled(bool enabled) { Instance().autoPlayEnabled = enabled; }
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
        case AnimationSpeed::Normal: return 0.5;
        case AnimationSpeed::Fast:   return 1.0;
        default:                     return 0.5;
        }
    }
    static double GetLastFrameDt() { return Instance().lastFrameDt; }
    static double GetLastFrameMs() { return Instance().lastFrameDt * 1000.0; }
    static double GetLastFrameFps() { return (Instance().lastFrameDt > 0.0) ? (1.0 / Instance().lastFrameDt) : 0.0; }
    static double GetRenderInterpolationAlpha() { return Instance().renderInterpolationAlpha; }
    static LastRunSummary GetLastRunSummary() { return Instance().lastRunSummary; }
    static void SetLastRunSummary(const LastRunSummary& summary) { Instance().lastRunSummary = summary; }

    template<typename T>
    static T* GetGSComponent() { return GetGameStateManager().GetGSComponent<T>(); }

    static ID3D11Device* GetDXDevice();
    static ID3D11DeviceContext* GetDXContext();
    static IDXGISwapChain* GetDXSwapChain();

    static void SetDX11(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain);

    void InitCore();
    void InitWindow(const char* windowName, int w, int h);

    void Shutdown();

    void Update();
    void Draw();

    bool IsGameFinished() const { return gameFinish; }
    void AddSpriteFont(const std::filesystem::path& fileName);

private:
    struct DX11State;

    using Clock = std::chrono::steady_clock;

    double ComputeDeltaSeconds();
    void UpdateGameObjects(double dt);

private:
    Clock::time_point lastTick = Clock::now();
    Clock::time_point fpsCalcTime = Clock::now();
    int frameCount = 0;
    double lastFrameDt = 1.0 / 60.0;
    double fixedStepAccumulator = 0.0;
    double renderInterpolationAlpha = 0.0;

    bool gameFinish = false;
    bool initialized = false;

    bool usesInternalWindow = false;
    bool autoPlayEnabled = false;

    Logger logger;
    GameStateManager gameStateManager;
    Input input;
    std::unique_ptr<Window> window;
    TextureManager textureManager;
    JobSystem jobSystem;
    ActionSystem actionSystem;
    AudioSystem audioSystem;
    CommandPool<2048, 64> commandPool;
    EventBus eventBus;

    std::unique_ptr<DX11State> dx11;

    static constexpr double TargetFPS = 60.0;
    static constexpr double FixedSimulationStepSec = 1.0 / 60.0;
    static constexpr int MaxFixedStepsPerFrame = 8;
    static constexpr int FPSIntervalSec = 5;
    int viewportWidth = 1280;
    int viewportHeight = 720;
    AnimationSpeed animationSpeedLevel = AnimationSpeed::Normal;
    bool collisionDebugDrawEnabled = false;
    LastRunSummary lastRunSummary{};
};
