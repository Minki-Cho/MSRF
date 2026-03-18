#include "Engine.h"
#include "EventTypes.h"
#include "GameCommands.h"
#include "TextureDX11.h"
#include "../external/imgui/imgui.h"
#include <SDL2/SDL.h>
#include "../Game/Splash.h"
#include "../Game/MainMenu.h"
#include "../Game/GamePlay1.h"
#include "../Game/ScreenMods.h"
#include "IProgram.h"
#include <cstdio>
#include <string>

namespace
{
    template<typename Cmd, typename... Args>
    void ExecutePooledCommand(Args&&... args)
    {
        auto& pool = Engine::GetCommandPool();
        Cmd* cmd = pool.Create<Cmd>(std::forward<Args>(args)...);
        if (!cmd)
        {
            Engine::GetLogger().LogError("[CommandPool] Allocation failed");
            return;
        }

        cmd->Execute();
        pool.Destroy(cmd);
    }

    std::string FormatRunClock(double survivalSec)
    {
        if (survivalSec < 0.0)
            survivalSec = 0.0;

        const int total = static_cast<int>(survivalSec + 0.5);
        const int minutes = total / 60;
        const int seconds = total % 60;

        char buffer[32] = {};
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d", minutes, seconds);
        return std::string(buffer);
    }
}

class Credit : public GameState
{
public:
    void Load() override
    {
        timer = 0.0;
        pendingAction = 0;
        creditImage = TextureDX11("assets/images/Credit.png", false);
    }

    void Draw() override
    {
        creditImage.DrawFitCenter({ (float)Engine::GetViewportWidth(), (float)Engine::GetViewportHeight() });

        ImGui::SetNextWindowPos(
            ImVec2(Engine::GetViewportWidth() * 0.5f, Engine::GetViewportHeight() * 0.5f),
            ImGuiCond_Always,
            ImVec2(0.5f, 0.5f));

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin("Mission Clear", nullptr, flags))
        {
            ImGui::TextUnformatted("Data Cores secured.");
            ImGui::Separator();

            const auto summary = Engine::GetLastRunSummary();
            if (summary.valid)
            {
                const float corePercent = (summary.coresTotal > 0)
                    ? (100.0f * static_cast<float>(summary.coresCollected) / static_cast<float>(summary.coresTotal))
                    : 0.0f;

                const std::string clock = FormatRunClock(summary.survivalSec);
                ImGui::Text("Survival Time: %s", clock.c_str());
                ImGui::Text("Enemies Defeated: %d", summary.killCount);
                ImGui::Text("Core Collection: %d/%d (%.0f%%)", summary.coresCollected, summary.coresTotal, corePercent);
            }
            else
            {
                ImGui::TextUnformatted("Run summary unavailable.");
            }

            ImGui::Separator();
            if (ImGui::Button("Main Menu", ImVec2(220.0f, 0.0f)))
                pendingAction = 1;
        }
        ImGui::End();
    }

    void Update(double dt) override
    {
        if (pendingAction == 1)
        {
            pendingAction = 0;
            Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::MainMenu) });
            return;
        }

        timer += dt;

        const bool skip = Engine::GetActionSystem().Has(ActionId::Skip);
        const bool click = Engine::GetInput().GetMouseReleasedThisFrame();
        const bool esc = Engine::GetInput().IsKeyPressed(InputKey::Keyboard::Escape);
        if (skip || click || esc || timer > 8.0)
        {
            pendingAction = 1;
        }
    }

    void Unload() override
    {
        creditImage.Reset();
        pendingAction = 0;
        timer = 0.0;
    }

    std::string GetName() override { return "Credit"; }

private:
    TextureDX11 creditImage;
    double timer = 0.0;
    int pendingAction = 0;
};


class HowToPlay : public GameState
{
public:
    void Load() override
    {
        pendingAction = 0;
        backgroundImage = TextureDX11("assets/images/MainMenu.png", false);
    }

    void Draw() override
    {
        backgroundImage.DrawFitCenter({ (float)Engine::GetViewportWidth(), (float)Engine::GetViewportHeight() });

        ImGui::SetNextWindowPos(
            ImVec2(Engine::GetViewportWidth() * 0.5f, Engine::GetViewportHeight() * 0.5f),
            ImGuiCond_Always,
            ImVec2(0.5f, 0.5f));

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin("How To Play", nullptr, flags))
        {
            ImGui::TextUnformatted("Collect all Data Cores and survive.");
            ImGui::Separator();
            ImGui::TextUnformatted("Move: Arrow Keys");
            ImGui::TextUnformatted("Fire: Space or Left Click");
            ImGui::TextUnformatted("Weapon Swap: 1 (Machine), 2 (Shotgun)");
            ImGui::TextUnformatted("Pause: Esc");
            ImGui::Separator();
            if (ImGui::Button("Start Game", ImVec2(180.0f, 0.0f)))
                pendingAction = 1;
            ImGui::SameLine();
            if (ImGui::Button("Back", ImVec2(180.0f, 0.0f)))
                pendingAction = 2;
        }
        ImGui::End();
    }

    void Update(double dt) override
    {
        (void)dt;

        if (pendingAction == 1)
        {
            pendingAction = 0;
            Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::GamePlay1) });
            return;
        }

        if (pendingAction == 2)
        {
            pendingAction = 0;
            Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::MainMenu) });
            return;
        }

        auto& input = Engine::GetInput();
        if (input.IsKeyPressed(InputKey::Keyboard::Enter) || input.IsKeyPressed(InputKey::Keyboard::Space))
        {
            pendingAction = 1;
        }
        else if (input.IsKeyPressed(InputKey::Keyboard::Escape))
        {
            pendingAction = 2;
        }
    }

    void Unload() override
    {
        backgroundImage.Reset();
        pendingAction = 0;
    }

    std::string GetName() override { return "HowToPlay"; }

private:
    TextureDX11 backgroundImage;
    int pendingAction = 0;
};

class GameOver : public GameState
{
public:
    void Load() override
    {
        pendingAction = 0;
        backgroundImage = TextureDX11("assets/images/MainMenu.png", false);
    }

    void Draw() override
    {
        backgroundImage.DrawFitCenter({ (float)Engine::GetViewportWidth(), (float)Engine::GetViewportHeight() });

        ImGui::SetNextWindowPos(
            ImVec2(Engine::GetViewportWidth() * 0.5f, Engine::GetViewportHeight() * 0.5f),
            ImGuiCond_Always,
            ImVec2(0.5f, 0.5f));

        constexpr ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin("Game Over", nullptr, flags))
        {
            ImGui::TextUnformatted("You were defeated.");
            ImGui::Separator();

            const auto summary = Engine::GetLastRunSummary();
            if (summary.valid)
            {
                const float corePercent = (summary.coresTotal > 0)
                    ? (100.0f * static_cast<float>(summary.coresCollected) / static_cast<float>(summary.coresTotal))
                    : 0.0f;

                const std::string clock = FormatRunClock(summary.survivalSec);
                ImGui::Text("Survival Time: %s", clock.c_str());
                ImGui::Text("Enemies Defeated: %d", summary.killCount);
                ImGui::Text("Core Collection: %d/%d (%.0f%%)", summary.coresCollected, summary.coresTotal, corePercent);
                ImGui::Separator();
            }

            if (ImGui::Button("Restart", ImVec2(140.0f, 0.0f)))
                pendingAction = 1;
            ImGui::SameLine();
            if (ImGui::Button("Main Menu", ImVec2(140.0f, 0.0f)))
                pendingAction = 2;
            ImGui::SameLine();
            if (ImGui::Button("Quit", ImVec2(120.0f, 0.0f)))
                pendingAction = 3;
        }
        ImGui::End();
    }

    void Update(double /*dt*/) override
    {
        auto& input = Engine::GetInput();
        if (pendingAction == 0)
        {
            if (input.IsKeyPressed(InputKey::Keyboard::Enter) || input.IsKeyPressed(InputKey::Keyboard::Space))
                pendingAction = 1;
            else if (input.IsKeyPressed(InputKey::Keyboard::Escape))
                pendingAction = 2;
        }

        if (pendingAction == 1)
        {
            pendingAction = 0;
            Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::GamePlay1) });
        }
        else if (pendingAction == 3)
        {
            pendingAction = 0;
            Engine::GetGameStateManager().Shutdown();
            SDL_Event quitEvent{};
            quitEvent.type = SDL_QUIT;
            SDL_PushEvent(&quitEvent);
        }
        else if (pendingAction == 2)
        {
            pendingAction = 0;
            Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::MainMenu) });
        }
    }

    void Unload() override
    {
        backgroundImage.Reset();
        pendingAction = 0;
    }

    std::string GetName() override { return "GameOver"; }

private:
    TextureDX11 backgroundImage;
    int pendingAction = 0;
};
class GameProgram final : public IProgram
{
public:
    GameProgram(int, int)
    {
        Engine& engine = Engine::Instance();
        engine.InitCore();
        engine.GetGameStateManager().AddGameState(splash);
        engine.GetGameStateManager().AddGameState(mainmenu);
        engine.GetGameStateManager().AddGameState(play);
        engine.GetGameStateManager().AddGameState(credit);
        engine.GetGameStateManager().AddGameState(howToPlay);
        engine.GetGameStateManager().AddGameState(gameOver);

        auto& bus = Engine::GetEventBus();
        stateChangeSubscription = bus.Subscribe<RequestStateChangeEvent>([](const RequestStateChangeEvent& e) {
            ExecutePooledCommand<RequestStateChangeCommand>(e.nextStateIndex);
        });

        menuActionSubscription = bus.Subscribe<MenuActionEvent>([](const MenuActionEvent& e) {
            ExecutePooledCommand<LogMenuActionCommand>(e.action);
        });
    }

    ~GameProgram() override
    {
        auto& bus = Engine::GetEventBus();
        bus.Unsubscribe<RequestStateChangeEvent>(stateChangeSubscription);
        bus.Unsubscribe<MenuActionEvent>(menuActionSubscription);
        Engine::Instance().Shutdown();
    }

    void Update() override { Engine::Instance().Update(); }
    void Draw() override { Engine::Instance().Draw(); }
    void HandleEvent(SDL_Window&, const SDL_Event&) override {}

private:
    Splash splash;
    MainMenu mainmenu;
    GamePlay1 play;
    Credit credit;
    HowToPlay howToPlay;
    GameOver gameOver;
    EventBus::SubscriptionId stateChangeSubscription = 0;
    EventBus::SubscriptionId menuActionSubscription = 0;
};

