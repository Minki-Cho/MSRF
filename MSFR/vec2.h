#pragma once

struct [[nodiscard]] vec2
{
    union
    {
        struct
        {
            float x, y;
        };
        struct
        {
            float u, v;
        };
        struct
        {
            float width, height;
        };
        float elements[2];
    };

    constexpr vec2() noexcept;
    constexpr explicit vec2(float repeated_float) noexcept;
    constexpr vec2(float fx, float fy) noexcept;
};

constexpr vec2& operator+=(vec2& v, const vec2& adding_vector) noexcept;

constexpr vec2 operator*(const vec2& v, float scale) noexcept;

constexpr vec2 operator/(const vec2& v, float divisor) noexcept;

constexpr vec2& operator*=(vec2& v, float scale) noexcept;

constexpr vec2& operator/=(vec2& v, float divisor) noexcept;

constexpr vec2 operator+(const vec2& v1, const vec2& v2) noexcept;

constexpr vec2 operator-(const vec2& v1, const vec2& v2) noexcept;

constexpr float dot(vec2 a, vec2 b) noexcept;

constexpr vec2 perpendicular_to(vec2 a) noexcept;

constexpr float magnitude_squared(vec2 a) noexcept;

float magnitude(vec2 a) noexcept;

vec2 normalize(vec2 a) noexcept;


////////////////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////////////////

constexpr vec2::vec2() noexcept : vec2(0.0f)
{
}

constexpr vec2::vec2(float repeated_float) noexcept : vec2(repeated_float, repeated_float)
{
}

constexpr vec2::vec2(float fx, float fy) noexcept : x(fx), y(fy)
{
}

constexpr vec2& operator+=(vec2& v, const vec2& adding_vector) noexcept
{
    v.x += adding_vector.x;
    v.y += adding_vector.y;
    return v;
}

constexpr vec2 operator*(const vec2& v, float scale) noexcept
{
    return vec2{ v.x * scale, v.y * scale };
}

constexpr vec2 operator/(const vec2& v, float divisor) noexcept
{
    return vec2(v.x / divisor, v.y / divisor);
}

constexpr vec2& operator*=(vec2& v, float scale) noexcept
{
    v.x *= scale;
    v.y *= scale;
    return v;
}

constexpr vec2& operator/=(vec2& v, float divisor) noexcept
{
    v.x /= divisor;
    v.y /= divisor;
    return v;
}

constexpr vec2 operator+(const vec2& v1, const vec2& v2) noexcept
{
    return vec2{ v1.x + v2.x, v1.y + v2.y };
}

constexpr vec2 operator-(const vec2& v1, const vec2& v2) noexcept
{
    return vec2{ v1.x - v2.x, v1.y - v2.y };
}

constexpr float dot(vec2 a, vec2 b) noexcept
{
    return a.x * b.x + a.y * b.y;
}

constexpr vec2 perpendicular_to(vec2 a) noexcept
{
    return vec2{ a.y, -a.x };
}

constexpr float magnitude_squared(vec2 a) noexcept
{
    return dot(a, a);
}
