#pragma once

#include <array>

namespace balance
{
    struct WeaponSettings
    {
        double machineGunFireIntervalSec = 0.08;
        double shotgunFireIntervalSec = 0.42;

        float machineMuzzleOffset = 42.0f;
        float machineBulletSpeed = 820.0f;
        double machineBulletLifeSec = 1.9;
        int machineBulletDamage = 1;
        float machineBulletHitRadius = 5.0f;

        float shotgunMuzzleOffset = 38.0f;
        int shotgunPelletCount = 7;
        float shotgunSpreadDeg = 14.0f;
        float shotgunBulletSpeed = 640.0f;
        double shotgunBulletLifeSec = 0.55;
        int shotgunBulletDamage = 1;
        float shotgunBulletHitRadius = 4.0f;
    };

    struct EnemyVariantSettings
    {
        float moveSpeed = 45.0f;
        int health = 20;
        float attackRadius = 32.0f;
        float knockbackForce = 145.0f;
        float scale = 1.0f;
    };

    struct EnemySettings
    {
        EnemyVariantSettings normal{};
        EnemyVariantSettings fast{ 72.0f, 12, 30.0f, 180.0f, 1.0f };
        EnemyVariantSettings heavy{ 34.0f, 32, 36.0f, 110.0f, 1.15f };

        double attackCooldownSec = 0.34;
        int contactDamage = 1;

        float overlapMinDistance = 58.0f;
        float overlapWorldPadding = 70.0f;

        float spawnEdgeInset = 90.0f;
        float spawnMinGap = 74.0f;
    };

    struct SpawnSettings
    {
        int totalCoreCount = 3;
        std::array<double, 3> intervalByTierSec{ 2.4, 1.8, 1.3 };
        std::array<int, 3> maxEnemiesByTier{ 12, 18, 24 };
    };

    struct PhaseSettings
    {
        double earlyEndSec = 45.0;
        double midEndSec = 95.0;
        std::array<double, 3> spawnIntervalMul{ 1.22, 1.08, 0.84 };
        std::array<double, 3> maxEnemiesMul{ 0.78, 0.92, 1.30 };
        std::array<double, 3> fireCooldownMul{ 0.92, 0.96, 1.06 };
    };

    struct Settings
    {
        WeaponSettings weapon{};
        EnemySettings enemy{};
        SpawnSettings spawn{};
        PhaseSettings phase{};
    };

    const Settings& Get();
    void Reload();
}
