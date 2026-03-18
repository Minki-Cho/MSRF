#define NOMINMAX
#include <Windows.h>

#include "DX11App.h"
#include "IProgram.h"
#include "DX11Services.h"
#include "Engine.h"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <d3d11.h>
#include <dxgi.h>

#include <stdexcept>
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>
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
    template <typename T>
    void SafeRelease(T*& p) noexcept
    {
        if (p)
        {
            p->Release();
            p = nullptr;
        }
    }

    std::runtime_error MakeError(const char* where, HRESULT hr)
    {
        std::ostringstream oss;
        oss << where << " failed. HRESULT=0x"
            << std::hex << std::uppercase << static_cast<unsigned>(hr);
        return std::runtime_error(oss.str());
    }

    [[noreturn]] void LogAndThrowHRESULT(const char* where, HRESULT hr)
    {
        ENGINE_LOG_HRESULT(Engine::GetLogger(), Logger::Severity::Fatal, "Renderer", where, hr);
        throw MakeError(where, hr);
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
    InitD3D11();
    DX11Services::Init(ptr_device, ptr_context, ptr_swapchain);

    Engine::SetDX11(ptr_device, ptr_context, ptr_swapchain);
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

    // Release backbuffer resources before swapchain/device
    ReleaseBackBufferResources();

    SafeRelease(ptr_swapchain);
    SafeRelease(ptr_context);
    SafeRelease(ptr_device);

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
    ImGui_ImplDX11_Init(ptr_device, ptr_context);

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

            const auto bulletStats = Engine::GetBulletPoolDebugStats();
            if (bulletStats.valid)
            {
                ImGui::Text("BulletPool(M): %zu / %zu active (overflow %zu)", bulletStats.machineActive, bulletStats.machineCapacity, bulletStats.machineOverflow);
                ImGui::Text("BulletPool(S): %zu / %zu active (overflow %zu)", bulletStats.shotgunActive, bulletStats.shotgunCapacity, bulletStats.shotgunOverflow);
            }

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

    const auto hud = Engine::GetGameplayHudStats();
    if (hud.valid)
    {
        ImGui::SetNextWindowBgAlpha(0.78f);
        ImGui::SetNextWindowPos(
            ImVec2(static_cast<float>(Engine::GetViewportWidth()) - 12.0f, 12.0f),
            ImGuiCond_Always,
            ImVec2(1.0f, 0.0f));

        constexpr ImGuiWindowFlags hudFlags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoInputs;

        if (ImGui::Begin("Gameplay HUD", nullptr, hudFlags))
        {
            const float hpRatio = (hud.hpMax > 0)
                ? std::clamp(static_cast<float>(hud.hp) / static_cast<float>(hud.hpMax), 0.0f, 1.0f)
                : 0.0f;

            ImVec4 hpColor = ImVec4(0.2f, 0.8f, 0.35f, 1.0f);
            if (hpRatio < 0.30f)
                hpColor = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
            else if (hpRatio < 0.60f)
                hpColor = ImVec4(0.95f, 0.75f, 0.2f, 1.0f);

            ImGui::Text("HP: %d / %d", hud.hp, hud.hpMax);
            ImGui::PushStyleColor(ImGuiCol_PlotHistogram, hpColor);
            ImGui::ProgressBar(hpRatio, ImVec2(210.0f, 0.0f));
            ImGui::PopStyleColor();

            ImGui::Separator();
            ImGui::Text("Cores: %d / %d", hud.coresCollected, hud.coresTotal);
            ImGui::Text("Weapon: %s", (hud.weaponMode == 1) ? "Shotgun" : "Machine Gun");
            ImGui::Text("Enemies Left: %d", hud.enemiesRemaining);
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

        ptr_context->OMSetRenderTargets(1, &ptr_rtv, ptr_dsv);
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
    return *ptr_device;
}

ID3D11DeviceContext& DX11App::GetContext() const noexcept
{
    return *ptr_context;
}

IDXGISwapChain& DX11App::GetSwapChain() const noexcept
{
    return *ptr_swapchain;
}

ID3D11RenderTargetView& DX11App::GetRTV() const noexcept
{
    return *ptr_rtv;
}

ID3D11DepthStencilView& DX11App::GetDSV() const noexcept
{
    return *ptr_dsv;
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

void DX11App::InitD3D11()
{
    // Create device/context
    UINT createFlags = 0;
#if defined(_DEBUG)
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1, // may fail on some setups but DX will fallback
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };

    D3D_FEATURE_LEVEL chosenLevel = D3D_FEATURE_LEVEL_11_0;

    HRESULT hr = D3D11CreateDevice(
        nullptr,                    // default adapter
        D3D_DRIVER_TYPE_HARDWARE,   // hardware
        nullptr,
        createFlags,
        featureLevels,
        static_cast<UINT>(std::size(featureLevels)),
        D3D11_SDK_VERSION,
        &ptr_device,
        &chosenLevel,
        &ptr_context);

    if (FAILED(hr))
    {
        LogAndThrowHRESULT("D3D11CreateDevice", hr);
    }

    // Build swapchain via DXGI factory from device
    IDXGIDevice* dxgiDevice = nullptr;
    hr = ptr_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr) || dxgiDevice == nullptr)
    {
        LogAndThrowHRESULT("QueryInterface(IDXGIDevice)", hr);
    }

    IDXGIAdapter* adapter = nullptr;
    hr = dxgiDevice->GetAdapter(&adapter);
    SafeRelease(dxgiDevice);
    if (FAILED(hr) || adapter == nullptr)
    {
        LogAndThrowHRESULT("IDXGIDevice::GetAdapter", hr);
    }

    IDXGIFactory* factory = nullptr;
    hr = adapter->GetParent(__uuidof(IDXGIFactory), (void**)&factory);
    SafeRelease(adapter);
    if (FAILED(hr) || factory == nullptr)
    {
        LogAndThrowHRESULT("IDXGIAdapter::GetParent(IDXGIFactory)", hr);
    }

    HWND hwnd = GetHWNDFromSDL(ptr_window);

    DXGI_SWAP_CHAIN_DESC scDesc = {};
    scDesc.BufferDesc.Width = static_cast<UINT>(viewport_width);
    scDesc.BufferDesc.Height = static_cast<UINT>(viewport_height);
    scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferDesc.RefreshRate.Numerator = 0;
    scDesc.BufferDesc.RefreshRate.Denominator = 0;
    scDesc.SampleDesc.Count = 1;
    scDesc.SampleDesc.Quality = 0;
    scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scDesc.BufferCount = 2; // double buffering
    scDesc.OutputWindow = hwnd;
    scDesc.Windowed = TRUE;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // broadly compatible
    scDesc.Flags = 0;

    hr = factory->CreateSwapChain(ptr_device, &scDesc, &ptr_swapchain);
    SafeRelease(factory);
    if (FAILED(hr))
    {
        LogAndThrowHRESULT("IDXGIFactory::CreateSwapChain", hr);
    }

    // Disable alt-enter fullscreen toggling (SDL handles windowing)
    // (If this fails, it?s not fatal)
    // Note: need IDXGIFactory again to call MakeWindowAssociation; skip to keep minimal.

    CreateBackBufferResources(viewport_width, viewport_height);
}

void DX11App::ReleaseBackBufferResources()
{
    SafeRelease(ptr_dsv);
    SafeRelease(ptr_rtv);
}

void DX11App::CreateBackBufferResources(int width, int height)
{
    ReleaseBackBufferResources();

    // RTV from swapchain backbuffer
    ID3D11Texture2D* backBuffer = nullptr;
    HRESULT hr = ptr_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
    if (FAILED(hr) || backBuffer == nullptr)
    {
        LogAndThrowHRESULT("IDXGISwapChain::GetBuffer(backbuffer)", hr);
    }

    hr = ptr_device->CreateRenderTargetView(backBuffer, nullptr, &ptr_rtv);
    SafeRelease(backBuffer);
    if (FAILED(hr))
    {
        LogAndThrowHRESULT("ID3D11Device::CreateRenderTargetView", hr);
    }

    // Depth buffer + DSV
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = static_cast<UINT>(std::max(1, width));
    depthDesc.Height = static_cast<UINT>(std::max(1, height));
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1;
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthDesc.SampleDesc.Count = 1;
    depthDesc.SampleDesc.Quality = 0;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ID3D11Texture2D* depthTex = nullptr;
    hr = ptr_device->CreateTexture2D(&depthDesc, nullptr, &depthTex);
    if (FAILED(hr) || depthTex == nullptr)
    {
        LogAndThrowHRESULT("ID3D11Device::CreateTexture2D(depth)", hr);
    }

    hr = ptr_device->CreateDepthStencilView(depthTex, nullptr, &ptr_dsv);
    SafeRelease(depthTex);
    if (FAILED(hr))
    {
        LogAndThrowHRESULT("ID3D11Device::CreateDepthStencilView", hr);
    }

    // Viewport
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width = static_cast<float>(std::max(1, width));
    vp.Height = static_cast<float>(std::max(1, height));
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    ptr_context->RSSetViewports(1, &vp);
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
            break;
        }

        if (k == SDLK_RETURN)      Engine::GetInput().OnKeyDown(InputKey::Keyboard::Enter);
        else if (k == SDLK_BACKQUOTE) Engine::GetInput().OnKeyDown(InputKey::Keyboard::Tilde);
        else if (k == SDLK_ESCAPE) Engine::GetInput().OnKeyDown(InputKey::Keyboard::Escape);
        else if (k == SDLK_SPACE)  Engine::GetInput().OnKeyDown(InputKey::Keyboard::Space);
        else if (k == SDLK_UP)     Engine::GetInput().OnKeyDown(InputKey::Keyboard::Up);
        else if (k == SDLK_DOWN)   Engine::GetInput().OnKeyDown(InputKey::Keyboard::Down);
        else if (k == SDLK_LEFT)   Engine::GetInput().OnKeyDown(InputKey::Keyboard::Left);
        else if (k == SDLK_RIGHT)  Engine::GetInput().OnKeyDown(InputKey::Keyboard::Right);
        else if (k == SDLK_1)      Engine::GetInput().OnKeyDown(InputKey::Keyboard::Num1);
        else if (k == SDLK_2)      Engine::GetInput().OnKeyDown(InputKey::Keyboard::Num2);
        else if (k == SDLK_F3)     Engine::GetInput().OnKeyDown(InputKey::Keyboard::F3);
        break;
    }

    case SDL_KEYUP:
    {
        const SDL_Keycode k = e.key.keysym.sym;

        if (k == SDLK_RETURN)      Engine::GetInput().OnKeyUp(InputKey::Keyboard::Enter);
        else if (k == SDLK_BACKQUOTE) Engine::GetInput().OnKeyUp(InputKey::Keyboard::Tilde);
        else if (k == SDLK_ESCAPE) Engine::GetInput().OnKeyUp(InputKey::Keyboard::Escape);
        else if (k == SDLK_SPACE)  Engine::GetInput().OnKeyUp(InputKey::Keyboard::Space);
        else if (k == SDLK_UP)     Engine::GetInput().OnKeyUp(InputKey::Keyboard::Up);
        else if (k == SDLK_DOWN)   Engine::GetInput().OnKeyUp(InputKey::Keyboard::Down);
        else if (k == SDLK_LEFT)   Engine::GetInput().OnKeyUp(InputKey::Keyboard::Left);
        else if (k == SDLK_RIGHT)  Engine::GetInput().OnKeyUp(InputKey::Keyboard::Right);
        else if (k == SDLK_1)      Engine::GetInput().OnKeyUp(InputKey::Keyboard::Num1);
        else if (k == SDLK_2)      Engine::GetInput().OnKeyUp(InputKey::Keyboard::Num2);
        else if (k == SDLK_F3)     Engine::GetInput().OnKeyUp(InputKey::Keyboard::F3);
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

            if (ptr_swapchain)
            {
                if (imguiInitialized)
                {
                    ImGui_ImplDX11_InvalidateDeviceObjects();
                }

                ID3D11RenderTargetView* nullRTV[1] = { nullptr };
                ptr_context->OMSetRenderTargets(1, nullRTV, nullptr);

                ReleaseBackBufferResources();

                HRESULT hr = ptr_swapchain->ResizeBuffers(
                    0,
                    (UINT)viewport_width,
                    (UINT)viewport_height,
                    DXGI_FORMAT_UNKNOWN,
                    0);

                if (FAILED(hr))
                {
                    LogAndThrowHRESULT("IDXGISwapChain::ResizeBuffers", hr);
                }

                CreateBackBufferResources(viewport_width, viewport_height);

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

    if (is_done || ptr_program == nullptr)
    {
        return;
    }

    Engine::GetActionSystem().PollFromInput(Engine::GetInput());

    // Bind + clear
    // 2D renderer: keep draw order deterministic (background -> actors -> UI)
    // by disabling depth testing for the main pass.
    ptr_context->OMSetRenderTargets(1, &ptr_rtv, nullptr);

    ptr_context->ClearRenderTargetView(ptr_rtv, clear_color);
    ptr_context->ClearDepthStencilView(ptr_dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    // User program
    ptr_program->Update();
    if (Engine::GetGameStateManager().HasGameEnded())
    {
        is_done = true;
        return;
    }
    ptr_program->Draw();

    // Schedule stress jobs right before profiler draw so the table can show Running states.
    RunThreadStressStep();

    BeginImGuiFrame();
    DrawProfilerOverlay();

    // Present
    // vsync=1 is nicer. If want uncapped, change first arg to 0.
    ptr_swapchain->Present(1, 0);
}






