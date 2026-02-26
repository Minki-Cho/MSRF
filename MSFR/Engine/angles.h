#pragma once

namespace util
{
    template <typename FLOAT> constexpr FLOAT PI = static_cast<FLOAT>(3.1415926535897932384626433832795028);
    template <typename FLOAT> constexpr FLOAT HALF_PI = PI<FLOAT> / 2;
    template <typename FLOAT> constexpr FLOAT QUARTER_PI = PI<FLOAT> / 4;
    template <typename FLOAT> constexpr FLOAT TWO_PI = 2 * PI<FLOAT>;

    template <typename FLOAT> constexpr FLOAT to_radians(FLOAT angle_in_degrees)
    {
        return angle_in_degrees * PI<FLOAT> / 180;
    }
    template <typename FLOAT> constexpr FLOAT to_degrees(FLOAT angle_in_radians)
    {
        return angle_in_radians * 180 / PI<FLOAT>;
    }
}
