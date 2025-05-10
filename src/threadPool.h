#ifndef _threadpool_h
#define _threadpool_h

#include <vector>
#include <mutex>
#include <queue>
#include <future>
#include <thread>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads);
    ~ThreadPool();

    // Submit a task to the pool and get a future
    template<typename Func, typename... Args>
    auto submit(Func&& func, Args&&... args)
        -> std::future<std::invoke_result_t<Func, Args...>>;

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop{false};

    void workerLoop();
};

template<typename Func, typename... Args>
auto ThreadPool::submit(Func&& func, Args&&... args)
    -> std::future<std::invoke_result_t<Func, Args...>>
{
    using return_type = std::invoke_result_t<Func, Args...>;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...)
    );

    std::future<return_type> result = task->get_future();
    {
        std::lock_guard lock(queueMutex);
        tasks.emplace([task]() { (*task)(); });
    }
    condition.notify_one();
    return result;
}

#endif
