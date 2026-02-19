#include "Sprite.h" //Sprite
#include "Engine.h" //GetLogger
#include "TextureDX11.h" //texturePtr
#include "Rect.h"
#include "Animation.h" //animations
#include "Collision.h" //Collision

#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <string>

static std::string Trim(std::string s)
{
    auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };

    while (!s.empty() && is_space((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && is_space((unsigned char)s.back()))  s.pop_back();
    return s;
}

// Reads the first "texture path" token from the .spt.
// Supports either:
//   1) token style:    assets/images/player.png
//   2) quoted style:   "assets/images/my sheet.png"
static std::string ReadFirstPathToken(std::ifstream& inFile)
{
    // skip leading whitespace/newlines
    inFile >> std::ws;

    // If next char is quote, read until ending quote
    if (inFile.peek() == '"')
    {
        inFile.get(); // consume opening quote
        std::string inside;
        std::getline(inFile, inside, '"'); // read until closing quote
        return Trim(inside);
    }

    // Otherwise read as a normal token
    std::string token;
    inFile >> token;
    return Trim(token);
}

Sprite::Sprite(const std::filesystem::path& spriteInfoFile, GameObject* object)
{
    Load(spriteInfoFile, object);
}

Sprite::~Sprite()
{
    for (Animation* anim : animations)
        delete anim;

    animations.clear();
}

void Sprite::Load(const std::filesystem::path& spriteInfoFile, GameObject* object)
{
    hotSpotList.clear();
    frameTexel.clear();
    animations.clear();

    if (spriteInfoFile.extension() != ".spt")
        throw std::runtime_error("Bad Filetype. " + spriteInfoFile.generic_string() + " not a sprite info file (.spt)");

    std::ifstream inFile(spriteInfoFile);
    if (!inFile.is_open())
        throw std::runtime_error("Failed to load " + spriteInfoFile.generic_string());

    // -----------------------------
    // FIX: resolve texture path based on .spt directory
    // -----------------------------
    std::string texToken = ReadFirstPathToken(inFile);
    if (texToken.empty())
        throw std::runtime_error("Sprite file has empty texture path: " + spriteInfoFile.generic_string());

    std::filesystem::path texPath = texToken;

    // If it's a relative path, interpret it relative to the .spt file location
    if (texPath.is_relative())
        texPath = spriteInfoFile.parent_path() / texPath;

    texPath = texPath.lexically_normal();

    Engine::GetLogger().LogEvent("Sprite SPT: " + spriteInfoFile.generic_string());
    Engine::GetLogger().LogEvent("Texture token: " + texToken);
    Engine::GetLogger().LogEvent("Resolved texture path: " + texPath.generic_string());

    if (!std::filesystem::exists(texPath))
    {
        Engine::GetLogger().LogError("Texture file not found: " + texPath.generic_string());
        throw std::runtime_error("Texture file not found: " + texPath.generic_string());
    }

    // TextureManager::Load가 string을 받는 버전이면 generic_string() 넘기기
    texturePtr = Engine::GetTextureManager().Load(
        Engine::GetDXDevice(),
        Engine::GetDXContext(),
        texPath.generic_string(),
        true);

    if (!texturePtr)
        throw std::runtime_error("Texture load failed (null texture): " + texPath.generic_string());

    frameSize = texturePtr->GetSize();

    // -----------------------------
    // Rest of .spt parsing unchanged
    // -----------------------------
    std::string text;
    inFile >> text;

    while (!inFile.eof())
    {
        if (text == "FrameSize")
        {
            inFile >> frameSize.x();
            inFile >> frameSize.y();
        }
        else if (text == "NumFrames")
        {
            int numFrames;
            inFile >> numFrames;
            for (int i = 0; i < numFrames; i++)
                frameTexel.push_back({ frameSize.x() * i, 0 });
        }
        else if (text == "Frame")
        {
            int frameLocationX, frameLocationY;
            inFile >> frameLocationX;
            inFile >> frameLocationY;
            frameTexel.push_back({ (float)frameLocationX, (float)frameLocationY });
        }
        else if (text == "HotSpot")
        {
            int hotSpotX, hotSpotY;
            inFile >> hotSpotX;
            inFile >> hotSpotY;
            hotSpotList.push_back({ (float)hotSpotX, (float)hotSpotY });
        }
        else if (text == "Anim")
        {
            inFile >> text;
            animations.push_back(new Animation{ text });
        }
        else if (text == "CollisionRect")
        {
            rect3 rect;
            inFile >> rect.point1.x() >> rect.point1.y() >> rect.point2.x() >> rect.point2.y();

            if (object == nullptr)
                Engine::GetLogger().LogError("Trying to add collision to a nullobject");
            else
                object->AddGOComponent(new RectCollision(rect, object));
        }
        else if (text == "CollisionCircle")
        {
            double radius;
            inFile >> radius;

            if (object == nullptr)
                Engine::GetLogger().LogError("Trying to add collision to a nullobject");
            else
                object->AddGOComponent(new CircleCollision(radius, object));
        }
        else
        {
            Engine::GetLogger().LogError("Unknown spt command " + text);
        }

        inFile >> text;
    }

    if (frameTexel.empty())
        frameTexel.push_back({ 0, 0 });

    if (animations.empty())
    {
        animations.push_back(new Animation{});
        PlayAnimation(0);
    }
}

void Sprite::Draw(mat3<float> displayMatrix)
{
    if (!texturePtr) return;

    texturePtr->Draw(
        Engine::GetDXContext(),
        displayMatrix * mat3<float>::build_translation(-GetHotSpot(0).x(), -GetHotSpot(0).y()),
        GetFrameTexel(animations[currAnim]->GetDisplayFrame()),
        GetFrameSize());
}

vec2 Sprite::GetHotSpot(int index)
{
    if (index < 0 || hotSpotList.size() <= (size_t)index)
    {
        Engine::GetLogger().LogError("Cannot find a hotspot of current index!");
        return vec2{ 0,0 };
    }
    return hotSpotList[index];
}

vec2 Sprite::GetFrameSize() const
{
    return frameSize;
}

void Sprite::PlayAnimation(int anim)
{
    if (anim < 0 || animations.size() <= (size_t)anim)
    {
        Engine::GetLogger().LogError(std::to_string(anim) + " is out of index!");
        currAnim = 0;
    }
    else
    {
        currAnim = anim;
        animations[currAnim]->ResetAnimation();
    }
}

void Sprite::Update(double dt)
{
    animations[currAnim]->Update(dt);
}

bool Sprite::IsAnimationDone()
{
    return animations[currAnim]->IsAnimationDone();
}

int Sprite::GetCurrentAnim() const
{
    return currAnim;
}

vec2 Sprite::GetFrameTexel(int frameNum) const
{
    if (frameNum < 0 || frameTexel.size() <= (size_t)frameNum)
    {
        Engine::GetLogger().LogError(std::to_string(frameNum) + " is out of index!");
        return vec2{ 0,0 };
    }
    return frameTexel[frameNum];
}
