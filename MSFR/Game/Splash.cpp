#include "../DX11Services.h"
#include "../Engine.h"

#include "Splash.h"
#include "ScreenMods.h"
Splash::Splash() : modeNext(InputKey::Keyboard::Enter), timer(5.0f)
{
	//Engine::GetWindow().SetBackgroundColor(1, 1, 1, 1);
	//DX11Services::SetClearColor(1.f, 1.f, 1.f, 1.f);
}

Splash::~Splash()
{
}

void Splash::Load()
{
	//Sounds preload!
	// Not yet

	timer = 5;
	auto* dev = Engine::GetDXDevice();
	auto* ctx = Engine::GetDXContext();

	SomeTexture_I_have_to_add_I_didnt_deside_yet = TextureDX11(dev, ctx, "assets/images/DigiPen_BLACK_1024px.png", false);
	//SomeTexture_I_have_to_add_I_didnt_deside_yet = TextureDX11{ "assets/images/DigiPen_BLACK_1024px.png", false };
}

void Splash::Update(double dt)
{
	//DX11Services::SetClearColor(1.f, 1.f, 1.f, 1.f);
	timer -= dt;
	if (modeNext.IsKeyReleased() == true || timer < 0)
	{
		Engine::GetGameStateManager().SetNextState(static_cast<int>(ScreenMods::Menu));
	}
}

void Splash::Draw()
{
	float x = 1280.f - SomeTexture_I_have_to_add_I_didnt_deside_yet.GetSize().x;
	float y = 720.f - SomeTexture_I_have_to_add_I_didnt_deside_yet.GetSize().y;
	//SomeTexture_I_have_to_add_I_didnt_deside_yet.Draw(mat3<float>::build_translation({ x / 2, y / 2 }));
}

void Splash::Unload()
{
}