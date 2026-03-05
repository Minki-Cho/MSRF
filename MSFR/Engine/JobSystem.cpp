#include "JobSystem.h"

#include <algorithm>
#include <chrono>

void JobSystem::Init(uint32_t workerCount)
{
    if (running.load(std::memory_order_acquire))
        return;

    // leave one core for main thread when possible
    if (workerCount == 0)
    {
        const uint32_t hc = std::max(1u, std::thread::hardware_concurrency());
        workerCount = (hc > 1) ? (hc - 1) : 1;
    }

    running.store(true, std::memory_order_release);

    workers.clear();
    workers.reserve(workerCount);

    workerStats.clear();
    workerStats.reserve(workerCount);

    for (uint32_t i = 0; i < workerCount; ++i)
    {
        workerStats.push_back(std::make_unique<WorkerStats>());
        workers.emplace_back([this, i] { WorkerLoop(i); });
    }
}

void JobSystem::Shutdown()
{
    const bool wasRunning = running.exchange(false, std::memory_order_acq_rel);
    if (!wasRunning)
        return;

    // Wake all workers
    cvWork.notify_all();

    for (auto& t : workers)
    {
        if (t.joinable())
            t.join();
    }
    workers.clear();

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::queue<QueueItem> empty;
        queue.swap(empty);
    }

    pendingJobs.store(0, std::memory_order_release);
    workerStats.clear();
}

void JobSystem::Enqueue(std::function<void()> job, const char* label)
{
    if (!job)
        return;

    // If no workers, run immediately on caller.
    if (workers.empty())
    {
        job();
        return;
    }

    pendingJobs.fetch_add(1, std::memory_order_relaxed);

    auto wrapped = [this, job = std::move(job)]() mutable {
        job();
        FinishOneJob();
        };

    QueueItem item;
    item.fn = std::move(wrapped);
    item.label = (label && *label) ? label : "Generic";

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.push(std::move(item));
    }

    cvWork.notify_one();
}

void JobSystem::WaitIdle()
{
    if (workers.empty())
        return;

    std::unique_lock<std::mutex> lock(idleMutex);
    cvIdle.wait(lock, [this] {
        return pendingJobs.load(std::memory_order_acquire) == 0;
        });
}

std::vector<JobSystem::WorkerStatSnapshot> JobSystem::GetWorkerStatsSnapshot() const
{
    std::vector<WorkerStatSnapshot> out;
    out.reserve(workerStats.size());

    for (uint32_t i = 0; i < static_cast<uint32_t>(workerStats.size()); ++i)
    {
        const WorkerStats* ws = workerStats[i].get();
        if (!ws)
            continue;

        const uint64_t completed = ws->completedJobs.load(std::memory_order_relaxed);
        const uint64_t busyNs = ws->busyTimeNs.load(std::memory_order_relaxed);
        const uint64_t lastNs = ws->lastJobNs.load(std::memory_order_relaxed);
        const bool busy = ws->activeJobs.load(std::memory_order_relaxed) > 0;

        WorkerStatSnapshot s;
        s.workerIndex = i;
        s.completedJobs = completed;
        s.totalBusyMs = static_cast<double>(busyNs) / 1000000.0;
        s.avgJobMs = (completed > 0) ? (s.totalBusyMs / static_cast<double>(completed)) : 0.0;
        s.lastJobMs = static_cast<double>(lastNs) / 1000000.0;
        s.busy = busy;

        {
            std::lock_guard<std::mutex> lock(ws->labelMutex);
            s.activeTask = ws->activeTask;
            s.lastTask = ws->lastTask;
        }

        out.push_back(std::move(s));
    }

    return out;
}

void JobSystem::WorkerLoop(uint32_t workerIndex)
{
    WorkerStats* stats = nullptr;
    if (workerIndex < workerStats.size())
        stats = workerStats[workerIndex].get();

    while (running.load(std::memory_order_acquire))
    {
        QueueItem item;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cvWork.wait(lock, [this] {
                return !queue.empty() || !running.load(std::memory_order_acquire);
                });

            if (!running.load(std::memory_order_acquire))
                return;

            item = std::move(queue.front());
            queue.pop();
        }

        if (item.fn)
        {
            using Clock = std::chrono::steady_clock;

            if (stats)
            {
                stats->activeJobs.fetch_add(1, std::memory_order_relaxed);
                std::lock_guard<std::mutex> lock(stats->labelMutex);
                stats->activeTask = item.label;
            }

            const Clock::time_point begin = Clock::now();
            item.fn();
            const uint64_t elapsedNs = static_cast<uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - begin).count());

            if (stats)
            {
                stats->completedJobs.fetch_add(1, std::memory_order_relaxed);
                stats->busyTimeNs.fetch_add(elapsedNs, std::memory_order_relaxed);
                stats->lastJobNs.store(elapsedNs, std::memory_order_relaxed);
                stats->activeJobs.fetch_sub(1, std::memory_order_relaxed);

                std::lock_guard<std::mutex> lock(stats->labelMutex);
                stats->lastTask = item.label;
                stats->activeTask = "Idle";
            }
        }
    }
}

void JobSystem::FinishOneJob()
{
    const uint32_t left = pendingJobs.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (left == 0)
    {
        std::lock_guard<std::mutex> lock(idleMutex);
        cvIdle.notify_all();
    }
}
