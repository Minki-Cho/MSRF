#pragma once
#include <vector> //vector array
#include "GameState.h" //GameState

class GameStateManager
{
public:
	GameStateManager();

	void AddGameState(GameState& gameState);
	void Update(double dt);
	void SetNextState(int initState);
	void Shutdown();
	void ReloadState();
	bool HasGameEnded();
	std::string GetName() { return currGameState->GetName(); };

	template<typename T>
	T* GetGSComponent() { return currGameState->GetGSComponent<T>(); }

private:
	enum class State
	{
		START,
		LOAD,
		UPDATE,
		UNLOAD,
		SHUTDOWN,
		EXIT,
	};

	std::vector<GameState*> gameStates;
	State state;
	GameState* currGameState;
	GameState* nextGameState;
};