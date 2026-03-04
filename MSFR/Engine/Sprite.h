#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "Component.h"
#include "GameObject.h"
#include "mat3.h"
#include "vec2.h"

class Animation;
class GameObject;
class TextureDX11;

class Sprite : public Component
{
public:
    Sprite(const std::filesystem::path& spriteInfoFile, GameObject* object);
    ~Sprite() override;

    void Load(const std::filesystem::path& spriteInfoFile, GameObject* object);
    void Draw(mat3<float> displayMatrix);

    vec2 GetHotSpot(int index);
    vec2 GetFrameSize() const;

    void PlayAnimation(int anim);
    void Update(double dt) override;
    bool IsAnimationDone();
    int GetCurrentAnim() const;

private:
    vec2 GetFrameTexel(int frameNum) const;

private:
    TextureDX11* texturePtr = nullptr;
    vec2 frameSize{ 0, 0 };

    std::vector<vec2> frameTexel;
    std::vector<vec2> hotSpotList;

    int currAnim = 0;
    std::vector<std::unique_ptr<Animation>> animations;
};
