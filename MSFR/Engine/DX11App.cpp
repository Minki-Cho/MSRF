#define NOMINMAX
#include <Windows.h>

#include "DX11App.h"
#include "IProgram.h"
#include "DX11Services.h"
#include "RenderBackend.h"
#include "DX11RenderBackend.h"
#include "Engine.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <d3d11.h>
#include <dxgi.h>

#include <stdexcept>
#include <string>
#include <algorithm>
#include <array>
#include <cmath>

#include "../external/imgui/imgui.h"
#include "../external/imgui/backends/imgui_impl_sdl2.h"
#include "../external/imgui/backends/imgui_impl_dx11.h"
 // Link libs
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

namespace
{
    struct KeyBinding
    {
        SDL_Keycode sdlKey = SDLK_UNKNOWN;
        InputKey::Keyboard inputKey = InputKey::Keyboard::None;
    };

    constexpr std::array<KeyBinding, 15> kKeyBindings{ {
        { SDLK_RETURN, InputKey::Keyboard::Enter },
        { SDLK_BACKQUOTE, InputKey::Keyboard::Tilde },
        { SDLK_ESCAPE, InputKey::Keyboard::Escape },
        { SDLK_SPACE, InputKey::Keyboard::Space },
        { SDLK_UP, InputKey::Keyboard::Up },
        { SDLK_DOWN, InputKey::Keyboard::Down },
        { SDLK_LEFT, InputKey::Keyboard::Left },
        { SDLK_RIGHT, InputKey::Keyboard::Right },
        { SDLK_w, InputKey::Keyboard::W },
        { SDLK_a, InputKey::Keyboard::A },
        { SDLK_s, InputKey::Keyboard::S },
        { SDLK_d, InputKey::Keyboard::D },
        { SDLK_1, InputKey::Keyboard::Num1 },
        { SDLK_2, InputKey::Keyboard::Num2 },
        { SDLK_h, InputKey::Keyboard::H },
    } };

    InputKey::Keyboard ToInputKey(SDL_Keycode key)
    {
        for (const KeyBinding& binding : kKeyBindings)
        {
            if (binding.sdlKey == key)
                return binding.inputKey;
        }
        return InputKey::Keyboard::None;
    }

    HWND GetHWNDFromSDL(SDL_Window* window)
    {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        if (SDL_GetWindowWMInfo(window, &wmInfo) == SDL_FALSE)
        {
            throw std::runtime_error("SDL_GetWindowWMInfo failed (cannot get HWND).");
        }
        return wmInfo.info.win.window;
    }

    static void ForceForeground(HWND hwnd)
    {
        if (!hwnd) return;

        ShowWindow(hwnd, SW_SHOW);

        SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER);

        DWORD fgThread = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
        DWORD thisThread = GetCurrentThreadId();

        if (fgThread != thisThread)
        {
            AttachThreadInput(fgThread, thisThread, TRUE);
            SetForegroundWindow(hwnd);
            SetFocus(hwnd);
            AttachThreadInput(fgThread, thisThread, FALSE);
        }
        else
        {
            SetForegroundWindow(hwnd);
            SetFocus(hwnd);
        }
    }
}

DX11App::DX11App(const char* title, int desired_width, int desired_height)
{
    InitSDLWindow(title, desired_width, desired_height);
    HWND hwnd = GetHWNDFromSDL(ptr_window);
    Engine::GetLogger().SetFocusRestoreHwnd(hwnd);
    ForceForeground(hwnd);

    renderBackend = std::make_unique<DX11RenderBackend>();
    if (!renderBackend->Initialize(hwnd, viewport_width, viewport_height))
    {
        throw std::runtime_error("DX11RenderBackend initialization failed.");
    }

    DX11Services::Init(
        static_cast<ID3D11Device*>(renderBackend->GetNativeDevice()),
        static_cast<ID3D11DeviceContext*>(renderBackend->GetNativeContext()),
        static_cast<IDXGISwapChain*>(renderBackend->GetNativeSwapChain()));

    Engine::SetDX11(
        static_cast<ID3D11Device*>(renderBackend->GetNativeDevice()),
        static_cast<ID3D11DeviceContext*>(renderBackend->GetNativeContext()),
        static_cast<IDXGISwapChain*>(renderBackend->GetNativeSwapChain()));
    Engine::SetViewportSize(viewport_width, viewport_height);

    ptr_program = create_program(viewport_width, viewport_height);

    if (ptr_program == nullptr)
    {
        throw std::runtime_error("create_program returned nullptr.");
    }

    InitImGui();
}

DX11App::~DX11App()
{
    // Program first (in case it references device resources)
    if (ptr_program)
    {
        delete ptr_program;
        ptr_program = nullptr;
    }

    ShutdownImGui();

    if (renderBackend)
    {
        renderBackend->Shutdown();
        renderBackend.reset();
    }

    if (ptr_window)
    {
        SDL_DestroyWindow(ptr_window);
        ptr_window = nullptr;
    }

    SDL_Quit();
}

void DX11App::InitImGui()
{
    if (imguiInitialized)
        return;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
#ifdef ImGuiConfigFlags_DockingEnable
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
#ifdef ImGuiConfigFlags_ViewportsEnable
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
#endif

    ImGui::StyleColorsDark();

#ifdef ImGuiConfigFlags_ViewportsEnable
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGuiStyle& style = ImGui::GetStyle();
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
#endif

    ImGui_ImplSDL2_InitForD3D(ptr_window);
    ImGui_ImplDX11_Init(
        static_cast<ID3D11Device*>(renderBackend->GetNativeDevice()),
        static_cast<ID3D11DeviceContext*>(renderBackend->GetNativeContext()));

    frameMsHistory.fill(0.0f);
    frameHistoryOffset = 0;
    imguiInitialized = true;
}

void DX11App::ShutdownImGui()
{
    if (!imguiInitialized)
        return;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    imguiInitialized = false;
}

void DX11App::BeginImGuiFrame()
{
    if (!imguiInitialized)
        return;

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void DX11App::DrawProfilerOverlay()
{
    if (!imguiInitialized)
        return;

    const float frameMs = static_cast<float>(Engine::GetLastFrameMs());
    frameMsHistory[frameHistoryOffset] = frameMs;
    frameHistoryOffset = (frameHistoryOffset + 1) % static_cast<int>(frameMsHistory.size());

    if (showProfiler)
    {
        ImGui::SetNextWindowBgAlpha(0.88f);
        ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_Once);

        if (ImGui::Begin("MSFR Profiler", &showProfiler, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Toggle overlay: F2");
            ImGui::Text("Stress test toggle: F3");
            ImGui::Separator();

            ImGui::Text("Frame: %.2f ms (%.1f FPS)", Engine::GetLastFrameMs(), Engine::GetLastFrameFps());
            ImGui::PlotLines("Frame Time (ms)", frameMsHistory.data(), static_cast<int>(frameMsHistory.size()), frameHistoryOffset, nullptr, 0.0f, 40.0f, ImVec2(280.0f, 80.0f));

            auto& js = Engine::GetJobSystem();
            ImGui::Text("Job Workers: %u", js.GetWorkerCount());
            ImGui::Text("Pending Jobs: %u", js.GetPendingJobs());

            auto& pool = Engine::GetCommandPool();
            ImGui::Text("CommandPool: %zu / %zu in-use", pool.InUse(), pool.InUse() + pool.Available());

            ImGui::Text("Viewport: %d x %d", Engine::GetViewportWidth(), Engine::GetViewportHeight());
            ImGui::Text("Thread Stress (F3): %s", stressTestEnabled ? "ON" : "OFF");
            ImGui::Text("Stress Accumulator: %llu", static_cast<unsigned long long>(stressAccumulator.load(std::memory_order_relaxed)));
            ImGui::SliderInt("Stress Jobs/Worker", &stressJobsPerWorker, 1, 32);
            ImGui::SliderInt("Stress Iter/Job (K)", &stressIterationsK, 1, 120);
#ifdef ImGuiConfigFlags_ViewportsEnable
            ImGui::TextUnformatted("Multi-Viewport: Enabled (drag this window out)");
#else
            ImGui::TextUnformatted("Multi-Viewport: Not available in current ImGui build");
#endif

            const auto workerStats = js.GetWorkerStatsSnapshot();
            if (!workerStats.empty())
            {
                ImGui::Separator();
                ImGui::Text("CPU Thread Profiler");

                if (ImGui::BeginTable("JobWorkerStats", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit))
                {
                    ImGui::TableSetupColumn("Worker");
                    ImGui::TableSetupColumn("Jobs");
                    ImGui::TableSetupColumn("Busy Total (ms)");
                    ImGui::TableSetupColumn("Avg Job (ms)");
                    ImGui::TableSetupColumn("Last Job (ms)");
                    ImGui::TableSetupColumn("State");
                    ImGui::TableSetupColumn("Task");
                    ImGui::TableHeadersRow();

                    for (const auto& s : workerStats)
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0); ImGui::Text("Worker %u", s.workerIndex);
                        ImGui::TableSetColumnIndex(1); ImGui::Text("%llu", static_cast<unsigned long long>(s.completedJobs));
                        ImGui::TableSetColumnIndex(2); ImGui::Text("%.2f", s.totalBusyMs);
                        ImGui::TableSetColumnIndex(3); ImGui::Text("%.3f", s.avgJobMs);
                        ImGui::TableSetColumnIndex(4); ImGui::Text("%.3f", s.lastJobMs);
                        ImGui::TableSetColumnIndex(5); ImGui::TextUnformatted(s.busy ? "Running" : "Idle");
                        ImGui::TableSetColumnIndex(6);
                        const std::string taskLabel = s.busy ? s.activeTask : s.lastTask;
                        ImGui::TextUnformatted(taskLabel.empty() ? "-" : taskLabel.c_str());
                    }

                    ImGui::EndTable();
                }
            }
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

#ifdef ImGuiConfigFlags_ViewportsEnable
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        renderBackend->BindMainRenderTarget();
    }
#endif
}

bool DX11App::IsDone() const noexcept
{
    return is_done;
}

int DX11App::GetWidth() const noexcept
{
    return viewport_width;
}

int DX11App::GetHeight() const noexcept
{
    return viewport_height;
}

void DX11App::SetClearColor(float r, float g, float b, float a) noexcept
{
    clear_color[0] = r;
    clear_color[1] = g;
    clear_color[2] = b;
    clear_color[3] = a;
}

ID3D11Device& DX11App::GetDevice() const noexcept
{
    return *static_cast<ID3D11Device*>(renderBackend->GetNativeDevice());
}

ID3D11DeviceContext& DX11App::GetContext() const noexcept
{
    return *static_cast<ID3D11DeviceContext*>(renderBackend->GetNativeContext());
}

IDXGISwapChain& DX11App::GetSwapChain() const noexcept
{
    return *static_cast<IDXGISwapChain*>(renderBackend->GetNativeSwapChain());
}

ID3D11RenderTargetView& DX11App::GetRTV() const noexcept
{
    return *static_cast<ID3D11RenderTargetView*>(renderBackend->GetNativeRTV());
}

ID3D11DepthStencilView& DX11App::GetDSV() const noexcept
{
    return *static_cast<ID3D11DepthStencilView*>(renderBackend->GetNativeDSV());
}

void DX11App::InitSDLWindow(const char* title, int desired_width, int desired_height)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
    {
        ENGINE_LOG_CTX(Engine::GetLogger(), Logger::Severity::Fatal, "Platform", std::string("SDL_Init failed: ") + SDL_GetError());
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }

    const Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

    ptr_window = SDL_CreateWindow(
        title,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        desired_width,
        desired_height,
        flags);

    if (ptr_window == nullptr)
    {
        ENGINE_LOG_CTX(Engine::GetLogger(), Logger::Severity::Fatal, "Platform", std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }
    
    // make window screen shown front
    viewport_width = desired_width;
    viewport_height = desired_height;

    SDL_ShowWindow(ptr_window);
    SDL_RaiseWindow(ptr_window);
    SDL_SetWindowInputFocus(ptr_window);

    HWND hwnd = GetHWNDFromSDL(ptr_window);
    ForceForeground(hwnd);
}

void DX11App::HandleSDLEvent(const SDL_Event& e)
{
    if (imguiInitialized)
    {
        ImGui_ImplSDL2_ProcessEvent(&e);
    }

    if (ptr_program)
    {
        ptr_program->HandleEvent(*ptr_window, e);
    }

    switch (e.type)
    {
    case SDL_QUIT:
        Engine::GetLogger().LogEvent("SDL_QUIT received");
        is_done = true;
        break;

    case SDL_MOUSEMOTION:
    {
        Engine::GetInput().OnMouseMove((float)e.motion.x, (float)e.motion.y);
        break;
    }

    case SDL_MOUSEBUTTONDOWN:
    {
        if (e.button.button == SDL_BUTTON_LEFT)
            Engine::GetInput().OnMouseDown(1);
        break;
    }

    case SDL_MOUSEBUTTONUP:
    {
        if (e.button.button == SDL_BUTTON_LEFT)
            Engine::GetInput().OnMouseUp(1);
        break;
    }

    case SDL_KEYDOWN:
    {
        if (e.key.repeat) break;

        const SDL_Keycode k = e.key.keysym.sym;

        if (k == SDLK_F1)
        {
            auto& lg = Engine::GetLogger();
            lg.SetUseConsole(!lg.IsUsingConsole());
            break;
        }

        if (k == SDLK_F2)
        {
            showProfiler = !showProfiler;
            break;
        }

        if (k == SDLK_F3)
        {
            stressTestEnabled = !stressTestEnabled;
            Engine::GetLogger().LogEvent(std::string("[Profiler] Thread stress: ") + (stressTestEnabled ? "ON" : "OFF"));
            Engine::GetInput().OnKeyDown(InputKey::Keyboard::F3);
            break;
        }

        const InputKey::Keyboard mapped = ToInputKey(k);
        if (mapped != InputKey::Keyboard::None)
            Engine::GetInput().OnKeyDown(mapped);
        break;
    }

    case SDL_KEYUP:
    {
        const SDL_Keycode k = e.key.keysym.sym;

        if (k == SDLK_F3)
        {
            Engine::GetInput().OnKeyUp(InputKey::Keyboard::F3);
            break;
        }

        const InputKey::Keyboard mapped = ToInputKey(k);
        if (mapped != InputKey::Keyboard::None)
            Engine::GetInput().OnKeyUp(mapped);
        break;
    }

    case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_CLOSE)
        {
            is_done = true;
        }
        else if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
        {
            const int newW = std::max(1, (int)e.window.data1);
            const int newH = std::max(1, (int)e.window.data2);

            viewport_width = newW;
            viewport_height = newH;

            Engine::SetViewportSize(viewport_width, viewport_height);

            if (renderBackend)
            {
                if (imguiInitialized)
                {
                    ImGui_ImplDX11_InvalidateDeviceObjects();
                }

                renderBackend->Resize(viewport_width, viewport_height);

                if (imguiInitialized)
                {
                    ImGui_ImplDX11_CreateDeviceObjects();
                }
            }
        }
        break;

    default:
        break;
    }
}

void DX11App::RunThreadStressStep()
{
    if (!stressTestEnabled)
        return;

    auto& js = Engine::GetJobSystem();
    const uint32_t workers = (std::max)(1u, js.GetWorkerCount());
    const uint32_t jobsPerWorker = static_cast<uint32_t>((std::max)(1, stressJobsPerWorker));
    const uint32_t jobCount = workers * jobsPerWorker;
    const uint32_t kIterationsPerJob = static_cast<uint32_t>((std::max)(1, stressIterationsK)) * 1000u;

    // Prevent unbounded queue growth while stress mode is on.
    const uint32_t pending = js.GetPendingJobs();
    if (pending > jobCount * 2u)
        return;

    js.Dispatch(jobCount, 1, [this, kIterationsPerJob](uint32_t index)
    {
        double x = static_cast<double>(index + 1u) * 0.123456789;
        double acc = 0.0;

        for (uint32_t i = 0; i < kIterationsPerJob; ++i)
        {
            x = std::sin(x * 1.000001 + static_cast<double>(i) * 0.00000031);
            acc += x * x;
        }

        const uint64_t packed = static_cast<uint64_t>(acc * 100000.0);
        stressAccumulator.fetch_add(packed, std::memory_order_relaxed);
    }, "Stress.Burn");
}
void DX11App::Update()
{
    Engine::GetInput().Update();
    // Events
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        HandleSDLEvent(e);
    }

    if (is_done || ptr_program == nullptr || !renderBackend)
    {
        return;
    }

    Engine::GetActionSystem().PollFromInput(Engine::GetInput());

    // Bind + clear
    renderBackend->BeginFrame(clear_color);

    BeginImGuiFrame();

    // User program
    ptr_program->Update();
    if (Engine::GetGameStateManager().HasGameEnded())
    {
        is_done = true;
    }
    else
    {
        ptr_program->Draw();

        // Schedule stress jobs right before profiler draw so the table can show Running states.
        RunThreadStressStep();
    }

    DrawProfilerOverlay();

    // Present
    renderBackend->EndFrame(true);
}





