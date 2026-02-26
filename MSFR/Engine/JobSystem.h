// JobSystem.h
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <memory>
#include <type_traits>
#include <thread>
#include <vector>

class JobSystem
{
public:
    JobSystem() = default;
    ~JobSystem() { Shutdown(); }

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;
    JobSystem(JobSystem&&) = delete;
    JobSystem& operator=(JobSystem&&) = delete;

    void Init(uint32_t workerCount = 0);
    void Shutdown();

    void Enqueue(std::function<void()> job);
    void WaitIdle();

    template <typename Fn>
    void Dispatch(uint32_t count, uint32_t chunkSize, Fn&& fn)
    {
        if (count == 0) return;
        if (chunkSize == 0) chunkSize = 1;

        // If no workers, just run on caller.
        if (workers.empty())
        {
            for (uint32_t i = 0; i < count; ++i)
                fn(i);
            return;
        }

        const uint32_t jobCount = (count + chunkSize - 1) / chunkSize;

        using FnT = std::decay_t<Fn>;
        auto fnPtr = std::make_shared<FnT>(std::forward<Fn>(fn));

        for (uint32_t j = 0; j < jobCount; ++j)
        {
            const uint32_t begin = j * chunkSize;
            const uint32_t end = (std::min)(count, begin + chunkSize);

            Enqueue([begin, end, fnPtr]() mutable {
                for (uint32_t i = begin; i < end; ++i)
                    (*fnPtr)(i);
                });
        }
    }

    uint32_t GetWorkerCount() const noexcept { return static_cast<uint32_t>(workers.size()); }

private:
    void WorkerLoop();
    void FinishOneJob();

private:
    std::vector<std::thread> workers;

    std::mutex queueMutex;
    std::condition_variable cvWork;
    std::queue<std::function<void()>> queue;

    std::mutex idleMutex;
    std::condition_variable cvIdle;

    std::atomic<bool> running{ false };
    std::atomic<uint32_t> pendingJobs{ 0 }; // jobs not yet finished
};
