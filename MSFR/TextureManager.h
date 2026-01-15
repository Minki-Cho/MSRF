#pragma once
#include <filesystem> //path
#include <map> //pathToTexture

class Texture;

class TextureManager
{
public:
    Texture* Load(const std::filesystem::path& filePath, bool enableTexel);
    void Unload();

private:
    std::map<std::filesystem::path, Texture*> pathToTexture;
};