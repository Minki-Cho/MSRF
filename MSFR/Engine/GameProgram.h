#include "Engine.h"
#include "EventTypes.h"
#include <SDL2/SDL.h>
#include "../Game/Splash.h"
#include "../Game/MainMenu.h"
#include "../Game/GamePlay1.h"
#include "IProgram.h"
#include <string>
// ...

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

        auto& bus = Engine::GetEventBus();
        stateChangeSubscription = bus.Subscribe<RequestStateChangeEvent>([](const RequestStateChangeEvent& e) {
            Engine::GetLogger().LogEvent("[EventBus] RequestStateChange -> " + std::to_string(e.nextStateIndex));
            Engine::GetGameStateManager().SetNextState(e.nextStateIndex);
        });

        menuActionSubscription = bus.Subscribe<MenuActionEvent>([](const MenuActionEvent& e) {
            const char* action = "Unknown";
            switch (e.action)
            {
            case MenuActionType::Play: action = "Play"; break;
            case MenuActionType::HowToPlay: action = "HowToPlay"; break;
            case MenuActionType::Quit: action = "Quit"; break;
            }
            Engine::GetLogger().LogEvent(std::string("[EventBus] MenuAction -> ") + action);
        });
        // ...
    }

    ~GameProgram() override
    {
        auto& bus = Engine::GetEventBus();
        bus.Unsubscribe<RequestStateChangeEvent>(stateChangeSubscription);
        bus.Unsubscribe<MenuActionEvent>(menuActionSubscription);
    }

    void Update() override { Engine::Instance().Update(); }
    void Draw() override { Engine::Instance().Draw(); }
    void HandleEvent(SDL_Window&, const SDL_Event&) override {}

private:
    Splash splash;
    MainMenu mainmenu;
    GamePlay1 play;
    EventBus::SubscriptionId stateChangeSubscription = 0;
    EventBus::SubscriptionId menuActionSubscription = 0;
    // ...
};
