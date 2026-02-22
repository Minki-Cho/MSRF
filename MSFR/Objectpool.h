#pragma once
#include <array>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>
#include <cassert>

struct NoLock {
    void lock() noexcept {}
    void unlock() noexcept {}
};

#include <mutex>
struct MutexLock {
    std::mutex m;
    void lock() noexcept { m.lock(); }
    void unlock() noexcept { m.unlock(); }
};

template <typename T, std::size_t Capacity, typename LockPolicy = NoLock>
class ObjectPool
{
    static_assert(Capacity > 0, "ObjectPool Capacity must be > 0");
    static_assert(std::is_destructible_v<T>, "T must be destructible");

public:
    ObjectPool()
    {
        for (std::size_t i = 0; i < Capacity - 1; ++i)
            m_next[i] = i + 1;
        m_next[Capacity - 1] = kInvalid;
        m_freeHead = 0;

#ifndef NDEBUG
        m_alive.fill(false);
#endif
    }

    ~ObjectPool()
    {
        Clear();
    }

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    ObjectPool(ObjectPool&&) = delete;
    ObjectPool& operator=(ObjectPool&&) = delete;

    template <typename... Args>
    T* Allocate(Args&&... args)
    {
        LockGuard guard(m_lock);

        if (m_freeHead == kInvalid)
            return nullptr; // pool exhausted

        const std::size_t index = m_freeHead;
        m_freeHead = m_next[index];

        void* mem = static_cast<void*>(&m_storage[index]);
        T* obj = ::new (mem) T(std::forward<Args>(args)...);

#ifndef NDEBUG
        assert(m_alive[index] == false && "ObjectPool: allocating an already alive slot");
        m_alive[index] = true;
#endif
        ++m_inUse;
        return obj;
    }

    void Free(T* ptr)
    {
        if (!ptr) return;

        LockGuard guard(m_lock);

        const std::size_t index = IndexOf(ptr);
#ifndef NDEBUG
        assert(m_alive[index] == true && "ObjectPool: double free or freeing foreign pointer");
        m_alive[index] = false;
#endif
        // Call destructor explicitly
        ptr->~T();

        // Push back into free list
        m_next[index] = m_freeHead;
        m_freeHead = index;

        assert(m_inUse > 0);
        --m_inUse;
    }

    void Clear()
    {
        LockGuard guard(m_lock);

        // Destroy all alive objects, rebuild freelist
#ifndef NDEBUG
        for (std::size_t i = 0; i < Capacity; ++i)
        {
            if (m_alive[i])
            {
                T* obj = reinterpret_cast<T*>(&m_storage[i]);
                obj->~T();
                m_alive[i] = false;
            }
        }
#else
        // In release, we don't know which are alive unless we track.
#endif

        for (std::size_t i = 0; i < Capacity - 1; ++i)
            m_next[i] = i + 1;
        m_next[Capacity - 1] = kInvalid;
        m_freeHead = 0;
        m_inUse = 0;
    }

    [[nodiscard]] std::size_t InUse() const noexcept { return m_inUse; }
    [[nodiscard]] constexpr std::size_t Max() const noexcept { return Capacity; }
    [[nodiscard]] std::size_t Available() const noexcept { return Capacity - m_inUse; }

    // Debug helper: is this pointer from this pool?
    [[nodiscard]] bool Owns(const T* ptr) const noexcept
    {
        auto p = reinterpret_cast<const std::byte*>(ptr);
        auto base = reinterpret_cast<const std::byte*>(&m_storage[0]);
        auto end = reinterpret_cast<const std::byte*>(&m_storage[Capacity]);

        return (p >= base) && (p < end);
    }

private:
    static constexpr std::size_t kInvalid = static_cast<std::size_t>(-1);

    // aligned raw storage for T
    using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>;
    std::array<Storage, Capacity> m_storage{};
    std::array<std::size_t, Capacity> m_next{};

#ifndef NDEBUG
    std::array<bool, Capacity> m_alive{};
#endif

    std::size_t m_freeHead = kInvalid;
    std::size_t m_inUse = 0;

    // lock policy + guard
    LockPolicy m_lock;

    struct LockGuard {
        LockPolicy& lock;
        explicit LockGuard(LockPolicy& l) : lock(l) { lock.lock(); }
        ~LockGuard() { lock.unlock(); }
    };

    std::size_t IndexOf(const T* ptr) const
    {
        const auto base = reinterpret_cast<const std::byte*>(&m_storage[0]);
        const auto p = reinterpret_cast<const std::byte*>(ptr);

        const std::ptrdiff_t diff = p - base;
        assert(diff >= 0 && "ObjectPool: pointer below storage base");
        assert((diff % sizeof(Storage)) == 0 && "ObjectPool: pointer not aligned to slot");

        const std::size_t index = static_cast<std::size_t>(diff / sizeof(Storage));
        assert(index < Capacity && "ObjectPool: pointer out of range");
        return index;
    }
};
