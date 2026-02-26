#include "../Engine/DX11Services.h"
#include "../Engine/Engine.h"

#include "Splash.h"
#include "ScreenMods.h"


Splash::Splash() : timer(5.0f)
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

    const bool wantSkip = Engine::GetActionSystem().Has(ActionId::Skip);
    if (wantSkip || timer < 0)
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