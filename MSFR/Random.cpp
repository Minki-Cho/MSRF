#include "Random.h"
#include <cassert>
#include <chrono>
#include <random>
#include <thread>


namespace
{
    using namespace std;
    using namespace chrono;
    thread_local mt19937_64 Engine{ static_cast<unsigned long long>(system_clock::now().time_since_epoch().count()) + hash<thread::id>{}(this_thread::get_id()) };
}

namespace util
{
    void random_seed(unsigned int seed) noexcept
    {
        Engine.seed(seed);
    }

    float random(float min_inclusive, float max_exclusive) noexcept
    {
        assert(min_inclusive < max_exclusive);
        return std::uniform_real_distribution<float>(min_inclusive, max_exclusive)(Engine);
    }

    float random(float max_exclusive) noexcept
    {
        return random(0.0f, max_exclusive);
    }

    float random() noexcept
    {
        return random(1.0f);
    }

    int random(int min_inclusive, int max_exclusive) noexcept
    {
        assert(min_inclusive < max_exclusive);
        return std::uniform_int_distribution<int>(min_inclusive, max_exclusive - 1)(Engine);
    }

    int random(int max_exclusive) noexcept
    {
        return random(0, max_exclusive);
    }
}
