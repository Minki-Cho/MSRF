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
    playerPtr = new Player(vec2{ 1120, 240 });
	gameObjectManager->Add(playerPtr);
    //MainMenuImage = TextureDX11("assets/images/MainMenu.png", false);
}

void GamePlay1::Update(double dt)
{
    auto& input = Engine::GetInput();

    if (!input.GetMouseReleasedThisFrame())
        return;

    const vec2 mouse = input.GetMousePos();

    Engine::GetLogger().LogEvent(
        "mouse win: " +
        std::to_string((int)mouse.x()) + ", " +
        std::to_string((int)mouse.y())
    );


    gameObjectManager->Update(dt);
}

void GamePlay1::Draw()
{
    MainMenuImage.DrawFitCenter({ (float)Engine::GetViewportWidth(), (float)Engine::GetViewportHeight() });
}

void GamePlay1::Unload()
{
    playerPtr = nullptr;
}