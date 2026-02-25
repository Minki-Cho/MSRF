#pragma once
#include "../GameObject.h"
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

private:

    float moveSpeed = 550.0f;
    vec2 startPos;

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
