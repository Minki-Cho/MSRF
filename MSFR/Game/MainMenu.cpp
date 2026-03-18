#include "../Engine/DX11Services.h"
#include "../Engine/Engine.h"
#include "../Engine/EventTypes.h"
#include "../external/imgui/imgui.h"
#include <SDL2/SDL.h>

#include "MainMenu.h"
#include "ScreenMods.h"

namespace
{
    struct RectF
    {
        float l, t, r, b;
        bool Contains(float x, float y) const
        {
            return x >= l && x <= r && y >= t && y <= b;
        }
    };
}

MainMenu::MainMenu() : modeNext(InputKey::Keyboard::Enter), timer(5.0f)
{
}

MainMenu::~MainMenu()
{
}

void MainMenu::Load()
{
    MainMenuImage = TextureDX11("assets/images/MainMenu.png", false);
    showQuitConfirm = false;
    requestQuitNow = false;
    timer = Engine::IsAutoPlayEnabled() ? 0.35 : 0.0;
}

void MainMenu::Update(double dt)
{
    auto& input = Engine::GetInput();

    auto requestPlay = []()
    {
        Engine::GetEventBus().Publish(MenuActionEvent{ MenuActionType::Play });
        Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::GamePlay1) });
    };

    auto requestHowToPlay = []()
    {
        Engine::GetEventBus().Publish(MenuActionEvent{ MenuActionType::HowToPlay });
        Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::Howtoplay) });
    };

    if (Engine::IsAutoPlayEnabled() && timer > 0.0)
    {
        timer -= dt;
        if (timer <= 0.0)
        {
            requestPlay();
            return;
        }
    }

    if (requestQuitNow)
    {
        requestQuitNow = false;
        Engine::GetEventBus().Publish(MenuActionEvent{ MenuActionType::Quit });
        Engine::GetGameStateManager().Shutdown();

        SDL_Event quitEvent{};
        quitEvent.type = SDL_QUIT;
        SDL_PushEvent(&quitEvent);
        return;
    }

    if (showQuitConfirm)
    {
        if (input.IsKeyPressed(InputKey::Keyboard::Escape))
        {
            showQuitConfirm = false;
        }
        else if (input.IsKeyPressed(InputKey::Keyboard::Enter))
        {
            showQuitConfirm = false;
            requestQuitNow = true;
        }

        tt.Update(dt);
        return;
    }

    if (input.IsKeyPressed(InputKey::Keyboard::Enter) || input.IsKeyPressed(InputKey::Keyboard::Space))
    {
        requestPlay();
        return;
    }

    if (input.IsKeyPressed(InputKey::Keyboard::H))
    {
        requestHowToPlay();
        return;
    }

    if (input.IsKeyPressed(InputKey::Keyboard::Escape))
    {
        showQuitConfirm = true;
        return;
    }

    if (input.GetMouseReleasedThisFrame())
    {
        const vec2 mouse = input.GetMousePos();

        Engine::GetLogger().LogEvent(
            "mouse win: " +
            std::to_string((int)mouse.x()) + ", " +
            std::to_string((int)mouse.y()));

        const RectF play{
            390.f, 220.f,
            800.f, 345.f
        };

        const RectF howToPlay{
            390.f, 380.f,
            800.f, 380.f + 125.f
        };

        const RectF quit{
            390.f, 530.f,
            800.f, 530.f + 125.f
        };

        if (play.Contains(mouse.x(), mouse.y()))
        {
            requestPlay();
        }
        else if (howToPlay.Contains(mouse.x(), mouse.y()))
        {
            requestHowToPlay();
        }
        else if (quit.Contains(mouse.x(), mouse.y()))
        {
            showQuitConfirm = true;
        }
    }

    tt.Update(dt);
}

void MainMenu::Draw()
{
    MainMenuImage.DrawFitCenter({ (float)Engine::GetViewportWidth(), (float)Engine::GetViewportHeight() });

    if (!showQuitConfirm)
        return;

    ImGui::SetNextWindowPos(
        ImVec2(Engine::GetViewportWidth() * 0.5f, Engine::GetViewportHeight() * 0.5f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f));

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("Quit Confirmation", nullptr, flags))
    {
        ImGui::TextUnformatted("Exit the game?");
        ImGui::Separator();
        if (ImGui::Button("Quit", ImVec2(150.0f, 0.0f)))
        {
            showQuitConfirm = false;
            requestQuitNow = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(150.0f, 0.0f)))
        {
            showQuitConfirm = false;
        }
    }
    ImGui::End();
}

void MainMenu::Unload()
{
    MainMenuImage.Reset();
    showQuitConfirm = false;
    requestQuitNow = false;
}
