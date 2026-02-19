#pragma once
#include <cstddef>   // size_t
#include <cassert>   // assert
#include <cmath>     // std::sqrt

struct [[nodiscard]] vec2
{
    float e[2]{};

    // Ctors
    constexpr vec2() noexcept = default;
    constexpr explicit vec2(float s) noexcept : e{ s, s } {}
    constexpr vec2(float fx, float fy) noexcept : e{ fx, fy } {}

    // Accessors (position)
    constexpr float& x() noexcept { return e[0]; }
    constexpr float& y() noexcept { return e[1]; }
    constexpr float  x() const noexcept { return e[0]; }
    constexpr float  y() const noexcept { return e[1]; }

    // Aliases (uv)
    constexpr float& u() noexcept { return e[0]; }
    constexpr float& v() noexcept { return e[1]; }
    constexpr float  u() const noexcept { return e[0]; }
    constexpr float  v() const noexcept { return e[1]; }

    // Aliases (dimensions)
    constexpr float& width() noexcept { return e[0]; }
    constexpr float& height() noexcept { return e[1]; }
    constexpr float  width() const noexcept { return e[0]; }
    constexpr float  height() const noexcept { return e[1]; }

    // Indexing
    constexpr float& operator[](std::size_t i) noexcept
    {
        assert(i < 2);
        return e[i];
    }
    constexpr float operator[](std::size_t i) const noexcept
    {
        assert(i < 2);
        return e[i];
    }

    static constexpr std::size_t size() noexcept { return 2; }
    constexpr float* data() noexcept { return e; }
    constexpr const float* data() const noexcept { return e; }
};

//
