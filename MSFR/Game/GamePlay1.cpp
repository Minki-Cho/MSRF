#include "../Engine/Engine.h"
#include "../Engine/EventTypes.h"
#include "../Engine/Random.h"
#include "../external/imgui/imgui.h"
#include <SDL2/SDL.h>

#include "DataCore.h"
#include "BalanceConfig.h"
#include "BulletPool.h"
#include "GamePlay1.h"
#include "ScreenMods.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <sstream>
#include <vector>

#ifdef PlaySound
#undef PlaySound
#endif

namespace
{
    bool IsTooClose(const vec2& a, const vec2& b, float minDist)
    {
        const float dx = a.x() - b.x();
        const float dy = a.y() - b.y();
        return (dx * dx + dy * dy) < (minDist * minDist);
    }
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

    collectedCoreCount = 0;
    clearTriggered = false;
    gameOverTriggered = false;
    pauseMenuOpen = false;
    pausePendingAction = 0;
    enemySpawnTimer = 0.2;
    fireCooldownTimer = 0.0;
    weaponMode = WeaponMode::MachineGun;

    machineBulletPool = std::make_unique<BulletPool>(640, "assets/images/weapons/bullet_machine.spt");
    shotgunBulletPool = std::make_unique<BulletPool>(280, "assets/images/weapons/bullet_shotgun.spt");

    map = TextureDX11("assets/images/map.png", false);
}

void GamePlay1::Update(double dt)
{
    if (!gameObjectManager)
        return;

    auto publishHudStats = [this]()
    {
        Engine::GameplayHudStats hud{};
        hud.valid = true;
        hud.coresTotal = totalCoreCount;
        hud.coresCollected = std::clamp(totalCoreCount - CountAliveByType(GameObjectType::DataCore), 0, totalCoreCount);
        hud.enemiesRemaining = CountAliveByType(GameObjectType::Enemy);
        hud.weaponMode = (weaponMode == WeaponMode::Shotgun) ? 1 : 0;

        if (playerPtr)
        {
            hud.hp = playerPtr->GetHP();
            hud.hpMax = playerPtr->GetMaxHP();
        }

        Engine::SetGameplayHudStats(hud);
    };

    if (HandlePauseMenu())
    {
        publishHudStats();
        return;
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

    HandleWeaponInput(dt);
    UpdateEnemyTargets();
    gameObjectManager->Update(dt);
    if (machineBulletPool) machineBulletPool->Update(dt);
    if (shotgunBulletPool) shotgunBulletPool->Update(dt);

    Engine::BulletPoolDebugStats bulletStats{};
    bulletStats.valid = true;
    if (machineBulletPool)
    {
        bulletStats.machineActive = machineBulletPool->ActiveCount();
        bulletStats.machineCapacity = machineBulletPool->Capacity();
        bulletStats.machineOverflow = machineBulletPool->OverflowCount();
    }
    if (shotgunBulletPool)
    {
        bulletStats.shotgunActive = shotgunBulletPool->ActiveCount();
        bulletStats.shotgunCapacity = shotgunBulletPool->Capacity();
        bulletStats.shotgunOverflow = shotgunBulletPool->OverflowCount();
    }
    Engine::SetBulletPoolDebugStats(bulletStats);

    ResolveEnemyOverlap();
    ResolveEnemyPlayerHits();

    if (playerPtr && playerPtr->IsDead() && !gameOverTriggered)
    {
        publishHudStats();
        PublishRunSummary(false);
        gameOverTriggered = true;
        Engine::GetEventBus().Publish(RequestStateChangeEvent{ static_cast<int>(ScreenMods::GameOver) });
        return;
    }

    ResolveBulletHits();

    const int aliveCoreCount = CountAliveByType(GameObjectType::DataCore);
    const int newCollectedCount = std::clamp(totalCoreCount - aliveCoreCount, 0, totalCoreCount);
    if (newCollectedCount != collectedCoreCount)
    {
        collectedCoreCount = newCollectedCount;
        enemySpawnTimer = 0.0;

        if (collectedCoreCount >= totalCoreCount)
        {
            publishHudStats();
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

    publishHudStats();
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

    if (!pauseMenuOpen)
        return;

    ImGui::SetNextWindowPos(
        ImVec2(Engine::GetViewportWidth() * 0.5f, Engine::GetViewportHeight() * 0.5f),
        ImGuiCond_Always,
        ImVec2(0.5f, 0.5f));

    constexpr ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("Paused", nullptr, flags))
    {
        ImGui::TextUnformatted("Game is paused.");
        ImGui::Separator();
        if (ImGui::Button("Resume", ImVec2(160.0f, 0.0f)))
            pausePendingAction = 1;
        if (ImGui::Button("Restart", ImVec2(160.0f, 0.0f)))
            pausePendingAction = 2;
        if (ImGui::Button("Main Menu", ImVec2(160.0f, 0.0f)))
            pausePendingAction = 3;
        if (ImGui::Button("Quit", ImVec2(160.0f, 0.0f)))
            pausePendingAction = 4;
    }
    ImGui::End();
}

void GamePlay1::Unload()
{
    ClearGSComponent();
    gameObjectManager = nullptr;
    playerPtr = nullptr;
    map.Reset();
    machineBulletPool.reset();
    shotgunBulletPool.reset();
    Engine::SetBulletPoolDebugStats(Engine::BulletPoolDebugStats{});
    Engine::SetGameplayHudStats(Engine::GameplayHudStats{});
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
    return intervals[static_cast<std::size_t>(tier)] * GetPhaseSpawnIntervalMultiplier();
}

int GamePlay1::GetMaxEnemyCountForTier(int enemyTier) const
{
    const auto& maxEnemies = balance::Get().spawn.maxEnemiesByTier;
    const int tier = std::clamp(enemyTier, 0, 2);
    const double scaled = static_cast<double>(maxEnemies[static_cast<std::size_t>(tier)]) * GetPhaseMaxEnemyMultiplier();
    return (std::max)(1, static_cast<int>(std::lround(scaled)));
}




