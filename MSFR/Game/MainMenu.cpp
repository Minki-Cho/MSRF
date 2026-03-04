#include "../Engine/DX11Services.h"
#include "../Engine/Engine.h"
#include "../Engine/EventTypes.h"

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
    //Sounds preload!
    // Not yet

    //timer = 5;
    MainMenuImage = TextureDX11("assets/images/MainMenu.png", false);
}

void MainMenu::Update(double dt)
{
    auto& input = Engine::GetInput();

    if (!input.GetMouseReleasedThisFrame())
        return;

    const vec2 mouse = input.GetMousePos();

    Engine::GetLogger().LogEvent(
        "mouse win: " +
        std::to_string((int)mouse.x()) + ", " +
        std::to_string((int)mouse.y())
    );


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
        Engine::GetEventBus().Publish(MenuActionEvent{ MenuActionType::Play });
        Engine::GetEventBus().Publish(RequestStateChangeEvent{ (int)ScreenMods::GamePlay1 });
    }
    else if (howToPlay.Contains(mouse.x(), mouse.y()))
    {
        Engine::GetEventBus().Publish(MenuActionEvent{ MenuActionType::HowToPlay });
    }
    else if (quit.Contains(mouse.x(), mouse.y()))
    {
        Engine::GetEventBus().Publish(MenuActionEvent{ MenuActionType::Quit });
    }

    tt.Update(dt);
}

void MainMenu::Draw()
{
    MainMenuImage.DrawFitCenter({ (float)Engine::GetViewportWidth(), (float)Engine::GetViewportHeight() });
}

void MainMenu::Unload()
{
}
