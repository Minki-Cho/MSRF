#include "BulletPool.h"

#include "../Engine/Sprite.h"

#include <algorithm>
#include <cmath>

namespace
{
    vec2 NormalizeOrFallback(const vec2& v, const vec2& fallback)
    {
        const float lenSq = v.x() * v.x() + v.y() * v.y();
        if (lenSq <= 0.000001f)
            return fallback;

        const float invLen = 1.0f / std::sqrt(lenSq);
        return vec2{ v.x() * invLen, v.y() * invLen };
    }
}

BulletProjectile::BulletProjectile(const char* spriteSptPath)
    : GameObject(vec2{ -100000.0f, -100000.0f })
{
    AddGOComponent(new Sprite(spriteSptPath, this));
    Deactivate();
}

void BulletProjectile::Activate(const vec2& origin, const vec2& direction, float speed, double lifeTimeSec, int damage, float hitRadius)
{
    const vec2 dir = NormalizeOrFallback(direction, vec2{ 1.0f, 0.0f });

    SetDestroyed(false);
    SetPosition(origin);
    SetVelocity(vec2{ dir.x() * speed, dir.y() * speed });

    lifeTimeSec_ = (std::max)(0.0, lifeTimeSec);
    damage_ = (std::max)(1, damage);
    hitRadius_ = (std::max)(0.0f, hitRadius);
    isActive_ = true;
}

void BulletProjectile::Deactivate()
{
    isActive_ = false;
    lifeTimeSec_ = 0.0;
    SetVelocity(vec2{ 0.0f, 0.0f });
}

void BulletProjectile::Update(double dt)
{
    if (!isActive_)
        return;

    lifeTimeSec_ -= dt;
    if (lifeTimeSec_ <= 0.0)
    {
        Deactivate();
        return;
    }

    GameObject::Update(dt);
}

void BulletProjectile::Draw(mat3<float> cameraMatrix)
{
    if (!isActive_)
        return;

    GameObject::Draw(cameraMatrix);
}

BulletPool::BulletPool(std::size_t capacity, const char* spriteSptPath)
{
    bullets_.reserve(capacity);
    activeIndices_.reserve(capacity);
    freeIndices_.reserve(capacity);

    for (std::size_t i = 0; i < capacity; ++i)
    {
        bullets_.push_back(std::make_unique<BulletProjectile>(spriteSptPath));
    }

    for (std::size_t i = 0; i < capacity; ++i)
    {
        freeIndices_.push_back(capacity - 1 - i);
    }
}

BulletProjectile* BulletPool::Spawn(const vec2& origin, const vec2& direction, float speed, double lifeTimeSec, int damage, float hitRadius)
{
    if (bullets_.empty())
        return nullptr;

    std::size_t index = 0;

    if (!freeIndices_.empty())
    {
        index = freeIndices_.back();
        freeIndices_.pop_back();
        activeIndices_.push_back(index);
    }
    else
    {
        if (activeIndices_.empty())
            return nullptr;

        ++overflowCount_;
        if (recycleCursor_ >= activeIndices_.size())
            recycleCursor_ = 0;

        index = activeIndices_[recycleCursor_];
        recycleCursor_ = (recycleCursor_ + 1) % activeIndices_.size();
    }

    BulletProjectile* bullet = bullets_[index].get();
    bullet->Activate(origin, direction, speed, lifeTimeSec, damage, hitRadius);
    return bullet;
}

void BulletPool::Update(double dt)
{
    std::size_t i = 0;
    while (i < activeIndices_.size())
    {
        const std::size_t index = activeIndices_[i];
        BulletProjectile* bullet = bullets_[index].get();
        bullet->Update(dt);

        if (!bullet->IsActive())
        {
            freeIndices_.push_back(index);
            activeIndices_[i] = activeIndices_.back();
            activeIndices_.pop_back();
            continue;
        }

        ++i;
    }

    if (recycleCursor_ >= activeIndices_.size())
        recycleCursor_ = 0;
}

void BulletPool::Draw(mat3<float> cameraMatrix)
{
    for (std::size_t index : activeIndices_)
    {
        bullets_[index]->Draw(cameraMatrix);
    }
}

void BulletPool::DeactivateAll()
{
    for (std::size_t index : activeIndices_)
    {
        bullets_[index]->Deactivate();
        freeIndices_.push_back(index);
    }

    activeIndices_.clear();
    recycleCursor_ = 0;
}

void BulletPool::GatherActive(std::vector<BulletProjectile*>& out) const
{
    out.reserve(out.size() + activeIndices_.size());
    for (std::size_t index : activeIndices_)
    {
        BulletProjectile* bullet = bullets_[index].get();
        if (bullet->IsActive())
            out.push_back(bullet);
    }
}

