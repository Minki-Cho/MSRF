#include "Player.h"
#include "../Engine/Collision.h"
#include "../Engine/Engine.h"
#include "../Engine/Sprite.h"

#include <algorithm>
#include <cmath>

namespace
{
    int ToAnimActionId(CharacterAnim anim)
    {
        switch (anim)
        {
        case CharacterAnim::None_F: return 0;
        case CharacterAnim::None_B: return 1;
        case CharacterAnim::None_L: return 2;
        case CharacterAnim::None_R: return 3;
        case CharacterAnim::Front:  return 4;
        case CharacterAnim::Back:   return 5;
        case CharacterAnim::Left:   return 6;
        case CharacterAnim::Right:  return 7;
        default:                    return 0;
        }
    }

}

Player::Player(vec2 startPos_) : startPos(startPos_), GameObject(startPos_)
{
    AddGOComponent(new Sprite("assets/images/characters/characters/thief/thief.spt", this));

    currState = &stateIdle;
    currState->Enter(this);
    direction = CharacterAnim::None_F;
}

void Player::Update(double dt)
{
    if (hitInvulnTimer > 0.0)
    {
        hitInvulnTimer -= dt;
        if (hitInvulnTimer < 0.0)
            hitInvulnTimer = 0.0;
    }

    if (hitFlashTimer > 0.0)
    {
        hitFlashTimer -= dt;
        if (hitFlashTimer < 0.0)
            hitFlashTimer = 0.0;
    }

    GameObject::Update(dt);
    const float mapWidth = static_cast<float>(Engine::GetViewportWidth());
    const float mapHeight = static_cast<float>(Engine::GetViewportHeight());

    const float worldMinX = -mapWidth;
    const float worldMinY = -mapHeight;
    const float worldMaxX = mapWidth * 2.0f;
    const float worldMaxY = mapHeight * 2.0f;

    float edgeOffsetX = 0.0f;
    float edgeOffsetY = 0.0f;
    if (auto* spr = GetGOComponent<Sprite>())
    {
        const vec2 frameSize = spr->GetFrameSize();
        edgeOffsetX = frameSize.x();
        edgeOffsetY = frameSize.y();
    }

    const float minX = worldMinX + edgeOffsetX;
    const float minY = worldMinY + edgeOffsetY;
    const float maxX = worldMaxX - edgeOffsetX;
    const float maxY = worldMaxY - edgeOffsetY;

    vec2 p = GetPosition();
    if (p.x() < minX) p.x() = minX;
    if (p.x() > maxX) p.x() = maxX;
    if (p.y() < minY) p.y() = minY;
    if (p.y() > maxY) p.y() = maxY;
    SetPosition(p);
}

bool Player::CanCollideWith(GameObjectType objectBType)
{
    return objectBType == GameObjectType::DataCore
        || objectBType == GameObjectType::ItemSpeed
        || objectBType == GameObjectType::ItemRapid
        || objectBType == GameObjectType::ItemHybrid;
}

void Player::ResolveCollision(GameObject* objectB)
{
    if (objectB == nullptr)
        return;

    switch (objectB->GetObjectType())
    {
    case GameObjectType::DataCore:
        objectB->SetDestroyed(true);
        break;
    case GameObjectType::ItemSpeed:
        objectB->SetDestroyed(true);
        ++pendingSpeedItemPickups;
        break;
    case GameObjectType::ItemRapid:
        objectB->SetDestroyed(true);
        ++pendingRapidItemPickups;
        break;
    case GameObjectType::ItemHybrid:
        objectB->SetDestroyed(true);
        ++pendingHybridItemPickups;
        break;
    default:
        break;
    }
}

bool Player::ApplyDamage(int damage)
{
    if (damage <= 0 || hp <= 0)
        return false;

    if (hitInvulnTimer > 0.0)
        return false;

    hp = (std::max)(0, hp - damage);
    hitInvulnTimer = 0.20;
    hitFlashTimer = 0.24;

    return true;
}

void Player::SetMoveSpeedMultiplier(float multiplier)
{
    moveSpeedMultiplier = std::clamp(multiplier, 0.1f, 8.0f);
}

int Player::ConsumeSpeedItemPickups()
{
    const int count = pendingSpeedItemPickups;
    pendingSpeedItemPickups = 0;
    return count;
}

int Player::ConsumeRapidItemPickups()
{
    const int count = pendingRapidItemPickups;
    pendingRapidItemPickups = 0;
    return count;
}

int Player::ConsumeHybridItemPickups()
{
    const int count = pendingHybridItemPickups;
    pendingHybridItemPickups = 0;
    return count;
}

void Player::Draw(mat3<float> TransformMatrix)
{
    const mat3<float>& modelToWorld = GetMatrix();
    const mat3<float> displayMatrix = TransformMatrix * modelToWorld;

    bool hideSprite = false;
    if (hitFlashTimer > 0.0)
    {
        const double flashPeriod = 0.07;
        const double phase = std::fmod(hitFlashTimer, flashPeriod);
        hideSprite = (phase < flashPeriod * 0.5);
    }

    if (!hideSprite)
    {
        if (auto* spr = GetGOComponent<Sprite>())
            spr->Draw(displayMatrix);
    }

    if (Engine::IsCollisionDebugDrawEnabled())
    {
        if (auto* col = GetGOComponent<Collision>())
            col->Draw(displayMatrix);
    }

}

void Player::StateIdle::Enter(GameObject* object)
{
    Player* player = static_cast<Player*>(object);
    player->SetVelocity(vec2{ 0.f, 0.f });

    if (auto* spr = player->GetGOComponent<Sprite>())
        spr->PlayAnimation(ToAnimActionId(player->direction));
}

void Player::StateIdle::Update(GameObject* object, double /*dt*/)
{
    Player* player = static_cast<Player*>(object);
    player->SetVelocity(vec2{ 0.f, 0.f });
}

void Player::StateIdle::TestForExit(GameObject* object)
{
    Player* p = static_cast<Player*>(object);

    if (p->IsDead())
        return;

    ActionSystem& actions = Engine::GetActionSystem();
    const bool L = actions.Has(ActionId::Left);
    const bool R = actions.Has(ActionId::Right);
    const bool U = actions.Has(ActionId::Up);
    const bool D = actions.Has(ActionId::Down);

    if (L || R || U || D)
        p->ChangeState(&p->stateMove);
}

void Player::StateMove::Enter(GameObject* object)
{
    Player* p = static_cast<Player*>(object);

    if (auto* spr = p->GetGOComponent<Sprite>())
        spr->PlayAnimation(ToAnimActionId(p->direction));
}

void Player::StateMove::Update(GameObject* object, double /*dt*/)
{
    Player* p = static_cast<Player*>(object);

    if (p->IsDead())
    {
        p->SetVelocity(vec2{ 0.0f, 0.0f });
        return;
    }

    auto& actions = Engine::GetActionSystem();
    const bool L = actions.Has(ActionId::Left);
    const bool R = actions.Has(ActionId::Right);
    const bool U = actions.Has(ActionId::Up);
    const bool D = actions.Has(ActionId::Down);

    vec2 v{ 0.f, 0.f };

    const float activeMoveSpeed = p->baseMoveSpeed * p->moveSpeedMultiplier;
    if (L) v.x() -= activeMoveSpeed;
    if (R) v.x() += activeMoveSpeed;
    if (U) v.y() += activeMoveSpeed;
    if (D) v.y() -= activeMoveSpeed;

    if (L) p->direction = CharacterAnim::Front;
    else if (R) p->direction = CharacterAnim::Back;
    else if (U) p->direction = CharacterAnim::Right;
    else if (D) p->direction = CharacterAnim::Left;


    if (!L && !R && !U && !D)
    {
        switch (p->direction)
        {
        case CharacterAnim::Left:  p->direction = CharacterAnim::None_L; break;
        case CharacterAnim::Right: p->direction = CharacterAnim::None_R; break;
        case CharacterAnim::Back:  p->direction = CharacterAnim::None_B; break;
        case CharacterAnim::Front: p->direction = CharacterAnim::None_F; break;
        default: break;
        }
    }

    p->SetVelocity(v);

    if (auto* spr = p->GetGOComponent<Sprite>())
        spr->PlayAnimation(ToAnimActionId(p->direction));
}

void Player::StateMove::TestForExit(GameObject* object)
{
    Player* p = static_cast<Player*>(object);

    ActionSystem& actions = Engine::GetActionSystem();
    const bool L = actions.Has(ActionId::Left);
    const bool R = actions.Has(ActionId::Right);
    const bool U = actions.Has(ActionId::Up);
    const bool D = actions.Has(ActionId::Down);

    if (!L && !R && !U && !D)
        p->ChangeState(&p->stateIdle);
}
