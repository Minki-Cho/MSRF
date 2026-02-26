#include "TextureManager.h"

#include "Engine.h"
#include "TextureDX11.h"

TextureDX11* TextureManager::Load(ID3D11Device* device, ID3D11DeviceContext* ctx,
    const std::filesystem::path& filePath, bool enableTexel)
{
    Key key{ filePath, enableTexel };

    auto it = pathToTexture.find(key);
    if (it == pathToTexture.end())
    {
        auto* tex = new TextureDX11(device, ctx, filePath, enableTexel);
        pathToTexture.emplace(key, tex);
        return tex;
    }
    return it->second;
}

TextureDX11* TextureManager::Load(const std::filesystem::path& filePath, bool enableTexel)
{
    return Load(Engine::GetDXDevice(), Engine::GetDXContext(), filePath, enableTexel);
}

void TextureManager::Unload()
{
    Engine::GetLogger().LogEvent("Clear Textures");

    for (auto& [key, tex] : pathToTexture)
    {
        delete tex;
        tex = nullptr;
    }
    pathToTexture.clear();
}
