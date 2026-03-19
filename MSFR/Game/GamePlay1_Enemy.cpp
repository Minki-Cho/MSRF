#include "../Engine/Engine.h"
#include "../Engine/Random.h"
#include "../Engine/Sprite.h"

#include "CharacterAnim.h"
#include "BalanceConfig.h"
#include "BulletPool.h"
#include "GamePlay1.h"

#include <algorithm>
#include <cstdint>
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
            EliteDash,
            EliteRanged,
            EliteSummoner,
        };

        EnemyChaser(vec2 startPos, Variant variant)
            : GameObject(startPos), variant_(variant)
        {
            const auto& enemySettings = balance::Get().enemy;
            const balance::EnemyVariantSettings* variantSettings = &enemySettings.normal;
            const char* spritePath = "assets/images/characters/characters/enemy/enemy.spt";

            switch (variant_)
            {
            case Variant::Normal:
                variantSettings = &enemySettings.normal;
                spritePath = "assets/images/characters/characters/enemy/enemy.spt";
                break;
            case Variant::Fast:
                variantSettings = &enemySettings.fast;
                spritePath = "assets/images/characters/characters/enemy_fast/enemy_fast.spt";
                break;
            case Variant::Heavy:
                variantSettings = &enemySettings.heavy;
                spritePath = "assets/images/characters/characters/enemy_heavy/enemy_heavy.spt";
                break;
            case Variant::EliteDash:
                variantSettings = &enemySettings.fast;
                spritePath = "assets/images/characters/characters/enemy_fast/enemy_fast.spt";
                break;
            case Variant::EliteRanged:
                variantSettings = &enemySettings.normal;
                spritePath = "assets/images/characters/characters/enemy/enemy.spt";
                break;
            case Variant::EliteSummoner:
                variantSettings = &enemySettings.heavy;
                spritePath = "assets/images/characters/characters/enemy_heavy/enemy_heavy.spt";
                break;
            }

            moveSpeed_ = variantSettings->moveSpeed;
            health_ = variantSettings->health;
            attackRadius_ = variantSettings->attackRadius;
            knockbackForce_ = variantSettings->knockbackForce;
            attackCooldownInterval_ = enemySettings.attackCooldownSec;

            switch (variant_)
            {
            case Variant::EliteDash:
                moveSpeed_ *= 1.35f;
                health_ += 4;
                attackRadius_ = (std::max)(attackRadius_, 46.0f);
                attackCooldownInterval_ *= 0.82;
                dashCooldownSec_ = util::random(1.5f, 2.2f);
                dashDurationSec_ = 0.28;
                dashSpeedMultiplier_ = 3.4f;
                SetScale(vec2{ variantSettings->scale * 1.05f, variantSettings->scale * 1.05f });
                break;
            case Variant::EliteRanged:
                moveSpeed_ *= 0.82f;
                health_ += 6;
                attackRadius_ = 240.0f;
                attackCooldownInterval_ = (std::max)(0.25, attackCooldownInterval_ * 1.75);
                rangedPreferredRange_ = 172.0f;
                strafeSign_ = (util::random(0, 2) == 0) ? -1.0f : 1.0f;
                SetScale(vec2{ variantSettings->scale * 1.03f, variantSettings->scale * 1.03f });
                break;
            case Variant::EliteSummoner:
                moveSpeed_ *= 0.72f;
                health_ += 10;
                attackRadius_ = (std::max)(attackRadius_, 52.0f);
                attackCooldownInterval_ *= 1.12;
                summonCooldownSec_ = util::random(5.4f, 7.2f);
                summonBurstCount_ = 2;
                SetScale(vec2{ variantSettings->scale * 1.10f, variantSettings->scale * 1.10f });
                break;
            default:
                SetScale(vec2{ variantSettings->scale, variantSettings->scale });
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

            attackCooldown_ = attackCooldownInterval_;
            return true;
        }

        float GetAttackRadius() const
        {
            return attackRadius_;
        }

        bool IsEliteSummoner() const
        {
            return variant_ == Variant::EliteSummoner;
        }

        int ConsumePendingSummons()
        {
            const int out = pendingSummonCount_;
            pendingSummonCount_ = 0;
            return out;
        }

        void ApplyDamage(int damage, const vec2& hitDirection)
        {
            if (damage <= 0 || GetDestroyed())
                return;

            const vec2 knockDir = NormalizeOrFallback(hitDirection, vec2{ 0.0f, 0.0f });
            knockbackVel_.x() += knockDir.x() * knockbackForce_;
            knockbackVel_.y() += knockDir.y() * knockbackForce_;
            hitFlashTimer_ = 0.16;

            health_ -= damage;
            if (health_ <= 0)
                SetDestroyed(true);
        }

        void Update(double dt) override
        {
            if (hitFlashTimer_ > 0.0)
            {
                hitFlashTimer_ -= dt;
                if (hitFlashTimer_ < 0.0)
                    hitFlashTimer_ = 0.0;
            }

            if (attackCooldown_ > 0.0)
            {
                attackCooldown_ -= dt;
                if (attackCooldown_ < 0.0)
                    attackCooldown_ = 0.0;
            }

            if (dashCooldownSec_ > 0.0)
            {
                dashCooldownSec_ -= dt;
                if (dashCooldownSec_ < 0.0)
                    dashCooldownSec_ = 0.0;
            }

            if (dashTimeRemainingSec_ > 0.0)
            {
                dashTimeRemainingSec_ -= dt;
                if (dashTimeRemainingSec_ < 0.0)
                    dashTimeRemainingSec_ = 0.0;
            }

            if (variant_ == Variant::EliteSummoner && hasTarget_)
            {
                summonCooldownSec_ -= dt;
                if (summonCooldownSec_ <= 0.0)
                {
                    pendingSummonCount_ += summonBurstCount_;
                    summonCooldownSec_ = util::random(5.4f, 7.6f);
                }
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
                    const float nx = dx / len;
                    const float ny = dy / len;

                    if (variant_ == Variant::EliteRanged)
                    {
                        const float kRangeSlack = 22.0f;
                        if (len > rangedPreferredRange_ + kRangeSlack)
                        {
                            chaseVelocity.x() = nx * moveSpeed_;
                            chaseVelocity.y() = ny * moveSpeed_;
                        }
                        else if (len < rangedPreferredRange_ - kRangeSlack)
                        {
                            chaseVelocity.x() = -nx * moveSpeed_;
                            chaseVelocity.y() = -ny * moveSpeed_;
                        }
                        else
                        {
                            // Orbiting movement makes ranged elite less linear.
                            chaseVelocity.x() = -ny * moveSpeed_ * 0.75f * strafeSign_;
                            chaseVelocity.y() = nx * moveSpeed_ * 0.75f * strafeSign_;
                        }
                    }
                    else
                    {
                        chaseVelocity.x() = nx * moveSpeed_;
                        chaseVelocity.y() = ny * moveSpeed_;

                        if (variant_ == Variant::EliteDash && dashTimeRemainingSec_ <= 0.0 && dashCooldownSec_ <= 0.0)
                        {
                            dashDirection_ = vec2{ nx, ny };
                            dashTimeRemainingSec_ = dashDurationSec_;
                            dashCooldownSec_ = util::random(1.65f, 2.45f);
                        }
                    }
                }
            }

            if (variant_ == Variant::EliteDash && dashTimeRemainingSec_ > 0.0)
            {
                chaseVelocity.x() = dashDirection_.x() * moveSpeed_ * dashSpeedMultiplier_;
                chaseVelocity.y() = dashDirection_.y() * moveSpeed_ * dashSpeedMultiplier_;
            }

            const float damping = (std::max)(0.0f, 1.0f - static_cast<float>(dt) * 8.0f);
            knockbackVel_.x() *= damping;
            knockbackVel_.y() *= damping;
            if (std::abs(knockbackVel_.x()) < 2.0f) knockbackVel_.x() = 0.0f;
            if (std::abs(knockbackVel_.y()) < 2.0f) knockbackVel_.y() = 0.0f;

            vec2 moveVelocity{ chaseVelocity.x() + knockbackVel_.x(), chaseVelocity.y() + knockbackVel_.y() };

            if (std::abs(moveVelocity.x()) > 0.001f || std::abs(moveVelocity.y()) > 0.001f)
            {
                if (std::abs(moveVelocity.x()) > std::abs(moveVelocity.y()))
                    direction_ = (moveVelocity.x() < 0.0f) ? CharacterAnim::Left : CharacterAnim::Right;
                else
                    direction_ = (moveVelocity.y() < 0.0f) ? CharacterAnim::Back : CharacterAnim::Front;
            }
            else
            {
                direction_ = ToIdle(direction_);
            }

            SetVelocity(moveVelocity);

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

        bool CanCollideWith(GameObjectType objectBType) override
        {
            return objectBType == GameObjectType::Player || objectBType == GameObjectType::Enemy;
        }

        void ResolveCollision(GameObject* objectB) override
        {
            if (!objectB || objectB->GetDestroyed() || GetDestroyed())
                return;

            if (objectB->GetObjectType() == GameObjectType::Player)
            {
                auto* player = static_cast<Player*>(objectB);
                if (!player || player->IsDead())
                    return;

                if (TryAttackPlayer())
                {
                    player->ApplyDamage(balance::Get().enemy.contactDamage);
                }
                return;
            }

            if (objectB->GetObjectType() != GameObjectType::Enemy)
                return;

            // Handle enemy-enemy separation once per pair.
            if (reinterpret_cast<std::uintptr_t>(this) >= reinterpret_cast<std::uintptr_t>(objectB))
                return;

            vec2 aPos = GetPosition();
            vec2 bPos = objectB->GetPosition();

            const auto& enemySettings = balance::Get().enemy;
            const float minDist = enemySettings.overlapMinDistance;
            if (minDist <= 0.0f)
                return;

            const float minDistSq = minDist * minDist;
            float dx = bPos.x() - aPos.x();
            float dy = bPos.y() - aPos.y();
            float distSq = dx * dx + dy * dy;
            if (distSq >= minDistSq)
                return;

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
            }
            else
            {
                dist = std::sqrt(distSq);
                nx = dx / dist;
                ny = dy / dist;
            }

            const float overlap = (minDist - dist);
            const float push = overlap * 0.5f + 0.01f;

            const float viewportWidth = static_cast<float>(Engine::GetViewportWidth());
            const float viewportHeight = static_cast<float>(Engine::GetViewportHeight());
            const float worldPadding = enemySettings.overlapWorldPadding;
            const float worldMinX = -viewportWidth + worldPadding;
            const float worldMinY = -viewportHeight + worldPadding;
            const float worldMaxX = viewportWidth * 2.0f - worldPadding;
            const float worldMaxY = viewportHeight * 2.0f - worldPadding;

            aPos.x() = std::clamp(aPos.x() - nx * push, worldMinX, worldMaxX);
            aPos.y() = std::clamp(aPos.y() - ny * push, worldMinY, worldMaxY);
            bPos.x() = std::clamp(bPos.x() + nx * push, worldMinX, worldMaxX);
            bPos.y() = std::clamp(bPos.y() + ny * push, worldMinY, worldMaxY);

            SetPosition(aPos);
            objectB->SetPosition(bPos);
        }

        GameObjectType GetObjectType() override { return GameObjectType::Enemy; }
        std::string GetObjectTypeName() override { return "Enemy"; }

    private:
        Variant variant_ = Variant::Normal;
        float moveSpeed_ = 45.0f;
        int health_ = 10;
        float attackRadius_ = 32.0f;
        float knockbackForce_ = 145.0f;
        bool hasTarget_ = false;
        vec2 targetPos_{ 0.f, 0.f };
        CharacterAnim direction_ = CharacterAnim::None_F;
        vec2 knockbackVel_{ 0.f, 0.f };
        double hitFlashTimer_ = 0.0;
        double attackCooldown_ = 0.0;
        double attackCooldownInterval_ = 0.34;
        double dashCooldownSec_ = 0.0;
        double dashTimeRemainingSec_ = 0.0;
        double dashDurationSec_ = 0.30;
        float dashSpeedMultiplier_ = 3.0f;
        vec2 dashDirection_{ 1.0f, 0.0f };
        float rangedPreferredRange_ = 170.0f;
        float strafeSign_ = 1.0f;
        double summonCooldownSec_ = 0.0;
        int summonBurstCount_ = 0;
        int pendingSummonCount_ = 0;
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
    int variantId = 0;
    if (enemyTier == 1)
        variantId = 1;
    else if (enemyTier >= 2)
        variantId = 2;

    SpawnEnemyVariantFromEdge(variantId);
}

void GamePlay1::SpawnEnemyVariantFromEdge(int variantId)
{
    if (!gameObjectManager || !playerPtr)
        return;

    EnemyChaser::Variant variant = EnemyChaser::Variant::Normal;
    switch (variantId)
    {
    case 1: variant = EnemyChaser::Variant::Fast; break;
    case 2: variant = EnemyChaser::Variant::Heavy; break;
    case 3: variant = EnemyChaser::Variant::EliteDash; break;
    case 4: variant = EnemyChaser::Variant::EliteRanged; break;
    case 5: variant = EnemyChaser::Variant::EliteSummoner; break;
    default: variant = EnemyChaser::Variant::Normal; break;
    }

    const float viewportWidth = static_cast<float>(Engine::GetViewportWidth());
    const float viewportHeight = static_cast<float>(Engine::GetViewportHeight());
    const float worldMinX = -viewportWidth;
    const float worldMinY = -viewportHeight;
    const float worldMaxX = viewportWidth * 2.0f;
    const float worldMaxY = viewportHeight * 2.0f;

    const auto& enemySettings = balance::Get().enemy;
    const float edgeInset = enemySettings.spawnEdgeInset;
    const bool isElite = (variantId >= 3);
    const float minSpawnGap = enemySettings.spawnMinGap * (isElite ? 1.2f : 1.0f);

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

void GamePlay1::SpawnEliteSetForCore()
{
    // One set per collected core: dash + ranged + summoner.
    SpawnEnemyVariantFromEdge(3);
    SpawnEnemyVariantFromEdge(4);
    SpawnEnemyVariantFromEdge(5);
}

void GamePlay1::ProcessEliteSummons()
{
    if (!gameObjectManager || !playerPtr)
        return;

    const float viewportWidth = static_cast<float>(Engine::GetViewportWidth());
    const float viewportHeight = static_cast<float>(Engine::GetViewportHeight());
    const float worldMinX = -viewportWidth + 24.0f;
    const float worldMinY = -viewportHeight + 24.0f;
    const float worldMaxX = viewportWidth * 2.0f - 24.0f;
    const float worldMaxY = viewportHeight * 2.0f - 24.0f;

    std::vector<vec2> summonPositions;
    summonPositions.reserve(16);

    for (const auto& owner : gameObjectManager->Objects())
    {
        GameObject* obj = owner.get();
        if (!obj || obj->GetDestroyed() || obj->GetObjectType() != GameObjectType::Enemy)
            continue;

        auto* enemy = static_cast<EnemyChaser*>(obj);
        if (!enemy->IsEliteSummoner())
            continue;

        const int summonCount = enemy->ConsumePendingSummons();
        if (summonCount <= 0)
            continue;

        const vec2 center = enemy->GetPosition();
        const int safeSummonCount = (std::min)(summonCount, 6);
        for (int i = 0; i < safeSummonCount; ++i)
        {
            const float angle = util::random(0.0f, 2.0f * kPi);
            const float dist = util::random(74.0f, 132.0f);
            vec2 p{
                center.x() + std::cos(angle) * dist,
                center.y() + std::sin(angle) * dist
            };

            p.x() = std::clamp(p.x(), worldMinX, worldMaxX);
            p.y() = std::clamp(p.y(), worldMinY, worldMaxY);
            summonPositions.push_back(p);

            if (summonPositions.size() >= 18)
                break;
        }

        if (summonPositions.size() >= 18)
            break;
    }

    for (const vec2& spawnPos : summonPositions)
    {
        auto summoned = std::make_unique<EnemyChaser>(spawnPos, EnemyChaser::Variant::Normal);
        summoned->SetTarget(playerPtr->GetPosition());
        gameObjectManager->Add(std::move(summoned));
    }
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

    const auto& enemySettings = balance::Get().enemy;
    const float minDist = enemySettings.overlapMinDistance;
    const float minDistSq = minDist * minDist;

    const float viewportWidth = static_cast<float>(Engine::GetViewportWidth());
    const float viewportHeight = static_cast<float>(Engine::GetViewportHeight());
    const float worldPadding = enemySettings.overlapWorldPadding;
    const float worldMinX = -viewportWidth + worldPadding;
    const float worldMinY = -viewportHeight + worldPadding;
    const float worldMaxX = viewportWidth * 2.0f - worldPadding;
    const float worldMaxY = viewportHeight * 2.0f - worldPadding;

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
            playerPtr->ApplyDamage(balance::Get().enemy.contactDamage);
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

        for (EnemyChaser* enemy : enemies)
        {
            if (!enemy || enemy->GetDestroyed())
                continue;

            if (bullet->DoesCollideWith(enemy))
            {
                const bool wasAlive = !enemy->GetDestroyed();
                enemy->ApplyDamage(bullet->GetDamage(), bullet->GetVelocity());
                if (wasAlive && enemy->GetDestroyed())
                {
                    ++runKillCount;
                    const int phaseIndex = GetPhaseIndex();
                    if (phaseIndex >= 0 && phaseIndex < 3)
                        ++phaseKillCount[phaseIndex];
                }
                SpawnHitParticles(bPos, bullet->GetVelocity());
                Engine::PlaySound("assets/sounds/hit_enemy.wav");
                bullet->Deactivate();
                break;
            }
        }
    }
}











