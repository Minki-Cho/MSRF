#pragma once
#include <memory>

#include "../GameState.h"
#include "../Input.h"
#include "../TextureDX11.h"

class Splash : public GameState
{
public:
	Splash();
	~Splash();
	void Load() override;
	void Draw() override;
	void Update(double dt) override;
	void Unload() override;
	std::string GetName() override { return "Spalsh"; }
private:
	InputKey modeNext;
	TextureDX11 SomeTexture;
	double timer;
};