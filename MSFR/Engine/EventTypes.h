#pragma once

enum class MenuActionType
{
    Play,
    HowToPlay,
    Quit
};

struct MenuActionEvent
{
    MenuActionType action = MenuActionType::Play;
};

struct RequestStateChangeEvent
{
    int nextStateIndex = -1;
};
