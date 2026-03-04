#pragma once

#include <string>

#include "Command.h"
#include "Engine.h"
#include "EventTypes.h"

class RequestStateChangeCommand final : public ICommand
{
public:
    explicit RequestStateChangeCommand(int stateIndex) : nextStateIndex(stateIndex) {}

    void Execute() override
    {
        Engine::GetLogger().LogEvent("[Command] RequestStateChange -> " + std::to_string(nextStateIndex));
        Engine::GetGameStateManager().SetNextState(nextStateIndex);
    }

private:
    int nextStateIndex = -1;
};

class LogMenuActionCommand final : public ICommand
{
public:
    explicit LogMenuActionCommand(MenuActionType menuAction) : action(menuAction) {}

    void Execute() override
    {
        const char* actionText = "Unknown";
        switch (action)
        {
        case MenuActionType::Play: actionText = "Play"; break;
        case MenuActionType::HowToPlay: actionText = "HowToPlay"; break;
        case MenuActionType::Quit: actionText = "Quit"; break;
        }

        Engine::GetLogger().LogEvent(std::string("[Command] MenuAction -> ") + actionText);
    }

private:
    MenuActionType action = MenuActionType::Play;
};
