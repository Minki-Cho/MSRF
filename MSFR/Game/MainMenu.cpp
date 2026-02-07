#include "../DX11Services.h"
#include "../Engine.h"

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
	//Engine::GetWindow().SetBackgroundColor(1, 1, 1, 1);
	//DX11Services::SetClearColor(1.f, 1.f, 1.f, 1.f);
    
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
        std::to_string((int)mouse.x) + ", " +
        std::to_string((int)mouse.y)
    );

    // === 버튼 영역 (window 기준) ===
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

    if (play.Contains(mouse.x, mouse.y))
    {
        Engine::GetLogger().LogEvent("[MainMenu] Play clicked");
        Engine::GetGameStateManager().SetNextState((int)ScreenMods::Splash);
    }
    else if (howToPlay.Contains(mouse.x, mouse.y))
    {
        Engine::GetLogger().LogEvent("[MainMenu] HowToPlay clicked");
    }
    else if (quit.Contains(mouse.x, mouse.y))
    {
        Engine::GetLogger().LogEvent("[MainMenu] Quit clicked");
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