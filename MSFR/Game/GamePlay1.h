#pragma once
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

    void HandleWeaponInput(double dt);
    void FireMachineGun();
    void FireShotgun();
    void SpawnBullet(const vec2& origin, const vec2& direction, float speed, double lifeTimeSec, int damage, float hitRadius, const char* spriteSptPath);
    vec2 GetFireDirection() const;

    void SpawnEnemyFromEdge(int enemyTier);
    void UpdateEnemyTargets();
    void ResolveEnemyOverlap();
    void ResolveEnemyPlayerHits();
    void ResolveBulletHits();
    void SpawnHitParticles(const vec2& origin, const vec2& bulletVelocity);
    int CountAliveByType(GameObjectType type) const;
    double GetSpawnIntervalForTier(int enemyTier) const;
    int GetMaxEnemyCountForTier(int enemyTier) const;

private:
    GameObjectManager* gameObjectManager{ nullptr };
    TextureDX11 map;
    double timer;
    Player* playerPtr{ nullptr };
    std::unique_ptr<BulletPool> machineBulletPool;
    std::unique_ptr<BulletPool> shotgunBulletPool;

    int totalCoreCount{ 3 };
    int collectedCoreCount{ 0 };
    double enemySpawnTimer{ 0.0 };
    bool clearTriggered{ false };

    WeaponMode weaponMode{ WeaponMode::MachineGun };
    double fireCooldownTimer{ 0.0 };
    double machineGunInterval{ 0.08 };
    double shotgunInterval{ 0.42 };
};






