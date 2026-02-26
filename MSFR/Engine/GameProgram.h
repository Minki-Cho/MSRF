#include "Engine.h"
#include <SDL2/SDL.h>
#include "../Game/Splash.h"
#include "../Game/MainMenu.h"
#include "../Game/GamePlay1.h"
#include "IProgram.h"
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
        // ...
    }

    void Update() override { Engine::Instance().Update(); }
    void Draw() override { Engine::Instance().Draw(); }
    void HandleEvent(SDL_Window&, const SDL_Event&) override {}

private:
    Splash splash;
    MainMenu mainmenu;
    GamePlay1 play;
    // ...
};
