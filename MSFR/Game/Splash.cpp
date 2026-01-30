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

	SomeTexture = TextureDX11(dev, ctx, "assets/images/1704515153572.jpg", false);
	//SomeTexture_I_have_to_add_I_didnt_deside_yet = TextureDX11{ "assets/images/DigiPen_BLACK_1024px.png", false };
}

void Splash::Update(double dt)
{
	timer -= dt;
	if (modeNext.IsKeyReleased() == true || timer < 0)
	{
		Engine::GetGameStateManager().SetNextState(static_cast<int>(ScreenMods::Menu));
	}
}

void Splash::Draw()
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

void Splash::Unload()
{
}