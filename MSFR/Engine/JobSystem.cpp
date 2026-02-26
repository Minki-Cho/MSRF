#include "JobSystem.h"

#include <algorithm>

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

    for (uint32_t i = 0; i < workerCount; ++i)
    {
        workers.emplace_back([this] { WorkerLoop(); });
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

    // Clear queue. If you want strict shutdown, call WaitIdle() before Shutdown().
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        std::queue<std::function<void()>> empty;
        queue.swap(empty);
    }

    pendingJobs.store(0, std::memory_order_release);
}

void JobSystem::Enqueue(std::function<void()> job)
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

    // Wrap so we always decrement pendingJobs exactly once.
    auto wrapped = [this, job = std::move(job)]() mutable {
        job();
        FinishOneJob();
        };

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        queue.push(std::move(wrapped));
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

void JobSystem::WorkerLoop()
{
    while (running.load(std::memory_order_acquire))
    {
        std::function<void()> job;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cvWork.wait(lock, [this] {
                return !queue.empty() || !running.load(std::memory_order_acquire);
                });

            if (!running.load(std::memory_order_acquire))
                return;

            job = std::move(queue.front());
            queue.pop();
        }

        if (job)
            job();
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
