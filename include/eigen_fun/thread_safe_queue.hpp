#pragma once
// An unbounded multi-producer / multi-consumer queue built from a mutex and a
// condition variable. close() wakes all waiting consumers so they can exit.

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

namespace ef::threading {

template <class T>
class ThreadSafeQueue {
public:
    // Returns false if the queue has been closed (the item is dropped).
    bool push(T value) {
        {
            std::lock_guard lock(mutex_);
            if (closed_) return false;
            items_.push_back(std::move(value));
        }
        cv_.notify_one();  // notify after unlocking so the waiter can grab the lock
        return true;
    }

    // Blocks until an item is available, or returns nullopt once the queue is
    // closed *and* drained.
    std::optional<T> pop() {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [&] { return closed_ || !items_.empty(); });  // handles spurious wakeups
        if (items_.empty()) return std::nullopt;
        T value = std::move(items_.front());
        items_.pop_front();
        return value;
    }

    std::optional<T> tryPop() {
        std::lock_guard lock(mutex_);
        if (items_.empty()) return std::nullopt;
        T value = std::move(items_.front());
        items_.pop_front();
        return value;
    }

    void close() {
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
        }
        cv_.notify_all();
    }

    bool closed() const {
        std::lock_guard lock(mutex_);
        return closed_;
    }

    std::size_t size() const {
        std::lock_guard lock(mutex_);
        return items_.size();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<T> items_;
    bool closed_ = false;
};

}  // namespace ef::threading
