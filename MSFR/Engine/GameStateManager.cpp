#include "GameStateManager.h" //GameStateManager
#include "GameObjectManager.h" //GetGSComponent
#include "Engine.h" //logger
//#include "..\Game\Splash.h" //load splash

GameStateManager::GameStateManager()
{
	gameStates = {};
	state = State::START;
	currGameState = nullptr;
	nextGameState = nullptr;
}

void GameStateManager::AddGameState(GameState& gameState)
{
	gameStates.push_back(&gameState);
}

void GameStateManager::Update(double dt)
{
	switch (state)
	{
	case State::START:
		if (gameStates.empty())
		{
			ENGINE_LOG_CTX(Engine::GetLogger(), Logger::Severity::Fatal, "State", "No registered game states. Entering shutdown.");
			state = State::SHUTDOWN;
		}
		else
		{
			nextGameState = gameStates[0];
			state = State::LOAD;
		}
		break;

	case State::LOAD:
		currGameState = nextGameState;
		ENGINE_LOG_CTX(Engine::GetLogger(), Logger::Severity::Event, "State", "Load " + currGameState->GetName());
		currGameState->Load();
		ENGINE_LOG_CTX(Engine::GetLogger(), Logger::Severity::Event, "State", "Load Complete");
		state = State::UPDATE;
		break;

	case State::UPDATE:
		if (currGameState != nextGameState)
		{
			state = State::UNLOAD;
		}
		else
		{
			ENGINE_LOG_CTX(Engine::GetLogger(), Logger::Severity::Verbose, "State", "Update " + currGameState->GetName());
			currGameState->Update(dt);
			if (Engine::GetGSComponent<GameObjectManager>() != nullptr)
			{
				Engine::GetGSComponent<GameObjectManager>()->CollideTest();
			}
			currGameState->Draw();
		}
		break;

	case State::UNLOAD:
		ENGINE_LOG_CTX(Engine::GetLogger(), Logger::Severity::Event, "State", "Unload " + currGameState->GetName());
		Engine::GetTextureManager().Unload();
		currGameState->Unload();
		if (nextGameState == nullptr)
		{
			state = State::SHUTDOWN;
		}
		else
		{
			state = State::LOAD;
		}
		break;

	case State::SHUTDOWN:
		state = State::EXIT;
		break;

	case State::EXIT:
		break;
	}
}

void GameStateManager::SetNextState(int initState)
{
	if (initState < 0 || initState >= static_cast<int>(gameStates.size()))
	{
		ENGINE_LOG_CTX(
			Engine::GetLogger(),
			Logger::Severity::Warning,
			"State",
			"SetNextState: invalid index " + std::to_string(initState) +
			" (size=" + std::to_string(gameStates.size()) + ")"
		);
		return;
	}

	nextGameState = gameStates[initState];
}

void GameStateManager::Shutdown()
{
	nextGameState = nullptr;
}

void GameStateManager::ReloadState()
{
	state = State::UNLOAD;
}

bool GameStateManager::HasGameEnded()
{
	return (state == State::EXIT);
}
