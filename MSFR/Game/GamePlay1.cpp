#include "../Engine/DX11Services.h"
#include "../Engine/Engine.h"

#include "GamePlay1.h"
#include "ScreenMods.h"
#include <algorithm>
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

    auto manager = std::make_unique<GameObjectManager>();
    gameObjectManager = manager.get();
    AddGSComponent(std::move(manager));

    auto player = std::make_unique<Player>(vec2{
        (float)Engine::GetViewportWidth() * 0.5f,
        (float)Engine::GetViewportHeight() * 0.5f
        });
    playerPtr = player.get();
    gameObjectManager->Add(std::move(player));

    MainMenuImage = TextureDX11("assets/images/map.png", false);
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

    mat3<float> cameraMatrix;
    if (playerPtr)
    {
        const vec2 playerPos = playerPtr->GetPosition();

        const float desiredCameraX = viewportWidth * 0.5f - playerPos.x();
        const float desiredCameraY = viewportHeight * 0.5f - playerPos.y();

        const float worldMinX = -viewportWidth;
        const float worldMinY = -viewportHeight;
        const float worldMaxX = viewportWidth * 2.0f;
        const float worldMaxY = viewportHeight * 2.0f;

        const float minCameraX = viewportWidth - worldMaxX;
        const float maxCameraX = -worldMinX;
        const float minCameraY = viewportHeight - worldMaxY;
        const float maxCameraY = -worldMinY;

        const float cameraX = std::clamp(desiredCameraX, minCameraX, maxCameraX);
        const float cameraY = std::clamp(desiredCameraY, minCameraY, maxCameraY);

        cameraMatrix = mat3<float>::build_translation(cameraX, cameraY);
    }

    if (gameObjectManager)
    {
        gameObjectManager->DrawAll(cameraMatrix);
    }

    const mat3<float> mapMatrix = cameraMatrix * mat3<float>::build_scale(1.0f, 1.0f);
    MainMenuImage.Draw(mapMatrix);
}

void GamePlay1::Unload()
{
    ClearGSComponent();
    gameObjectManager = nullptr;
    playerPtr = nullptr;
}
