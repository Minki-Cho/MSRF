#include "Engine.h"
#include <SDL2/SDL.h>
#include "Game/Splash.h"
//#include "Game/MainMenu.h"
// ...

class GameProgram final : public IProgram
{
public:
    GameProgram(int, int)
    {
        Engine& engine = Engine::Instance();
        engine.InitCore();
        engine.GetGameStateManager().AddGameState(splash);
        //engine.GetGameStateManager().AddGameState(mainmenu);
        // ...
    }

    void Update() override { Engine::Instance().Update(); }
    void Draw() override { Engine::Instance().Draw(); }
    void HandleEvent(SDL_Window&, const SDL_Event&) override {}

private:
    Splash splash;
    //MainMenu mainmenu;
    // ...
};
