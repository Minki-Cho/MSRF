#pragma once
#include "vec2.h"
#include "vec3.h"
#include <cassert>

template <typename T>
struct [[nodiscard]] mat3
{
public:
    union
    {
        T elements[3][3];
        struct
        {
            vec3 column0;
            vec3 column1;
            vec3 column2;
        };
    };

public:
    constexpr mat3() noexcept; // initializes to identity matrix
    constexpr mat3(vec3 first_column, vec3 second_column, vec3 third_column) noexcept;
    constexpr mat3(T column0_row0, T column0_row1, T column0_row2, T column1_row0, T column1_row1, T column1_row2, T column2_row0, T column2_row1,
        T column2_row2) noexcept;

public:
    static constexpr mat3 transpose(const mat3& m) noexcept;
    static constexpr mat3 build_scale(T scale) noexcept;
    static constexpr mat3 build_scale(T scale_x, T scale_y) noexcept;
    static constexpr mat3 build_scale(const vec2& scale) noexcept;
    static mat3           build_rotation(T angle_in_radians) noexcept;
    static constexpr mat3 build_translation(T translate_x, T translate_y) noexcept;
    static constexpr mat3 build_translation(const vec2& translation) noexcept;
};

//static_assert(sizeof(mat3) == sizeof(float) * 3 * 3, "mat3 should be 3x3 floats");

template <typename T>
constexpr mat3<T> operator*(const mat3<T>& m1, const mat3<T>& m2) noexcept;

template <typename T>
constexpr void operator*=(mat3<T>& m1, const mat3<T>& m2) noexcept;

template <typename T>
constexpr vec3 operator*(const mat3<T>& m, vec3 v) noexcept;

////////////////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
constexpr mat3<T>::mat3() noexcept : mat3<T>(vec3{ 1, 0, 0 }, vec3{ 0, 1, 0 }, vec3{ 0, 0, 1 })
{
}

template <typename T>
constexpr mat3<T>::mat3(vec3 first_column, vec3 second_column, vec3 third_column) noexcept : column0(first_column), column1(second_column), column2(third_column)
{
}

template <typename T>
constexpr mat3<T>::mat3(T column0_row0, T column0_row1, T column0_row2, T column1_row0, T column1_row1, T column1_row2, T column2_row0, T column2_row1,
    T column2_row2) noexcept
    : column0{ column0_row0, column0_row1, column0_row2 }, column1{ column1_row0, column1_row1, column1_row2 }, column2{ column2_row0, column2_row1, column2_row2 }
{
}

template <typename T>
constexpr mat3<T> operator*(const mat3<T>& m1, const mat3<T>& m2) noexcept
{
    mat3<T> results;
    results.elements[0][0] = m1.elements[0][0] * m2.elements[0][0] + m1.elements[1][0] * m2.elements[0][1] + m1.elements[2][0] * m2.elements[0][2];
    results.elements[0][1] = m1.elements[0][1] * m2.elements[0][0] + m1.elements[1][1] * m2.elements[0][1] + m1.elements[2][1] * m2.elements[0][2];
    results.elements[0][2] = m1.elements[0][2] * m2.elements[0][0] + m1.elements[1][2] * m2.elements[0][1] + m1.elements[2][2] * m2.elements[0][2];
    results.elements[1][0] = m1.elements[0][0] * m2.elements[1][0] + m1.elements[1][0] * m2.elements[1][1] + m1.elements[2][0] * m2.elements[1][2];
    results.elements[1][1] = m1.elements[0][1] * m2.elements[1][0] + m1.elements[1][1] * m2.elements[1][1] + m1.elements[2][1] * m2.elements[1][2];
    results.elements[1][2] = m1.elements[0][2] * m2.elements[1][0] + m1.elements[1][2] * m2.elements[1][1] + m1.elements[2][2] * m2.elements[1][2];
    results.elements[2][0] = m1.elements[0][0] * m2.elements[2][0] + m1.elements[1][0] * m2.elements[2][1] + m1.elements[2][0] * m2.elements[2][2];
    results.elements[2][1] = m1.elements[0][1] * m2.elements[2][0] + m1.elements[1][1] * m2.elements[2][1] + m1.elements[2][1] * m2.elements[2][2];
    results.elements[2][2] = m1.elements[0][2] * m2.elements[2][0] + m1.elements[1][2] * m2.elements[2][1] + m1.elements[2][2] * m2.elements[2][2];
    return results;
}

template <typename T>
constexpr void operator*=(mat3<T>& m1, const mat3<T>& m2) noexcept
{
    m1 = m1 * m2;
}

template <typename T>
constexpr vec3 operator*(const mat3<T>& m, vec3 v) noexcept
{
    return { m.elements[0][0] * v.x + m.elements[1][0] * v.y + m.elements[2][0] * v.z, m.elements[0][1] * v.x + m.elements[1][1] * v.y + m.elements[2][1] * v.z,
            m.elements[0][2] * v.x + m.elements[1][2] * v.y + m.elements[2][2] * v.z };
}

template <typename T>
constexpr mat3<T> mat3<T>::transpose(const mat3& m) noexcept
{
    return { {m.elements[0][0], m.elements[1][0], m.elements[2][0]}, {m.elements[0][1], m.elements[1][1], m.elements[2][1]}, {m.elements[0][2], m.elements[1][2], m.elements[2][2]} };
}

template <typename T>
constexpr mat3<T> mat3<T>::build_scale(T scale_x, T scale_y) noexcept
{
    return { scale_x, 0.0f, 0.0f, 0.0f, scale_y, 0.0f, 0.0f, 0.0f, 1.0f };
}

template <typename T>
constexpr mat3<T> mat3<T>::build_scale(T scale) noexcept
{
    return build_scale(scale, scale);
}

template <typename T>
constexpr mat3<T> mat3<T>::build_scale(const vec2& scale) noexcept
{
    return build_scale(scale.width, scale.height);
}

template <typename T>
constexpr mat3<T> mat3<T>::build_translation(T translate_x, T translate_y) noexcept
{
    return { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, translate_x, translate_y, 1.0f };
}

template <typename T>
constexpr mat3<T> mat3<T>::build_translation(const vec2& translation) noexcept
{
    return build_translation(translation.x, translation.y);
}
