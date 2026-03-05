#pragma once

#include "../Engine/GameObject.h"
#include "GameObjectType.h"

class DataCore : public GameObject
{
public:
    explicit DataCore(vec2 startPos);
    ~DataCore() override = default;

    void Update(double dt) override;
    bool CanCollideWith(GameObjectType objectBType) override;
    void ResolveCollision(GameObject* objectB) override;

    GameObjectType GetObjectType() override { return GameObjectType::DataCore; }
    std::string GetObjectTypeName() override { return "DataCore"; }

private:
    vec2 anchorPos;
    double timeSec{ 0.0 };
    double phase{ 0.0 };
    float bobAmplitude{ 8.0f };
    float bobSpeed{ 2.5f };
    float spinSpeed{ 1.8f };
};
