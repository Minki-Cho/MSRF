#include "../DX11Services.h"
#include "../Engine.h"

#include "GamePlay1.h"
#include "ScreenMods.h"

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
    

    if (gameObjectManager)
    {
        mat3<float> cameraMatrix;
        gameObjectManager->DrawAll(cameraMatrix);
    }
    MainMenuImage.DrawFitCenter({ (float)Engine::GetViewportWidth(), (float)Engine::GetViewportHeight() });
}

void GamePlay1::Unload()
{
    ClearGSComponent();
    gameObjectManager = nullptr;
    playerPtr = nullptr;
}