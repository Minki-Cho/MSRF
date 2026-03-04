#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

class EventBus
{
public:
    using SubscriptionId = std::uint64_t;

    template<typename Event, typename Fn>
    SubscriptionId Subscribe(Fn&& fn)
    {
        const SubscriptionId id = nextSubscriptionId.fetch_add(1, std::memory_order_relaxed);
        HandlerEntry entry;
        entry.id = id;
        entry.callback = [handler = std::forward<Fn>(fn)](const void* payload) {
            handler(*static_cast<const Event*>(payload));
        };

        std::lock_guard<std::mutex> lock(handlerMutex);
        handlers[std::type_index(typeid(Event))].push_back(std::move(entry));
        return id;
    }

    template<typename Event>
    void Unsubscribe(SubscriptionId id)
    {
        std::lock_guard<std::mutex> lock(handlerMutex);
        auto found = handlers.find(std::type_index(typeid(Event)));
        if (found == handlers.end())
            return;

        auto& list = found->second;
        list.erase(
            std::remove_if(list.begin(), list.end(),
                [id](const HandlerEntry& entry) { return entry.id == id; }),
            list.end());
    }

    template<typename Event>
    void Publish(const Event& event)
    {
        QueuedEvent queued;
        queued.type = std::type_index(typeid(Event));
        queued.payload = std::make_shared<Event>(event);

        std::lock_guard<std::mutex> lock(queueMutex);
        queue.push(std::move(queued));
    }

    void DispatchQueued()
    {
        std::queue<QueuedEvent> localQueue;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            queue.swap(localQueue);
        }

        while (!localQueue.empty())
        {
            const QueuedEvent& e = localQueue.front();

            std::vector<HandlerEntry> listeners;
            {
                std::lock_guard<std::mutex> lock(handlerMutex);
                auto found = handlers.find(e.type);
                if (found != handlers.end())
                    listeners = found->second;
            }

            for (const auto& listener : listeners)
                listener.callback(e.payload.get());

            localQueue.pop();
        }
    }

    void Clear()
    {
        {
            std::lock_guard<std::mutex> lock(handlerMutex);
            handlers.clear();
        }
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            std::queue<QueuedEvent> empty;
            queue.swap(empty);
        }
    }

private:
    struct HandlerEntry
    {
        SubscriptionId id = 0;
        std::function<void(const void*)> callback;
    };

    struct QueuedEvent
    {
        std::type_index type{ typeid(void) };
        std::shared_ptr<const void> payload;
    };

    std::atomic<SubscriptionId> nextSubscriptionId{ 1 };

    std::mutex handlerMutex;
    std::unordered_map<std::type_index, std::vector<HandlerEntry>> handlers;

    std::mutex queueMutex;
    std::queue<QueuedEvent> queue;
};
