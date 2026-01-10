namespace math
{
    //--------------------------------------------------------------
    //vec2
    //Can do v += v
    constexpr vec2 vec2::operator+=(const vec2& v) noexcept
    {
        (*this).x = (*this).x + v.x;
        (*this).y = (*this).y + v.y;
        return *this;
    }
    //Can do v -= v
    constexpr vec2 vec2::operator-=(const vec2& v) noexcept
    {
        (*this).x = (*this).x - v.x;
        (*this).y = (*this).y - v.y;
        return *this;
    }
    //Vector *= Number
    constexpr vec2 vec2::operator*=(const double& scalar) noexcept
    {
        (*this).x = (*this).x * scalar;
        (*this).y = (*this).y * scalar;
        return *this;
    }
    //Vector /= Number
    constexpr vec2 vec2::operator/=(const double& divider) noexcept
    {
        (*this).x = (*this).x / divider;
        (*this).y = (*this).y / divider;
        return *this;
    }
    //Can do v + v
    [[nodiscard]] constexpr vec2 operator+(const vec2& v1, const vec2& v2) noexcept
    {
        vec2 result;
        result.x = v1.x + v2.x;
        result.y = v1.y + v2.y;
        return result;
    }
    //Can do v - v
    [[nodiscard]] constexpr vec2 operator-(const vec2& v1, const vec2& v2) noexcept
    {
        vec2 result;
        result.x = v1.x - v2.x;
        result.y = v1.y - v2.y;
        return result;
    }
    //can do -v(negate)
    [[nodiscard]] constexpr vec2 operator-(const vec2& v) noexcept
    {
        vec2 result;
        result.x = -v.x;
        result.y = -v.y;
        return result;
    }
    //Number * Vector => Vector
    [[nodiscard]] constexpr vec2 operator*(const double& scalar, const vec2& v) noexcept
    {
        vec2 result;
        result.x = scalar * v.x;
        result.y = scalar * v.y;
        return result;
    }
    //Vector * Number => Vector
    [[nodiscard]] constexpr vec2 operator*(const vec2& v, const double& scalar) noexcept
    {
        vec2 result;
        result.x = v.x * scalar;
        result.y = v.y * scalar;
        return result;
    }
    //Vector / Number => Vector
    [[nodiscard]] constexpr vec2 operator/(const vec2& v, const double& divider) noexcept
    {
        vec2 result;
        result.x = v.x / divider;
        result.y = v.y / divider;
        return result;
    }
    //Can do v1 == v2
    [[nodiscard]] constexpr bool operator==(const vec2& v1, const vec2& v2) noexcept
    {
        bool result = is_equal(v1.x, v2.x) && is_equal(v1.y, v2.y);
        return result;
    }
    //Can do v1 != v2
    [[nodiscard]] constexpr bool operator!=(const vec2& v1, const vec2& v2) noexcept
    {
        bool result = !is_equal(v1.x, v2.x) || !is_equal(v1.y, v2.y);
        return result;
    }
    //Has a LengthSquared() method
    [[nodiscard]] constexpr double vec2::LengthSquared() noexcept
    {
        return (*this).x * (*this).x + (*this).y * (*this).y;
    }
    //Has a Normalize() method
    [[nodiscard]] inline vec2 vec2::Normalize() noexcept
    {
        return  (*this) / sqrt(LengthSquared());
    }
    //--------------------------------------------------------------

    //--------------------------------------------------------------
    //ivec2
    //Can do v += v
    constexpr ivec2 ivec2::operator+=(const ivec2& v) noexcept
    {
        (*this).x = (*this).x + v.x;
        (*this).y = (*this).y + v.y;
        return *this;
    }
    //Can do v -= v
    constexpr ivec2 ivec2::operator-=(const ivec2& v) noexcept
    {
        (*this).x = (*this).x - v.x;
        (*this).y = (*this).y - v.y;
        return *this;
    }
    //Vector *= int
    constexpr ivec2 ivec2::operator*=(const int& scalar) noexcept
    {
        (*this).x = (*this).x * scalar;
        (*this).y = (*this).y * scalar;
        return *this;
    }
    //Vector /= int
    constexpr ivec2 ivec2::operator/=(const int& divider) noexcept
    {
        (*this).x = (*this).x / divider;
        (*this).y = (*this).y / divider;
        return *this;
    }
    //Can do v + v
    [[nodiscard]] constexpr ivec2 operator+(const ivec2& v1, const ivec2& v2) noexcept
    {
        ivec2 result;
        result.x = v1.x + v2.x;
        result.y = v1.y + v2.y;
        return result;
    }
    //Can do v - v
    [[nodiscard]] constexpr ivec2 operator-(const ivec2& v1, const ivec2& v2) noexcept
    {
        ivec2 result;
        result.x = v1.x - v2.x;
        result.y = v1.y - v2.y;
        return result;
    }
    //can do -v(negate)
    [[nodiscard]] constexpr ivec2 operator-(const ivec2& v) noexcept
    {
        ivec2 result;
        result.x = -v.x;
        result.y = -v.y;
        return result;
    }
    //double * ivec2 => vec2
    [[nodiscard]] constexpr vec2 operator*(const double& scalar, const ivec2& v) noexcept
    {
        vec2 result;
        result.x = scalar * v.x;
        result.y = scalar * v.y;
        return result;
    }
    //ivec2 * double => vec2
    [[nodiscard]] constexpr vec2 operator*(const ivec2& v, const double& scalar) noexcept
    {
        vec2 result;
        result.x = v.x * scalar;
        result.y = v.y * scalar;
        return result;
    }
    //ivec2 / double => vec2
    [[nodiscard]] constexpr vec2 operator/(const ivec2& v, const double& divider) noexcept
    {
        vec2 result;
        result.x = v.x / divider;
        result.y = v.y / divider;
        return result;
    }
    //int * Vector => Vector
    [[nodiscard]] constexpr ivec2 operator*(const int& scalar, const ivec2& v) noexcept
    {
        ivec2 result;
        result.x = scalar * v.x;
        result.y = scalar * v.y;
        return result;
    }
    //Vector * int => Vector
    [[nodiscard]] constexpr ivec2 operator*(const ivec2& v, const int& scalar) noexcept
    {
        ivec2 result;
        result.x = v.x * scalar;
        result.y = v.y * scalar;
        return result;
    }
    //Vector / int => Vector
    [[nodiscard]] constexpr ivec2 operator/(const ivec2& v, const int& divider) noexcept
    {
        ivec2 result;
        result.x = v.x / divider;
        result.y = v.y / divider;
        return result;
    }
    //Can do v1 == v2
    [[nodiscard]] constexpr bool operator==(const ivec2& v1, const ivec2& v2) noexcept
    {
        bool result = is_equal(v1.x, v2.x) && is_equal(v1.y, v2.y);
        return result;
    }
    //Can do v1 != v2
    [[nodiscard]] constexpr bool operator!=(const ivec2& v1, const ivec2& v2) noexcept
    {
        bool result = !is_equal(v1.x, v2.x) || !is_equal(v1.y, v2.y);
        return result;
    }
    //conversion operator
    [[nodiscard]] constexpr ivec2::operator vec2() noexcept
    {
        vec2 convert;
        convert.x = static_cast<double>((*this).x);
        convert.y = static_cast<double>((*this).y);
        return convert;
    }
    //--------------------------------------------------------------
}