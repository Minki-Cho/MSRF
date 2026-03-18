#include "GameObject.h"

#include <cmath>
#include <string>
#include <thread>
#include <sstream>
#include <algorithm>

#include "Engine.h"
#include "Sprite.h"
#include "Collision.h"

// Ctors / Dtor
GameObject::GameObject()
    : currState(&state_nothing),
    updateMatrix(true),
    rotation(0.0),
    previousRotation(0.0),
    scale(1.0f, 1.0f),
    previousScale(1.0f, 1.0f),
    position(0.0f, 0.0f),
    previousPosition(0.0f, 0.0f),
    velocity(0.0f, 0.0f)
{
    currState->Enter(this);
}

GameObject::GameObject(vec2 position_)
    : currState(&state_nothing),
    updateMatrix(true),
    rotation(0.0),
    previousRotation(0.0),
    scale(1.0f, 1.0f),
    previousScale(1.0f, 1.0f),
    position(position_),
    previousPosition(position_),
    velocity(0.0f, 0.0f)
{
    currState->Enter(this);
}

GameObject::GameObject(vec2 position_, double rotation_, vec2 scale_)
    : currState(&state_nothing),
    updateMatrix(true),
    rotation(rotation_),
    previousRotation(rotation_),
    scale(scale_),
    previousScale(scale_),
    position(position_),
    previousPosition(position_),
    velocity(0.0f, 0.0f)
{
    currState->Enter(this);
}

GameObject::~GameObject()
{
    ClearGOComponents();
}

void GameObject::Update(double dt)
{
    previousPosition = position;
    previousScale = scale;
    previousRotation = rotation;

    if (currState)
    {
        currState->Update(this, dt);
        currState->TestForExit(this);
    }

    if (velocity.x() != 0.0f || velocity.y() != 0.0f)
    {
        UpdatePosition(vec2{ static_cast<float>(velocity.x() * dt), static_cast<float>(velocity.y() * dt) });
    }

    UpdateGOComponents(dt);
}

void GameObject::Draw(mat3<float> cameraMatrix)
{
    const float alpha = static_cast<float>(std::clamp(Engine::GetRenderInterpolationAlpha(), 0.0, 1.0));
    constexpr float kEps = 0.0001f;

    const bool noTransformChange =
        std::fabs(position.x() - previousPosition.x()) <= kEps &&
        std::fabs(position.y() - previousPosition.y()) <= kEps &&
        std::fabs(scale.x() - previousScale.x()) <= kEps &&
        std::fabs(scale.y() - previousScale.y()) <= kEps &&
        std::fabs(rotation - previousRotation) <= static_cast<double>(kEps);

    const mat3<float> modelToWorld =
        (alpha <= kEps || noTransformChange)
        ? GetMatrix()
        : BuildMatrixFor(
            vec2{
                previousPosition.x() + (position.x() - previousPosition.x()) * alpha,
                previousPosition.y() + (position.y() - previousPosition.y()) * alpha
            },
            previousRotation + (rotation - previousRotation) * static_cast<double>(alpha),
            vec2{
                previousScale.x() + (scale.x() - previousScale.x()) * alpha,
                previousScale.y() + (scale.y() - previousScale.y()) * alpha
            });

    const mat3<float> displayMatrix = cameraMatrix * modelToWorld;

    if (auto* spr = GetGOComponent<Sprite>())
    {
        spr->Draw(displayMatrix);
    }

    if (Engine::IsCollisionDebugDrawEnabled())
    {
        if (auto* col = GetGOComponent<Collision>())
        {
            col->Draw(displayMatrix);
        }
    }
}

mat3<float> GameObject::BuildMatrixFor(const vec2& pos, double rot, const vec2& scl) const
{
    const mat3<float> S = mat3<float>::build_scale(scl.x(), scl.y());
    const mat3<float> R = mat3<float>::build_rotation(static_cast<float>(rot));
    const mat3<float> T = mat3<float>::build_translation(pos.x(), pos.y());
    return T * R * S;
}

// Transform getters
const mat3<float>& GameObject::GetMatrix()
{
    if (updateMatrix)
    {
        const mat3<float> S = mat3<float>::build_scale(scale.x(), scale.y());
        const mat3<float> R = mat3<float>::build_rotation(static_cast<float>(rotation));
        const mat3<float> T = mat3<float>::build_translation(position.x(), position.y());

        objectMatrix = T * R * S;
        updateMatrix = false;
    }
    return objectMatrix;
}

const vec2& GameObject::GetPosition() const
{
    return position;
}

const vec2& GameObject::GetVelocity() const
{
    return velocity;
}

const vec2& GameObject::GetScale() const
{
    return scale;
}

double GameObject::GetRotation() const
{
    return rotation;
}

void GameObject::SetPosition(vec2 newPosition)
{
    position = newPosition;
    updateMatrix = true;
}

bool GameObject::GetDestroyed()
{
    return shouldDestroyed;
}

void GameObject::SetDestroyed(bool b)
{
    shouldDestroyed = b;
}

// State machine
void GameObject::ChangeState(State* newState)
{
    if (newState == nullptr)
        newState = &state_nothing;

    currState = newState;
    currState->Enter(this);
}

// Transform mutators
void GameObject::UpdatePosition(vec2 adjustPosition)
{
    position.x() += adjustPosition.x();
    position.y() += adjustPosition.y();
    updateMatrix = true;
}

void GameObject::SetVelocity(vec2 newVelocity)
{
    velocity = newVelocity;
}

void GameObject::UpdateVelocity(vec2 adjustVelocity)
{
    velocity.x() += adjustVelocity.x();
    velocity.y() += adjustVelocity.y();
}

void GameObject::SetScale(vec2 newScale)
{
    scale = newScale;
    updateMatrix = true;
}

void GameObject::SetRotation(double newRotationAmount)
{
    rotation = newRotationAmount;
    updateMatrix = true;
}

void GameObject::UpdateRotation(double newRotationAmount)
{
    rotation += newRotationAmount;
    updateMatrix = true;
}

// Collision
bool GameObject::CanCollideWith(GameObjectType /*objectBType*/)
{
    return true;
}

bool GameObject::DoesCollideWith(GameObject* objectB)
{
    if (objectB == nullptr)
        return false;

    auto* colA = GetGOComponent<Collision>();
    auto* colB = objectB->GetGOComponent<Collision>();
    if (!colA || !colB)
        return false;

    return colA->DoesCollideWith(objectB);
}

bool GameObject::DoesCollideWith(vec2 point)
{
    auto* col = GetGOComponent<Collision>();
    if (!col)
        return false;

    return col->DoesCollideWith(point);
}

void GameObject::ResolveCollision(GameObject* /*other*/)
{
}
