#pragma once
#include <memory>

#include "../GameState.h"
#include "../Input.h"
#include "../TextureDX11.h"

class MainMenu : public GameState
{
public:
	MainMenu();
	~MainMenu();
	void Load() override;
	void Draw() override;
	void Update(double dt) override;
	void Unload() override;
	std::string GetName() override { return "MainMenu"; }
private:
	InputKey modeNext;
	TextureDX11 SomeTexture;
	double timer;
};