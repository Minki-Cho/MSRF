#pragma once
#include <filesystem>
#include <wrl/client.h>
#include <d3d11.h>
#include <DirectXMath.h>

#include "vec2.h"
#include "mat3.h"

class TextureDX11
{
public:
    TextureDX11() = default;
    
    TextureDX11(const std::filesystem::path& filePath, bool enableTexel = false);
    TextureDX11(ID3D11Device* device, ID3D11DeviceContext* ctx,
        const std::filesystem::path& filePath, bool enableTexel);

    void Load(ID3D11Device* device, ID3D11DeviceContext* ctx,
        const std::filesystem::path& filePath);

    void Draw(ID3D11DeviceContext* ctx, const mat3<float>& displayMatrix);
    void Draw(ID3D11DeviceContext* ctx, const mat3<float>& displayMatrix,
        vec2 texelPos, vec2 frameSize);
    void Draw(const mat3<float>& displayMatrix);
    void Draw(const mat3<float>& displayMatrix, vec2 texelPos, vec2 frameSize);
    vec2 GetSize() const;

private:
    void CreateQuad(ID3D11Device* device);
    void CreateStates(ID3D11Device* device);
    void CreateShaders(ID3D11Device* device);

    // ---- GPU resources ----
    Microsoft::WRL::ComPtr<ID3D11Buffer> vertexBuffer;
    Microsoft::WRL::ComPtr<ID3D11Buffer> indexBuffer;
    Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;

    Microsoft::WRL::ComPtr<ID3D11VertexShader> vs;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> psTexture;
    Microsoft::WRL::ComPtr<ID3D11PixelShader> psTexel;

    Microsoft::WRL::ComPtr<ID3D11Buffer> constantBuffer;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2D;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> sampler;

    Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterState;

    // ---- CPU cached info ----
    uint32_t width = 0;
    uint32_t height = 0;
    bool enableTexel = false;
};
