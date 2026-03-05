#include "../Engine/DX11Services.h"
#include "../Engine/Engine.h"

#include "GamePlay1.h"
#include "ScreenMods.h"
#include <algorithm>
#include <cmath>
#include <memory>

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

    auto player = std::make_unique<Player>(vec2{
        (float)Engine::GetViewportWidth() * 0.5f,
        (float)Engine::GetViewportHeight() * 0.5f
        });
    playerPtr = player.get();
    gameObjectManager->Add(std::move(player));

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



