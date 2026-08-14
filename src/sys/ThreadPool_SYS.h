// ThreadPool_SYS.h — persistent worker pool with a blocking parallel-for.
// Layer: SYS. Spins up its workers once and reuses them, so hot passes never pay
// thread-creation cost per call. ParallelFor partitions a range into fixed chunks
// (partition depends only on range + worker count, not on runtime scheduling — safe
// for the deterministic path when the per-index work is independent) and blocks until
// all chunks finish. A pool of size 0 runs everything inline on the caller.
#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

namespace SanmapGen {
namespace Sys {

class ThreadPool {
public:
    explicit ThreadPool(unsigned workerCount = 0) : stopping(false) {
        if (workerCount == 0) workerCount = std::thread::hardware_concurrency();
        for (unsigned i = 0; i < workerCount; ++i)
            workers.emplace_back([this] { WorkerLoop(); });
    }
    ~ThreadPool() {
        { std::lock_guard<std::mutex> lock(queueMutex); stopping = true; }
        taskAvailable.notify_all();
        for (std::thread& worker : workers) worker.join();
    }
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    unsigned WorkerCount() const { return static_cast<unsigned>(workers.size()); }

    // Runs body(index) for index in [begin, end); blocks until every index is done.
    template <typename Function>
    void ParallelFor(int begin, int end, Function&& body) {
        int total = end - begin;
        if (total <= 0) return;
        unsigned chunkCount = static_cast<unsigned>(workers.size());
        if (chunkCount <= 1 || static_cast<int>(chunkCount) > total) {
            if (workers.empty() || total < 2) { for (int i = begin; i < end; ++i) body(i); return; }
            chunkCount = static_cast<unsigned>(total);
        }
        int chunkSize = (total + static_cast<int>(chunkCount) - 1) / static_cast<int>(chunkCount);
        int remaining = static_cast<int>(chunkCount);   // guarded entirely by doneMutex
        std::mutex doneMutex;
        std::condition_variable doneSignal;
        for (unsigned c = 0; c < chunkCount; ++c) {
            int chunkBegin = begin + static_cast<int>(c) * chunkSize;
            int chunkEnd = chunkBegin + chunkSize;
            if (chunkEnd > end) chunkEnd = end;
            if (chunkBegin >= chunkEnd) {
                std::lock_guard<std::mutex> lock(doneMutex);
                if (--remaining == 0) doneSignal.notify_one();
                continue;
            }
            Enqueue([&body, &remaining, &doneMutex, &doneSignal, chunkBegin, chunkEnd] {
                for (int i = chunkBegin; i < chunkEnd; ++i) body(i);
                std::lock_guard<std::mutex> lock(doneMutex);   // notify completes under the
                if (--remaining == 0) doneSignal.notify_one(); // lock, before the waiter returns
            });
        }
        std::unique_lock<std::mutex> lock(doneMutex);
        doneSignal.wait(lock, [&remaining] { return remaining == 0; });
    }

private:
    void WorkerLoop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                taskAvailable.wait(lock, [this] { return stopping || !tasks.empty(); });
                if (stopping && tasks.empty()) return;
                task = std::move(tasks.front());
                tasks.pop();
            }
            task();
        }
    }
    void Enqueue(std::function<void()> task) {
        { std::lock_guard<std::mutex> lock(queueMutex); tasks.push(std::move(task)); }
        taskAvailable.notify_one();
    }

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queueMutex;
    std::condition_variable taskAvailable;
    bool stopping;
};

} // namespace Sys
} // namespace SanmapGen
