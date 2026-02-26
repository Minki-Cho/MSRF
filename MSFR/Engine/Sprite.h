#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "vec2.h"
#include "mat3.h"
#include "Component.h"
#include "GameObject.h"

class TextureDX11;
class Animation;
class GameObject;

class Sprite : public Component
{
public:
    Sprite(const std::filesystem::path& spriteInfoFile, GameObject* object);
    ~Sprite();

    void Load(const std::filesystem::path& spriteInfoFile, GameObject* object);
    void Draw(mat3<float> displayMatrix);

    vec2 GetHotSpot(int index);
    vec2 GetFrameSize() const;

    // animation
    void PlayAnimation(int anim);
    void Update(double dt) override;
    bool IsAnimationDone();
    int GetCurrentAnim() const;

private:
    vec2 GetFrameTexel(int frameNum) const;

private:
    TextureDX11* texturePtr = nullptr;  //DX11 texture
    vec2 frameSize{ 0, 0 };

    std::vector<vec2> frameTexel;
    std::vector<vec2> hotSpotList;

    int currAnim = 0;
    std::vector<Animation*> animations;
};
