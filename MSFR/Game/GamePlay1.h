#pragma once
#include <memory>

#include "../Engine/GameObjectManager.h"
#include "../Engine/GameState.h"
#include "../Engine/Input.h"
#include "../Engine/TextureDX11.h"
#include "Player.h"

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
    GameObjectManager* gameObjectManager{ nullptr };
    TextureDX11 map;
    double timer;
    Player* playerPtr{ nullptr };
};
