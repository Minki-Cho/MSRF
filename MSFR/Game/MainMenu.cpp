#include "../DX11Services.h"
#include "../Engine.h"

#include "MainMenu.h"
#include "ScreenMods.h"


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
	SomeTexture = TextureDX11("assets/images/1700094944318.jpg", false);
}

void MainMenu::Update(double dt)
{
	//timer -= dt;
	//if (modeNext.IsKeyReleased() == true || timer < 0)
	//{
	//	Engine::GetGameStateManager().SetNextState(static_cast<int>(ScreenMods::Menu));
	//}
}

void MainMenu::Draw()
{
	const float screenW = (float)Engine::GetViewportWidth();
	const float screenH = (float)Engine::GetViewportHeight();

	const vec2 tex = SomeTexture.GetSize();

	const float sx = screenW / tex.x;
	const float sy = screenH / tex.y;
	const float s = (std::min)(sx, sy);

	const float drawW = tex.x * s;
	const float drawH = tex.y * s;

	const float x = (screenW - drawW) * 0.5f;
	const float y = (screenH - drawH) * 0.5f;

	mat3<float> M;
	M.column0.x = s;
	M.column1.y = s;
	M.column2.x = x;
	M.column2.y = y;

	SomeTexture.Draw(M);
}

void MainMenu::Unload()
{
}