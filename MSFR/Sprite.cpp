#include "Sprite.h"
#include "Engine.h"
#include "TextureDX11.h"
#include "Rect.h"
#include "Animation.h"
#include "Collision.h"

#include <fstream>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <cctype> // std::isspace

static std::string Trim(std::string s)
{
    auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!s.empty() && is_space((unsigned char)s.front())) s.erase(s.begin());
    while (!s.empty() && is_space((unsigned char)s.back()))  s.pop_back();
    return s;
}

// Reads first token or "quoted string"
static std::string ReadFirstPathToken(std::ifstream& inFile)
{
    inFile >> std::ws;

    if (inFile.peek() == '"')
    {
        inFile.get();
        std::string inside;
        std::getline(inFile, inside, '"');
        return Trim(inside);
    }

    std::string token;
    inFile >> token;
    return Trim(token);
}

static std::filesystem::path ResolvePathFromSPT(
    const std::filesystem::path& spriteInfoFile,
    const std::string& token)
{
    std::filesystem::path tok = std::filesystem::path(token).lexically_normal();
    std::filesystem::path spt = spriteInfoFile.lexically_normal();
    std::filesystem::path dir = spt.parent_path();

    // 1) absolute token
    if (tok.is_absolute())
        return tok;

    // 2) token as-is exists (covers "assets/..." style)
    if (std::filesystem::exists(tok))
        return tok;

    std::filesystem::path cand = (dir / tok).lexically_normal();
    if (std::filesystem::exists(cand))
        return cand;

    {
        const std::string d = dir.generic_string();
        const std::string t = tok.generic_string();
        if (!d.empty() && t.rfind(d, 0) == 0)
            return tok; // may still not exist; caller can error
    }

    return cand; // caller will throw with a good log
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

    // --- texture path (first line) ---
    std::string texToken = ReadFirstPathToken(inFile);
    if (texToken.empty())
        throw std::runtime_error("Sprite file has empty texture path: " + spriteInfoFile.generic_string());

    std::filesystem::path texPath = ResolvePathFromSPT(spriteInfoFile, texToken);

    Engine::GetLogger().LogEvent("Sprite SPT: " + spriteInfoFile.generic_string());
    Engine::GetLogger().LogEvent("Texture token: " + texToken);
    Engine::GetLogger().LogEvent("Resolved texture path: " + texPath.generic_string());

    if (!std::filesystem::exists(texPath))
    {
        Engine::GetLogger().LogError("Texture file not found: " + texPath.generic_string());
        throw std::runtime_error("Texture file not found: " + texPath.generic_string());
    }

    texturePtr = Engine::GetTextureManager().Load(
        Engine::GetDXDevice(),
        Engine::GetDXContext(),
        texPath.generic_string(),
        true);

    if (!texturePtr)
        throw std::runtime_error("Texture load failed (null texture): " + texPath.generic_string());

    frameSize = texturePtr->GetSize();

    std::string text;
    while (inFile >> text) 
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
            inFile >> frameLocationX >> frameLocationY;
            frameTexel.push_back({ (float)frameLocationX, (float)frameLocationY });
        }
        else if (text == "HotSpot")
        {
            int hotSpotX, hotSpotY;
            inFile >> hotSpotX >> hotSpotY;
            hotSpotList.push_back({ (float)hotSpotX, (float)hotSpotY });
        }
        else if (text == "Anim")
        {
            std::string animToken;
            inFile >> animToken;

            std::filesystem::path animPath = ResolvePathFromSPT(spriteInfoFile, animToken);
            animations.push_back(new Animation{ animPath.generic_string() });
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
    if (anim == currAnim)
    {
        return;
    }
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
    const double animDt = dt * Engine::GetAnimationSpeedMultiplier();
    animations[currAnim]->Update(animDt);
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
