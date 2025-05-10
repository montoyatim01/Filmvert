
#include <condition_variable>
#include <functional>
#include <atomic>
#include "threadPool.h"

ThreadPool::ThreadPool(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i)
        workers.emplace_back([this]() { this->workerLoop(); });
}

void ThreadPool::workerLoop() {
    while (!stop) {
        std::function<void()> task;

        {
            std::unique_lock lock(queueMutex);
            condition.wait(lock, [this]() {
                return stop || !tasks.empty();
            });

            if (stop && tasks.empty())
                return;

            task = std::move(tasks.front());
            tasks.pop();
        }

        task();
    }
}

ThreadPool::~ThreadPool() {
    stop = true;
    condition.notify_all();
    for (auto& t : workers)
        t.join();
}
