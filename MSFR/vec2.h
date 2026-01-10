#pragma once
#include <cmath> //math
#include <limits> //numeric_limits

namespace math
{
    [[nodiscard]] constexpr double abs(double d) noexcept { return (d < 0.0) ? -d : d; }

    [[nodiscard]] constexpr bool is_equal(double d1, double d2) noexcept
    {
        return abs(d1 - d2) <= std::numeric_limits<double>::epsilon() * abs(d1 + d2);
    }

    [[nodiscard]] constexpr bool is_equal(int i1, int i2) noexcept { return i1 == i2; }

    struct vec2
    {
        double x{ 0 };
        double y{ 0 };
        constexpr vec2() noexcept = default;
        explicit constexpr vec2(double value) noexcept : x(value), y(value) {};
        constexpr vec2(double d1, double d2) noexcept : x(d1), y(d2) {};
        //Can do v += v
        constexpr vec2 operator+=(const vec2& v) noexcept;
        //Can do v -= v
        constexpr vec2 operator-=(const vec2& v) noexcept;
        //Vector *= Number
        constexpr vec2 operator*=(const double& scalar) noexcept;
        //Vector /= Number
        constexpr vec2 operator/=(const double& divider) noexcept;
        //Has a LengthSquared() method
        [[nodiscard]] constexpr double LengthSquared() noexcept;
        //Has a Normalize() method
        [[nodiscard]] inline vec2 Normalize() noexcept;
    };

    struct ivec2
    {
        int x{ 0 };
        int y{ 0 };
        constexpr ivec2() noexcept = default;
        explicit constexpr ivec2(int value) noexcept : x(value), y(value) {};
        constexpr ivec2(int d1, int d2) noexcept : x(d1), y(d2) {};
        //Can do v += v
        constexpr ivec2 operator+=(const ivec2& v) noexcept;
        //Can do v -= v
        constexpr ivec2 operator-=(const ivec2& v) noexcept;
        //Vector *= int
        constexpr ivec2 operator*=(const int& scalar) noexcept;
        //Vector /= int
        constexpr ivec2 operator/=(const int& divider) noexcept;
        //conversion operator
        [[nodiscard]] constexpr operator vec2() noexcept;
    };
}

#include "vec2.inl"