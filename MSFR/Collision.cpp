#include "Collision.h"

#include "GameObject.h"
#include "Engine.h"
#include "DX11Services.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <array>
#include <vector>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <string>

#pragma comment(lib, "d3dcompiler.lib")

namespace
{
    using Microsoft::WRL::ComPtr;

    template <typename T>
    void ThrowIfFailed(HRESULT hr, const T& msg)
    {
        if (FAILED(hr))
        {
            throw std::runtime_error(msg);
        }
    }

    struct PerDrawCB
    {
        float m[16]; // float4x4
    };

    // NOTE:
    // - HLSL: o.pos = mul(uModelToNDC, float4(pos,0,1));
    // - HLSL matrix default is column-major.
    // 아래는 너 mat3가 내부적으로 어떻게 저장되는지에 따라 transpose가 필요할 수 있음.
    // 일단 "너가 OpenGL에서 uniform으로 보내던 관례"에 맞춰서 채움.
    PerDrawCB Mat3ToFloat4x4(const mat3<float>& a)
    {
        PerDrawCB o{};

        // 2D affine mat3 -> 4x4 확장
        // [ a00 a01 a02 ]
        // [ a10 a11 a12 ]
        // [ a20 a21 a22 ]
        //
        // float4x4:
        // x,y축 + translation을 포함하는 형태로 매핑
        //
        // 필요시 아래를 transpose해서 넣어야 할 수도 있음.
        o.m[0] = a.elements[0][0]; o.m[1] = a.elements[0][1]; o.m[2] = 0.f; o.m[3] = a.elements[0][2];
        o.m[4] = a.elements[1][0]; o.m[5] = a.elements[1][1]; o.m[6] = 0.f; o.m[7] = a.elements[1][2];
        o.m[8] = 0.f;              o.m[9] = 0.f;              o.m[10] = 1.f; o.m[11] = 0.f;
        o.m[12] = a.elements[2][0]; o.m[13] = a.elements[2][1]; o.m[14] = 0.f; o.m[15] = a.elements[2][2];

        return o;
    }

    void CompileFromFile(const wchar_t* path, const char* entry, const char* target, ComPtr<ID3DBlob>& outBlob)
    {
        UINT flags = 0;
#if defined(_DEBUG)
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        ComPtr<ID3DBlob> err;

        HRESULT hr = D3DCompileFromFile(
            path,
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entry,
            target,
            flags,
            0,
            outBlob.GetAddressOf(),
            err.GetAddressOf());

        if (FAILED(hr))
        {
            std::string msg = "D3DCompileFromFile failed: ";
            if (err)
            {
                msg += (const char*)err->GetBufferPointer();
            }
            throw std::runtime_error(msg);
        }
    }

    ComPtr<ID3D11Buffer> CreateImmutableVB(ID3D11Device* dev, const void* data, UINT bytes)
    {
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = bytes;
        bd.Usage = D3D11_USAGE_IMMUTABLE;
        bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = data;

        ComPtr<ID3D11Buffer> b;
        ThrowIfFailed(dev->CreateBuffer(&bd, &init, b.GetAddressOf()), "CreateBuffer(VB) failed.");
        return b;
    }

    ComPtr<ID3D11Buffer> CreateImmutableIB(ID3D11Device* dev, const void* data, UINT bytes)
    {
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = bytes;
        bd.Usage = D3D11_USAGE_IMMUTABLE;
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = data;

        ComPtr<ID3D11Buffer> b;
        ThrowIfFailed(dev->CreateBuffer(&bd, &init, b.GetAddressOf()), "CreateBuffer(IB) failed.");
        return b;
    }

    ComPtr<ID3D11Buffer> CreateDynamicCB(ID3D11Device* dev, UINT bytes)
    {
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = bytes;
        bd.Usage = D3D11_USAGE_DYNAMIC;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        ComPtr<ID3D11Buffer> b;
        ThrowIfFailed(dev->CreateBuffer(&bd, nullptr, b.GetAddressOf()), "CreateBuffer(CB) failed.");
        return b;
    }

    void UpdateDynamicCB(ID3D11DeviceContext* ctx, ID3D11Buffer* cb, const void* data, UINT bytes)
    {
        D3D11_MAPPED_SUBRESOURCE ms{};
        ThrowIfFailed(ctx->Map(cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms), "Map(CB) failed.");
        std::memcpy(ms.pData, data, bytes);
        ctx->Unmap(cb, 0);
    }

    // common debug shader path
    constexpr const wchar_t* kDebug2DShaderPath = L"assets/shaders/debug2d.hlsl";
}

// ======================= RectCollision =======================

RectCollision::RectCollision(rect3 r, GameObject* obj)
    : objectPtr(obj), rect(r)
{
    CreateGpuResources();
}

void RectCollision::CreateGpuResources()
{
    ID3D11Device* dev = DX11Services::Device();
    ID3D11DeviceContext* ctx = DX11Services::Context();
    if (!dev || !ctx)
        throw std::runtime_error("DX11Services not initialized. Call DX11Services::Init(...) before creating collisions.");

    // shaders
    ComPtr<ID3DBlob> vsBlob, psBlob;
    CompileFromFile(kDebug2DShaderPath, "VSMain", "vs_5_0", vsBlob);
    CompileFromFile(kDebug2DShaderPath, "PSMain", "ps_5_0", psBlob);

    ThrowIfFailed(dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vs.GetAddressOf()),
        "CreateVertexShader failed.");
    ThrowIfFailed(dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, ps.GetAddressOf()),
        "CreatePixelShader failed.");

    // Input layout (two streams: slot0=pos, slot1=color)
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,     0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT,  1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ThrowIfFailed(dev->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), inputLayout.GetAddressOf()),
        "CreateInputLayout failed.");

    // geometry: line strip box
    constexpr std::array<vec2, 4> positions = { vec2{0.f,0.f}, vec2{1.f,0.f}, vec2{1.f,1.f}, vec2{0.f,1.f} };
    constexpr std::array<color3, 4> colors = { color3{0,0,0}, color3{0,0,0}, color3{0,0,0}, color3{0,0,0} };
    constexpr std::array<uint16_t, 5> indices = { 0, 1, 2, 3, 0 };

    vbPos = CreateImmutableVB(dev, positions.data(), (UINT)(sizeof(vec2) * positions.size()));
    vbCol = CreateImmutableVB(dev, colors.data(), (UINT)(sizeof(color3) * colors.size()));
    ib = CreateImmutableIB(dev, indices.data(), (UINT)(sizeof(uint16_t) * indices.size()));
    indexCount = (UINT)indices.size();

    cbPerDraw = CreateDynamicCB(dev, (UINT)sizeof(PerDrawCB));
}

void RectCollision::Draw(mat3<float>)
{
    ID3D11DeviceContext* ctx = DX11Services::Context();
    if (!ctx) return;

    // ==== 너 기존 model_to_ndc 계산 로직 최대 유지 ====
    mat3<float> translation = mat3<float>::build_translation(
        GetWorldCoorRect().Left() - (1280.f - Engine::GetWindow().GetClientWidth()) / 2.f,
        GetWorldCoorRect().Bottom() - (720.f - Engine::GetWindow().GetClientHeight()) / 2.f);

    mat3<float> scale = mat3<float>::build_scale(GetWorldCoorRect().Size().x, GetWorldCoorRect().Size().y);
    mat3<float> to_bottom_left = mat3<float>::build_translation(-Engine::GetWindow().GetClientWidth() / 2.f,
        -Engine::GetWindow().GetClientHeight() / 2.f);

    const mat3<float> model_to_world = to_bottom_left * translation * scale;
    const mat3<float> extent = mat3<float>::build_scale(2.f / 1280.f, 2.f / 720.f);
    const mat3<float> model_to_ndc = extent * model_to_world;

    // update cbuffer
    const PerDrawCB cb = Mat3ToFloat4x4(model_to_ndc);
    UpdateDynamicCB(ctx, cbPerDraw.Get(), &cb, (UINT)sizeof(cb));

    // bind pipeline
    ctx->IASetInputLayout(inputLayout.Get());
    ctx->VSSetShader(vs.Get(), nullptr, 0);
    ctx->PSSetShader(ps.Get(), nullptr, 0);

    ID3D11Buffer* cbuffers[] = { cbPerDraw.Get() };
    ctx->VSSetConstantBuffers(0, 1, cbuffers);

    ID3D11Buffer* vbs[] = { vbPos.Get(), vbCol.Get() };
    UINT strides[] = { (UINT)sizeof(vec2), (UINT)sizeof(color3) };
    UINT offsets[] = { 0, 0 };
    ctx->IASetVertexBuffers(0, 2, vbs, strides, offsets);

    ctx->IASetIndexBuffer(ib.Get(), DXGI_FORMAT_R16_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);

    ctx->DrawIndexed(indexCount, 0, 0);
}

rect3 RectCollision::GetWorldCoorRect()
{
    return { objectPtr->GetMatrix() * rect.point1,
             objectPtr->GetMatrix() * rect.point2 };
}

bool RectCollision::DoesCollideWith(GameObject* objectB)
{
    if (objectB->GetGOComponent<Collision>() != nullptr &&
        objectB->GetGOComponent<Collision>()->GetCollideType() == CollideType::Rect_Collide)
    {
        rect3 b = objectB->GetGOComponent<RectCollision>()->GetWorldCoorRect();
        rect3 a = GetWorldCoorRect();

        if (a.Right() > b.Left() && a.Left() < b.Right())
        {
            if (a.Top() > b.Bottom() && a.Bottom() < b.Top())
            {
                return true;
            }
        }
    }
    return false;
}

bool RectCollision::DoesCollideWith(vec2 point)
{
    rect3 a = GetWorldCoorRect();

    return (point.x >= a.Left() && point.x <= a.Right() &&
        point.y >= a.Bottom() && point.y <= a.Top());
}

// ======================= CircleCollision =======================

CircleCollision::CircleCollision(double r, GameObject* obj)
    : objectPtr(obj), radius(r)
{
    CreateGpuResources();
}

void CircleCollision::CreateGpuResources()
{
    ID3D11Device* dev = DX11Services::Device();
    ID3D11DeviceContext* ctx = DX11Services::Context();
    if (!dev || !ctx)
        throw std::runtime_error("DX11Services not initialized. Call DX11Services::Init(...) before creating collisions.");

    // shaders
    ComPtr<ID3DBlob> vsBlob, psBlob;
    CompileFromFile(kDebug2DShaderPath, "VSMain", "vs_5_0", vsBlob);
    CompileFromFile(kDebug2DShaderPath, "PSMain", "ps_5_0", psBlob);

    ThrowIfFailed(dev->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vs.GetAddressOf()),
        "CreateVertexShader failed.");
    ThrowIfFailed(dev->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, ps.GetAddressOf()),
        "CreatePixelShader failed.");

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,     0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT,  1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ThrowIfFailed(dev->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), inputLayout.GetAddressOf()),
        "CreateInputLayout failed.");

    // circle vertices (line strip)
    constexpr int slices = 30;
    std::vector<vec2> pos(slices + 1);
    for (int i = 0; i <= slices; ++i)
    {
        const float t = (float)i * (2.0f * 3.14159265358979323846f / (float)slices);
        pos[i] = vec2{ std::cos(t), std::sin(t) };
    }

    std::vector<color3> col(slices + 1, color3{ 1, 0, 0 });

    vbPos = CreateImmutableVB(dev, pos.data(), (UINT)(sizeof(vec2) * pos.size()));
    vbCol = CreateImmutableVB(dev, col.data(), (UINT)(sizeof(color3) * col.size()));
    vertexCount = (UINT)pos.size();

    cbPerDraw = CreateDynamicCB(dev, (UINT)sizeof(PerDrawCB));
}

void CircleCollision::Draw(mat3<float> cameraMatrix)
{
    ID3D11DeviceContext* ctx = DX11Services::Context();
    if (!ctx) return;

    // ==== 너 기존 계산 로직 최대 유지 ====
    mat3<float> scale = mat3<float>::build_scale((float)(radius * 2.0));
    mat3<float> translation = mat3<float>::build_translation(cameraMatrix.column2.x, cameraMatrix.column2.y);
    const mat3<float> model_to_world = translation * scale;

    mat3<float> extent = mat3<float>::build_scale(1.f / Engine::GetWindow().GetClientWidth(),
        1.f / Engine::GetWindow().GetClientHeight());
    mat3<float> to_bottom_left = mat3<float>::build_translation(-Engine::GetWindow().GetClientWidth() / 2.f,
        -Engine::GetWindow().GetClientHeight() / 2.f);

    const mat3<float> model_to_ndc = extent * to_bottom_left * model_to_world;

    const PerDrawCB cb = Mat3ToFloat4x4(model_to_ndc);
    UpdateDynamicCB(ctx, cbPerDraw.Get(), &cb, (UINT)sizeof(cb));

    // bind pipeline
    ctx->IASetInputLayout(inputLayout.Get());
    ctx->VSSetShader(vs.Get(), nullptr, 0);
    ctx->PSSetShader(ps.Get(), nullptr, 0);

    ID3D11Buffer* cbuffers[] = { cbPerDraw.Get() };
    ctx->VSSetConstantBuffers(0, 1, cbuffers);

    ID3D11Buffer* vbs[] = { vbPos.Get(), vbCol.Get() };
    UINT strides[] = { (UINT)sizeof(vec2), (UINT)sizeof(color3) };
    UINT offsets[] = { 0, 0 };
    ctx->IASetVertexBuffers(0, 2, vbs, strides, offsets);

    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
    ctx->Draw(vertexCount, 0);
}

double CircleCollision::GetRadius()
{
    return (mat3<float>::build_scale(objectPtr->GetScale()) * vec3 { (float)radius, 0, 1.0f }).x;
}

bool CircleCollision::DoesCollideWith(GameObject* objectB)
{
    if (objectB->GetGOComponent<Collision>() != nullptr &&
        objectB->GetGOComponent<Collision>()->GetCollideType() == CollideType::Circle_Collide)
    {
        double d_x = (objectPtr->GetPosition().x - objectB->GetPosition().x);
        double d_y = (objectPtr->GetPosition().y - objectB->GetPosition().y);
        double distance = (d_x * d_x) + (d_y * d_y);

        double d_r = GetRadius() + objectB->GetGOComponent<CircleCollision>()->GetRadius();
        if (distance < d_r * d_r)
        {
            return true;
        }
    }
    return false;
}

bool CircleCollision::DoesCollideWith(vec2 point)
{
    double d_x = (objectPtr->GetPosition().x - point.x);
    double d_y = (objectPtr->GetPosition().y - point.y);
    double distance = (d_x * d_x) + (d_y * d_y);

    if (distance <= GetRadius() * GetRadius())
    {
        return true;
    }
    return false;
}
