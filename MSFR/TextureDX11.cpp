#include "TextureDX11.h"

#include <d3dcompiler.h>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <filesystem>
#pragma comment(lib, "d3dcompiler.lib")

#define NOMINMAX
#include <Windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#pragma comment(lib, "windowscodecs.lib")

#include "Engine.h"

namespace
{
    using Microsoft::WRL::ComPtr;

    template <typename T>
    void ThrowIfFailed(HRESULT hr, const T& msg)
    {
        if (FAILED(hr)) throw std::runtime_error(msg);
    }

    // - WIC helpers
    struct WICImageRGBA
    {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<uint8_t> rgba; // width * height * 4
    };

    void EnsureCOM()
    {
        static bool once = false;
        if (once) return;

        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
        {
            ThrowIfFailed(hr, "CoInitializeEx failed.");
        }
        once = true;
    }

    WICImageRGBA LoadImageRGBA_WIC(const std::wstring& filename)
    {
        EnsureCOM();

        ComPtr<IWICImagingFactory> factory;
        ThrowIfFailed(CoCreateInstance(
            CLSID_WICImagingFactory,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(factory.GetAddressOf())),
            "CoCreateInstance(IWICImagingFactory) failed.");

        ComPtr<IWICBitmapDecoder> decoder;
        ThrowIfFailed(factory->CreateDecoderFromFilename(
            filename.c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnDemand,
            decoder.GetAddressOf()),
            "CreateDecoderFromFilename failed.");

        ComPtr<IWICBitmapFrameDecode> frame;
        ThrowIfFailed(decoder->GetFrame(0, frame.GetAddressOf()),
            "decoder->GetFrame(0) failed.");

        UINT w = 0, h = 0;
        ThrowIfFailed(frame->GetSize(&w, &h), "frame->GetSize failed.");

        ComPtr<IWICFormatConverter> converter;
        ThrowIfFailed(factory->CreateFormatConverter(converter.GetAddressOf()),
            "CreateFormatConverter failed.");

        ThrowIfFailed(converter->Initialize(
            frame.Get(),
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0,
            WICBitmapPaletteTypeCustom),
            "converter->Initialize(32bppRGBA) failed.");

        WICImageRGBA img;
        img.width = static_cast<uint32_t>(w);
        img.height = static_cast<uint32_t>(h);

        const uint32_t rowPitch = img.width * 4;
        const uint32_t imageSize = rowPitch * img.height;

        img.rgba.resize(imageSize);

        ThrowIfFailed(converter->CopyPixels(
            nullptr,
            rowPitch,
            imageSize,
            img.rgba.data()),
            "converter->CopyPixels failed.");

        return img;
    }

    struct VertexPT
    {
        float px, py;
        float u, v;
    };

    struct CBPerDraw
    {
        float m[16];
        float texelPos[2];
        float frameSize[2];
    };

    CBPerDraw MakeCB(const mat3<float>& modelToNdc, vec2 texelPosN, vec2 frameSizeN)
    {
        CBPerDraw cb{};
        cb.m[0] = modelToNdc.elements[0][0]; cb.m[1] = modelToNdc.elements[0][1]; cb.m[2] = 0.f; cb.m[3] = modelToNdc.elements[0][2];
        cb.m[4] = modelToNdc.elements[1][0]; cb.m[5] = modelToNdc.elements[1][1]; cb.m[6] = 0.f; cb.m[7] = modelToNdc.elements[1][2];
        cb.m[8] = 0.f;                      cb.m[9] = 0.f;                      cb.m[10] = 1.f; cb.m[11] = 0.f;
        cb.m[12] = modelToNdc.elements[2][0]; cb.m[13] = modelToNdc.elements[2][1]; cb.m[14] = 0.f; cb.m[15] = modelToNdc.elements[2][2];

        cb.texelPos[0] = texelPosN.x;
        cb.texelPos[1] = texelPosN.y;
        cb.frameSize[0] = frameSizeN.x;
        cb.frameSize[1] = frameSizeN.y;
        return cb;
    }

    void UpdateDynamicCB(ID3D11DeviceContext* ctx, ID3D11Buffer* cb, const void* data, UINT bytes)
    {
        D3D11_MAPPED_SUBRESOURCE ms{};
        ThrowIfFailed(ctx->Map(cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms), "Map(CB) failed.");
        std::memcpy(ms.pData, data, bytes);
        ctx->Unmap(cb, 0);
    }

    ComPtr<ID3DBlob> CompileFromFile(const wchar_t* path, const char* entry, const char* target)
    {
        //std::wcout << L"CWD: " << std::filesystem::current_path().c_str() << L"\n";
        //std::wcout << L"Shader path: " << path << L"\n";
        //std::wcout << L"Exists? " << (std::filesystem::exists(path) ? L"YES" : L"NO") << L"\n";
        UINT flags = 0;
#if defined(_DEBUG)
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        ComPtr<ID3DBlob> blob;
        ComPtr<ID3DBlob> err;

        HRESULT hr = D3DCompileFromFile(
            path, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entry, target, flags, 0,
            blob.GetAddressOf(), err.GetAddressOf());

        if (FAILED(hr))
        {
            std::string msg = "D3DCompileFromFile failed: ";
            if (err) msg += (const char*)err->GetBufferPointer();
            throw std::runtime_error(msg);
        }
        return blob;
    }
    static mat3<float> BuildSpriteModelToNDC(
        float x, float y, float w, float h,
        float screenW, float screenH)
    {

        mat3<float> m; // identity

        m.column0 = { (2.0f * w) / screenW, 0.0f, 0.0f };
        m.column1 = { 0.0f, (2.0f * h) / screenH, 0.0f };

        m.column2 = {
            (-1.0f + (2.0f * x) / screenW),
            (-1.0f + (2.0f * y) / screenH),
            1.0f
        };
        return m;
    }
}

TextureDX11::TextureDX11(const std::filesystem::path& filePath, bool enableTexel_)
    : enableTexel(enableTexel_)
{
    Load(Engine::GetDXDevice(), Engine::GetDXContext(), filePath);
}

TextureDX11::TextureDX11(ID3D11Device* device, ID3D11DeviceContext* ctx,
    const std::filesystem::path& filePath, bool enableTexel_)
    : enableTexel(enableTexel_)
{
    Load(device, ctx, filePath);
}

void TextureDX11::Load(ID3D11Device* device, ID3D11DeviceContext* ctx,
    const std::filesystem::path& filePath)
{
    if (!device || !ctx)
        throw std::runtime_error("TextureDX11::Load: device/context is null.");

    // 1) CPU load image via WIC
    WICImageRGBA img = LoadImageRGBA_WIC(filePath.wstring());
    width = img.width;
    height = img.height;

    // 2) Create GPU texture
    D3D11_TEXTURE2D_DESC td{};
    td.Width = width;
    td.Height = height;
    td.MipLevels = 1;
    td.ArraySize = 1;
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_IMMUTABLE;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = img.rgba.data();
    init.SysMemPitch = width * 4;

    ThrowIfFailed(device->CreateTexture2D(&td, &init, texture2D.GetAddressOf()),
        "CreateTexture2D failed.");

    // 3) SRV
    ThrowIfFailed(device->CreateShaderResourceView(texture2D.Get(), nullptr, srv.GetAddressOf()),
        "CreateShaderResourceView failed.");

    // 4) Shared pipeline resources
    if (!vertexBuffer || !indexBuffer) CreateQuad(device);
    if (!sampler || !blendState || !rasterState) CreateStates(device);
    if (!vs || !psTexture || !psTexel || !inputLayout) CreateShaders(device);

    if (!constantBuffer)
    {
        D3D11_BUFFER_DESC cbd{};
        cbd.ByteWidth = sizeof(CBPerDraw);
        cbd.Usage = D3D11_USAGE_DYNAMIC;
        cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        ThrowIfFailed(device->CreateBuffer(&cbd, nullptr, constantBuffer.GetAddressOf()),
            "CreateBuffer(ConstantBuffer) failed.");
    }
}

vec2 TextureDX11::GetSize() const
{
    return { (float)width, (float)height };
}

void TextureDX11::CreateQuad(ID3D11Device* device)
{
    const VertexPT verts[4] =
    {
        { 0.f, 0.f, 0.f, 1.f },
        { 1.f, 0.f, 1.f, 1.f },
        { 1.f, 1.f, 1.f, 0.f },
        { 0.f, 1.f, 0.f, 0.f },
    };

    const uint16_t indices[6] = { 0, 1, 2, 0, 2, 3 };

    D3D11_BUFFER_DESC vbd{};
    vbd.ByteWidth = sizeof(verts);
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    D3D11_SUBRESOURCE_DATA vinit{};
    vinit.pSysMem = verts;

    ThrowIfFailed(device->CreateBuffer(&vbd, &vinit, vertexBuffer.GetAddressOf()),
        "CreateBuffer(VertexBuffer) failed.");

    D3D11_BUFFER_DESC ibd{};
    ibd.ByteWidth = sizeof(indices);
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    D3D11_SUBRESOURCE_DATA iinit{};
    iinit.pSysMem = indices;

    ThrowIfFailed(device->CreateBuffer(&ibd, &iinit, indexBuffer.GetAddressOf()),
        "CreateBuffer(IndexBuffer) failed.");
}

void TextureDX11::CreateStates(ID3D11Device* device)
{
    D3D11_SAMPLER_DESC samp{};
    samp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp.MaxLOD = D3D11_FLOAT32_MAX;

    ThrowIfFailed(device->CreateSamplerState(&samp, sampler.GetAddressOf()),
        "CreateSamplerState failed.");

    D3D11_BLEND_DESC bd{};
    bd.RenderTarget[0].BlendEnable = TRUE;
    bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    ThrowIfFailed(device->CreateBlendState(&bd, blendState.GetAddressOf()),
        "CreateBlendState failed.");

    D3D11_RASTERIZER_DESC rd{};
    rd.FillMode = D3D11_FILL_SOLID;
    rd.CullMode = D3D11_CULL_NONE;
    rd.ScissorEnable = FALSE;
    rd.DepthClipEnable = TRUE;

    ThrowIfFailed(device->CreateRasterizerState(&rd, rasterState.GetAddressOf()),
        "CreateRasterizerState failed.");
}

void TextureDX11::CreateShaders(ID3D11Device* device)
{
    const auto vsBlob = CompileFromFile(L"assets/shaders/texture_dx11.hlsl", "VSMain", "vs_5_0");
    const auto psTexBlob = CompileFromFile(L"assets/shaders/texture_dx11.hlsl", "PSTexture", "ps_5_0");
    const auto psTexelBlob = CompileFromFile(L"assets/shaders/texture_dx11.hlsl", "PSTexel", "ps_5_0");


    ThrowIfFailed(device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, vs.GetAddressOf()),
        "CreateVertexShader failed.");

    ThrowIfFailed(device->CreatePixelShader(psTexBlob->GetBufferPointer(), psTexBlob->GetBufferSize(), nullptr, psTexture.GetAddressOf()),
        "CreatePixelShader(texture) failed.");

    ThrowIfFailed(device->CreatePixelShader(psTexelBlob->GetBufferPointer(), psTexelBlob->GetBufferSize(), nullptr, psTexel.GetAddressOf()),
        "CreatePixelShader(texel) failed.");

    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ThrowIfFailed(device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), inputLayout.GetAddressOf()),
        "CreateInputLayout failed.");
}

void TextureDX11::Draw(ID3D11DeviceContext* ctx, const mat3<float>& displayMatrix)
{
    if (!ctx) return;

    const float screenW = (float)Engine::GetViewportWidth();
    const float screenH = (float)Engine::GetViewportHeight();

    const float x = displayMatrix.column2.x;
    const float y = displayMatrix.column2.y;
    const float sx = displayMatrix.column0.x;
    const float sy = displayMatrix.column1.y;

    const float w = (float)width * sx;
    const float h = (float)height * sy;

    const mat3<float> model_to_ndc = BuildSpriteModelToNDC(x, y, w, h, screenW, screenH);

    const CBPerDraw cb = MakeCB(model_to_ndc, { 0,0 }, { 1,1 });
    UpdateDynamicCB(ctx, constantBuffer.Get(), &cb, sizeof(cb));

    ctx->IASetInputLayout(inputLayout.Get());
    ctx->VSSetShader(vs.Get(), nullptr, 0);
    ctx->PSSetShader(psTexture.Get(), nullptr, 0);

    ID3D11Buffer* cbs[] = { constantBuffer.Get() };
    ctx->VSSetConstantBuffers(0, 1, cbs);

    ID3D11ShaderResourceView* srvs[] = { srv.Get() };
    ctx->PSSetShaderResources(0, 1, srvs);

    ID3D11SamplerState* samps[] = { sampler.Get() };
    ctx->PSSetSamplers(0, 1, samps);

    float blendFactor[4] = { 0,0,0,0 };
    ctx->OMSetBlendState(blendState.Get(), blendFactor, 0xFFFFFFFF);
    ctx->RSSetState(rasterState.Get());

    UINT stride = sizeof(VertexPT);
    UINT offset = 0;
    ID3D11Buffer* vb = vertexBuffer.Get();
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    ctx->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ctx->DrawIndexed(6, 0, 0);
}

void TextureDX11::Draw(ID3D11DeviceContext* ctx, const mat3<float>& displayMatrix,
    vec2 texelPos, vec2 frameSize)
{
    if (!ctx) return;

    const float screenW = 1280.f;
    const float screenH = 720.f;

    const float x = displayMatrix.column2.x;
    const float y = displayMatrix.column2.y;
    const float sx = displayMatrix.column0.x;
    const float sy = displayMatrix.column1.y;

    const float w = frameSize.x * sx;
    const float h = frameSize.y * sy;

    const mat3<float> model_to_ndc = BuildSpriteModelToNDC(x, y, w, h, screenW, screenH);

    vec2 texelPosN = { texelPos.x / (float)width,  texelPos.y / (float)height };
    vec2 frameSizeN = { frameSize.x / (float)width, frameSize.y / (float)height };

    const CBPerDraw cb = MakeCB(model_to_ndc, texelPosN, frameSizeN);
    UpdateDynamicCB(ctx, constantBuffer.Get(), &cb, sizeof(cb));

    ctx->IASetInputLayout(inputLayout.Get());
    ctx->VSSetShader(vs.Get(), nullptr, 0);
    ctx->PSSetShader(psTexel.Get(), nullptr, 0);

    ID3D11Buffer* cbs[] = { constantBuffer.Get() };
    ctx->VSSetConstantBuffers(0, 1, cbs);
    ctx->PSSetConstantBuffers(0, 1, cbs);

    ID3D11ShaderResourceView* srvs[] = { srv.Get() };
    ctx->PSSetShaderResources(0, 1, srvs);

    ID3D11SamplerState* samps[] = { sampler.Get() };
    ctx->PSSetSamplers(0, 1, samps);

    float blendFactor[4] = { 0,0,0,0 };
    ctx->OMSetBlendState(blendState.Get(), blendFactor, 0xFFFFFFFF);
    ctx->RSSetState(rasterState.Get());

    UINT stride = sizeof(VertexPT);
    UINT offset = 0;
    ID3D11Buffer* vb = vertexBuffer.Get();
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    ctx->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    ctx->DrawIndexed(6, 0, 0);
}

void TextureDX11::Draw(const mat3<float>& displayMatrix)
{

    Draw(Engine::GetDXContext(), displayMatrix);
}

void TextureDX11::Draw(const mat3<float>& displayMatrix, vec2 texelPos, vec2 frameSize)
{
    Draw(Engine::GetDXContext(), displayMatrix, texelPos, frameSize);
}
