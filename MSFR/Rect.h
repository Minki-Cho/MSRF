#pragma once

#include <algorithm> //min, max

#include "vec3.h" //Vec3 variables

struct rect3
{
	vec3 point1{ 1.0f, 1.0f, 1.0f };
	vec3 point2{ 1.0f, 1.0f, 1.0f };

	constexpr vec3 Size() const noexcept { return { Right() - Left(), Top() - Bottom(), 1.f }; }
	constexpr float Left() const noexcept { return std::min(point1.x, point2.x); } 
	constexpr float Right() const noexcept { return std::max(point1.x, point2.x); }
	constexpr float Top() const noexcept { return std::max(point1.y, point2.y); }
	constexpr float Bottom() const noexcept { return std::min(point1.y, point2.y); }
};