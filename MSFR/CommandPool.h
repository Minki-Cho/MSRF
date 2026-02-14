#pragma once
#include <array>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>
#include <cassert>

class ICommand;

template<std::size_t Capacity, std::size_t MaxBytes, std::size_t Align = alignof(std::max_align_t)>
class CommandPool
{
public:
    CommandPool()
    {
        for (std::size_t i = 0; i < Capacity - 1; ++i) next[i] = i + 1;
        next[Capacity - 1] = kInvalid;
        freeHead = 0;

#ifndef NDEBUG
        alive.fill(false);
#endif
    }

    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;

    template<typename Cmd, typename... Args>
    Cmd* Create(Args&&... args)
    {
        static_assert(std::is_base_of_v<ICommand, Cmd>, "Cmd must derive from ICommand");
        static_assert(sizeof(Cmd) <= MaxBytes, "Cmd too large for CommandPool slot");
        static_assert(alignof(Cmd) <= Align, "Cmd alignment too large for CommandPool slot");

        if (freeHead == kInvalid) return nullptr;

        const std::size_t idx = freeHead;
        freeHead = next[idx];

#ifndef NDEBUG
        assert(!alive[idx] && "CommandPool: allocating an already-alive slot");
        alive[idx] = true;
#endif

        Slot& s = slots[idx];
        s.destroyFn = [](void* p) { static_cast<Cmd*>(p)->~Cmd(); };

        void* mem = static_cast<void*>(s.storage.data());
        Cmd* cmd = ::new (mem) Cmd(std::forward<Args>(args)...);

        ++inUse;
        return cmd;
    }

    void Destroy(ICommand* cmd)
    {
        if (!cmd) return;

        const std::size_t idx = IndexOf(cmd);

#ifndef NDEBUG
        assert(alive[idx] && "CommandPool: double free or foreign pointer");
        alive[idx] = false;
#endif

        Slot& s = slots[idx];
        assert(s.destroyFn && "CommandPool: missing destroy function");
        s.destroyFn(static_cast<void*>(s.storage.data()));
        s.destroyFn = nullptr;

        next[idx] = freeHead;
        freeHead = idx;

        assert(inUse > 0);
        --inUse;
    }

    std::size_t InUse() const noexcept { return inUse; }
    std::size_t Available() const noexcept { return Capacity - inUse; }

    bool Owns(const ICommand* cmd) const noexcept
    {
        auto p = reinterpret_cast<const std::byte*>(cmd);
        auto base = reinterpret_cast<const std::byte*>(&slots[0]);
        auto end = reinterpret_cast<const std::byte*>(&slots[Capacity]);
        return p >= base && p < end;
    }

private:
    static constexpr std::size_t kInvalid = static_cast<std::size_t>(-1);

    struct Slot
    {
        alignas(Align) std::array<std::byte, MaxBytes> storage{};
        void (*destroyFn)(void*) = nullptr;
    };

    std::array<Slot, Capacity> slots{};
    std::array<std::size_t, Capacity> next{};
#ifndef NDEBUG
    std::array<bool, Capacity> alive{};
#endif

    std::size_t freeHead = kInvalid;
    std::size_t inUse = 0;

    std::size_t IndexOf(const ICommand* cmd) const
    {
        const auto base = reinterpret_cast<const std::byte*>(&slots[0]);
        const auto p = reinterpret_cast<const std::byte*>(cmd);

        const std::ptrdiff_t diff = p - base;
        assert(diff >= 0);

        const std::size_t slotSize = sizeof(Slot);
        const std::size_t idx = static_cast<std::size_t>(diff / slotSize);
        assert(idx < Capacity);

        const auto slotBase = reinterpret_cast<const std::byte*>(&slots[idx]);
        const auto storageBase = reinterpret_cast<const std::byte*>(slots[idx].storage.data());
        const auto storageEnd = storageBase + MaxBytes;

        assert(reinterpret_cast<const std::byte*>(cmd) >= storageBase &&
            reinterpret_cast<const std::byte*>(cmd) < storageEnd &&
            "CommandPool: pointer not inside slot storage");

        return idx;
    }
};
