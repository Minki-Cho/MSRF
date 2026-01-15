/*
 * Rudy Castan
 * CS200
 * Fall 2022
 */

#pragma once

struct vec3
{
    union
    {
        struct
        {
            float x, y, z;
        };
        struct
        {
            float red, green, blue;
        };
        struct
        {
            float width, height, depth;
        };
        float elements[3];
    };

    constexpr vec3() noexcept;
    constexpr explicit vec3(float repeated_float) noexcept;
    constexpr vec3(float fx, float fy, float fz) noexcept;
};

////////////////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////////////////

constexpr vec3::vec3() noexcept : vec3(0.0f)
{
}

constexpr vec3::vec3(float repeated_float) noexcept : vec3(repeated_float, repeated_float, repeated_float)
{
}

constexpr vec3::vec3(float fx, float fy, float fz) noexcept : x(fx), y(fy), z(fz)
{
}

struct vec4
{
    union
    {
        struct
        {
            float x, y, z, w;
        };
        struct
        {
            float red, green, blue, alpha;
        };
        struct
        {
            float width, height, depth, w;
        };
        float elements[4];
    };

    constexpr vec4() noexcept;
    constexpr explicit vec4(float repeated_float) noexcept;
    constexpr vec4(float fx, float fy, float fz, float fw) noexcept;
};

////////////////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////////////////

constexpr vec4::vec4() noexcept : vec4(0.0f)
{
}

constexpr vec4::vec4(float repeated_float) noexcept : vec4(repeated_float, repeated_float, repeated_float, repeated_float)
{
}

constexpr vec4::vec4(float fx, float fy, float fz, float fw) noexcept : x(fx), y(fy), z(fz), w(fw)
{
}
