#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "DX11App.h"
#include <exception>

int main(int, char**)
{
    try
    {
        DX11App app("My game Engine", 1280, 720);

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