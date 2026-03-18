#include "Engine.h"
#include "EventTypes.h"
#include "GameCommands.h"
#include "TextureDX11.h"
#include <SDL2/SDL.h>
#include "../Game/Splash.h"
#include "../Game/MainMenu.h"
#include "../Game/GamePlay1.h"
#include "../Game/ScreenMods.h"
#include "IProgram.h"
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
}

class Credit : public GameState
{
public:
    void Load() override
    {
        timer = 0.0;
        creditImage = TextureDX11("assets/images/Credit.png", false);
    }

    void Draw() override
    {
        creditImage.DrawFitCenter({ (float)Engine::GetViewportWidth(), (float)Engine::GetViewportHeight() });
    }

    void Update(double dt) override
    {
        timer += dt;

        const bool skip = Engine::GetActionSystem().Has(ActionId::Skip);
        const bool click = Engine::GetInput().GetMouseReleasedThisFrame();
        if (skip || click || timer > 8.0)
        {
            Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::MainMenu) });
        }
    }

    void Unload() override
    {
    }

    std::string GetName() override { return "Credit"; }

private:
    TextureDX11 creditImage;
    double timer = 0.0;
};


class HowToPlay : public GameState
{
public:
    void Load() override
    {
        inputLockTimer = 0.15;
        shownHelpDialog = false;
        backgroundImage = TextureDX11("assets/images/MainMenu.png", false);
    }

    void Draw() override
    {
        backgroundImage.DrawFitCenter({ (float)Engine::GetViewportWidth(), (float)Engine::GetViewportHeight() });
    }

    void Update(double dt) override
    {
        if (!shownHelpDialog)
        {
            shownHelpDialog = true;
            SDL_ShowSimpleMessageBox(
                SDL_MESSAGEBOX_INFORMATION,
                "How To Play",
                "Collect 3 Data Cores and survive.\n"
                "1: Machine gun\n"
                "2: Shotgun\n"
                "Space/Left Click: Fire\n"
                "Move: Arrow Keys\n"
                "\nPress Enter, Esc, or Click to return.",
                nullptr);
        }

        inputLockTimer -= dt;
        if (inputLockTimer > 0.0)
            return;

        auto& input = Engine::GetInput();
        const bool back =
            Engine::GetActionSystem().Has(ActionId::Skip) ||
            input.IsKeyPressed(InputKey::Keyboard::Escape) ||
            input.IsKeyPressed(InputKey::Keyboard::Enter) ||
            input.GetMouseReleasedThisFrame();

        if (back)
        {
            Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::MainMenu) });
        }
    }

    void Unload() override
    {
    }

    std::string GetName() override { return "HowToPlay"; }

private:
    TextureDX11 backgroundImage;
    double inputLockTimer = 0.0;
    bool shownHelpDialog = false;
};

class GameOver : public GameState
{
public:
    void Load() override
    {
        shownPrompt = false;
        backgroundImage = TextureDX11("assets/images/MainMenu.png", false);
    }

    void Draw() override
    {
        backgroundImage.DrawFitCenter({ (float)Engine::GetViewportWidth(), (float)Engine::GetViewportHeight() });
    }

    void Update(double /*dt*/) override
    {
        if (shownPrompt)
            return;

        shownPrompt = true;

        const SDL_MessageBoxButtonData buttons[] = {
            { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Restart" },
            { 0, 2, "Main Menu" },
            { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 3, "Quit" }
        };

        const SDL_MessageBoxData data = {
            SDL_MESSAGEBOX_INFORMATION,
            nullptr,
            "Game Over",
            "You were defeated.\nChoose the next action.",
            SDL_arraysize(buttons),
            buttons,
            nullptr
        };

        int selectedButton = 2;
        const int showResult = SDL_ShowMessageBox(&data, &selectedButton);
        if (showResult < 0)
            selectedButton = 2;

        if (selectedButton == 1)
        {
            Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::GamePlay1) });
        }
        else if (selectedButton == 3)
        {
            Engine::GetGameStateManager().Shutdown();
            SDL_Event quitEvent{};
            quitEvent.type = SDL_QUIT;
            SDL_PushEvent(&quitEvent);
        }
        else
        {
            Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::MainMenu) });
        }
    }

    void Unload() override
    {
    }

    std::string GetName() override { return "GameOver"; }

private:
    TextureDX11 backgroundImage;
    bool shownPrompt = false;
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

