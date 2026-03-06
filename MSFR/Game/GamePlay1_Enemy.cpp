#include "../Engine/Engine.h"
#include "../Engine/Random.h"
#include "../Engine/Sprite.h"

#include "CharacterAnim.h"
#include "BulletPool.h"
#include "GamePlay1.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#ifdef PlaySound
#undef PlaySound
#endif

namespace
{
    constexpr float kPi = 3.14159265358979323846f;

    int ToAnimActionId(CharacterAnim anim)
    {
        switch (anim)
        {
        case CharacterAnim::None_F: return 0;
        case CharacterAnim::None_B: return 1;
        case CharacterAnim::None_L: return 2;
        case CharacterAnim::None_R: return 3;
        case CharacterAnim::Front:  return 4;
        case CharacterAnim::Back:   return 5;
        case CharacterAnim::Left:   return 6;
        case CharacterAnim::Right:  return 7;
        default:                    return 0;
        }
    }

    CharacterAnim ToIdle(CharacterAnim moveAnim)
    {
        switch (moveAnim)
        {
        case CharacterAnim::Front: return CharacterAnim::None_F;
        case CharacterAnim::Back:  return CharacterAnim::None_B;
        case CharacterAnim::Left:  return CharacterAnim::None_L;
        case CharacterAnim::Right: return CharacterAnim::None_R;
        default:                   return moveAnim;
        }
    }

    bool IsTooClose(const vec2& a, const vec2& b, float minDist)
    {
        const float dx = a.x() - b.x();
        const float dy = a.y() - b.y();
        return (dx * dx + dy * dy) < (minDist * minDist);
    }

    vec2 NormalizeOrFallback(const vec2& v, const vec2& fallback)
    {
        const float lenSq = v.x() * v.x() + v.y() * v.y();
        if (lenSq <= 0.000001f)
            return fallback;

        const float invLen = 1.0f / std::sqrt(lenSq);
        return vec2{ v.x() * invLen, v.y() * invLen };
    }

    vec2 Rotate(const vec2& v, float radians)
    {
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        return vec2{ v.x() * c - v.y() * s, v.x() * s + v.y() * c };
    }

    class EnemyChaser : public GameObject
    {
    public:
        enum class Variant
        {
            Normal,
            Fast,
            Heavy,
        };

        EnemyChaser(vec2 startPos, Variant variant)
            : GameObject(startPos), variant_(variant)
        {
            const char* spritePath = "assets/images/characters/characters/enemy/enemy.spt";
            switch (variant_)
            {
            case Variant::Normal:
                moveSpeed_ = 45.0f;
                health_ = 20;
                spritePath = "assets/images/characters/characters/enemy/enemy.spt";
                break;
            case Variant::Fast:
                moveSpeed_ = 72.0f;
                health_ = 12;
                spritePath = "assets/images/characters/characters/enemy_fast/enemy_fast.spt";
                break;
            case Variant::Heavy:
                moveSpeed_ = 34.0f;
                health_ = 32;
                spritePath = "assets/images/characters/characters/enemy_heavy/enemy_heavy.spt";
                SetScale(vec2{ 1.15f, 1.15f });
                break;
            }

            AddGOComponent(new Sprite(spritePath, this));

            if (auto* spr = GetGOComponent<Sprite>())
                spr->PlayAnimation(ToAnimActionId(direction_));
        }

        

        void SetTarget(vec2 targetPos)
        {
            targetPos_ = targetPos;
            hasTarget_ = true;
        }
        bool TryAttackPlayer()
        {
            if (attackCooldown_ > 0.0)
                return false;

            attackCooldown_ = 0.34;
            return true;
        }

        float GetAttackRadius() const
        {
            switch (variant_)
            {
            case Variant::Fast:  return 30.0f;
            case Variant::Heavy: return 36.0f;
            default:             return 32.0f;
            }
        }

        void ApplyDamage(int damage, const vec2& hitDirection)
        {
            if (damage <= 0 || GetDestroyed())
                return;

            const vec2 knockDir = NormalizeOrFallback(hitDirection, vec2{ 0.0f, 0.0f });
            float knockForce = 145.0f;
            switch (variant_)
            {
            case Variant::Fast:  knockForce = 180.0f; break;
            case Variant::Heavy: knockForce = 110.0f; break;
            default:             knockForce = 145.0f; break;
            }

            knockbackVel_.x() += knockDir.x() * knockForce;
            knockbackVel_.y() += knockDir.y() * knockForce;
            hitFlashTimer_ = 0.16;

            health_ -= damage;
            if (health_ <= 0)
                SetDestroyed(true);
        }

        void Update(double dt) override
        {
            
            if (attackCooldown_ > 0.0)
            {
                attackCooldown_ -= dt;
                if (attackCooldown_ < 0.0)
                    attackCooldown_ = 0.0;
            }

            vec2 chaseVelocity{ 0.f, 0.f };

            if (hasTarget_)
            {
                const vec2 pos = GetPosition();
                const float dx = targetPos_.x() - pos.x();
                const float dy = targetPos_.y() - pos.y();
                const float lenSq = dx * dx + dy * dy;

                if (lenSq > 1.0f)
                {
                    const float len = std::sqrt(lenSq);
                    chaseVelocity.x() = (dx / len) * moveSpeed_;
                    chaseVelocity.y() = (dy / len) * moveSpeed_;
                }
            }

            const float damping = (std::max)(0.0f, 1.0f - static_cast<float>(dt) * 8.0f);
            knockbackVel_.x() *= damping;
            knockbackVel_.y() *= damping;
            if (std::abs(knockbackVel_.x()) < 2.0f) knockbackVel_.x() = 0.0f;
            if (std::abs(knockbackVel_.y()) < 2.0f) knockbackVel_.y() = 0.0f;

            vec2 velocity{ chaseVelocity.x() + knockbackVel_.x(), chaseVelocity.y() + knockbackVel_.y() };

            if (std::abs(velocity.x()) > 0.001f || std::abs(velocity.y()) > 0.001f)
            {
                if (std::abs(velocity.x()) > std::abs(velocity.y()))
                    direction_ = (velocity.x() < 0.0f) ? CharacterAnim::Left : CharacterAnim::Right;
                else
                    direction_ = (velocity.y() < 0.0f) ? CharacterAnim::Back : CharacterAnim::Front;
            }
            else
            {
                direction_ = ToIdle(direction_);
            }

            SetVelocity(velocity);

            if (auto* spr = GetGOComponent<Sprite>())
                spr->PlayAnimation(ToAnimActionId(direction_));

            GameObject::Update(dt);
        }

        void Draw(mat3<float> cameraMatrix) override
        {
            if (hitFlashTimer_ > 0.0)
            {
                const double flashPeriod = 0.06;
                const double phase = std::fmod(hitFlashTimer_, flashPeriod);
                if (phase < flashPeriod * 0.5)
                    return;
            }

            GameObject::Draw(cameraMatrix);
        }

        bool CanCollideWith(GameObjectType /*objectBType*/) override
        {
            return false;
        }

        void ResolveCollision(GameObject* /*objectB*/) override
        {
        }

        GameObjectType GetObjectType() override { return GameObjectType::Enemy; }
        std::string GetObjectTypeName() override { return "Enemy"; }

    private:
        Variant variant_ = Variant::Normal;
        float moveSpeed_ = 45.0f;
        int health_ = 10;
        bool hasTarget_ = false;
        vec2 targetPos_{ 0.f, 0.f };
        CharacterAnim direction_ = CharacterAnim::None_F;
        vec2 knockbackVel_{ 0.f, 0.f };
        double hitFlashTimer_ = 0.0;
        double attackCooldown_ = 0.0;
    };

    class HitParticle : public GameObject
    {
    public:
        HitParticle(vec2 startPos, vec2 direction, float speed, double lifeTimeSec, float scale)
            : GameObject(startPos),
              lifeTimeSec_(lifeTimeSec)
        {
            AddGOComponent(new Sprite("assets/images/weapons/bullet_shotgun.spt", this));

            SetScale(vec2{ scale, scale });
            SetRotation(util::random(0.0f, 2.0f * kPi));

            const vec2 dir = NormalizeOrFallback(direction, vec2{ 1.0f, 0.0f });
            SetVelocity(vec2{ dir.x() * speed, dir.y() * speed });
        }

        void Update(double dt) override
        {
            lifeTimeSec_ -= dt;
            if (lifeTimeSec_ <= 0.0)
            {
                SetDestroyed(true);
                return;
            }

            vec2 v = GetVelocity();
            const float damping = (std::max)(0.0f, 1.0f - static_cast<float>(dt) * 10.0f);
            v.x() *= damping;
            v.y() *= damping;
            SetVelocity(v);

            GameObject::Update(dt);
        }

        bool CanCollideWith(GameObjectType /*objectBType*/) override { return false; }
        void ResolveCollision(GameObject* /*objectB*/) override {}

        GameObjectType GetObjectType() override { return GameObjectType::Particle; }
        std::string GetObjectTypeName() override { return "Particle"; }

    private:
        double lifeTimeSec_ = 0.0;
    };
}

void GamePlay1::SpawnHitParticles(const vec2& origin, const vec2& bulletVelocity)
{
    if (!gameObjectManager)
        return;

    const vec2 baseDir = NormalizeOrFallback(bulletVelocity, vec2{ 1.0f, 0.0f });

    constexpr int kParticleCount = 6;
    for (int i = 0; i < kParticleCount; ++i)
    {
        const float angle = util::random(-1.0f, 1.0f);
        const vec2 dir = Rotate(baseDir, angle);
        const float speed = util::random(90.0f, 210.0f);
        const double life = util::random(0.07f, 0.17f);
        const float scale = util::random(0.55f, 0.95f);

        auto particle = std::make_unique<HitParticle>(origin, dir, speed, life, scale);
        gameObjectManager->Add(std::move(particle));
    }
}

void GamePlay1::SpawnEnemyFromEdge(int enemyTier)
{
    if (!gameObjectManager || !playerPtr)
        return;

    EnemyChaser::Variant variant = EnemyChaser::Variant::Normal;
    if (enemyTier == 1)
        variant = EnemyChaser::Variant::Fast;
    else if (enemyTier >= 2)
        variant = EnemyChaser::Variant::Heavy;

    const float viewportWidth = static_cast<float>(Engine::GetViewportWidth());
    const float viewportHeight = static_cast<float>(Engine::GetViewportHeight());
    const float worldMinX = -viewportWidth;
    const float worldMinY = -viewportHeight;
    const float worldMaxX = viewportWidth * 2.0f;
    const float worldMaxY = viewportHeight * 2.0f;

    const float edgeInset = 90.0f;
    const float minSpawnGap = 74.0f;

    auto makeCandidate = [&]() -> vec2
    {
        const int side = util::random(0, 4);
        switch (side)
        {
        case 0: return vec2{ worldMinX + edgeInset, util::random(worldMinY + edgeInset, worldMaxY - edgeInset) };
        case 1: return vec2{ worldMaxX - edgeInset, util::random(worldMinY + edgeInset, worldMaxY - edgeInset) };
        case 2: return vec2{ util::random(worldMinX + edgeInset, worldMaxX - edgeInset), worldMinY + edgeInset };
        default: return vec2{ util::random(worldMinX + edgeInset, worldMaxX - edgeInset), worldMaxY - edgeInset };
        }
    };

    vec2 spawn = makeCandidate();

    for (int attempt = 0; attempt < 16; ++attempt)
    {
        spawn = makeCandidate();

        bool overlap = false;
        for (const auto& owner : gameObjectManager->Objects())
        {
            GameObject* obj = owner.get();
            if (!obj || obj->GetDestroyed() || obj->GetObjectType() != GameObjectType::Enemy)
                continue;

            if (IsTooClose(spawn, obj->GetPosition(), minSpawnGap))
            {
                overlap = true;
                break;
            }
        }

        if (!overlap)
            break;
    }

    auto enemy = std::make_unique<EnemyChaser>(spawn, variant);
    enemy->SetTarget(playerPtr->GetPosition());
    gameObjectManager->Add(std::move(enemy));
}

void GamePlay1::UpdateEnemyTargets()
{
    if (!gameObjectManager || !playerPtr)
        return;

    const vec2 targetPos = playerPtr->GetPosition();
    for (const auto& owner : gameObjectManager->Objects())
    {
        GameObject* obj = owner.get();
        if (!obj || obj->GetDestroyed())
            continue;

        if (obj->GetObjectType() == GameObjectType::Enemy)
        {
            auto* enemy = static_cast<EnemyChaser*>(obj);
            enemy->SetTarget(targetPos);
        }
    }
}

void GamePlay1::ResolveEnemyOverlap()
{
    if (!gameObjectManager)
        return;

    std::vector<GameObject*> enemies;
    enemies.reserve(32);

    for (const auto& owner : gameObjectManager->Objects())
    {
        GameObject* obj = owner.get();
        if (!obj || obj->GetDestroyed())
            continue;

        if (obj->GetObjectType() == GameObjectType::Enemy)
            enemies.push_back(obj);
    }

    if (enemies.size() < 2)
        return;

    const float minDist = 58.0f;
    const float minDistSq = minDist * minDist;

    const float viewportWidth = static_cast<float>(Engine::GetViewportWidth());
    const float viewportHeight = static_cast<float>(Engine::GetViewportHeight());
    const float worldMinX = -viewportWidth + 70.0f;
    const float worldMinY = -viewportHeight + 70.0f;
    const float worldMaxX = viewportWidth * 2.0f - 70.0f;
    const float worldMaxY = viewportHeight * 2.0f - 70.0f;

    for (size_t i = 0; i + 1 < enemies.size(); ++i)
    {
        for (size_t j = i + 1; j < enemies.size(); ++j)
        {
            vec2 aPos = enemies[i]->GetPosition();
            vec2 bPos = enemies[j]->GetPosition();

            float dx = bPos.x() - aPos.x();
            float dy = bPos.y() - aPos.y();
            float distSq = dx * dx + dy * dy;

            if (distSq >= minDistSq)
                continue;

            float nx = 0.0f;
            float ny = 0.0f;
            float dist = 0.0f;

            if (distSq < 0.0001f)
            {
                const int dir = util::random(0, 4);
                switch (dir)
                {
                case 0: nx = 1.0f; ny = 0.0f; break;
                case 1: nx = -1.0f; ny = 0.0f; break;
                case 2: nx = 0.0f; ny = 1.0f; break;
                default: nx = 0.0f; ny = -1.0f; break;
                }
                dist = 0.0f;
            }
            else
            {
                dist = std::sqrt(distSq);
                nx = dx / dist;
                ny = dy / dist;
            }

            const float overlap = (minDist - dist);
            const float push = overlap * 0.5f + 0.01f;

            aPos.x() -= nx * push;
            aPos.y() -= ny * push;
            bPos.x() += nx * push;
            bPos.y() += ny * push;

            aPos.x() = std::clamp(aPos.x(), worldMinX, worldMaxX);
            aPos.y() = std::clamp(aPos.y(), worldMinY, worldMaxY);
            bPos.x() = std::clamp(bPos.x(), worldMinX, worldMaxX);
            bPos.y() = std::clamp(bPos.y(), worldMinY, worldMaxY);

            enemies[i]->SetPosition(aPos);
            enemies[j]->SetPosition(bPos);
        }
    }
}


void GamePlay1::ResolveEnemyPlayerHits()
{
    if (!gameObjectManager || !playerPtr || playerPtr->IsDead())
        return;

    const vec2 playerPos = playerPtr->GetPosition();

    for (const auto& owner : gameObjectManager->Objects())
    {
        GameObject* obj = owner.get();
        if (!obj || obj->GetDestroyed() || obj->GetObjectType() != GameObjectType::Enemy)
            continue;

        auto* enemy = static_cast<EnemyChaser*>(obj);
        const vec2 ePos = enemy->GetPosition();
        const float dx = ePos.x() - playerPos.x();
        const float dy = ePos.y() - playerPos.y();
        const float radius = enemy->GetAttackRadius();

        if ((dx * dx + dy * dy) > (radius * radius))
            continue;

        if (enemy->TryAttackPlayer())
        {
            playerPtr->ApplyDamage(1);
        }
    }
}
void GamePlay1::ResolveBulletHits()
{
    if (!gameObjectManager)
        return;

    std::vector<BulletProjectile*> bullets;
    std::vector<EnemyChaser*> enemies;
    bullets.reserve(96);
    enemies.reserve(32);

    if (machineBulletPool)
        machineBulletPool->GatherActive(bullets);
    if (shotgunBulletPool)
        shotgunBulletPool->GatherActive(bullets);

    for (const auto& owner : gameObjectManager->Objects())
    {
        GameObject* obj = owner.get();
        if (!obj || obj->GetDestroyed())
            continue;

        if (obj->GetObjectType() == GameObjectType::Enemy)
            enemies.push_back(static_cast<EnemyChaser*>(obj));
    }

    for (BulletProjectile* bullet : bullets)
    {
        if (!bullet || !bullet->IsActive())
            continue;

        const vec2 bPos = bullet->GetPosition();
        const float bRadius = bullet->GetHitRadius();

        for (EnemyChaser* enemy : enemies)
        {
            if (!enemy || enemy->GetDestroyed())
                continue;

            bool isHit = enemy->DoesCollideWith(bPos);
            if (!isHit && bRadius > 0.0f)
            {
                const vec2 probes[4] =
                {
                    vec2{ bPos.x() + bRadius, bPos.y() },
                    vec2{ bPos.x() - bRadius, bPos.y() },
                    vec2{ bPos.x(), bPos.y() + bRadius },
                    vec2{ bPos.x(), bPos.y() - bRadius }
                };

                for (const vec2& probe : probes)
                {
                    if (enemy->DoesCollideWith(probe))
                    {
                        isHit = true;
                        break;
                    }
                }
            }

            if (isHit)
            {
                enemy->ApplyDamage(bullet->GetDamage(), bullet->GetVelocity());
                SpawnHitParticles(bPos, bullet->GetVelocity());
                Engine::PlaySound("assets/sounds/hit_enemy.wav");
                bullet->Deactivate();
                break;
            }
        }
    }
}











