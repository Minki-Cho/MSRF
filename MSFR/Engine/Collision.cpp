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
#include <algorithm>
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

    PerDrawCB Mat3ToFloat4x4(const mat3<float>& a)
    {
        PerDrawCB o{};

        const float a00 = a.e[0][0], a01 = a.e[1][0], a02 = a.e[2][0];
        const float a10 = a.e[0][1], a11 = a.e[1][1], a12 = a.e[2][1];
        const float a20 = a.e[0][2], a21 = a.e[1][2], a22 = a.e[2][2];

        // Matches the row_major float4x4 packing used by TextureDX11.
        o.m[0] = a00; o.m[1] = a01; o.m[2] = 0.f; o.m[3] = a02;
        o.m[4] = a10; o.m[5] = a11; o.m[6] = 0.f; o.m[7] = a12;
        o.m[8] = 0.f; o.m[9] = 0.f; o.m[10] = 1.f; o.m[11] = 0.f;
        o.m[8] = 0.f; o.m[9] = 0.f; o.m[10] = 1.f; o.m[11] = 0.f;
        o.m[12] = a20; o.m[13] = a21; o.m[14] = 0.f; o.m[15] = a22;

        return o;
    }
    static std::string HrToString(HRESULT hr)
    {
        char* buf = nullptr;
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR)&buf, 0, nullptr);

        std::string s = buf ? buf : "";
        if (buf) LocalFree(buf);
        return s;
    }

    void CompileFromFile(const wchar_t* path, const char* entry, const char* target, ComPtr<ID3DBlob>& outBlob)
    {
        UINT flags = 0;
#if defined(_DEBUG)
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        ComPtr<ID3DBlob> err;

        HRESULT hr = D3DCompileFromFile(
            path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entry, target, flags, 0,
            outBlob.GetAddressOf(), err.GetAddressOf());

        if (FAILED(hr))
        {
            std::string msg = "D3DCompileFromFile failed: ";
            msg += std::string("path=") + std::string(std::filesystem::path(path).string()) + "\n";
            msg += "hr=" + std::to_string((unsigned)hr) + " (" + HrToString(hr) + ")\n";

            if (err) msg += (const char*)err->GetBufferPointer();
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

    bool CircleIntersectsRect(const vec2& center, float radius, const rect3& rect)
    {
        const float nearestX = (std::max)(rect.Left(), (std::min)(center.x(), rect.Right()));
        const float nearestY = (std::max)(rect.Bottom(), (std::min)(center.y(), rect.Top()));

        const float dx = center.x() - nearestX;
        const float dy = center.y() - nearestY;
        return (dx * dx + dy * dy) <= (radius * radius);
    }

    constexpr const wchar_t* kDebug2DShaderPath = L"assets/shaders/debug2d.hlsl";
}


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

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,     0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT,  1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ThrowIfFailed(dev->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), inputLayout.GetAddressOf()),
        "CreateInputLayout failed.");

    // geometry: line strip box
    constexpr std::array<vec2, 4> positions = { vec2{0.f,0.f}, vec2{1.f,0.f}, vec2{1.f,1.f}, vec2{0.f,1.f} };
    constexpr std::array<color3, 4> colors = { color3{1,0,0}, color3{1,0,0}, color3{1,0,0}, color3{1,0,0} };
    constexpr std::array<uint16_t, 5> indices = { 0, 1, 2, 3, 0 };

    vbPos = CreateImmutableVB(dev, positions.data(), (UINT)(sizeof(vec2) * positions.size()));
    vbCol = CreateImmutableVB(dev, colors.data(), (UINT)(sizeof(color3) * colors.size()));
    ib = CreateImmutableIB(dev, indices.data(), (UINT)(sizeof(uint16_t) * indices.size()));
    indexCount = (UINT)indices.size();

    cbPerDraw = CreateDynamicCB(dev, (UINT)sizeof(PerDrawCB));
}

void RectCollision::Draw(mat3<float> cameraMatrix)
{
    ID3D11DeviceContext* ctx = DX11Services::Context();
    if (!ctx) return;


    const vec3 rectSize = rect.Size();
    const mat3<float> localRect =
        mat3<float>::build_translation(rect.Left(), rect.Bottom()) *
        mat3<float>::build_scale(rectSize.x(), rectSize.y());

   
    const mat3<float> model_to_world = cameraMatrix * localRect;
    const mat3<float> extent = mat3<float>::build_scale(
        2.f / static_cast<float>(Engine::GetViewportWidth()),
        2.f / static_cast<float>(Engine::GetViewportHeight()));
    const mat3<float> to_bottom_left = mat3<float>::build_translation(
        -static_cast<float>(Engine::GetViewportWidth()) / 2.f,
        -static_cast<float>(Engine::GetViewportHeight()) / 2.f);

    const mat3<float> model_to_ndc = extent * to_bottom_left * model_to_world;
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

    if (objectB->GetGOComponent<Collision>() != nullptr &&
        objectB->GetGOComponent<Collision>()->GetCollideType() == CollideType::Circle_Collide)
    {
        auto* circle = objectB->GetGOComponent<CircleCollision>();
        if (!circle)
            return false;

        return CircleIntersectsRect(objectB->GetPosition(), static_cast<float>(circle->GetRadius()), GetWorldCoorRect());
    }

    return false;
}

bool RectCollision::DoesCollideWith(vec2 point)
{
    rect3 a = GetWorldCoorRect();

    return (point.x() >= a.Left() && point.x() <= a.Right() &&
        point.y() >= a.Bottom() && point.y() <= a.Top());
}

// CircleCollision

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

    const mat3<float> model_to_world = cameraMatrix *
        mat3<float>::build_scale((float)(radius * 2.0));

    const mat3<float> extent = mat3<float>::build_scale(
        2.f / static_cast<float>(Engine::GetViewportWidth()),
        2.f / static_cast<float>(Engine::GetViewportHeight()));
    const mat3<float> to_bottom_left = mat3<float>::build_translation(
        -static_cast<float>(Engine::GetViewportWidth()) / 2.f,
        -static_cast<float>(Engine::GetViewportHeight()) / 2.f);

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
    return (mat3<float>::build_scale(objectPtr->GetScale()) * vec3 { (float)radius, 0, 1.0f }).x();
}

bool CircleCollision::DoesCollideWith(GameObject* objectB)
{
    if (objectB->GetGOComponent<Collision>() != nullptr &&
        objectB->GetGOComponent<Collision>()->GetCollideType() == CollideType::Circle_Collide)
    {
        double d_x = (objectPtr->GetPosition().x() - objectB->GetPosition().x());
        double d_y = (objectPtr->GetPosition().y() - objectB->GetPosition().y());
        double distance = (d_x * d_x) + (d_y * d_y);

        double d_r = GetRadius() + objectB->GetGOComponent<CircleCollision>()->GetRadius();
        if (distance < d_r * d_r)
        {
            return true;
        }
    }

    if (objectB->GetGOComponent<Collision>() != nullptr &&
        objectB->GetGOComponent<Collision>()->GetCollideType() == CollideType::Rect_Collide)
    {
        auto* rect = objectB->GetGOComponent<RectCollision>();
        if (!rect)
            return false;

        return CircleIntersectsRect(objectPtr->GetPosition(), static_cast<float>(GetRadius()), rect->GetWorldCoorRect());
    }

    return false;
}

bool CircleCollision::DoesCollideWith(vec2 point)
{
    double d_x = (objectPtr->GetPosition().x() - point.x());
    double d_y = (objectPtr->GetPosition().y() - point.y());
    double distance = (d_x * d_x) + (d_y * d_y);

    if (distance <= GetRadius() * GetRadius())
    {
        return true;
    }
    return false;
}


