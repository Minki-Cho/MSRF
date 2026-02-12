#pragma once
#include <cstdint>

enum class ActionId : uint16_t
{
    Skip,        // Splash, cutscene
    // Jump, Fire, Pause...
};
