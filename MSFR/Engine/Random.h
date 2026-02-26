#pragma once

namespace util
{
    void  random_seed(unsigned int seed) noexcept;
    float random(float min_inclusive, float max_exclusive) noexcept;
    float random(float max_exclusive) noexcept;
    float random() noexcept;
    int   random(int min_inclusive, int max_exclusive) noexcept;
    int   random(int max_exclusive) noexcept;
}
