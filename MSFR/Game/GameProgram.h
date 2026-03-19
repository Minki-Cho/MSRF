#include "../Engine/Engine.h"
#include "../Engine/EventTypes.h"
#include "../Engine/GameCommands.h"
#include "../Engine/TextureDX11.h"
#include "../Engine/UIFramework.h"
#include <SDL2/SDL.h>
#include "Splash.h"
#include "MainMenu.h"
#include "GamePlay1.h"
#include "ScreenMods.h"
#include "../Engine/IProgram.h"
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
        auto& ui = UI::Get();
        ui.BeginFrame();

        const auto& theme = ui.GetTheme();
        const float vw = static_cast<float>(Engine::GetViewportWidth());
        const float vh = static_cast<float>(Engine::GetViewportHeight());
        const UI::Rect panel{ vw * 0.5f - 250.0f, vh * 0.5f - 170.0f, 500.0f, 340.0f };

        ui.Panel(panel, theme.panelBg, theme.panelBorder, 3.0f);
        ui.LabelCentered(UI::Rect{ panel.x, panel.y + 16.0f, panel.w, 26.0f }, "MISSION CLEAR", 2.5f, theme.text);
        ui.Label(panel.x + 30.0f, panel.y + 54.0f, "DATA CORES SECURED", 1.8f, theme.textMuted);

        const auto summary = Engine::GetLastRunSummary();
        if (summary.valid)
        {
            const float corePercent = (summary.coresTotal > 0)
                ? (100.0f * static_cast<float>(summary.coresCollected) / static_cast<float>(summary.coresTotal))
                : 0.0f;

            const std::string clock = FormatRunClock(summary.survivalSec);
            char line1[80] = {};
            char line2[80] = {};
            char line3[96] = {};
            std::snprintf(line1, sizeof(line1), "SURVIVAL %s", clock.c_str());
            std::snprintf(line2, sizeof(line2), "ENEMIES %d", summary.killCount);
            std::snprintf(line3, sizeof(line3), "CORES %d/%d %.0f%%", summary.coresCollected, summary.coresTotal, corePercent);

            ui.Label(panel.x + 30.0f, panel.y + 94.0f, line1, 1.9f, theme.text);
            ui.Label(panel.x + 30.0f, panel.y + 120.0f, line2, 1.9f, theme.text);
            ui.Label(panel.x + 30.0f, panel.y + 146.0f, line3, 1.9f, theme.text);
        }
        else
        {
            ui.Label(panel.x + 30.0f, panel.y + 110.0f, "RUN SUMMARY UNAVAILABLE", 1.8f, theme.textMuted);
        }

        if (ui.Button(UI::Rect{ panel.x + 120.0f, panel.y + panel.h - 68.0f, panel.w - 240.0f, 44.0f }, "MAIN MENU"))
            pendingAction = 1;

        ui.EndFrame();
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

        auto& ui = UI::Get();
        ui.BeginFrame();

        const auto& theme = ui.GetTheme();
        const float vw = static_cast<float>(Engine::GetViewportWidth());
        const float vh = static_cast<float>(Engine::GetViewportHeight());
        const UI::Rect panel{ vw * 0.5f - 280.0f, vh * 0.5f - 190.0f, 560.0f, 380.0f };

        ui.Panel(panel, theme.panelBg, theme.panelBorder, 3.0f);
        ui.LabelCentered(UI::Rect{ panel.x, panel.y + 18.0f, panel.w, 26.0f }, "HOW TO PLAY", 2.5f, theme.text);

        ui.Label(panel.x + 34.0f, panel.y + 66.0f, "COLLECT ALL DATA CORES AND SURVIVE", 1.8f, theme.textMuted);
        ui.Label(panel.x + 34.0f, panel.y + 102.0f, "MOVE  ARROW KEYS", 1.8f, theme.text);
        ui.Label(panel.x + 34.0f, panel.y + 128.0f, "FIRE  SPACE OR LEFT CLICK", 1.8f, theme.text);
        ui.Label(panel.x + 34.0f, panel.y + 154.0f, "WEAPON SWAP  1 MACHINE  2 SHOTGUN", 1.8f, theme.text);
        ui.Label(panel.x + 34.0f, panel.y + 180.0f, "PAUSE  ESC", 1.8f, theme.text);

        if (ui.Button(UI::Rect{ panel.x + 52.0f, panel.y + panel.h - 68.0f, 210.0f, 44.0f }, "START GAME"))
            pendingAction = 1;
        if (ui.Button(UI::Rect{ panel.x + panel.w - 262.0f, panel.y + panel.h - 68.0f, 210.0f, 44.0f }, "BACK"))
            pendingAction = 2;

        ui.EndFrame();
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
        auto& ui = UI::Get();
        ui.BeginFrame();

        const auto& theme = ui.GetTheme();
        const float vw = static_cast<float>(Engine::GetViewportWidth());
        const float vh = static_cast<float>(Engine::GetViewportHeight());
        const UI::Rect panel{ vw * 0.5f - 280.0f, vh * 0.5f - 190.0f, 560.0f, 380.0f };

        ui.Panel(panel, theme.panelBg, theme.panelBorder, 3.0f);
        ui.LabelCentered(UI::Rect{ panel.x, panel.y + 16.0f, panel.w, 26.0f }, "GAME OVER", 2.6f, theme.text);
        ui.Label(panel.x + 34.0f, panel.y + 54.0f, "YOU WERE DEFEATED", 1.8f, theme.textMuted);

        const auto summary = Engine::GetLastRunSummary();
        if (summary.valid)
        {
            const float corePercent = (summary.coresTotal > 0)
                ? (100.0f * static_cast<float>(summary.coresCollected) / static_cast<float>(summary.coresTotal))
                : 0.0f;

            const std::string clock = FormatRunClock(summary.survivalSec);
            char line1[80] = {};
            char line2[80] = {};
            char line3[96] = {};
            std::snprintf(line1, sizeof(line1), "SURVIVAL %s", clock.c_str());
            std::snprintf(line2, sizeof(line2), "ENEMIES %d", summary.killCount);
            std::snprintf(line3, sizeof(line3), "CORES %d/%d %.0f%%", summary.coresCollected, summary.coresTotal, corePercent);

            ui.Label(panel.x + 34.0f, panel.y + 94.0f, line1, 1.9f, theme.text);
            ui.Label(panel.x + 34.0f, panel.y + 120.0f, line2, 1.9f, theme.text);
            ui.Label(panel.x + 34.0f, panel.y + 146.0f, line3, 1.9f, theme.text);
        }

        if (ui.Button(UI::Rect{ panel.x + 34.0f, panel.y + panel.h - 66.0f, 160.0f, 44.0f }, "RESTART"))
            pendingAction = 1;
        if (ui.Button(UI::Rect{ panel.x + panel.w * 0.5f - 80.0f, panel.y + panel.h - 66.0f, 160.0f, 44.0f }, "MAIN MENU"))
            pendingAction = 2;
        if (ui.Button(UI::Rect{ panel.x + panel.w - 194.0f, panel.y + panel.h - 66.0f, 160.0f, 44.0f }, "QUIT", true))
            pendingAction = 3;

        ui.EndFrame();
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
