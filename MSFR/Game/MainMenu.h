#pragma once
#include <memory>

#include "../Engine/GameState.h"
#include "../Engine/Input.h"
#include "../Engine/TextureDX11.h"
#include "../Engine/GameObjectManager.h"

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
	TextureDX11 MainMenuImage;
	double timer;
	GameObjectManager tt;
};