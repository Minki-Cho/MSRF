#pragma once
#include <filesystem>
#include <map>
#include <utility>

class TextureDX11;
struct ID3D11Device;
struct ID3D11DeviceContext;

class TextureManager
{
public:
    TextureDX11* Load(ID3D11Device* device, ID3D11DeviceContext* ctx,
        const std::filesystem::path& filePath, bool enableTexel);
    TextureDX11* Load(const std::filesystem::path& filePath, bool enableTexel);

    void Unload();
    
private:
    using Key = std::pair<std::filesystem::path, bool>;
    std::map<Key, TextureDX11*> pathToTexture;
};
