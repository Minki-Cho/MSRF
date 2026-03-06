#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "../Engine/GameObject.h"
#include "GameObjectType.h"

class BulletProjectile : public GameObject
{
public:
    explicit BulletProjectile(const char* spriteSptPath);

    void Activate(const vec2& origin, const vec2& direction, float speed, double lifeTimeSec, int damage, float hitRadius);
    void Deactivate();
    bool IsActive() const noexcept { return isActive_; }

    void Update(double dt) override;
    void Draw(mat3<float> cameraMatrix) override;

    int GetDamage() const noexcept { return damage_; }
    float GetHitRadius() const noexcept { return hitRadius_; }

    bool CanCollideWith(GameObjectType /*objectBType*/) override { return false; }
    void ResolveCollision(GameObject* /*objectB*/) override {}

    GameObjectType GetObjectType() override { return GameObjectType::Bullet; }
    std::string GetObjectTypeName() override { return "Bullet"; }

private:
    bool isActive_ = false;
    double lifeTimeSec_ = 0.0;
    int damage_ = 1;
    float hitRadius_ = 5.0f;
};

class BulletPool
{
public:
    BulletPool(std::size_t capacity, const char* spriteSptPath);

    BulletProjectile* Spawn(const vec2& origin, const vec2& direction, float speed, double lifeTimeSec, int damage, float hitRadius);
    void Update(double dt);
    void Draw(mat3<float> cameraMatrix);
    void DeactivateAll();
    void GatherActive(std::vector<BulletProjectile*>& out) const;

    std::size_t Capacity() const noexcept { return bullets_.size(); }
    std::size_t ActiveCount() const noexcept { return activeIndices_.size(); }
    std::size_t OverflowCount() const noexcept { return overflowCount_; }

private:
    std::vector<std::unique_ptr<BulletProjectile>> bullets_;
    std::vector<std::size_t> activeIndices_;
    std::vector<std::size_t> freeIndices_;
    std::size_t recycleCursor_ = 0;
    std::size_t overflowCount_ = 0;
};

