#include "../Engine/Engine.h"

#include "CharacterAnim.h"
#include "BulletPool.h"
#include "GamePlay1.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#ifdef PlaySound
#undef PlaySound
#endif

namespace
{
    constexpr float kPi = 3.14159265358979323846f;

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

    vec2 AimFallbackFromAnim(CharacterAnim anim)
    {
        switch (anim)
        {
        case CharacterAnim::Front:
        case CharacterAnim::None_F:
            return vec2{ -1.0f, 0.0f };
        case CharacterAnim::Back:
        case CharacterAnim::None_B:
            return vec2{ 1.0f, 0.0f };
        case CharacterAnim::Left:
        case CharacterAnim::None_L:
            return vec2{ 0.0f, -1.0f };
        case CharacterAnim::Right:
        case CharacterAnim::None_R:
            return vec2{ 0.0f, 1.0f };
        default:
            return vec2{ 0.0f, 1.0f };
        }
    }
}

void GamePlay1::HandleWeaponInput(double dt)
{
    if (!playerPtr || !gameObjectManager || playerPtr->IsDead())
        return;

    auto& input = Engine::GetInput();

    if (input.IsKeyPressed(InputKey::Keyboard::Num1))
        weaponMode = WeaponMode::MachineGun;
    if (input.IsKeyPressed(InputKey::Keyboard::Num2))
        weaponMode = WeaponMode::Shotgun;

    fireCooldownTimer -= dt;
    if (fireCooldownTimer < 0.0)
        fireCooldownTimer = 0.0;

    const bool wantsFire = input.IsKeyDown(InputKey::Keyboard::Space) || input.GetMouseDown();
    if (!wantsFire)
        return;

    if (fireCooldownTimer > 0.0)
        return;

    if (weaponMode == WeaponMode::MachineGun)
    {
        FireMachineGun();
        fireCooldownTimer = machineGunInterval;
    }
    else
    {
        FireShotgun();
        fireCooldownTimer = shotgunInterval;
    }
}

void GamePlay1::FireMachineGun()
{
    const vec2 dir = GetFireDirection();
    const vec2 playerPos = playerPtr->GetPosition();
    const vec2 origin{ playerPos.x() + dir.x() * 42.0f, playerPos.y() + dir.y() * 42.0f };

    Engine::PlaySound("assets/sounds/gun_fire.wav");

    SpawnBullet(
        origin,
        dir,
        820.0f,
        1.9,
        1,
        5.0f,
        "assets/images/weapons/bullet_machine.spt");
}

void GamePlay1::FireShotgun()
{
    const vec2 baseDir = GetFireDirection();
    const vec2 playerPos = playerPtr->GetPosition();
    const vec2 origin{ playerPos.x() + baseDir.x() * 38.0f, playerPos.y() + baseDir.y() * 38.0f };

    Engine::PlaySound("assets/sounds/gun_shotgun.wav");

    constexpr int pelletCount = 7;
    constexpr float spreadDeg = 14.0f;

    for (int i = 0; i < pelletCount; ++i)
    {
        const float centered = static_cast<float>(i) - static_cast<float>(pelletCount - 1) * 0.5f;
        const float angle = centered * spreadDeg * (kPi / 180.0f);
        const vec2 dir = Rotate(baseDir, angle);

        SpawnBullet(
            origin,
            dir,
            640.0f,
            0.55,
            1,
            4.0f,
            "assets/images/weapons/bullet_shotgun.spt");
    }
}

void GamePlay1::SpawnBullet(const vec2& origin, const vec2& direction, float speed, double lifeTimeSec, int damage, float hitRadius, const char* spriteSptPath)
{
    if (!spriteSptPath)
        return;

    BulletPool* pool = nullptr;
    if (std::strcmp(spriteSptPath, "assets/images/weapons/bullet_shotgun.spt") == 0)
        pool = shotgunBulletPool.get();
    else
        pool = machineBulletPool.get();

    if (!pool)
        return;

    pool->Spawn(origin, direction, speed, lifeTimeSec, damage, hitRadius);
}

vec2 GamePlay1::GetFireDirection() const
{
    if (!playerPtr)
        return vec2{ 0.0f, 1.0f };

    const float viewportWidth = static_cast<float>(Engine::GetViewportWidth());
    const float viewportHeight = static_cast<float>(Engine::GetViewportHeight());
    const float worldMinX = -viewportWidth;
    const float worldMinY = -viewportHeight;
    const float worldMaxX = viewportWidth * 2.0f;
    const float worldMaxY = viewportHeight * 2.0f;

    const vec2 playerPos = playerPtr->GetPosition();

    const float desiredCameraX = viewportWidth * 0.5f - playerPos.x();
    const float desiredCameraY = viewportHeight * 0.5f - playerPos.y();

    const float minCameraX = viewportWidth - worldMaxX;
    const float maxCameraX = -worldMinX;
    const float minCameraY = viewportHeight - worldMaxY;
    const float maxCameraY = -worldMinY;

    const float cameraX = std::clamp(desiredCameraX, minCameraX, maxCameraX);
    const float cameraY = std::clamp(desiredCameraY, minCameraY, maxCameraY);

    const vec2 mouseScreen = Engine::GetInput().GetMousePos();
    const vec2 mouseWorld{ mouseScreen.x() - cameraX, (viewportHeight - mouseScreen.y()) - cameraY };

    const vec2 aimVector{ mouseWorld.x() - playerPos.x(), mouseWorld.y() - playerPos.y() };
    return NormalizeOrFallback(aimVector, AimFallbackFromAnim(playerPtr->GetDirection()));
}


