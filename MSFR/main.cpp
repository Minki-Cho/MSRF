#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "DX11App.h"
#include <exception>
#include "Engine.h"

int main(int, char**)
{
    try
    {
        Engine& engine = Engine::Instance();
        DX11App app("DX11 + SDL2 Engine", 1280, 720);

        while (!app.IsDone())
        {
            app.Update();
        }
    }
    catch (const std::exception& e)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", e.what(), nullptr);
        return 1;
    }

    return 0;
}
