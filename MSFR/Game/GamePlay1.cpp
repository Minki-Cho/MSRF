#include "../Engine/DX11Services.h"
#include "../Engine/Engine.h"

#include "GamePlay1.h"
#include "ScreenMods.h"
#include <algorithm>
GamePlay1::GamePlay1() : timer(5.0f)
{

}

GamePlay1::~GamePlay1()
{
}

void GamePlay1::Load()
{
    Engine::SetAnimationSpeedLevel(Engine::AnimationSpeed::Normal);
    gameObjectManager = new GameObjectManager();
    AddGSComponent(gameObjectManager);

    playerPtr = new Player(vec2{
    (float)Engine::GetViewportWidth() * 0.5f,
    (float)Engine::GetViewportHeight() * 0.5f
        });
    gameObjectManager->Add(playerPtr);
    MainMenuImage = TextureDX11("assets/images/map.png", false);
}

void GamePlay1::Update(double dt)
{
    gameObjectManager->Update(dt);
    /*auto& input = Engine::GetInput();*/

    //if (!input.GetMouseReleasedThisFrame())
    //    return;

    //const vec2 mouse = input.GetMousePos();

    //if (input.GetMouseReleasedThisFrame())
    //{
    //    const vec2 mouse = input.GetMousePos();

    //    Engine::GetLogger().LogEvent(
    //        "mouse win: " +
    //        std::to_string((int)mouse.x()) + ", " +
    //        std::to_string((int)mouse.y())
    //    );
    //}

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

        // 플레이어 이동 가능 범위와 동일한 월드 경계(3배 영역)
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

    const mat3<float> mapMatrix = cameraMatrix *
        mat3<float>::build_scale(1.0f, 1.0f);
    MainMenuImage.Draw(mapMatrix);
}

void GamePlay1::Unload()
{
    ClearGSComponent();
    gameObjectManager = nullptr;
    playerPtr = nullptr;
}