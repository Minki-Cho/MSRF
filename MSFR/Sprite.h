#pragma once
#include <filesystem> //path
#include <string> //std::string

#include "vec2.h" //Vec2 variable
#include "mat3.h"
#include "Component.h" //Component inheritance
#include "GameObject.h" //Ctor, Load

class Texture;
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

    //animation
    void PlayAnimation(int anim);
    void Update(double dt) override;
    bool IsAnimationDone();
    int GetCurrentAnim() const;
private:
    vec2 GetFrameTexel(int frameNum) const;

    Texture* texturePtr;
    vec2 frameSize;
    std::vector<vec2> frameTexel;
    std::vector<vec2> hotSpotList;

    //animation
    int currAnim;
    std::vector<Animation*> animations;
};