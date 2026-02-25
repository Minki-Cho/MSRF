#include "Player.h"
#include "../Engine.h"
#include "../Sprite.h"

Player::Player(vec2 startPos_)
    : Up(InputKey::Keyboard::Up)
    , Down(InputKey::Keyboard::Down)
    , Left(InputKey::Keyboard::Left)
    , Right(InputKey::Keyboard::Right)
    , startPos(startPos_)
    , GameObject(startPos_)
{
    
    AddGOComponent(new Sprite("assets/images/characters/characters/thief/thief.spt", this));

    currState = &stateIdle;
    currState->Enter(this);
    direction = CharacterAnim::None_F;
}

void Player::Update(double dt)
{
    GameObject::Update(dt);

    const float minX = 0.0f;
    const float minY = 0.0f;
    const float maxX = static_cast<float>(Engine::GetViewportWidth());
    const float maxY = static_cast<float>(Engine::GetViewportHeight());

    const vec2 p = GetPosition();
    if (p.x() < minX || p.x() > maxX || p.y() < minY || p.y() > maxY)
        SetPosition(startPos);
}

bool Player::CanCollideWith(GameObjectType /*objectBType*/)
{
    return false;
}

void Player::ResolveCollision(GameObject* /*objectB*/)
{
}
void Player::Draw(mat3<float> TransformMatrix)
{
    GameObject::Draw(TransformMatrix);
}

void Player::StateIdle::Enter(GameObject* object)
{
    Player* player = static_cast<Player*>(object);
    player->SetVelocity(vec2{ 0.f, 0.f });

    if (auto* spr = player->GetGOComponent<Sprite>())
        spr->PlayAnimation(static_cast<int>(player->direction));
}

void Player::StateIdle::Update(GameObject* object, double /*dt*/)
{
    Player* player = static_cast<Player*>(object);
    player->SetVelocity(vec2{ 0.f, 0.f });
}

void Player::StateIdle::TestForExit(GameObject* object)
{
    Player* p = static_cast<Player*>(object);

    const bool L = p->Left.IsKeyDown();
    const bool R = p->Right.IsKeyDown();
    const bool U = p->Up.IsKeyDown();
    const bool D = p->Down.IsKeyDown();

    if (L || R || U || D)
        p->ChangeState(&p->stateMove);
}

// -------------------- StateMove --------------------

void Player::StateMove::Enter(GameObject* object)
{
    Player* p = static_cast<Player*>(object);


    if (auto* spr = p->GetGOComponent<Sprite>())
        spr->PlayAnimation(static_cast<int>(p->direction));
}

void Player::StateMove::Update(GameObject* object, double /*dt*/)
{
    Player* p = static_cast<Player*>(object);

    const bool L = p->Left.IsKeyDown();
    const bool R = p->Right.IsKeyDown();
    const bool U = p->Up.IsKeyDown();
    const bool D = p->Down.IsKeyDown();

    vec2 v{ 0.f, 0.f };

    if (L) v.x() -= p->moveSpeed;
    if (R) v.x() += p->moveSpeed;
    if (U) v.y() += p->moveSpeed;
    if (D) v.y() -= p->moveSpeed;

    if (L) p->direction = CharacterAnim::Left;
    else if (R) p->direction = CharacterAnim::Right;
    else if (U) p->direction = CharacterAnim::Back;
    else if (D) p->direction = CharacterAnim::Front;


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
        spr->PlayAnimation(static_cast<int>(p->direction));
}

void Player::StateMove::TestForExit(GameObject* object)
{
    Player* p = static_cast<Player*>(object);

    const bool L = p->Left.IsKeyDown();
    const bool R = p->Right.IsKeyDown();
    const bool U = p->Up.IsKeyDown();
    const bool D = p->Down.IsKeyDown();

    if (!L && !R && !U && !D)
        p->ChangeState(&p->stateIdle);
}
