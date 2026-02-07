#include "../DX11Services.h"
#include "../Engine.h"

#include "Splash.h"
#include "ScreenMods.h"

#include <algorithm>
#define NOMINMAX
#undef min
#undef max

Splash::Splash() : modeNext(InputKey::Keyboard::Enter), timer(5.0f)
{
}

Splash::~Splash()
{
}

void Splash::Load()
{
	//Sounds preload!

	timer = 5;
	SplashImage = TextureDX11("assets/images/Splash.png", false);
}

void Splash::Update(double dt)
{
	timer -= dt;
	if (modeNext.IsKeyReleased() == true || timer < 0)
	{
		Engine::GetGameStateManager().SetNextState(static_cast<int>(ScreenMods::MainMenu));
	}
}

void Splash::Draw()
{
	SplashImage.DrawFitCenter({ (float)Engine::GetViewportWidth(), (float)Engine::GetViewportHeight() });
}

void Splash::Unload()
{
}