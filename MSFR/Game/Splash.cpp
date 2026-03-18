#include "../Engine/DX11Services.h"
#include "../Engine/Engine.h"
#include "../Engine/EventTypes.h"

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
    Engine::PlaySound("assets/sounds/enter_gameplay.wav");

    timer = 5;
    SplashImage = TextureDX11("assets/images/Splash.png", false);
}

void Splash::Update(double dt)
{
    timer -= dt;

    const bool wantSkip = Engine::GetActionSystem().Has(ActionId::Skip);
    if (wantSkip || timer < 0)
    {
        Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::MainMenu) });
    }
}

void Splash::Draw()
{
    SplashImage.DrawFitCenter({ (float)Engine::GetViewportWidth(), (float)Engine::GetViewportHeight() });
}

void Splash::Unload()
{
    SplashImage.Reset();
}


