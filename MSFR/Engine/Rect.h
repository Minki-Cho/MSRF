#pragma once

#include <algorithm>
#include "vec3.h"

struct rect3
{
    vec3 point1{ 1.0f, 1.0f, 1.0f };
    vec3 point2{ 1.0f, 1.0f, 1.0f };

    constexpr float Left()   const noexcept { return (std::min)(point1.x(), point2.x()); }
    constexpr float Right()  const noexcept { return (std::max)(point1.x(), point2.x()); }
    constexpr float Bottom() const noexcept { return (std::min)(point1.y(), point2.y()); }
    constexpr float Top()    const noexcept { return (std::max)(point1.y(), point2.y()); }

    constexpr float Back()   const noexcept { return (std::min)(point1.z(), point2.z()); }
    constexpr float Front()  const noexcept { return (std::max)(point1.z(), point2.z()); }

    constexpr vec3 Size() const noexcept
    {
        return vec3{ Right() - Left(), Top() - Bottom(), Front() - Back() };
    }
};
