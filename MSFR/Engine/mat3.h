#pragma once
#include "vec2.h"
#include "vec3.h"
#include <cmath>
#include <cstddef>
#include <cassert>

template <typename T>
struct mat3
{
    // Column-major: e[c][r]
    T e[3][3]{};

    // ---- Accessors ----
    constexpr T& at(std::size_t c, std::size_t r) noexcept
    {
        assert(c < 3 && r < 3);
        return e[c][r];
    }
    constexpr T at(std::size_t c, std::size_t r) const noexcept
    {
        assert(c < 3 && r < 3);
        return e[c][r];
    }

    // Columns as vec3 (by value) + setters (no aliasing tricks)
    constexpr vec3 column0() const noexcept { return vec3{ T(e[0][0]), T(e[0][1]), T(e[0][2]) }; }
    constexpr vec3 column1() const noexcept { return vec3{ T(e[1][0]), T(e[1][1]), T(e[1][2]) }; }
    constexpr vec3 column2() const noexcept { return vec3{ T(e[2][0]), T(e[2][1]), T(e[2][2]) }; }

    constexpr void set_column0(const vec3& c) noexcept { e[0][0] = T(c.x()); e[0][1] = T(c.y()); e[0][2] = T(c.z()); }
    constexpr void set_column1(const vec3& c) noexcept { e[1][0] = T(c.x()); e[1][1] = T(c.y()); e[1][2] = T(c.z()); }
    constexpr void set_column2(const vec3& c) noexcept { e[2][0] = T(c.x()); e[2][1] = T(c.y()); e[2][2] = T(c.z()); }

    // ---- Ctors ----
    constexpr mat3() noexcept
        : mat3(vec3{ 1,0,0 }, vec3{ 0,1,0 }, vec3{ 0,0,1 })
    {
    }

    constexpr mat3(vec3 c0, vec3 c1, vec3 c2) noexcept
    {
        set_column0(c0);
        set_column1(c1);
        set_column2(c2);
    }

    constexpr mat3(T c0r0, T c0r1, T c0r2,
        T c1r0, T c1r1, T c1r2,
        T c2r0, T c2r1, T c2r2) noexcept
    {
        e[0][0] = c0r0; e[0][1] = c0r1; e[0][2] = c0r2;
        e[1][0] = c1r0; e[1][1] = c1r1; e[1][2] = c1r2;
        e[2][0] = c2r0; e[2][1] = c2r1; e[2][2] = c2r2;
    }

    // ---- Builders ----
    static constexpr mat3 transpose(const mat3& m) noexcept
    {
        return {
            m.e[0][0], m.e[1][0], m.e[2][0],
            m.e[0][1], m.e[1][1], m.e[2][1],
            m.e[0][2], m.e[1][2], m.e[2][2]
        };
    }

    static constexpr mat3 build_scale(T sx, T sy) noexcept
    {
        return { sx, T(0), T(0),
                 T(0), sy, T(0),
                 T(0), T(0), T(1) };
    }

    static constexpr mat3 build_scale(T s) noexcept { return build_scale(s, s); }

    static constexpr mat3 build_scale(const vec2& s) noexcept
    {
        return build_scale(T(s.width()), T(s.height()));
    }

    static mat3 build_rotation(T angle) noexcept
    {
        const T c = static_cast<T>(std::cos(angle));
        const T s = static_cast<T>(std::sin(angle));

        return { c, -s, T(0),
                 s,  c, T(0),
                 T(0), T(0), T(1) };
    }

    static constexpr mat3 build_translation(T tx, T ty) noexcept
    {
        return { T(1), T(0), T(0),
                 T(0), T(1), T(0),
                 tx,   ty,   T(1) };
    }

    static constexpr mat3 build_translation(const vec2& t) noexcept
    {
        return build_translation(T(t.x()), T(t.y()));
    }
};

// ---- Operators ----
template <typename T>
constexpr mat3<T> operator*(const mat3<T>& a, const mat3<T>& b) noexcept
{
    mat3<T> r;

    // r[c][r] = sum_k a[k][r] * b[c][k]  (column-major multiply)
    for (std::size_t c = 0; c < 3; ++c)
    {
        for (std::size_t row = 0; row < 3; ++row)
        {
            T sum = T(0);
            for (std::size_t k = 0; k < 3; ++k)
            {
                sum += a.e[k][row] * b.e[c][k];
            }
            r.e[c][row] = sum;
        }
    }
    return r;
}

template <typename T>
constexpr void operator*=(mat3<T>& a, const mat3<T>& b) noexcept
{
    a = a * b;
}

template <typename T>
constexpr vec3 operator*(const mat3<T>& m, vec3 v) noexcept
{
    return {
        T(m.e[0][0]) * v.x() + T(m.e[1][0]) * v.y() + T(m.e[2][0]) * v.z(),
        T(m.e[0][1]) * v.x() + T(m.e[1][1]) * v.y() + T(m.e[2][1]) * v.z(),
        T(m.e[0][2]) * v.x() + T(m.e[1][2]) * v.y() + T(m.e[2][2]) * v.z()
    };
}
inline mat3<float> PixelToNdc(float w, float h)
{
    // column-major e[c][r]
    // [ 2/w    0   -1 ]
    // [  0   -2/h  +1 ]
    // [  0     0    1 ]
    return mat3<float>(
        2.0f / w, 0.0f, 0.0f,
        0.0f, -2.0f / h, 0.0f,
        -1.0f, 1.0f, 1.0f
    );
}
