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
    EventBus::SubscriptionId stateChangeSubscription = 0;
    EventBus::SubscriptionId menuActionSubscription = 0;
};
