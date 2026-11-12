#pragma once
// A fixed-size thread pool. submit() wraps any callable in a std::packaged_task
// and returns a std::future for its result (exceptions propagate through it).

#include "eigen_fun/thread_safe_queue.hpp"

#include <functional>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>
#include <vector>

namespace ef::threading {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threads = std::thread::hardware_concurrency()) {
        if (threads == 0) threads = 1;
        workers_.reserve(threads);
        for (std::size_t i = 0; i < threads; ++i) {
            workers_.emplace_back([this] {
                while (auto job = jobs_.pop()) (*job)();
            });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Pending jobs are finished before the workers exit. std::jthread joins in
    // its destructor, so we only need to close the queue.
    ~ThreadPool() { jobs_.close(); }

    template <class F, class... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using R = std::invoke_result_t<F, Args...>;
        // packaged_task is move-only but std::function requires copyable, so we
        // hold it in a shared_ptr. (C++23's std::move_only_function fixes this.)
        auto task = std::make_shared<std::packaged_task<R()>>(
            [fn = std::forward<F>(f), ... as = std::forward<Args>(args)]() mutable {
                return std::invoke(std::move(fn), std::move(as)...);
            });
        std::future<R> result = task->get_future();
        if (!jobs_.push([task] { (*task)(); }))
            throw std::runtime_error("ThreadPool::submit on a stopped pool");
        return result;
    }

    std::size_t size() const { return workers_.size(); }

private:
    ThreadSafeQueue<std::function<void()>> jobs_;
    std::vector<std::jthread> workers_;  // declared last: destroyed (joined) first
};

}  // namespace ef::threading
