#pragma once
#include <GLVertexArray.h>
#include <GLShader.h>

#include "Rect.h" //rect
#include "Component.h" //Component inheritance
#include "mat3.h"

class GameObject;

class Collision : public Component
{
public:
    enum class CollideType
    {
        Rect_Collide,
        Circle_Collide,
    };
    virtual void Draw(mat3<float> cameraMatrix) = 0;
    virtual CollideType GetCollideType() = 0;
    virtual bool DoesCollideWith(GameObject* objectB) = 0;
    virtual bool DoesCollideWith(vec2 point) = 0;
};

class RectCollision : public Collision
{
public:
    RectCollision(rect3 rect, GameObject* objectPtr);
    void Draw(mat3<float> cameraMatrix) override;
    CollideType GetCollideType() override { return Collision::CollideType::Rect_Collide; };
    rect3 GetWorldCoorRect();
    bool DoesCollideWith(GameObject* objectB) override;
    virtual bool DoesCollideWith(vec2 point);
private:
    GameObject* objectPtr;
    rect3 rect;

    GLShader shader;
    GLVertexArray collisionBox;
};

class CircleCollision : public Collision
{
public:
    CircleCollision(double radius, GameObject* objectPtr);
    void Draw(mat3<float> cameraMatrix) override;
    CollideType GetCollideType() override { return Collision::CollideType::Circle_Collide; };
    double GetRadius();
    bool DoesCollideWith(GameObject* objectB) override;
    virtual bool DoesCollideWith(vec2 point);
private:
    GameObject* objectPtr;
    double radius;

    GLShader shader;
    GLVertexArray collisionCircle;
};