#include "DataCore.h"

#include <cmath>

#include "../Engine/Sprite.h"

DataCore::DataCore(vec2 startPos)
    : GameObject(startPos),
      anchorPos(startPos)
{
    AddGOComponent(new Sprite("assets/images/items/data_core/data_core.spt", this));


    phase = (static_cast<double>(startPos.x()) * 0.07) + (static_cast<double>(startPos.y()) * 0.03);
}

void DataCore::Update(double dt)
{
    timeSec += dt;

    const float bob = std::sin(static_cast<float>(timeSec * bobSpeed + phase)) * bobAmplitude;
    SetPosition(vec2{ anchorPos.x(), anchorPos.y() + bob });
    UpdateRotation(spinSpeed * dt);

    GameObject::Update(dt);
}

bool DataCore::CanCollideWith(GameObjectType /*objectBType*/)
{
    return false;
}

void DataCore::ResolveCollision(GameObject* /*objectB*/)
{
}
