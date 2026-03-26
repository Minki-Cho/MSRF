#pragma once
#include "../Engine/GameObject.h"
#include "CharacterAnim.h"
#include "GameObjectType.h"

class Player : public GameObject
{
public:
    explicit Player(vec2 startPos);
    ~Player() override = default;

    void Update(double dt) override;
    bool CanCollideWith(GameObjectType objectBType) override;
    void ResolveCollision(GameObject* objectB) override;
    void Draw(mat3<float> TransformMatrix);
    GameObjectType GetObjectType() override { return GameObjectType::Player; }
    std::string GetObjectTypeName() override { return "Player"; }
    CharacterAnim GetDirection() const { return direction; }
    bool ApplyDamage(int damage);
    void SetMoveSpeedMultiplier(float multiplier);
    int ConsumeSpeedItemPickups();
    int ConsumeRapidItemPickups();
    int ConsumeHybridItemPickups();
    int GetHP() const { return hp; }
    int GetMaxHP() const { return maxHp; }
    bool IsDead() const { return hp <= 0; }

private:
    float baseMoveSpeed = 65.0f;
    float moveSpeedMultiplier = 1.0f;
    int pendingSpeedItemPickups = 0;
    int pendingRapidItemPickups = 0;
    int pendingHybridItemPickups = 0;
    vec2 startPos;
    int maxHp = 100;
    int hp = 100;
    double hitInvulnTimer = 0.0;
    double hitFlashTimer = 0.0;

    CharacterAnim direction = CharacterAnim::None_F;

    class StateIdle : public State
    {
    public:
        void Enter(GameObject* object) override;
        void Update(GameObject* object, double dt) override;
        void TestForExit(GameObject* object) override;
        std::string GetName() override { return "Idle"; }
    };

    class StateMove : public State
    {
    public:
        void Enter(GameObject* object) override;
        void Update(GameObject* object, double dt) override;
        void TestForExit(GameObject* object) override;
        std::string GetName() override { return "Move"; }
    };

    StateIdle stateIdle;
    StateMove stateMove;
};
