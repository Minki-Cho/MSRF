// JobSystem.h
#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

class JobSystem
{
public:
    struct WorkerStatSnapshot
    {
        uint32_t workerIndex = 0;
        uint64_t completedJobs = 0;
        double totalBusyMs = 0.0;
        double avgJobMs = 0.0;
        double lastJobMs = 0.0;
        bool busy = false;
        std::string activeTask;
        std::string lastTask;
    };

    JobSystem() = default;
    ~JobSystem() { Shutdown(); }

    JobSystem(const JobSystem&) = delete;
    JobSystem& operator=(const JobSystem&) = delete;
    JobSystem(JobSystem&&) = delete;
    JobSystem& operator=(JobSystem&&) = delete;

    void Init(uint32_t workerCount = 0);
    void Shutdown();

    void Enqueue(std::function<void()> job, const char* label = "Generic");
    void WaitIdle();

    template <typename Fn>
    void Dispatch(uint32_t count, uint32_t chunkSize, Fn&& fn, const char* label = "Dispatch")
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
                }, label);
        }
    }

    uint32_t GetWorkerCount() const noexcept { return static_cast<uint32_t>(workers.size()); }
    uint32_t GetPendingJobs() const noexcept { return pendingJobs.load(std::memory_order_acquire); }
    std::vector<WorkerStatSnapshot> GetWorkerStatsSnapshot() const;

private:
    struct WorkerStats
    {
        std::atomic<uint64_t> completedJobs{ 0 };
        std::atomic<uint64_t> busyTimeNs{ 0 };
        std::atomic<uint64_t> lastJobNs{ 0 };
        std::atomic<uint32_t> activeJobs{ 0 };

        mutable std::mutex labelMutex;
        std::string activeTask = "Idle";
        std::string lastTask = "None";
    };

    struct QueueItem
    {
        std::function<void()> fn;
        std::string label;
    };

    void WorkerLoop(uint32_t workerIndex);
    void FinishOneJob();

private:
    std::vector<std::thread> workers;

    std::mutex queueMutex;
    std::condition_variable cvWork;
    std::queue<QueueItem> queue;

    std::mutex idleMutex;
    std::condition_variable cvIdle;

    std::atomic<bool> running{ false };
    std::atomic<uint32_t> pendingJobs{ 0 }; // jobs not yet finished

    std::vector<std::unique_ptr<WorkerStats>> workerStats;
};
