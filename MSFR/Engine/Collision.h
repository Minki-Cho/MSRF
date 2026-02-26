#pragma once

#include "Rect.h"
#include "Component.h"
#include "mat3.h"
#include "vec2.h"
#include "color3.h"

#include <d3d11.h>
#include <wrl/client.h>

class GameObject;

class Collision : public Component
{
public:
    enum class CollideType { Rect_Collide, Circle_Collide };

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
    CollideType GetCollideType() override { return CollideType::Rect_Collide; }

    rect3 GetWorldCoorRect();
    bool DoesCollideWith(GameObject* objectB) override;
    bool DoesCollideWith(vec2 point) override;

private:
    void CreateGpuResources();

private:
    GameObject* objectPtr = nullptr;
    rect3 rect{};

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  ps;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>       vbPos;
    Microsoft::WRL::ComPtr<ID3D11Buffer>       vbCol;
    Microsoft::WRL::ComPtr<ID3D11Buffer>       ib;
    Microsoft::WRL::ComPtr<ID3D11Buffer>       cbPerDraw;

    UINT indexCount = 0;
};

class CircleCollision : public Collision
{
public:
    CircleCollision(double radius, GameObject* objectPtr);

    void Draw(mat3<float> cameraMatrix) override;
    CollideType GetCollideType() override { return CollideType::Circle_Collide; }

    double GetRadius();
    bool DoesCollideWith(GameObject* objectB) override;
    bool DoesCollideWith(vec2 point) override;

private:
    void CreateGpuResources();

private:
    GameObject* objectPtr = nullptr;
    double radius = 0.0;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader>  ps;
    Microsoft::WRL::ComPtr<ID3D11InputLayout>  inputLayout;
    Microsoft::WRL::ComPtr<ID3D11Buffer>       vbPos;
    Microsoft::WRL::ComPtr<ID3D11Buffer>       vbCol;
    Microsoft::WRL::ComPtr<ID3D11Buffer>       cbPerDraw;

    UINT vertexCount = 0;
};
