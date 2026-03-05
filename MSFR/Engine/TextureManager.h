#pragma once
#include <filesystem>
#include <map>
#include <memory>
#include <utility>

class TextureDX11;
struct ID3D11Device;
struct ID3D11DeviceContext;

struct TextureDX11Deleter
{
    void operator()(TextureDX11* ptr) const;
};

class TextureManager
{
public:
    ~TextureManager();

    TextureDX11* Load(ID3D11Device* device, ID3D11DeviceContext* ctx,
        const std::filesystem::path& filePath, bool enableTexel);
    TextureDX11* Load(const std::filesystem::path& filePath, bool enableTexel);

    void Unload();

private:
    using Key = std::pair<std::filesystem::path, bool>;
    using TexturePtr = std::unique_ptr<TextureDX11, TextureDX11Deleter>;
    std::map<Key, TexturePtr> pathToTexture;
};
