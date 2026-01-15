#include "Engine.h" //GetLogger
#include "TextureManager.h" //TextureManager
#include "Texture.h" //new Texture

Texture* TextureManager::Load(const std::filesystem::path& filePath, bool enableTexel)
{
	if (pathToTexture.count(filePath) == 0)
	{
		pathToTexture.insert(std::make_pair(filePath, new Texture{ filePath, enableTexel }));
	}
	return pathToTexture[filePath];
}

void TextureManager::Unload()
{
	Engine::GetLogger().LogEvent("Clear Textures");
	std::map<std::filesystem::path, Texture*>::iterator iter = pathToTexture.begin();
	for (; iter != pathToTexture.end(); iter++)
	{
		delete iter->second;
		iter->second = nullptr;
	}
	pathToTexture.clear();
}