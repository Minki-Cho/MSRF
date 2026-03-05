#include "TextureManager.h"

#include "Engine.h"
#include "TextureDX11.h"

void TextureDX11Deleter::operator()(TextureDX11* ptr) const
{
    delete ptr;
}

TextureManager::~TextureManager() = default;

TextureDX11* TextureManager::Load(ID3D11Device* device, ID3D11DeviceContext* ctx,
    const std::filesystem::path& filePath, bool enableTexel)
{
    Key key{ filePath, enableTexel };

    auto it = pathToTexture.find(key);
    if (it == pathToTexture.end())
    {
        TexturePtr tex(new TextureDX11(device, ctx, filePath, enableTexel));
        TextureDX11* raw = tex.get();
        pathToTexture.emplace(std::move(key), std::move(tex));
        return raw;
    }
    return it->second.get();
}

TextureDX11* TextureManager::Load(const std::filesystem::path& filePath, bool enableTexel)
{
    return Load(Engine::GetDXDevice(), Engine::GetDXContext(), filePath, enableTexel);
}

void TextureManager::Unload()
{
    Engine::GetLogger().LogEvent("Clear Textures");
    pathToTexture.clear();
}
