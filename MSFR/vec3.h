#pragma once
#include <cstddef>   // size_t
#include <cassert>   // assert

struct vec3
{
    float e[3]{};

    // Ctors
    constexpr vec3() noexcept = default;
    constexpr explicit vec3(float s) noexcept : e{ s, s, s } {}
    constexpr vec3(float fx, float fy, float fz) noexcept : e{ fx, fy, fz } {}

    // Accessors (position)
    constexpr float& x() noexcept { return e[0]; }
    constexpr float& y() noexcept { return e[1]; }
    constexpr float& z() noexcept { return e[2]; }
    constexpr float  x() const noexcept { return e[0]; }
    constexpr float  y() const noexcept { return e[1]; }
    constexpr float  z() const noexcept { return e[2]; }

    // Aliases (color)
    constexpr float& red()   noexcept { return e[0]; }
    constexpr float& green() noexcept { return e[1]; }
    constexpr float& blue()  noexcept { return e[2]; }
    constexpr float  red()   const noexcept { return e[0]; }
    constexpr float  green() const noexcept { return e[1]; }
    constexpr float  blue()  const noexcept { return e[2]; }

    // Aliases (dimensions)
    constexpr float& width()  noexcept { return e[0]; }
    constexpr float& height() noexcept { return e[1]; }
    constexpr float& depth()  noexcept { return e[2]; }
    constexpr float  width()  const noexcept { return e[0]; }
    constexpr float  height() const noexcept { return e[1]; }
    constexpr float  depth()  const noexcept { return e[2]; }

    // Indexing
    constexpr float& operator[](std::size_t i) noexcept
    {
        assert(i < 3);
        return e[i];
    }
    constexpr float operator[](std::size_t i) const noexcept
    {
        assert(i < 3);
        return e[i];
    }

    // Optional helpers
    static constexpr std::size_t size() noexcept { return 3; }
    constexpr float* data() noexcept { return e; }
    constexpr const float* data() const noexcept { return e; }
};

struct vec4
{
    float e[4]{};

    // Ctors
    constexpr vec4() noexcept = default;
    constexpr explicit vec4(float s) noexcept : e{ s, s, s, s } {}
    constexpr vec4(float fx, float fy, float fz, float fw) noexcept : e{ fx, fy, fz, fw } {}

    // Accessors (position)
    constexpr float& x() noexcept { return e[0]; }
    constexpr float& y() noexcept { return e[1]; }
    constexpr float& z() noexcept { return e[2]; }
    constexpr float& w() noexcept { return e[3]; }
    constexpr float  x() const noexcept { return e[0]; }
    constexpr float  y() const noexcept { return e[1]; }
    constexpr float  z() const noexcept { return e[2]; }
    constexpr float  w() const noexcept { return e[3]; }

    // Aliases (color)
    constexpr float& red()   noexcept { return e[0]; }
    constexpr float& green() noexcept { return e[1]; }
    constexpr float& blue()  noexcept { return e[2]; }
    constexpr float& alpha() noexcept { return e[3]; }
    constexpr float  red()   const noexcept { return e[0]; }
    constexpr float  green() const noexcept { return e[1]; }
    constexpr float  blue()  const noexcept { return e[2]; }
    constexpr float  alpha() const noexcept { return e[3]; }

    // Aliases (dimensions)
    constexpr float& width()  noexcept { return e[0]; }
    constexpr float& height() noexcept { return e[1]; }
    constexpr float& depth()  noexcept { return e[2]; }
    constexpr float  width()  const noexcept { return e[0]; }
    constexpr float  height() const noexcept { return e[1]; }
    constexpr float  depth()  const noexcept { return e[2]; }

    // Indexing
    constexpr float& operator[](std::size_t i) noexcept
    {
        assert(i < 4);
        return e[i];
    }
    constexpr float operator[](std::size_t i) const noexcept
    {
        assert(i < 4);
        return e[i];
    }

    static constexpr std::size_t size() noexcept { return 4; }
    constexpr float* data() noexcept { return e; }
    constexpr const float* data() const noexcept { return e; }
};
