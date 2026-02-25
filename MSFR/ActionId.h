#pragma once
#include <cstdint>

enum class ActionId : uint16_t
{
    Skip,        // Splash, cutscene
    Up, Down, Right, Left,
    // Jump, Fire, Pause...
};
