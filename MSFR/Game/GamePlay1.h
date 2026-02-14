#pragma once
#include <memory>

#include "../GameState.h"
#include "../Input.h"
#include "../TextureDX11.h"
#include "../GameObjectManager.h"

class GamePlay1 : public GameState
{
public:
	GamePlay1();
	~GamePlay1();
	void Load() override;
	void Draw() override;
	void Update(double dt) override;
	void Unload() override;
	std::string GetName() override { return "GamePlay1"; }
private:
	TextureDX11 MainMenuImage;
	double timer;
	GameObjectManager tt;
};