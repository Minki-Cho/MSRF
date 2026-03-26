#include "../Engine/Engine.h"

#include "CharacterAnim.h"
#include "BalanceConfig.h"
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
    constexpr double kRapidIntervalDivisor = 3.0;

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
    const bool hybridActive = (hybridBoostTimer > 0.0);
    if (hybridActive && isReloading)
    {
        isReloading = false;
        reloadTimer = 0.0;
    }

    if (input.IsKeyPressed(InputKey::Keyboard::Num1) && buildPath != BuildPath::Shotgun)
        weaponMode = WeaponMode::MachineGun;
    if (input.IsKeyPressed(InputKey::Keyboard::Num2) && buildPath == BuildPath::Shotgun)
        weaponMode = WeaponMode::Shotgun;

    if (buildPath == BuildPath::Basic || buildPath == BuildPath::Machine)
        weaponMode = WeaponMode::MachineGun;
    else if (buildPath == BuildPath::Shotgun)
        weaponMode = WeaponMode::Shotgun;

    fireCooldownTimer -= dt;
    if (fireCooldownTimer < 0.0)
        fireCooldownTimer = 0.0;

    if (isReloading && !hybridActive)
        return;

    if (!hybridActive && ammoInMagazine <= 0)
    {
        StartReload();
        return;
    }

    const bool wantsFire = input.IsKeyDown(InputKey::Keyboard::Space) || input.GetMouseDown();
    if (!wantsFire)
        return;

    if (fireCooldownTimer > 0.0)
        return;

    const double phaseCooldownMul = GetPhaseFireCooldownMultiplier();
    const double rapidIntervalMul = (rapidBoostTimer > 0.0) ? (1.0 / kRapidIntervalDivisor) : 1.0;
    const double upgradeIntervalMul = GetUpgradeFireIntervalMultiplier();
    int ammoUsed = 0;

    if (hybridActive)
    {
        const int configuredPellets = GetUpgradedShotgunPellets(balance::Get().weapon.shotgunPelletCount);
        const int hybridPelletsCap = std::clamp(configuredPellets / 2 + 1, 2, 6);
        const int pelletsToFire = hybridPelletsCap;
        FireHybridBurst(pelletsToFire);
        ammoUsed = 0;

        const double hybridInterval = (machineGunInterval + shotgunInterval) * 0.5;
        fireCooldownTimer = hybridInterval * phaseCooldownMul * rapidIntervalMul * upgradeIntervalMul;
    }
    else if (weaponMode == WeaponMode::MachineGun)
    {
        FireMachineGun();
        fireCooldownTimer = machineGunInterval * phaseCooldownMul * rapidIntervalMul * upgradeIntervalMul;
        ammoUsed = 1;
    }
    else
    {
        const int configuredPellets = GetUpgradedShotgunPellets(balance::Get().weapon.shotgunPelletCount);
        const int pelletsToFire = (std::min)(ammoInMagazine, configuredPellets);
        if (pelletsToFire <= 0)
        {
            StartReload();
            return;
        }

        FireShotgun(pelletsToFire);
        fireCooldownTimer = shotgunInterval * phaseCooldownMul * rapidIntervalMul * upgradeIntervalMul;
        ammoUsed = pelletsToFire;
    }

    if (!hybridActive)
    {
        ammoInMagazine -= ammoUsed;
        if (ammoInMagazine <= 0)
        {
            ammoInMagazine = 0;
            StartReload();
        }
    }
}

void GamePlay1::FireMachineGun()
{
    const auto& weapon = balance::Get().weapon;
    const vec2 dir = GetFireDirection();
    const vec2 playerPos = playerPtr->GetPosition();
    const vec2 origin{ playerPos.x() + dir.x() * weapon.machineMuzzleOffset, playerPos.y() + dir.y() * weapon.machineMuzzleOffset };

    Engine::PlaySound("assets/sounds/gun_fire.wav");

    SpawnBullet(
        origin,
        dir,
        weapon.machineBulletSpeed,
        weapon.machineBulletLifeSec,
        weapon.machineBulletDamage,
        weapon.machineBulletHitRadius,
        "assets/images/weapons/bullet_machine.spt");

    TriggerWeaponVisual(origin, dir);
}

void GamePlay1::FireShotgun(int pelletCountToFire)
{
    const auto& weapon = balance::Get().weapon;
    const vec2 baseDir = GetFireDirection();
    const vec2 playerPos = playerPtr->GetPosition();
    const vec2 origin{ playerPos.x() + baseDir.x() * weapon.shotgunMuzzleOffset, playerPos.y() + baseDir.y() * weapon.shotgunMuzzleOffset };

    Engine::PlaySound("assets/sounds/gun_shotgun.wav");

    const int pelletCount = (std::max)(1, pelletCountToFire);
    const float spreadDeg = GetUpgradedShotgunSpread(weapon.shotgunSpreadDeg);

    for (int i = 0; i < pelletCount; ++i)
    {
        const float centered = static_cast<float>(i) - static_cast<float>(pelletCount - 1) * 0.5f;
        const float angle = centered * spreadDeg * (kPi / 180.0f);
        const vec2 dir = Rotate(baseDir, angle);

        SpawnBullet(
            origin,
            dir,
            weapon.shotgunBulletSpeed,
            weapon.shotgunBulletLifeSec,
            weapon.shotgunBulletDamage,
            weapon.shotgunBulletHitRadius,
            "assets/images/weapons/bullet_shotgun.spt");
    }

    TriggerWeaponVisual(origin, baseDir);
}

void GamePlay1::FireHybridBurst(int pelletCountToFire)
{
    const auto& weapon = balance::Get().weapon;
    const vec2 baseDir = GetFireDirection();
    const vec2 playerPos = playerPtr->GetPosition();

    const vec2 machineOrigin{
        playerPos.x() + baseDir.x() * weapon.machineMuzzleOffset,
        playerPos.y() + baseDir.y() * weapon.machineMuzzleOffset
    };

    const vec2 shotgunOrigin{
        playerPos.x() + baseDir.x() * weapon.shotgunMuzzleOffset,
        playerPos.y() + baseDir.y() * weapon.shotgunMuzzleOffset
    };

    Engine::PlaySound("assets/sounds/gun_fire.wav");
    Engine::PlaySound("assets/sounds/gun_shotgun.wav");

    SpawnBullet(
        machineOrigin,
        baseDir,
        weapon.machineBulletSpeed,
        weapon.machineBulletLifeSec,
        weapon.machineBulletDamage,
        weapon.machineBulletHitRadius,
        "assets/images/weapons/bullet_machine.spt");

    if (pelletCountToFire > 0)
    {
        const int pelletCount = (std::max)(1, pelletCountToFire);
        const float spreadDeg = GetUpgradedShotgunSpread(weapon.shotgunSpreadDeg);

        for (int i = 0; i < pelletCount; ++i)
        {
            const float centered = static_cast<float>(i) - static_cast<float>(pelletCount - 1) * 0.5f;
            const float angle = centered * spreadDeg * (kPi / 180.0f);
            const vec2 dir = Rotate(baseDir, angle);

            SpawnBullet(
                shotgunOrigin,
                dir,
                weapon.shotgunBulletSpeed,
                weapon.shotgunBulletLifeSec,
                weapon.shotgunBulletDamage,
                weapon.shotgunBulletHitRadius,
                "assets/images/weapons/bullet_shotgun.spt");
        }
    }

    TriggerWeaponVisual(machineOrigin, baseDir);
}

void GamePlay1::TriggerWeaponVisual(const vec2& origin, const vec2& direction)
{
    const vec2 dir = NormalizeOrFallback(direction, vec2{ 1.0f, 0.0f });
    weaponFireOverlayPos = vec2{
        origin.x() + dir.x() * 10.0f,
        origin.y() + dir.y() * 10.0f
    };
    weaponFireOverlayRotationRad = -std::atan2(static_cast<double>(dir.y()), static_cast<double>(dir.x()));
    weaponFireOverlayTimer = 0.09;
}

void GamePlay1::StartReload()
{
    if (isReloading)
        return;

    isReloading = true;
    reloadTimer = reloadDurationSec;
}

void GamePlay1::SpawnBullet(const vec2& origin, const vec2& direction, float speed, double lifeTimeSec, int damage, float hitRadius, const char* spriteSptPath)
{
    if (!spriteSptPath)
        return;

    const bool isShotgunBullet = (std::strcmp(spriteSptPath, "assets/images/weapons/bullet_shotgun.spt") == 0);
    BulletPool* pool = nullptr;
    if (isShotgunBullet)
        pool = shotgunBulletPool.get();
    else
        pool = machineBulletPool.get();

    if (!pool)
        return;

    int upgradedDamage = damage;
    if (buildPath == BuildPath::Machine && !isShotgunBullet)
        upgradedDamage += 1;
    if (buildPath == BuildPath::Shotgun && isShotgunBullet)
        upgradedDamage += 1 + upgradeSpreadLevel / 2;

    int pierceCount = 0;
    if (!isShotgunBullet)
        pierceCount = upgradePierceLevel;

    float explosionRadius = 0.0f;
    int explosionDamage = 0;
    if (upgradeExplosiveLevel > 0)
    {
        explosionRadius = isShotgunBullet
            ? (52.0f + static_cast<float>(upgradeExplosiveLevel) * 14.0f)
            : (40.0f + static_cast<float>(upgradeExplosiveLevel) * 10.0f);
        explosionDamage = (std::max)(1, upgradedDamage + upgradeExplosiveLevel - 1);
    }

    pool->Spawn(origin, direction, speed, lifeTimeSec, upgradedDamage, hitRadius, pierceCount, explosionRadius, explosionDamage);
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
