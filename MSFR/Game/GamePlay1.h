#pragma once
#include <array>
#include <memory>

#include "../Engine/GameObjectManager.h"
#include "../Engine/GameState.h"
#include "../Engine/Input.h"
#include "../Engine/TextureDX11.h"
#include "GameObjectType.h"
#include "Player.h"

class BulletPool;

class GamePlay1 : public GameState
{
public:
    GamePlay1();
    ~GamePlay1();
    void Load() override;
    void Draw() override;
    void Update(double dt) override;
    void Unload() override;
    std::string GetName() override { return "GamePlay1"; }

private:
    enum class WeaponMode
    {
        MachineGun,
        Shotgun,
    };

    enum class BuildPath
    {
        Basic,
        Machine,
        Shotgun,
    };

    enum class UpgradeId
    {
        None,
        ChooseMachinePath,
        ChooseShotgunPath,
        PiercingRounds,
        RapidMechanism,
        SpreadBoost,
        ExplosiveRounds,
    };

    void HandleWeaponInput(double dt);
    bool HandleUpgradeMenu();
    void CheckUpgradeMilestones();
    void BuildUpgradeChoices();
    void ApplyUpgradeChoice(int choiceIndex);
    double GetUpgradeFireIntervalMultiplier() const;
    int GetUpgradedShotgunPellets(int basePellets) const;
    float GetUpgradedShotgunSpread(float baseSpread) const;
    bool HandlePauseMenu();
    int GetPhaseIndex() const;
    double GetPhaseSpawnIntervalMultiplier() const;
    double GetPhaseMaxEnemyMultiplier() const;
    double GetPhaseFireCooldownMultiplier() const;
    void MaybeEmitBalanceLog();
    void UpdateItemPowerups(double dt);
    void FireMachineGun();
    void FireShotgun(int pelletCountToFire);
    void FireHybridBurst(int pelletCountToFire);
    void TriggerWeaponVisual(const vec2& origin, const vec2& direction);
    void StartReload();
    void SpawnBullet(const vec2& origin, const vec2& direction, float speed, double lifeTimeSec, int damage, float hitRadius, const char* spriteSptPath);
    vec2 GetFireDirection() const;

    void SpawnEnemyFromEdge(int enemyTier);
    void SpawnEnemyVariantFromEdge(int variantId);
    void SpawnEliteSetForCore();
    void ProcessEliteSummons();
    void UpdateEnemyTargets();
    void ResolveEnemyOverlap();
    void ResolveEnemyPlayerHits();
    void ResolveBulletHits();
    void SpawnHitParticles(const vec2& origin, const vec2& bulletVelocity);
    void PublishRunSummary(bool cleared);
    int CountAliveByType(GameObjectType type) const;
    double GetSpawnIntervalForTier(int enemyTier) const;
    int GetMaxEnemyCountForTier(int enemyTier) const;

private:
    GameObjectManager* gameObjectManager{ nullptr };
    TextureDX11 map;
    TextureDX11 weaponFireOverlay;
    double timer;
    Player* playerPtr{ nullptr };
    std::unique_ptr<BulletPool> machineBulletPool;
    std::unique_ptr<BulletPool> shotgunBulletPool;

    int totalCoreCount{ 3 };
    int collectedCoreCount{ 0 };
    double enemySpawnTimer{ 0.0 };
    bool clearTriggered{ false };
    bool gameOverTriggered{ false };
    bool pauseMenuOpen{ false };
    int pausePendingAction{ 0 }; // 0:none, 1:resume, 2:restart, 3:menu, 4:quit
    double runElapsedSec{ 0.0 };
    int runKillCount{ 0 };
    std::array<double, 3> phaseElapsedSec{ 0.0, 0.0, 0.0 };
    std::array<int, 3> phaseKillCount{ 0, 0, 0 };
    int currentPhaseIndex{ -1 };
    double nextBalanceLogSec{ 10.0 };

    WeaponMode weaponMode{ WeaponMode::MachineGun };
    BuildPath buildPath{ BuildPath::Basic };
    double fireCooldownTimer{ 0.0 };
    double machineGunInterval{ 0.08 };
    double shotgunInterval{ 0.42 };
    double speedBoostTimer{ 0.0 };
    double rapidBoostTimer{ 0.0 };
    double hybridBoostTimer{ 0.0 };
    vec2 weaponFireOverlayPos{ 0.0f, 0.0f };
    double weaponFireOverlayRotationRad{ 0.0 };
    double weaponFireOverlayTimer{ 0.0 };
    int ammoInMagazine{ 50 };
    static constexpr int kMagazineSize = 50;
    bool isReloading{ false };
    double reloadTimer{ 0.0 };
    double reloadDurationSec{ 1.25 };
    double enemyHitFxTimer{ 0.0 };
    double enemyKillFxTimer{ 0.0 };
    double playerHitFxTimer{ 0.0 };
    double killToastTimer{ 0.0 };
    int killToastCount{ 0 };
    double killComboTimer{ 0.0 };
    int killComboCount{ 0 };
    double layeredHitSfxCooldown{ 0.0 };
    double layeredKillSfxCooldown{ 0.0 };
    int lastKnownPlayerHp{ 100 };
    bool upgradeMenuOpen{ false };
    int upgradePendingChoice{ -1 };
    int queuedUpgradeCount{ 0 };
    int nextUpgradeKillMilestone{ 12 };
    int upgradeKillStep{ 12 };
    std::array<UpgradeId, 3> upgradeChoices{ UpgradeId::None, UpgradeId::None, UpgradeId::None };
    int upgradePierceLevel{ 0 };
    int upgradeFireRateLevel{ 0 };
    int upgradeSpreadLevel{ 0 };
    int upgradeExplosiveLevel{ 0 };
};
