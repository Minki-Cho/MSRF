#include "UIFramework.h"

#include "Engine.h"
#include "Input.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#pragma comment(lib, "d3dcompiler.lib")

namespace
{
    using Microsoft::WRL::ComPtr;

    template<typename T>
    void ThrowIfFailed(HRESULT hr, const T& msg)
    {
        if (FAILED(hr))
            throw std::runtime_error(msg);
    }

    struct VertexPC
    {
        float x, y;
        float r, g, b;
    };

    struct PerDrawCB
    {
        float m[16];
    };

    PerDrawCB MakeIdentityCB()
    {
        PerDrawCB cb{};
        cb.m[0] = 1.0f;
        cb.m[5] = 1.0f;
        cb.m[10] = 1.0f;
        cb.m[15] = 1.0f;
        return cb;
    }

    float ToNdcX(float x, float viewportW)
    {
        return (x / viewportW) * 2.0f - 1.0f;
    }

    float ToNdcY(float y, float viewportH)
    {
        return 1.0f - (y / viewportH) * 2.0f;
    }

    ComPtr<ID3DBlob> CompileFromFile(const wchar_t* path, const char* entry, const char* target)
    {
        UINT flags = 0;
#if defined(_DEBUG)
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        ComPtr<ID3DBlob> blob;
        ComPtr<ID3DBlob> err;
        HRESULT hr = D3DCompileFromFile(path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entry, target, flags, 0, blob.GetAddressOf(), err.GetAddressOf());
        if (FAILED(hr))
        {
            std::string msg = "UIFramework shader compile failed: ";
            if (err)
                msg += static_cast<const char*>(err->GetBufferPointer());
            throw std::runtime_error(msg);
        }
        return blob;
    }
}

struct UI::Framework::Impl
{
    static constexpr std::size_t MaxVertices = 1u << 16;

    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
    ComPtr<ID3D11InputLayout> inputLayout;
    ComPtr<ID3D11Buffer> vb;
    ComPtr<ID3D11Buffer> cbPerDraw;
    ComPtr<ID3D11BlendState> blendState;
    ComPtr<ID3D11RasterizerState> rasterState;

    std::vector<VertexPC> vertices;

    float viewportW = 1.0f;
    float viewportH = 1.0f;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    bool mouseDown = false;
    bool mouseReleased = false;
    bool initialized = false;
};

void UI::Framework::EnsureRenderer()
{
    if (!impl)
    {
        impl = new Impl();
        impl->vertices.reserve(Impl::MaxVertices);
    }

    if (impl->initialized)
        return;

    ID3D11Device* device = Engine::GetDXDevice();
    if (!device)
        return;

    const auto vsBlob = CompileFromFile(L"assets/shaders/debug2d.hlsl", "VSMain", "vs_5_0");
    const auto psBlob = CompileFromFile(L"assets/shaders/debug2d.hlsl", "PSMain", "ps_5_0");

    ThrowIfFailed(
        device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, impl->vs.GetAddressOf()),
        "UIFramework CreateVertexShader failed.");
    ThrowIfFailed(
        device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, impl->ps.GetAddressOf()),
        "UIFramework CreatePixelShader failed.");

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,     0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT,  0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ThrowIfFailed(
        device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), impl->inputLayout.GetAddressOf()),
        "UIFramework CreateInputLayout failed.");

    D3D11_BUFFER_DESC vbd{};
    vbd.ByteWidth = static_cast<UINT>(Impl::MaxVertices * sizeof(VertexPC));
    vbd.Usage = D3D11_USAGE_DYNAMIC;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ThrowIfFailed(device->CreateBuffer(&vbd, nullptr, impl->vb.GetAddressOf()), "UIFramework CreateBuffer(VB) failed.");

    D3D11_BUFFER_DESC cbd{};
    cbd.ByteWidth = sizeof(PerDrawCB);
    cbd.Usage = D3D11_USAGE_DYNAMIC;
    cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ThrowIfFailed(device->CreateBuffer(&cbd, nullptr, impl->cbPerDraw.GetAddressOf()), "UIFramework CreateBuffer(CB) failed.");

    D3D11_BLEND_DESC bd{};
    bd.RenderTarget[0].BlendEnable = FALSE;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ThrowIfFailed(device->CreateBlendState(&bd, impl->blendState.GetAddressOf()), "UIFramework CreateBlendState failed.");

    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.DepthClipEnable = TRUE;
    ThrowIfFailed(device->CreateRasterizerState(&rd, impl->rasterState.GetAddressOf()), "UIFramework CreateRasterizerState failed.");

    impl->initialized = true;
}

void UI::Framework::BeginFrame()
{
    EnsureRenderer();
    if (!impl)
        return;

    impl->vertices.clear();
    impl->viewportW = static_cast<float>((std::max)(1, Engine::GetViewportWidth()));
    impl->viewportH = static_cast<float>((std::max)(1, Engine::GetViewportHeight()));

    const vec2 mouse = Engine::GetInput().GetMousePos();
    impl->mouseX = mouse.x();
    impl->mouseY = mouse.y();
    impl->mouseDown = Engine::GetInput().GetMouseDown();
    impl->mouseReleased = Engine::GetInput().GetMouseReleasedThisFrame();
}

void UI::Framework::EndFrame()
{
    if (!impl || !impl->initialized || impl->vertices.empty())
        return;

    ID3D11DeviceContext* ctx = Engine::GetDXContext();
    if (!ctx)
        return;

    D3D11_MAPPED_SUBRESOURCE ms{};
    ThrowIfFailed(ctx->Map(impl->vb.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms), "UIFramework Map(VB) failed.");
    std::memcpy(ms.pData, impl->vertices.data(), impl->vertices.size() * sizeof(VertexPC));
    ctx->Unmap(impl->vb.Get(), 0);

    const PerDrawCB cb = MakeIdentityCB();
    ThrowIfFailed(ctx->Map(impl->cbPerDraw.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &ms), "UIFramework Map(CB) failed.");
    std::memcpy(ms.pData, &cb, sizeof(cb));
    ctx->Unmap(impl->cbPerDraw.Get(), 0);

    ctx->IASetInputLayout(impl->inputLayout.Get());
    ctx->VSSetShader(impl->vs.Get(), nullptr, 0);
    ctx->PSSetShader(impl->ps.Get(), nullptr, 0);

    ID3D11Buffer* cbs[] = { impl->cbPerDraw.Get() };
    ctx->VSSetConstantBuffers(0, 1, cbs);

    UINT stride = sizeof(VertexPC);
    UINT offset = 0;
    ID3D11Buffer* vb = impl->vb.Get();
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    float blendFactor[4] = { 0, 0, 0, 0 };
    ctx->OMSetBlendState(impl->blendState.Get(), blendFactor, 0xFFFFFFFF);
    ctx->RSSetState(impl->rasterState.Get());

    ctx->Draw(static_cast<UINT>(impl->vertices.size()), 0);
}

void UI::Framework::EmitRectPx(float x, float y, float w, float h, const Color& color)
{
    if (!impl || w <= 0.0f || h <= 0.0f)
        return;

    if (impl->vertices.size() + 6 > Impl::MaxVertices)
        return;

    const float x0 = ToNdcX(x, impl->viewportW);
    const float y0 = ToNdcY(y, impl->viewportH);
    const float x1 = ToNdcX(x + w, impl->viewportW);
    const float y1 = ToNdcY(y + h, impl->viewportH);

    const VertexPC v0{ x0, y0, color.r, color.g, color.b };
    const VertexPC v1{ x1, y0, color.r, color.g, color.b };
    const VertexPC v2{ x1, y1, color.r, color.g, color.b };
    const VertexPC v3{ x0, y1, color.r, color.g, color.b };

    impl->vertices.push_back(v0);
    impl->vertices.push_back(v1);
    impl->vertices.push_back(v2);
    impl->vertices.push_back(v0);
    impl->vertices.push_back(v2);
    impl->vertices.push_back(v3);
}

void UI::Framework::FillRect(const Rect& rect, const Color& color)
{
    EmitRectPx(rect.x, rect.y, rect.w, rect.h, color);
}

void UI::Framework::Panel(const Rect& rect, const Color& bg, const Color& border, float borderPx)
{
    EmitRectPx(rect.x, rect.y, rect.w, rect.h, border);
    EmitRectPx(
        rect.x + borderPx,
        rect.y + borderPx,
        (std::max)(0.0f, rect.w - borderPx * 2.0f),
        (std::max)(0.0f, rect.h - borderPx * 2.0f),
        bg);
}

void UI::Framework::ProgressBar(const Rect& rect, float ratio, const Color& fill, const Color& bg, const Color& border)
{
    Panel(rect, bg, border, 2.0f);
    const float clamped = std::clamp(ratio, 0.0f, 1.0f);
    const Rect inner{
        rect.x + 3.0f,
        rect.y + 3.0f,
        (rect.w - 6.0f) * clamped,
        (std::max)(0.0f, rect.h - 6.0f)
    };
    FillRect(inner, fill);
}

char UI::Framework::ToUpperAscii(char c)
{
    if (c >= 'a' && c <= 'z')
        return static_cast<char>('A' + (c - 'a'));
    return c;
}

const unsigned char* UI::Framework::GetGlyphRows(char c)
{
    // 5x7 bitmap font rows, MSB uses left-most pixel in 5-bit row.
    switch (c)
    {
    case 'A': { static const unsigned char g[7] = { 0x0E,0x11,0x11,0x1F,0x11,0x11,0x11 }; return g; }
    case 'B': { static const unsigned char g[7] = { 0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E }; return g; }
    case 'C': { static const unsigned char g[7] = { 0x0E,0x11,0x10,0x10,0x10,0x11,0x0E }; return g; }
    case 'D': { static const unsigned char g[7] = { 0x1E,0x11,0x11,0x11,0x11,0x11,0x1E }; return g; }
    case 'E': { static const unsigned char g[7] = { 0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F }; return g; }
    case 'F': { static const unsigned char g[7] = { 0x1F,0x10,0x10,0x1E,0x10,0x10,0x10 }; return g; }
    case 'G': { static const unsigned char g[7] = { 0x0E,0x11,0x10,0x13,0x11,0x11,0x0F }; return g; }
    case 'H': { static const unsigned char g[7] = { 0x11,0x11,0x11,0x1F,0x11,0x11,0x11 }; return g; }
    case 'I': { static const unsigned char g[7] = { 0x0E,0x04,0x04,0x04,0x04,0x04,0x0E }; return g; }
    case 'J': { static const unsigned char g[7] = { 0x01,0x01,0x01,0x01,0x11,0x11,0x0E }; return g; }
    case 'K': { static const unsigned char g[7] = { 0x11,0x12,0x14,0x18,0x14,0x12,0x11 }; return g; }
    case 'L': { static const unsigned char g[7] = { 0x10,0x10,0x10,0x10,0x10,0x10,0x1F }; return g; }
    case 'M': { static const unsigned char g[7] = { 0x11,0x1B,0x15,0x15,0x11,0x11,0x11 }; return g; }
    case 'N': { static const unsigned char g[7] = { 0x11,0x19,0x15,0x13,0x11,0x11,0x11 }; return g; }
    case 'O': { static const unsigned char g[7] = { 0x0E,0x11,0x11,0x11,0x11,0x11,0x0E }; return g; }
    case 'P': { static const unsigned char g[7] = { 0x1E,0x11,0x11,0x1E,0x10,0x10,0x10 }; return g; }
    case 'Q': { static const unsigned char g[7] = { 0x0E,0x11,0x11,0x11,0x15,0x12,0x0D }; return g; }
    case 'R': { static const unsigned char g[7] = { 0x1E,0x11,0x11,0x1E,0x14,0x12,0x11 }; return g; }
    case 'S': { static const unsigned char g[7] = { 0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E }; return g; }
    case 'T': { static const unsigned char g[7] = { 0x1F,0x04,0x04,0x04,0x04,0x04,0x04 }; return g; }
    case 'U': { static const unsigned char g[7] = { 0x11,0x11,0x11,0x11,0x11,0x11,0x0E }; return g; }
    case 'V': { static const unsigned char g[7] = { 0x11,0x11,0x11,0x11,0x11,0x0A,0x04 }; return g; }
    case 'W': { static const unsigned char g[7] = { 0x11,0x11,0x11,0x15,0x15,0x15,0x0A }; return g; }
    case 'X': { static const unsigned char g[7] = { 0x11,0x11,0x0A,0x04,0x0A,0x11,0x11 }; return g; }
    case 'Y': { static const unsigned char g[7] = { 0x11,0x11,0x0A,0x04,0x04,0x04,0x04 }; return g; }
    case 'Z': { static const unsigned char g[7] = { 0x1F,0x01,0x02,0x04,0x08,0x10,0x1F }; return g; }
    case '0': { static const unsigned char g[7] = { 0x0E,0x11,0x13,0x15,0x19,0x11,0x0E }; return g; }
    case '1': { static const unsigned char g[7] = { 0x04,0x0C,0x04,0x04,0x04,0x04,0x0E }; return g; }
    case '2': { static const unsigned char g[7] = { 0x0E,0x11,0x01,0x02,0x04,0x08,0x1F }; return g; }
    case '3': { static const unsigned char g[7] = { 0x1F,0x02,0x04,0x02,0x01,0x11,0x0E }; return g; }
    case '4': { static const unsigned char g[7] = { 0x02,0x06,0x0A,0x12,0x1F,0x02,0x02 }; return g; }
    case '5': { static const unsigned char g[7] = { 0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E }; return g; }
    case '6': { static const unsigned char g[7] = { 0x06,0x08,0x10,0x1E,0x11,0x11,0x0E }; return g; }
    case '7': { static const unsigned char g[7] = { 0x1F,0x01,0x02,0x04,0x08,0x08,0x08 }; return g; }
    case '8': { static const unsigned char g[7] = { 0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E }; return g; }
    case '9': { static const unsigned char g[7] = { 0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C }; return g; }
    case ':': { static const unsigned char g[7] = { 0x00,0x04,0x00,0x00,0x04,0x00,0x00 }; return g; }
    case '.': { static const unsigned char g[7] = { 0x00,0x00,0x00,0x00,0x00,0x06,0x06 }; return g; }
    case '/': { static const unsigned char g[7] = { 0x01,0x02,0x04,0x08,0x10,0x00,0x00 }; return g; }
    case '-': { static const unsigned char g[7] = { 0x00,0x00,0x00,0x1F,0x00,0x00,0x00 }; return g; }
    case '%': { static const unsigned char g[7] = { 0x19,0x19,0x02,0x04,0x08,0x13,0x13 }; return g; }
    case '?': { static const unsigned char g[7] = { 0x0E,0x11,0x01,0x02,0x04,0x00,0x04 }; return g; }
    case '!': { static const unsigned char g[7] = { 0x04,0x04,0x04,0x04,0x04,0x00,0x04 }; return g; }
    case '(': { static const unsigned char g[7] = { 0x02,0x04,0x08,0x08,0x08,0x04,0x02 }; return g; }
    case ')': { static const unsigned char g[7] = { 0x08,0x04,0x02,0x02,0x02,0x04,0x08 }; return g; }
    case '+': { static const unsigned char g[7] = { 0x00,0x04,0x04,0x1F,0x04,0x04,0x00 }; return g; }
    case ',': { static const unsigned char g[7] = { 0x00,0x00,0x00,0x00,0x06,0x06,0x04 }; return g; }
    case '\'': { static const unsigned char g[7] = { 0x04,0x04,0x00,0x00,0x00,0x00,0x00 }; return g; }
    case ' ': { static const unsigned char g[7] = { 0,0,0,0,0,0,0 }; return g; }
    default:
        break;
    }

    static const unsigned char unknown[7] = { 0x0E,0x11,0x01,0x02,0x00,0x00,0x04 };
    return unknown;
}

void UI::Framework::EmitTextPx(float x, float y, std::string_view text, float pixelSize, const Color& color)
{
    if (!impl || pixelSize <= 0.0f || text.empty())
        return;

    float penX = x;
    float penY = y;
    const float startX = x;
    const float advX = 6.0f * pixelSize;
    const float advY = 9.0f * pixelSize;

    for (char raw : text)
    {
        if (raw == '\n')
        {
            penX = startX;
            penY += advY;
            continue;
        }

        const char c = ToUpperAscii(raw);
        const unsigned char* rows = GetGlyphRows(c);
        for (int row = 0; row < 7; ++row)
        {
            const unsigned char bits = rows[row];
            for (int col = 0; col < 5; ++col)
            {
                const unsigned char mask = static_cast<unsigned char>(1u << (4 - col));
                if ((bits & mask) == 0)
                    continue;

                EmitRectPx(
                    penX + static_cast<float>(col) * pixelSize,
                    penY + static_cast<float>(row) * pixelSize,
                    pixelSize,
                    pixelSize,
                    color);
            }
        }

        penX += advX;
    }
}

float UI::Framework::MeasureTextWidthPx(std::string_view text, float pixelSize) const
{
    if (pixelSize <= 0.0f)
        return 0.0f;

    const float advX = 6.0f * pixelSize;
    float lineWidth = 0.0f;
    float maxWidth = 0.0f;

    for (char c : text)
    {
        if (c == '\n')
        {
            maxWidth = (std::max)(maxWidth, lineWidth);
            lineWidth = 0.0f;
            continue;
        }
        lineWidth += advX;
    }

    maxWidth = (std::max)(maxWidth, lineWidth);
    return maxWidth;
}

void UI::Framework::Label(float x, float y, std::string_view text, float pixelSize, const Color& color)
{
    EmitTextPx(x, y, text, pixelSize, color);
}

void UI::Framework::LabelCentered(const Rect& rect, std::string_view text, float pixelSize, const Color& color)
{
    const float textWidth = MeasureTextWidthPx(text, pixelSize);
    const float textHeight = 7.0f * pixelSize;
    const float tx = rect.x + (rect.w - textWidth) * 0.5f;
    const float ty = rect.y + (rect.h - textHeight) * 0.5f;
    EmitTextPx(tx, ty, text, pixelSize, color);
}

bool UI::Framework::IsMouseReleasedIn(const Rect& rect) const
{
    if (!impl)
        return false;

    return impl->mouseReleased && rect.Contains(impl->mouseX, impl->mouseY);
}

bool UI::Framework::IsMouseHovering(const Rect& rect) const
{
    if (!impl)
        return false;

    return rect.Contains(impl->mouseX, impl->mouseY);
}

bool UI::Framework::Button(const Rect& rect, std::string_view text, bool danger)
{
    if (!impl)
        return false;

    const bool hovered = IsMouseHovering(rect);
    const bool pressed = hovered && impl->mouseDown;

    Color fill = danger ? theme.buttonDanger : theme.buttonIdle;
    if (pressed)
    {
        fill = theme.buttonPressed;
    }
    else if (hovered && !danger)
    {
        fill = theme.buttonHover;
    }

    Panel(rect, fill, theme.panelBorder, 2.0f);
    LabelCentered(rect, text, 2.0f, theme.text);

    return hovered && impl->mouseReleased;
}

UI::Framework& UI::Get()
{
    static Framework framework;
    return framework;
}
