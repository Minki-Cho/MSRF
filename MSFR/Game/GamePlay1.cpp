#include "../Engine/Engine.h"
#include "../Engine/EventTypes.h"
#include "../Engine/Random.h"
#include "../Engine/Sprite.h"
#include "../Engine/UIFramework.h"
#include <SDL2/SDL.h>

#include "DataCore.h"
#include "BalanceConfig.h"
#include "BulletPool.h"
#include "GamePlay1.h"
#include "ScreenMods.h"

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <memory>
#include <sstream>
#include <vector>

#ifdef PlaySound
#undef PlaySound
#endif

namespace
{
    constexpr int kItemsPerType = 3;
    constexpr float kSpeedMoveMultiplier = 1.85f;
    constexpr float kRapidFireMultiplier = 3.0f;
    constexpr double kSpeedBuffDurationSec = 10.0;
    constexpr double kRapidBuffDurationSec = 9.0;
    constexpr double kHybridBuffDurationSec = 12.0;

    bool IsTooClose(const vec2& a, const vec2& b, float minDist)
    {
        const float dx = a.x() - b.x();
        const float dy = a.y() - b.y();
        return (dx * dx + dy * dy) < (minDist * minDist);
    }

    class PowerItem : public GameObject
    {
    public:
        enum class Kind
        {
            Speed,
            Rapid,
            Hybrid,
        };

        PowerItem(vec2 startPos, Kind kind)
            : GameObject(startPos),
              anchorPos_(startPos),
              kind_(kind)
        {
            const char* spritePath = "assets/images/items/item_speed/item_speed.spt";
            switch (kind_)
            {
            case Kind::Speed: spritePath = "assets/images/items/item_speed/item_speed.spt"; break;
            case Kind::Rapid: spritePath = "assets/images/items/item_rapid/item_rapid.spt"; break;
            case Kind::Hybrid: spritePath = "assets/images/items/item_hybrid/item_hybrid.spt"; break;
            }

            AddGOComponent(new Sprite(spritePath, this));
            phase_ = (static_cast<double>(startPos.x()) * 0.05) + (static_cast<double>(startPos.y()) * 0.03);

            switch (kind_)
            {
            case Kind::Speed:
                bobAmplitude_ = 9.0f;
                bobSpeed_ = 3.2f;
                spinSpeed_ = 2.2f;
                SetScale(vec2{ 0.92f, 0.92f });
                break;
            case Kind::Rapid:
                bobAmplitude_ = 8.0f;
                bobSpeed_ = 3.8f;
                spinSpeed_ = 3.1f;
                SetScale(vec2{ 0.85f, 0.85f });
                break;
            case Kind::Hybrid:
                bobAmplitude_ = 10.0f;
                bobSpeed_ = 2.7f;
                spinSpeed_ = 1.7f;
                SetScale(vec2{ 1.0f, 1.0f });
                break;
            }
        }

        void Update(double dt) override
        {
            timeSec_ += dt;
            const float bob = std::sin(static_cast<float>(timeSec_ * bobSpeed_ + phase_)) * bobAmplitude_;
            SetPosition(vec2{ anchorPos_.x(), anchorPos_.y() + bob });
            UpdateRotation(spinSpeed_ * dt);
            GameObject::Update(dt);
        }

        bool CanCollideWith(GameObjectType objectBType) override
        {
            return objectBType == GameObjectType::Player;
        }

        void ResolveCollision(GameObject* objectB) override
        {
            if (!objectB || objectB->GetObjectType() != GameObjectType::Player)
                return;

            SetDestroyed(true);
        }

        GameObjectType GetObjectType() override
        {
            switch (kind_)
            {
            case Kind::Speed: return GameObjectType::ItemSpeed;
            case Kind::Rapid: return GameObjectType::ItemRapid;
            case Kind::Hybrid: return GameObjectType::ItemHybrid;
            default: return GameObjectType::ItemSpeed;
            }
        }

        std::string GetObjectTypeName() override
        {
            switch (kind_)
            {
            case Kind::Speed: return "ItemSpeed";
            case Kind::Rapid: return "ItemRapid";
            case Kind::Hybrid: return "ItemHybrid";
            default: return "ItemSpeed";
            }
        }

    private:
        vec2 anchorPos_{ 0.0f, 0.0f };
        Kind kind_{ Kind::Speed };
        double timeSec_{ 0.0 };
        double phase_{ 0.0 };
        float bobAmplitude_{ 8.0f };
        float bobSpeed_{ 3.0f };
        float spinSpeed_{ 2.0f };
    };
}

GamePlay1::GamePlay1() : timer(5.0f)
{
}

GamePlay1::~GamePlay1()
{
}

void GamePlay1::Load()
{
    Engine::SetAnimationSpeedLevel(Engine::AnimationSpeed::Normal);
    Engine::PlaySound("assets/sounds/enter_gameplay.wav");
    balance::Reload();

    const auto& settings = balance::Get();
    totalCoreCount = settings.spawn.totalCoreCount;
    machineGunInterval = settings.weapon.machineGunFireIntervalSec;
    shotgunInterval = settings.weapon.shotgunFireIntervalSec;
    runElapsedSec = 0.0;
    runKillCount = 0;
    phaseElapsedSec = { 0.0, 0.0, 0.0 };
    phaseKillCount = { 0, 0, 0 };
    currentPhaseIndex = -1;
    nextBalanceLogSec = 10.0;
    Engine::SetLastRunSummary(Engine::LastRunSummary{});

    auto manager = std::make_unique<GameObjectManager>();
    gameObjectManager = manager.get();
    AddGSComponent(std::move(manager));

    const float viewportWidth = static_cast<float>(Engine::GetViewportWidth());
    const float viewportHeight = static_cast<float>(Engine::GetViewportHeight());

    const vec2 playerStart{
        viewportWidth * 0.5f,
        viewportHeight * 0.5f
    };

    auto player = std::make_unique<Player>(playerStart);
    playerPtr = player.get();
    gameObjectManager->Add(std::move(player));

    const float worldMinX = -viewportWidth;
    const float worldMinY = -viewportHeight;
    const float worldMaxX = viewportWidth * 2.0f;
    const float worldMaxY = viewportHeight * 2.0f;

    const float spawnMargin = 140.0f;
    const float minDistToPlayer = 220.0f;
    const float minDistBetweenCores = 170.0f;

    std::vector<vec2> coreSpawn;
    coreSpawn.reserve(totalCoreCount);

    int attempts = 0;
    constexpr int kMaxAttempts = 200;
    while (static_cast<int>(coreSpawn.size()) < totalCoreCount && attempts < kMaxAttempts)
    {
        ++attempts;

        vec2 candidate{
            util::random(worldMinX + spawnMargin, worldMaxX - spawnMargin),
            util::random(worldMinY + spawnMargin, worldMaxY - spawnMargin)
        };

        if (IsTooClose(candidate, playerStart, minDistToPlayer))
            continue;

        bool overlap = false;
        for (const vec2& existing : coreSpawn)
        {
            if (IsTooClose(candidate, existing, minDistBetweenCores))
            {
                overlap = true;
                break;
            }
        }

        if (!overlap)
            coreSpawn.push_back(candidate);
    }

    while (static_cast<int>(coreSpawn.size()) < totalCoreCount)
    {
        coreSpawn.push_back(vec2{
            util::random(worldMinX + spawnMargin, worldMaxX - spawnMargin),
            util::random(worldMinY + spawnMargin, worldMaxY - spawnMargin)
            });
    }

    for (const vec2& p : coreSpawn)
        gameObjectManager->Add(std::make_unique<DataCore>(p));

    const float itemSpawnMargin = 120.0f;
    const float minDistToPlayerItem = 180.0f;
    const float minDistBetweenItems = 130.0f;
    const float minDistItemToCore = 110.0f;

    std::vector<vec2> itemSpawns;
    itemSpawns.reserve(kItemsPerType * 3);

    auto isItemCandidateValid = [&](const vec2& candidate) -> bool
    {
        if (IsTooClose(candidate, playerStart, minDistToPlayerItem))
            return false;

        for (const vec2& corePos : coreSpawn)
        {
            if (IsTooClose(candidate, corePos, minDistItemToCore))
                return false;
        }

        for (const vec2& existing : itemSpawns)
        {
            if (IsTooClose(candidate, existing, minDistBetweenItems))
                return false;
        }

        return true;
    };

    auto randomItemPos = [&]() -> vec2
    {
        return vec2{
            util::random(worldMinX + itemSpawnMargin, worldMaxX - itemSpawnMargin),
            util::random(worldMinY + itemSpawnMargin, worldMaxY - itemSpawnMargin)
        };
    };

    auto spawnItemsByType = [&](PowerItem::Kind kind)
    {
        for (int i = 0; i < kItemsPerType; ++i)
        {
            vec2 candidate = randomItemPos();

            bool found = false;
            for (int attempt = 0; attempt < 220; ++attempt)
            {
                candidate = randomItemPos();
                if (!isItemCandidateValid(candidate))
                    continue;

                found = true;
                break;
            }

            if (!found)
            {
                // Keep progression robust even on dense maps.
                candidate = randomItemPos();
            }

            itemSpawns.push_back(candidate);
            gameObjectManager->Add(std::make_unique<PowerItem>(candidate, kind));
        }
    };

    spawnItemsByType(PowerItem::Kind::Speed);
    spawnItemsByType(PowerItem::Kind::Rapid);
    spawnItemsByType(PowerItem::Kind::Hybrid);

    collectedCoreCount = 0;
    clearTriggered = false;
    gameOverTriggered = false;
    pauseMenuOpen = false;
    pausePendingAction = 0;
    enemySpawnTimer = 0.2;
    fireCooldownTimer = 0.0;
    weaponMode = WeaponMode::MachineGun;
    speedBoostTimer = 0.0;
    rapidBoostTimer = 0.0;
    hybridBoostTimer = 0.0;
    if (playerPtr)
        playerPtr->SetMoveSpeedMultiplier(1.0f);

    machineBulletPool = std::make_unique<BulletPool>(640, "assets/images/weapons/bullet_machine.spt");
    shotgunBulletPool = std::make_unique<BulletPool>(280, "assets/images/weapons/bullet_shotgun.spt");

    map = TextureDX11("assets/images/map.png", false);
    weaponFireOverlay = TextureDX11("assets/images/weapons/gun_fire.png", false);
    weaponFireOverlayPos = vec2{ 0.0f, 0.0f };
    weaponFireOverlayRotationRad = 0.0;
    weaponFireOverlayTimer = 0.0;
    ammoInMagazine = kMagazineSize;
    isReloading = false;
    reloadTimer = 0.0;
    enemyHitFxTimer = 0.0;
    enemyKillFxTimer = 0.0;
    playerHitFxTimer = 0.0;
    killToastTimer = 0.0;
    killToastCount = 0;
    killComboTimer = 0.0;
    killComboCount = 0;
    layeredHitSfxCooldown = 0.0;
    layeredKillSfxCooldown = 0.0;
    lastKnownPlayerHp = playerPtr ? playerPtr->GetHP() : 100;
}

void GamePlay1::Update(double dt)
{
    if (!gameObjectManager)
        return;

    if (HandlePauseMenu())
        return;

    if (weaponFireOverlayTimer > 0.0)
    {
        weaponFireOverlayTimer -= dt;
        if (weaponFireOverlayTimer < 0.0)
            weaponFireOverlayTimer = 0.0;
    }

    auto decayTimer = [dt](double& t)
    {
        if (t > 0.0)
        {
            t -= dt;
            if (t < 0.0)
                t = 0.0;
        }
    };
    decayTimer(enemyHitFxTimer);
    decayTimer(enemyKillFxTimer);
    decayTimer(playerHitFxTimer);
    decayTimer(killToastTimer);
    decayTimer(killComboTimer);
    decayTimer(layeredHitSfxCooldown);
    decayTimer(layeredKillSfxCooldown);
    if (killComboTimer <= 0.0)
        killComboCount = 0;

    if (isReloading)
    {
        reloadTimer -= dt;
        if (reloadTimer <= 0.0)
        {
            reloadTimer = 0.0;
            isReloading = false;
            ammoInMagazine = kMagazineSize;
        }
    }

    runElapsedSec += dt;
    const int phaseIndex = GetPhaseIndex();
    phaseElapsedSec[phaseIndex] += dt;
    if (currentPhaseIndex != phaseIndex)
    {
        currentPhaseIndex = phaseIndex;
        Engine::GetLogger().LogEvent(
            "[BalanceLog] PhaseEnter phase=" + std::to_string(currentPhaseIndex) +
            " t=" + std::to_string(runElapsedSec) +
            " hp=" + std::to_string(playerPtr ? playerPtr->GetHP() : 0) +
            " cores=" + std::to_string(collectedCoreCount) + "/" + std::to_string(totalCoreCount) +
            " kills=" + std::to_string(runKillCount));
    }
    MaybeEmitBalanceLog();
    UpdateItemPowerups(dt);

    HandleWeaponInput(dt);
    UpdateEnemyTargets();
    gameObjectManager->Update(dt);
    ProcessEliteSummons();
    if (machineBulletPool) machineBulletPool->Update(dt);
    if (shotgunBulletPool) shotgunBulletPool->Update(dt);

    if (playerPtr && playerPtr->IsDead() && !gameOverTriggered)
    {
        PublishRunSummary(false);
        gameOverTriggered = true;
        Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::GameOver) });
        return;
    }

    ResolveBulletHits();

    if (playerPtr)
    {
        const int currentHp = playerPtr->GetHP();
        if (currentHp < lastKnownPlayerHp)
            playerHitFxTimer = 0.22;
        lastKnownPlayerHp = currentHp;
    }

    const int aliveCoreCount = CountAliveByType(GameObjectType::DataCore);
    const int newCollectedCount = std::clamp(totalCoreCount - aliveCoreCount, 0, totalCoreCount);
    if (newCollectedCount != collectedCoreCount)
    {
        const int gainedCoreCount = (std::max)(0, newCollectedCount - collectedCoreCount);
        collectedCoreCount = newCollectedCount;
        enemySpawnTimer = 0.0;

        for (int i = 0; i < gainedCoreCount; ++i)
            SpawnEliteSetForCore();

        if (collectedCoreCount >= totalCoreCount)
        {
            PublishRunSummary(true);
            clearTriggered = true;
            Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::Credit) });
            return;
        }
    }

    if (clearTriggered)
        return;

    const int enemyTier = (std::min)(collectedCoreCount, 2);
    const int aliveEnemyCount = CountAliveByType(GameObjectType::Enemy);
    const int maxEnemyCount = GetMaxEnemyCountForTier(enemyTier);

    enemySpawnTimer -= dt;
    if (aliveEnemyCount < maxEnemyCount && enemySpawnTimer <= 0.0)
    {
        SpawnEnemyFromEdge(enemyTier);
        enemySpawnTimer = GetSpawnIntervalForTier(enemyTier);
    }
}

bool GamePlay1::HandlePauseMenu()
{
    auto& input = Engine::GetInput();

    if (!pauseMenuOpen)
    {
        if (!input.IsKeyPressed(InputKey::Keyboard::Escape))
            return false;

        pauseMenuOpen = true;
        pausePendingAction = 0;
        return true;
    }

    if (pausePendingAction == 1)
    {
        pauseMenuOpen = false;
        pausePendingAction = 0;
        return true;
    }
    if (pausePendingAction == 2)
    {
        pauseMenuOpen = false;
        pausePendingAction = 0;
        Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::GamePlay1) });
        return true;
    }
    if (pausePendingAction == 3)
    {
        pauseMenuOpen = false;
        pausePendingAction = 0;
        Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::MainMenu) });
        return true;
    }
    if (pausePendingAction == 4)
    {
        pauseMenuOpen = false;
        pausePendingAction = 0;
        Engine::GetGameStateManager().Shutdown();
        SDL_Event quitEvent{};
        quitEvent.type = SDL_QUIT;
        SDL_PushEvent(&quitEvent);
        return true;
    }

    if (input.IsKeyPressed(InputKey::Keyboard::Escape))
    {
        pauseMenuOpen = false;
        pausePendingAction = 0;
    }

    return true;
}

void GamePlay1::Draw()
{
    const float viewportWidth = static_cast<float>(Engine::GetViewportWidth());
    const float viewportHeight = static_cast<float>(Engine::GetViewportHeight());
    const float worldMinX = -viewportWidth;
    const float worldMinY = -viewportHeight;
    const float worldMaxX = viewportWidth * 2.0f;
    const float worldMaxY = viewportHeight * 2.0f;

    float cameraX = 0.0f;
    float cameraY = 0.0f;
    if (playerPtr)
    {
        const vec2 playerPos = playerPtr->GetPosition();

        const float desiredCameraX = viewportWidth * 0.5f - playerPos.x();
        const float desiredCameraY = viewportHeight * 0.5f - playerPos.y();

        const float minCameraX = viewportWidth - worldMaxX;
        const float maxCameraX = -worldMinX;
        const float minCameraY = viewportHeight - worldMaxY;
        const float maxCameraY = -worldMinY;

        cameraX = std::clamp(desiredCameraX, minCameraX, maxCameraX);
        cameraY = std::clamp(desiredCameraY, minCameraY, maxCameraY);
    }
    mat3<float> cameraMatrix = mat3<float>::build_translation(cameraX, cameraY);

    const vec2 tileSize = map.GetSize();
    if (tileSize.x() > 0.0f && tileSize.y() > 0.0f)
    {
        const float tileW = tileSize.x();
        const float tileH = tileSize.y();

        // Compute the visible world-space rectangle from the current camera.
        const float visibleMinX = -cameraX;
        const float visibleMinY = -cameraY;
        const float visibleMaxX = visibleMinX + viewportWidth;
        const float visibleMaxY = visibleMinY + viewportHeight;

        // Add a one-tile guard band to avoid edge cut-offs during movement.
        const int tileStartX = static_cast<int>(std::floor((visibleMinX - tileW) / tileW));
        const int tileEndX = static_cast<int>(std::ceil((visibleMaxX + tileW) / tileW));
        const int tileStartY = static_cast<int>(std::floor((visibleMinY - tileH) / tileH));
        const int tileEndY = static_cast<int>(std::ceil((visibleMaxY + tileH) / tileH));

        // Slight overlap hides sub-pixel raster cracks between adjacent tiles.
        const float overlapX = (std::min)(1.0f, tileW * 0.25f);
        const float overlapY = (std::min)(1.0f, tileH * 0.25f);
        const float drawScaleX = (tileW + overlapX) / tileW;
        const float drawScaleY = (tileH + overlapY) / tileH;
        const float drawOffsetX = overlapX * 0.5f;
        const float drawOffsetY = overlapY * 0.5f;

        for (int y = tileStartY; y < tileEndY; ++y)
        {
            for (int x = tileStartX; x < tileEndX; ++x)
            {
                const float worldX = static_cast<float>(x) * tileW - drawOffsetX;
                const float worldY = static_cast<float>(y) * tileH - drawOffsetY;
                const mat3<float> tileMatrix =
                    cameraMatrix *
                    mat3<float>::build_translation(worldX, worldY) *
                    mat3<float>::build_scale(drawScaleX, drawScaleY);
                map.Draw(tileMatrix);
            }
        }
    }

    gameObjectManager->DrawAll(cameraMatrix);
    if (machineBulletPool) machineBulletPool->Draw(cameraMatrix);
    if (shotgunBulletPool) shotgunBulletPool->Draw(cameraMatrix);
    if (weaponFireOverlayTimer > 0.0)
    {
        const vec2 texSize = weaponFireOverlay.GetSize();
        if (texSize.x() > 0.0f && texSize.y() > 0.0f)
        {
            constexpr float overlayDrawW = 72.0f;
            constexpr float overlayDrawH = 30.0f;
            const float sx = overlayDrawW / texSize.x();
            const float sy = overlayDrawH / texSize.y();
            const mat3<float> overlayMatrix =
                cameraMatrix *
                mat3<float>::build_translation(weaponFireOverlayPos.x(), weaponFireOverlayPos.y()) *
                mat3<float>::build_rotation(static_cast<float>(weaponFireOverlayRotationRad)) *
                mat3<float>::build_scale(sx, sy) *
                mat3<float>::build_translation(-texSize.x() * 0.5f, -texSize.y() * 0.5f);
            weaponFireOverlay.Draw(overlayMatrix);
        }
    }

    auto& ui = UI::Get();
    ui.BeginFrame();
    const auto& theme = ui.GetTheme();

    if (playerPtr)
    {
        const vec2 playerPos = playerPtr->GetPosition();
        const float playerScreenX = playerPos.x() + cameraX;
        const float playerScreenY = viewportHeight - (playerPos.y() + cameraY);
        const float labelX = playerScreenX + 36.0f;
        const float labelY = playerScreenY - 56.0f;

        if (isReloading)
        {
            const float progress = std::clamp(
                static_cast<float>((reloadDurationSec - reloadTimer) / (std::max)(0.001, reloadDurationSec)),
                0.0f,
                1.0f);
            char reloadText[64] = {};
            std::snprintf(reloadText, sizeof(reloadText), "RELOADING %.0f%%", progress * 100.0f);
            ui.Label(labelX, labelY, reloadText, 1.8f, UI::Color{ 1.0f, 0.82f, 0.42f });
            ui.ProgressBar(
                UI::Rect{ labelX, labelY + 20.0f, 112.0f, 14.0f },
                progress,
                UI::Color{ 0.95f, 0.55f, 0.26f },
                UI::Color{ 0.12f, 0.14f, 0.18f },
                theme.panelBorder);
        }
        else
        {
            char ammoText[64] = {};
            std::snprintf(ammoText, sizeof(ammoText), "AMMO %d/%d", ammoInMagazine, kMagazineSize);
            const UI::Color ammoColor = (ammoInMagazine <= 10)
                ? UI::Color{ 1.0f, 0.48f, 0.48f }
                : UI::Color{ 0.70f, 0.86f, 1.0f };
            ui.Label(labelX, labelY, ammoText, 1.8f, ammoColor);
        }
    }

    {
        const int hp = playerPtr ? playerPtr->GetHP() : 0;
        const int hpMax = playerPtr ? playerPtr->GetMaxHP() : 0;
        const int coresCollected = std::clamp(totalCoreCount - CountAliveByType(GameObjectType::DataCore), 0, totalCoreCount);
        const int enemiesRemaining = CountAliveByType(GameObjectType::Enemy);
        const float hpRatio = (hpMax > 0)
            ? std::clamp(static_cast<float>(hp) / static_cast<float>(hpMax), 0.0f, 1.0f)
            : 0.0f;

        UI::Color hpFill = UI::Color{ 0.2f, 0.8f, 0.35f };
        if (hpRatio < 0.30f)
            hpFill = UI::Color{ 0.9f, 0.2f, 0.2f };
        else if (hpRatio < 0.60f)
            hpFill = UI::Color{ 0.95f, 0.75f, 0.2f };

        const UI::Rect hudPanel{ viewportWidth - 286.0f, 12.0f, 274.0f, 252.0f };
        ui.Panel(hudPanel, theme.panelBg, theme.panelBorder, 3.0f);

        char hpLabel[64] = {};
        std::snprintf(hpLabel, sizeof(hpLabel), "HP %d/%d", hp, hpMax);
        ui.Label(hudPanel.x + 16.0f, hudPanel.y + 16.0f, hpLabel, 1.9f, theme.text);

        ui.ProgressBar(
            UI::Rect{ hudPanel.x + 16.0f, hudPanel.y + 38.0f, hudPanel.w - 32.0f, 18.0f },
            hpRatio,
            hpFill,
            UI::Color{ 0.10f, 0.13f, 0.18f },
            theme.panelBorder);

        char hpPercentText[24] = {};
        std::snprintf(hpPercentText, sizeof(hpPercentText), "%d%%", static_cast<int>(std::lround(hpRatio * 100.0f)));
        ui.LabelCentered(UI::Rect{ hudPanel.x + 16.0f, hudPanel.y + 38.0f, hudPanel.w - 32.0f, 18.0f }, hpPercentText, 1.5f, theme.text);

        char coresText[64] = {};
        std::snprintf(coresText, sizeof(coresText), "CORES %d/%d", coresCollected, totalCoreCount);
        ui.Label(hudPanel.x + 16.0f, hudPanel.y + 66.0f, coresText, 1.7f, theme.text);

        const bool hybridActive = hybridBoostTimer > 0.0;
        const char* weaponName = hybridActive
            ? "HYBRID"
            : ((weaponMode == WeaponMode::Shotgun) ? "SHOTGUN" : "MACHINE");
        char weaponText[64] = {};
        std::snprintf(weaponText, sizeof(weaponText), "WEAPON %s", weaponName);
        ui.Label(hudPanel.x + 16.0f, hudPanel.y + 85.0f, weaponText, 1.7f, theme.text);

        char enemyText[64] = {};
        std::snprintf(enemyText, sizeof(enemyText), "ENEMIES %d", enemiesRemaining);
        ui.Label(hudPanel.x + 16.0f, hudPanel.y + 104.0f, enemyText, 1.7f, theme.text);

        if (machineBulletPool)
        {
            char poolM[96] = {};
            std::snprintf(poolM, sizeof(poolM), "M %llu/%llu O %llu",
                static_cast<unsigned long long>(machineBulletPool->ActiveCount()),
                static_cast<unsigned long long>(machineBulletPool->Capacity()),
                static_cast<unsigned long long>(machineBulletPool->OverflowCount()));
            ui.Label(hudPanel.x + 16.0f, hudPanel.y + 132.0f, poolM, 1.5f, theme.textMuted);
        }

        if (shotgunBulletPool)
        {
            char poolS[96] = {};
            std::snprintf(poolS, sizeof(poolS), "S %llu/%llu O %llu",
                static_cast<unsigned long long>(shotgunBulletPool->ActiveCount()),
                static_cast<unsigned long long>(shotgunBulletPool->Capacity()),
                static_cast<unsigned long long>(shotgunBulletPool->OverflowCount()));
            ui.Label(hudPanel.x + 16.0f, hudPanel.y + 149.0f, poolS, 1.5f, theme.textMuted);
        }

        const float cardY = hudPanel.y + 174.0f;
        const float cardW = 78.0f;
        const float cardH = 56.0f;
        const float cardGap = 8.0f;
        const float cardStartX = hudPanel.x + 14.0f;

        auto drawBuffCard = [&](float x, const char* icon, const char* key, double timer, double duration, const UI::Color& activeColor)
        {
            const bool active = timer > 0.0;
            const UI::Rect card{ x, cardY, cardW, cardH };
            const UI::Color cardBg = active ? UI::Color{ 0.13f, 0.18f, 0.24f } : UI::Color{ 0.09f, 0.11f, 0.15f };
            const UI::Color cardBorder = active ? activeColor : UI::Color{ 0.24f, 0.28f, 0.34f };
            ui.Panel(card, cardBg, cardBorder, 2.0f);

            const UI::Rect iconRect{ card.x + 5.0f, card.y + 6.0f, 20.0f, 20.0f };
            ui.Panel(iconRect, active ? activeColor : UI::Color{ 0.16f, 0.20f, 0.24f }, UI::Color{ 0.08f, 0.10f, 0.14f }, 1.0f);
            ui.LabelCentered(iconRect, icon, 1.6f, UI::Color{ 0.95f, 0.97f, 1.0f });

            ui.Label(card.x + 29.0f, card.y + 7.0f, key, 1.3f, active ? UI::Color{ 0.96f, 0.98f, 1.0f } : theme.textMuted);

            char t[24] = {};
            std::snprintf(t, sizeof(t), "%.1fs", (std::max)(0.0, timer));
            ui.Label(card.x + 29.0f, card.y + 18.0f, t, 1.2f, active ? UI::Color{ 0.96f, 0.98f, 1.0f } : theme.textMuted);

            const float ratio = (duration > 0.001) ? static_cast<float>(std::clamp(timer / duration, 0.0, 1.0)) : 0.0f;
            ui.ProgressBar(
                UI::Rect{ card.x + 6.0f, card.y + 33.0f, card.w - 12.0f, 15.0f },
                ratio,
                active ? activeColor : UI::Color{ 0.24f, 0.28f, 0.34f },
                UI::Color{ 0.10f, 0.13f, 0.18f },
                UI::Color{ 0.24f, 0.28f, 0.34f });
        };

        drawBuffCard(cardStartX, "S", "SPD", speedBoostTimer, kSpeedBuffDurationSec, UI::Color{ 0.32f, 0.92f, 0.55f });
        drawBuffCard(cardStartX + cardW + cardGap, "R", "ROF", rapidBoostTimer, kRapidBuffDurationSec, UI::Color{ 0.42f, 0.72f, 1.0f });
        drawBuffCard(cardStartX + (cardW + cardGap) * 2.0f, "H", "HYB", hybridBoostTimer, kHybridBuffDurationSec, UI::Color{ 1.0f, 0.66f, 0.30f });
    }

    auto drawScreenBorder = [&](const UI::Color& color, float thickness)
    {
        const float t = (std::max)(1.0f, thickness);
        ui.FillRect(UI::Rect{ 0.0f, 0.0f, viewportWidth, t }, color);
        ui.FillRect(UI::Rect{ 0.0f, viewportHeight - t, viewportWidth, t }, color);
        ui.FillRect(UI::Rect{ 0.0f, 0.0f, t, viewportHeight }, color);
        ui.FillRect(UI::Rect{ viewportWidth - t, 0.0f, t, viewportHeight }, color);
    };

    if (enemyHitFxTimer > 0.0)
    {
        const float pulse = static_cast<float>(enemyHitFxTimer);
        drawScreenBorder(UI::Color{ 0.32f + pulse * 0.5f, 0.64f + pulse * 0.2f, 1.0f }, 2.0f + pulse * 8.0f);
    }
    if (enemyKillFxTimer > 0.0)
    {
        const float pulse = static_cast<float>(enemyKillFxTimer);
        drawScreenBorder(UI::Color{ 1.0f, 0.72f + pulse * 0.2f, 0.32f }, 3.0f + pulse * 10.0f);
    }
    if (playerHitFxTimer > 0.0)
    {
        const float pulse = static_cast<float>(playerHitFxTimer);
        drawScreenBorder(UI::Color{ 1.0f, 0.28f + pulse * 0.2f, 0.26f }, 3.0f + pulse * 10.0f);
    }

    if (killToastTimer > 0.0)
    {
        const float panelW = 232.0f;
        const float panelH = 56.0f;
        const UI::Rect toast{ viewportWidth * 0.5f - panelW * 0.5f, 18.0f, panelW, panelH };
        ui.Panel(toast, UI::Color{ 0.13f, 0.10f, 0.08f }, UI::Color{ 0.97f, 0.68f, 0.32f }, 3.0f);

        char killText[64] = {};
        if (killToastCount >= 2)
            std::snprintf(killText, sizeof(killText), "KILL COMBO X%d", killToastCount);
        else
            std::snprintf(killText, sizeof(killText), "ENEMY DOWN");

        ui.LabelCentered(toast, killText, 2.1f, UI::Color{ 1.0f, 0.93f, 0.77f });
    }

    if (pauseMenuOpen)
    {
        const UI::Rect panel{ viewportWidth * 0.5f - 180.0f, viewportHeight * 0.5f - 150.0f, 360.0f, 300.0f };
        ui.Panel(panel, theme.panelBg, theme.panelBorder, 3.0f);
        ui.LabelCentered(UI::Rect{ panel.x, panel.y + 20.0f, panel.w, 28.0f }, "PAUSED", 2.6f, theme.text);

        const float bx = panel.x + 60.0f;
        const float bw = panel.w - 120.0f;
        if (ui.Button(UI::Rect{ bx, panel.y + 70.0f, bw, 46.0f }, "RESUME"))
            pausePendingAction = 1;
        if (ui.Button(UI::Rect{ bx, panel.y + 124.0f, bw, 46.0f }, "RESTART"))
            pausePendingAction = 2;
        if (ui.Button(UI::Rect{ bx, panel.y + 178.0f, bw, 46.0f }, "MAIN MENU"))
            pausePendingAction = 3;
        if (ui.Button(UI::Rect{ bx, panel.y + 232.0f, bw, 46.0f }, "QUIT", true))
            pausePendingAction = 4;
    }

    ui.EndFrame();
}

void GamePlay1::Unload()
{
    if (playerPtr)
        playerPtr->SetMoveSpeedMultiplier(1.0f);

    ClearGSComponent();
    gameObjectManager = nullptr;
    playerPtr = nullptr;
    map.Reset();
    weaponFireOverlay.Reset();
    machineBulletPool.reset();
    shotgunBulletPool.reset();
    collectedCoreCount = 0;
    enemySpawnTimer = 0.0;
    clearTriggered = false;
    gameOverTriggered = false;
    pauseMenuOpen = false;
    pausePendingAction = 0;
    runElapsedSec = 0.0;
    runKillCount = 0;
    phaseElapsedSec = { 0.0, 0.0, 0.0 };
    phaseKillCount = { 0, 0, 0 };
    currentPhaseIndex = -1;
    nextBalanceLogSec = 10.0;
    fireCooldownTimer = 0.0;
    weaponMode = WeaponMode::MachineGun;
    weaponFireOverlayPos = vec2{ 0.0f, 0.0f };
    weaponFireOverlayRotationRad = 0.0;
    weaponFireOverlayTimer = 0.0;
    ammoInMagazine = kMagazineSize;
    isReloading = false;
    reloadTimer = 0.0;
    speedBoostTimer = 0.0;
    rapidBoostTimer = 0.0;
    hybridBoostTimer = 0.0;
    enemyHitFxTimer = 0.0;
    enemyKillFxTimer = 0.0;
    playerHitFxTimer = 0.0;
    killToastTimer = 0.0;
    killToastCount = 0;
    killComboTimer = 0.0;
    killComboCount = 0;
    layeredHitSfxCooldown = 0.0;
    layeredKillSfxCooldown = 0.0;
    lastKnownPlayerHp = 100;
}

int GamePlay1::GetPhaseIndex() const
{
    const auto& phase = balance::Get().phase;
    if (runElapsedSec < phase.earlyEndSec) return 0;
    if (runElapsedSec < phase.midEndSec) return 1;
    return 2;
}

double GamePlay1::GetPhaseSpawnIntervalMultiplier() const
{
    return balance::Get().phase.spawnIntervalMul[static_cast<std::size_t>(GetPhaseIndex())];
}

double GamePlay1::GetPhaseMaxEnemyMultiplier() const
{
    return balance::Get().phase.maxEnemiesMul[static_cast<std::size_t>(GetPhaseIndex())];
}

double GamePlay1::GetPhaseFireCooldownMultiplier() const
{
    return balance::Get().phase.fireCooldownMul[static_cast<std::size_t>(GetPhaseIndex())];
}

void GamePlay1::MaybeEmitBalanceLog()
{
    if (runElapsedSec < nextBalanceLogSec)
        return;

    while (runElapsedSec >= nextBalanceLogSec)
    {
        Engine::GetLogger().LogEvent(
            "[BalanceLog] Tick t=" + std::to_string(nextBalanceLogSec) +
            " phase=" + std::to_string(GetPhaseIndex()) +
            " hp=" + std::to_string(playerPtr ? playerPtr->GetHP() : 0) +
            " cores=" + std::to_string(collectedCoreCount) + "/" + std::to_string(totalCoreCount) +
            " enemies=" + std::to_string(CountAliveByType(GameObjectType::Enemy)) +
            " kills=" + std::to_string(runKillCount));
        nextBalanceLogSec += 10.0;
    }
}

void GamePlay1::UpdateItemPowerups(double dt)
{
    if (!playerPtr)
        return;

    const int speedPicked = playerPtr->ConsumeSpeedItemPickups();
    const int rapidPicked = playerPtr->ConsumeRapidItemPickups();
    const int hybridPicked = playerPtr->ConsumeHybridItemPickups();

    if (speedPicked > 0)
        speedBoostTimer += static_cast<double>(speedPicked) * kSpeedBuffDurationSec;
    if (rapidPicked > 0)
        rapidBoostTimer += static_cast<double>(rapidPicked) * kRapidBuffDurationSec;
    if (hybridPicked > 0)
    {
        hybridBoostTimer += static_cast<double>(hybridPicked) * kHybridBuffDurationSec;
        isReloading = false;
        reloadTimer = 0.0;
    }

    if (speedBoostTimer > 0.0)
        speedBoostTimer = (std::max)(0.0, speedBoostTimer - dt);
    if (rapidBoostTimer > 0.0)
        rapidBoostTimer = (std::max)(0.0, rapidBoostTimer - dt);
    if (hybridBoostTimer > 0.0)
        hybridBoostTimer = (std::max)(0.0, hybridBoostTimer - dt);

    const bool speedActive = speedBoostTimer > 0.0;
    playerPtr->SetMoveSpeedMultiplier(speedActive ? kSpeedMoveMultiplier : 1.0f);
}

void GamePlay1::PublishRunSummary(bool cleared)
{
    Engine::LastRunSummary summary{};
    summary.valid = true;
    summary.cleared = cleared;
    summary.survivalSec = runElapsedSec;
    summary.killCount = runKillCount;
    summary.coresTotal = totalCoreCount;
    summary.coresCollected = std::clamp(totalCoreCount - CountAliveByType(GameObjectType::DataCore), 0, totalCoreCount);
    Engine::SetLastRunSummary(summary);

    std::ostringstream oss;
    oss << "[BalanceLog] RunEnd result=" << (cleared ? "clear" : "death")
        << " t=" << summary.survivalSec
        << " kills=" << summary.killCount
        << " cores=" << summary.coresCollected << "/" << summary.coresTotal
        << " phaseTime=[" << phaseElapsedSec[0] << "," << phaseElapsedSec[1] << "," << phaseElapsedSec[2] << "]"
        << " phaseKills=[" << phaseKillCount[0] << "," << phaseKillCount[1] << "," << phaseKillCount[2] << "]";
    Engine::GetLogger().LogEvent(oss.str());
}

int GamePlay1::CountAliveByType(GameObjectType type) const
{
    if (!gameObjectManager)
        return 0;

    int count = 0;
    for (const auto& owner : gameObjectManager->Objects())
    {
        GameObject* obj = owner.get();
        if (!obj || obj->GetDestroyed())
            continue;

        if (obj->GetObjectType() == type)
            ++count;
    }
    return count;
}

double GamePlay1::GetSpawnIntervalForTier(int enemyTier) const
{
    const auto& intervals = balance::Get().spawn.intervalByTierSec;
    const int tier = std::clamp(enemyTier, 0, 2);
    constexpr double kSpawnRateMultiplier = 2.0;
    constexpr double kMinSpawnIntervalSec = 0.08;
    const double interval = intervals[static_cast<std::size_t>(tier)] * GetPhaseSpawnIntervalMultiplier();
    return (std::max)(kMinSpawnIntervalSec, interval / kSpawnRateMultiplier);
}

int GamePlay1::GetMaxEnemyCountForTier(int enemyTier) const
{
    const auto& maxEnemies = balance::Get().spawn.maxEnemiesByTier;
    const int tier = std::clamp(enemyTier, 0, 2);
    const double scaled = static_cast<double>(maxEnemies[static_cast<std::size_t>(tier)]) * GetPhaseMaxEnemyMultiplier();
    return (std::max)(1, static_cast<int>(std::lround(scaled)));
}
