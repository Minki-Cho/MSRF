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
	MainMenuImage = TextureDX11("assets/images/MainMenu.png", false);
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
	MainMenuImage.DrawFitCenter({ (float)Engine::GetViewportWidth(), (float)Engine::GetViewportHeight() });
}

void MainMenu::Unload()
{
}