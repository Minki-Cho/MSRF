#pragma once
#include "vec2.h"
#include "vec3.h"
#include <cmath>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4201)
#endif
template <typename T>
struct mat3
{
public:
    union Storage
    {
        T elements[3][3];
        struct Columns
        {
            vec3 column0;
            vec3 column1;
            vec3 column2;
        } cols;

        constexpr Storage() : elements{} {}
    };

    Storage storage;

    vec3& column0() noexcept { return storage.cols.column0; }
    vec3& column1() noexcept { return storage.cols.column1; }
    vec3& column2() noexcept { return storage.cols.column2; }
    const vec3& column0() const noexcept { return storage.cols.column0; }
    const vec3& column1() const noexcept { return storage.cols.column1; }
    const vec3& column2() const noexcept { return storage.cols.column2; }

public:
    constexpr mat3() noexcept
        : mat3(vec3{ 1,0,0 }, vec3{ 0,1,0 }, vec3{ 0,0,1 })
    {
    }

    constexpr mat3(vec3 first_column, vec3 second_column, vec3 third_column) noexcept
    {
        storage.cols.column0 = first_column;
        storage.cols.column1 = second_column;
        storage.cols.column2 = third_column;
    }

    constexpr mat3(T c0r0, T c0r1, T c0r2,
        T c1r0, T c1r1, T c1r2,
        T c2r0, T c2r1, T c2r2) noexcept
    {
        storage.cols.column0 = { c0r0, c0r1, c0r2 };
        storage.cols.column1 = { c1r0, c1r1, c1r2 };
        storage.cols.column2 = { c2r0, c2r1, c2r2 };
    }

public:
    static constexpr mat3 transpose(const mat3& m) noexcept
    {
        return {
            { m.storage.elements[0][0], m.storage.elements[1][0], m.storage.elements[2][0] },
            { m.storage.elements[0][1], m.storage.elements[1][1], m.storage.elements[2][1] },
            { m.storage.elements[0][2], m.storage.elements[1][2], m.storage.elements[2][2] }
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
        return build_scale(T(s.width), T(s.height));
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
        return build_translation(T(t.x), T(t.y));
    }
};

template <typename T>
constexpr mat3<T> operator*(const mat3<T>& a, const mat3<T>& b) noexcept
{
    mat3<T> r;

    r.storage.elements[0][0] = a.storage.elements[0][0] * b.storage.elements[0][0] + a.storage.elements[1][0] * b.storage.elements[0][1] + a.storage.elements[2][0] * b.storage.elements[0][2];
    r.storage.elements[0][1] = a.storage.elements[0][1] * b.storage.elements[0][0] + a.storage.elements[1][1] * b.storage.elements[0][1] + a.storage.elements[2][1] * b.storage.elements[0][2];
    r.storage.elements[0][2] = a.storage.elements[0][2] * b.storage.elements[0][0] + a.storage.elements[1][2] * b.storage.elements[0][1] + a.storage.elements[2][2] * b.storage.elements[0][2];

    r.storage.elements[1][0] = a.storage.elements[0][0] * b.storage.elements[1][0] + a.storage.elements[1][0] * b.storage.elements[1][1] + a.storage.elements[2][0] * b.storage.elements[1][2];
    r.storage.elements[1][1] = a.storage.elements[0][1] * b.storage.elements[1][0] + a.storage.elements[1][1] * b.storage.elements[1][1] + a.storage.elements[2][1] * b.storage.elements[1][2];
    r.storage.elements[1][2] = a.storage.elements[0][2] * b.storage.elements[1][0] + a.storage.elements[1][2] * b.storage.elements[1][1] + a.storage.elements[2][2] * b.storage.elements[1][2];

    r.storage.elements[2][0] = a.storage.elements[0][0] * b.storage.elements[2][0] + a.storage.elements[1][0] * b.storage.elements[2][1] + a.storage.elements[2][0] * b.storage.elements[2][2];
    r.storage.elements[2][1] = a.storage.elements[0][1] * b.storage.elements[2][0] + a.storage.elements[1][1] * b.storage.elements[2][1] + a.storage.elements[2][1] * b.storage.elements[2][2];
    r.storage.elements[2][2] = a.storage.elements[0][2] * b.storage.elements[2][0] + a.storage.elements[1][2] * b.storage.elements[2][1] + a.storage.elements[2][2] * b.storage.elements[2][2];

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
        m.storage.elements[0][0] * v.x + m.storage.elements[1][0] * v.y + m.storage.elements[2][0] * v.z,
        m.storage.elements[0][1] * v.x + m.storage.elements[1][1] * v.y + m.storage.elements[2][1] * v.z,
        m.storage.elements[0][2] * v.x + m.storage.elements[1][2] * v.y + m.storage.elements[2][2] * v.z
    };
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif