#include "../Engine/DX11Services.h"
#include "../Engine/Engine.h"
#include "../Engine/Random.h"

#include "DataCore.h"
#include "GamePlay1.h"
#include "ScreenMods.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

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

    const int coreCount = 3;
    const float spawnMargin = 140.0f;
    const float minDistToPlayer = 220.0f;
    const float minDistBetweenCores = 170.0f;

    std::vector<vec2> coreSpawn;
    coreSpawn.reserve(coreCount);

    int attempts = 0;
    constexpr int kMaxAttempts = 200;
    while (static_cast<int>(coreSpawn.size()) < coreCount && attempts < kMaxAttempts)
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

    while (static_cast<int>(coreSpawn.size()) < coreCount)
    {
        coreSpawn.push_back(vec2{
            util::random(worldMinX + spawnMargin, worldMaxX - spawnMargin),
            util::random(worldMinY + spawnMargin, worldMaxY - spawnMargin)
            });
    }

    for (const vec2& p : coreSpawn)
    {
        gameObjectManager->Add(std::make_unique<DataCore>(p));
    }

    map = TextureDX11("assets/images/map.png", false);
}

void GamePlay1::Update(double dt)
{
    if (gameObjectManager)
        gameObjectManager->Update(dt);
}

void GamePlay1::Draw()
{
    const float viewportWidth = static_cast<float>(Engine::GetViewportWidth());
    const float viewportHeight = static_cast<float>(Engine::GetViewportHeight());
    const float worldMinX = -viewportWidth;
    const float worldMinY = -viewportHeight;
    const float worldMaxX = viewportWidth * 2.0f;
    const float worldMaxY = viewportHeight * 2.0f;

    mat3<float> cameraMatrix;
    if (playerPtr)
    {
        const vec2 playerPos = playerPtr->GetPosition();

        const float desiredCameraX = viewportWidth * 0.5f - playerPos.x();
        const float desiredCameraY = viewportHeight * 0.5f - playerPos.y();

        const float minCameraX = viewportWidth - worldMaxX;
        const float maxCameraX = -worldMinX;
        const float minCameraY = viewportHeight - worldMaxY;
        const float maxCameraY = -worldMinY;

        const float cameraX = std::clamp(desiredCameraX, minCameraX, maxCameraX);
        const float cameraY = std::clamp(desiredCameraY, minCameraY, maxCameraY);

        cameraMatrix = mat3<float>::build_translation(cameraX, cameraY);
    }

    const vec2 tileSize = map.GetSize();
    if (tileSize.x() > 0.0f && tileSize.y() > 0.0f)
    {
        const int tileStartX = static_cast<int>(std::floor(worldMinX / tileSize.x()));
        const int tileEndX = static_cast<int>(std::ceil(worldMaxX / tileSize.x()));
        const int tileStartY = static_cast<int>(std::floor(worldMinY / tileSize.y()));
        const int tileEndY = static_cast<int>(std::ceil(worldMaxY / tileSize.y()));

        for (int y = tileStartY; y < tileEndY; ++y)
        {
            for (int x = tileStartX; x < tileEndX; ++x)
            {
                const float worldX = static_cast<float>(x) * tileSize.x();
                const float worldY = static_cast<float>(y) * tileSize.y();
                const mat3<float> tileMatrix = cameraMatrix * mat3<float>::build_translation(worldX, worldY);
                map.Draw(tileMatrix);
            }
        }
    }

    if (gameObjectManager)
    {
        gameObjectManager->DrawAll(cameraMatrix);
    }
}

void GamePlay1::Unload()
{
    ClearGSComponent();
    gameObjectManager = nullptr;
    playerPtr = nullptr;
}
