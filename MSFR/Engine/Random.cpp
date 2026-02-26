#include "Random.h"
#include <cassert>
#include <chrono>
#include <random>
#include <thread>


namespace
{
    using namespace std;
    using namespace chrono;
    inline uint64_t mix64(uint64_t x) {
        x += 0x9e3779b97f4a7c15ull;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
        return x ^ (x >> 31);
    }
    // More powerful random (splitmix64 mixer)
    thread_local mt19937_64 Engine = [] {
        uint64_t t = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
        uint64_t tid = (uint64_t)std::hash<std::thread::id>{}(std::this_thread::get_id());

        uint64_t seed64 = mix64(t ^ (tid + 0x9e3779b97f4a7c15ull));

        std::mt19937_64 e;
        e.seed(seed64);
        return e;
        }();
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
