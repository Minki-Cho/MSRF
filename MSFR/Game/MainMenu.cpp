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

    struct FitRect
    {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
        float scale = 1.0f;
    };

    constexpr float kMenuRefWidth = 1080.0f;
    constexpr float kMenuRefHeight = 720.0f;

    FitRect ComputeFitRect(const vec2& textureSize, float viewportWidth, float viewportHeight)
    {
        FitRect fit{};
        if (textureSize.x() <= 0.0f || textureSize.y() <= 0.0f || viewportWidth <= 0.0f || viewportHeight <= 0.0f)
            return fit;

        const float sx = viewportWidth / textureSize.x();
        const float sy = viewportHeight / textureSize.y();
        fit.scale = (std::min)(sx, sy);
        fit.w = textureSize.x() * fit.scale;
        fit.h = textureSize.y() * fit.scale;
        fit.x = (viewportWidth - fit.w) * 0.5f;
        fit.y = (viewportHeight - fit.h) * 0.5f;
        return fit;
    }

    bool TryWindowToMenuUv(const vec2& mouseWindowPos, const FitRect& fit, vec2& outUv)
    {
        if (fit.w <= 0.0f || fit.h <= 0.0f || fit.scale <= 0.0f)
            return false;

        if (mouseWindowPos.x() < fit.x || mouseWindowPos.x() > (fit.x + fit.w) ||
            mouseWindowPos.y() < fit.y || mouseWindowPos.y() > (fit.y + fit.h))
        {
            return false;
        }

        outUv.x() = (mouseWindowPos.x() - fit.x) / fit.w;
        outUv.y() = (mouseWindowPos.y() - fit.y) / fit.h;
        return true;
    }
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
    wasMouseDown = Engine::GetInput().GetMouseDown();
    timer = Engine::IsAutoPlayEnabled() ? 0.35 : 0.0;
}

void MainMenu::Update(double dt)
{
    auto& input = Engine::GetInput();
    const bool mouseDownNow = input.GetMouseDown();
    const bool mouseDownStarted = mouseDownNow && !wasMouseDown;
    wasMouseDown = mouseDownNow;

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

    if (input.GetMouseReleasedThisFrame() || mouseDownStarted)
    {
        const vec2 mouseWindow = input.GetMousePos();
        const vec2 menuTextureSize = MainMenuImage.GetSize();
        const FitRect menuFit = ComputeFitRect(
            menuTextureSize,
            static_cast<float>(Engine::GetViewportWidth()),
            static_cast<float>(Engine::GetViewportHeight()));

        vec2 mouseUv = { 0.0f, 0.0f };
        if (!TryWindowToMenuUv(mouseWindow, menuFit, mouseUv))
        {
            return;
        }

        const vec2 mouseRef = {
            mouseUv.x() * kMenuRefWidth,
            mouseUv.y() * kMenuRefHeight
        };

        Engine::GetLogger().LogEvent(
            "mouse win: " +
            std::to_string((int)mouseWindow.x()) + ", " +
            std::to_string((int)mouseWindow.y()) +
            " -> ref: " +
            std::to_string((int)mouseRef.x()) + ", " +
            std::to_string((int)mouseRef.y()));

        const RectF play{
            330.f, 220.f,
            800.f, 345.f
        };

        const RectF howToPlay{
            330.f, 380.f,
            800.f, 380.f + 125.f
        };

        const RectF quit{
            330.f, 530.f,
            800.f, 530.f + 125.f
        };

        if (play.Contains(mouseRef.x(), mouseRef.y()))
        {
            requestPlay();
        }
        else if (howToPlay.Contains(mouseRef.x(), mouseRef.y()))
        {
            requestHowToPlay();
        }
        else if (quit.Contains(mouseRef.x(), mouseRef.y()))
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
    wasMouseDown = false;
}
