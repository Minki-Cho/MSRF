#include "GameObject.h"

#include <cmath>
#include <string>

#include "Engine.h"
#include "Sprite.h"
#include "Collision.h"


void GameObject::ChangeState(State* newState)
{
}

// Ctors / Dtor
GameObject::GameObject()
    : currState(&state_nothing),
    updateMatrix(true),
    rotation(0.0),
    scale(1.0f, 1.0f),
    position(0.0f, 0.0f),
    velocity(0.0f, 0.0f)
{
    currState->Enter(this);
}

GameObject::GameObject(vec2 position_)
    : currState(&state_nothing),
    updateMatrix(true),
    rotation(0.0),
    scale(1.0f, 1.0f),
    position(position_),
    velocity(0.0f, 0.0f)
{
    currState->Enter(this);
}

GameObject::GameObject(vec2 position_, double rotation_, vec2 scale_)
    : currState(&state_nothing),
    updateMatrix(true),
    rotation(rotation_),
    scale(scale_),
    position(position_),
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
    if (currState)
    {
        currState->Update(this, dt);
        currState->TestForExit(this);
    }

    if (velocity.x != 0.0f || velocity.y != 0.0f)
    {
        UpdatePosition(vec2{ static_cast<float>(velocity.x * dt), static_cast<float>(velocity.y * dt) });
    }

    UpdateGOComponents(dt);
}

void GameObject::Draw(mat3<float> cameraMatrix)
{
    // 모델->월드 행렬
    const mat3<float>& modelToWorld = GetMatrix();

    // 카메라까지 포함한 최종 행렬
    const mat3<float> displayMatrix = cameraMatrix * modelToWorld;

    // Sprite 컴포넌트가 있으면 그리기
    if (auto* spr = GetGOComponent<Sprite>())
    {
        spr->Draw(displayMatrix);
    }

    // Collision 디버그 드로우 (원하면 켜고 끄는 플래그로 감싸도 됨)
    if (auto* col = GetGOComponent<Collision>())
    {
        col->Draw(displayMatrix);
    }
}

// ---------------------------------------------
// Transform getters
// ---------------------------------------------
const mat3<float>& GameObject::GetMatrix()
{
    if (updateMatrix)
    {
        // 보통 2D는 Scale -> Rotate -> Translate 순 (로컬 -> 월드)
        // 네 mat3 곱셈 방향(좌/우 결합)이 엔진마다 다를 수 있는데,
        // 기존 코드(translation * scale 등) 흐름에 맞춰 아래처럼 구성.
        const mat3<float> S = mat3<float>::build_scale(scale.x, scale.y);
        const mat3<float> R = mat3<float>::build_rotation(static_cast<float>(rotation));
        const mat3<float> T = mat3<float>::build_translation(position.x, position.y);

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

// ---------------------------------------------
// Destroy flag
// ---------------------------------------------
bool GameObject::GetDestroyed()
{
    return shouldDestroyed;
}

void GameObject::SetDestroyed(bool b)
{
    shouldDestroyed = b;
}

// ---------------------------------------------
// State machine
// ---------------------------------------------
//void GameObject::ChangeState(State* newState)
//{
//    if (newState == nullptr)
//        newState = &state_nothing;
//
//    currState = newState;
//    currState->Enter(this);
//}

// ---------------------------------------------
// Transform mutators
// ---------------------------------------------
void GameObject::UpdatePosition(vec2 adjustPosition)
{
    position.x += adjustPosition.x;
    position.y += adjustPosition.y;
    updateMatrix = true;
}

void GameObject::SetVelocity(vec2 newVelocity)
{
    velocity = newVelocity;
}

void GameObject::UpdateVelocity(vec2 adjustVelocity)
{
    velocity.x += adjustVelocity.x;
    velocity.y += adjustVelocity.y;
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

// ---------------------------------------------
// Collision
// ---------------------------------------------
bool GameObject::CanCollideWith(GameObjectType /*objectBType*/)
{
    // 기본은 충돌 가능. (자식에서 오버라이드)
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

    // 타입 체크/필터링은 여기서 걸어도 되고,
    // Collision::DoesCollideWith 내부에서 타입 분기해도 됨.
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
    // 기본은 아무 것도 안 함. (자식에서 오버라이드)
}
