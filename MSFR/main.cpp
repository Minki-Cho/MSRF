#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "Engine/DX11App.h"
#include "Engine/Engine.h"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <string>

namespace
{
    std::uint64_t ParseAutoExitMs(int argc, char** argv)
    {
        constexpr const char* kPrefix = "--auto-exit-ms=";
        constexpr std::size_t kPrefixLen = 15;

        for (int i = 1; i < argc; ++i)
        {
            const char* arg = argv[i];
            if (!arg)
                continue;

            const std::string s(arg);
            if (s.rfind(kPrefix, 0) == 0)
            {
                const std::string value = s.substr(kPrefixLen);
                if (value.empty())
                    return 0;

                char* endPtr = nullptr;
                const unsigned long long parsed = std::strtoull(value.c_str(), &endPtr, 10);
                if (endPtr == value.c_str() || *endPtr != '\0')
                    return 0;

                return static_cast<std::uint64_t>(parsed);
            }
        }

        return 0;
    }
}

int main(int argc, char** argv)
{
    try
    {
        const std::uint64_t autoExitMs = ParseAutoExitMs(argc, argv);
        const std::uint64_t startTick = SDL_GetTicks64();

        DX11App app("My game Engine", 1080, 720);
        while (!app.IsDone())
        {
            app.Update();

            if (autoExitMs > 0)
            {
                const std::uint64_t elapsed = SDL_GetTicks64() - startTick;
                if (elapsed >= autoExitMs)
                    break;
            }
        }
    }
    catch (const std::exception& e)
    {
        ENGINE_LOG_CTX(Engine::GetLogger(), Logger::Severity::Fatal, "App", std::string("Unhandled exception: ") + e.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal Error", e.what(), nullptr);
        return 1;
    }

    return 0;
}
