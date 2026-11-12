#include "eigen_fun/thread_pool.hpp"

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <latch>
#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>

using ef::threading::ThreadPool;
using namespace std::chrono_literals;

TEST(ThreadPool, ReturnsResultsThroughFutures) {
    ThreadPool pool(4);
    auto f1 = pool.submit([] { return 6 * 7; });
    auto f2 = pool.submit([](int a, int b) { return a + b; }, 2, 3);
    auto f3 = pool.submit([](std::string s) { return s + "!"; }, std::string("hi"));
    EXPECT_EQ(f1.get(), 42);
    EXPECT_EQ(f2.get(), 5);
    EXPECT_EQ(f3.get(), "hi!");
}

TEST(ThreadPool, AcceptsMoveOnlyCallablesAndArguments) {
    ThreadPool pool(2);
    auto owned = std::make_unique<int>(10);
    auto f = pool.submit([p = std::move(owned)] { return *p * 2; });
    auto g = pool.submit([](std::unique_ptr<int> p) { return *p + 1; }, std::make_unique<int>(4));
    EXPECT_EQ(f.get(), 20);
    EXPECT_EQ(g.get(), 5);
}

TEST(ThreadPool, ExceptionsPropagateToTheCaller) {
    ThreadPool pool(2);
    auto f = pool.submit([]() -> int { throw std::runtime_error("boom"); });
    EXPECT_THROW((void)f.get(), std::runtime_error);
    // The worker survived and can keep processing jobs.
    EXPECT_EQ(pool.submit([] { return 1; }).get(), 1);
}

TEST(ThreadPool, RunsJobsConcurrently) {
    constexpr int kThreads = 4;
    ThreadPool pool(kThreads);
    // Every job blocks until all of them have started: this can only finish if
    // kThreads jobs really are running at the same time.
    std::latch allStarted(kThreads);
    std::mutex m;
    std::set<std::thread::id> ids;
    std::vector<std::future<void>> fs;
    for (int i = 0; i < kThreads; ++i)
        fs.push_back(pool.submit([&] {
            {
                std::lock_guard lock(m);
                ids.insert(std::this_thread::get_id());
            }
            allStarted.arrive_and_wait();
        }));
    for (auto& f : fs) ASSERT_EQ(f.wait_for(5s), std::future_status::ready) << "deadlock?";
    EXPECT_EQ(ids.size(), static_cast<size_t>(kThreads));
}

TEST(ThreadPool, DestructorFinishesQueuedWork) {
    std::atomic<int> done{0};
    {
        ThreadPool pool(2);
        for (int i = 0; i < 100; ++i) pool.submit([&] { ++done; });
    }
    EXPECT_EQ(done, 100);
}

TEST(Atomics, UnsynchronisedIncrementVsAtomicVsMutex) {
    constexpr int kThreads = 8, kIters = 50'000;
    std::atomic<int> atomicCount{0};
    int mutexCount = 0;
    std::mutex m;
    {
        std::vector<std::jthread> ts;
        for (int t = 0; t < kThreads; ++t)
            ts.emplace_back([&] {
                for (int i = 0; i < kIters; ++i) {
                    atomicCount.fetch_add(1, std::memory_order_relaxed);
                    std::lock_guard lock(m);
                    ++mutexCount;
                }
            });
    }
    // (A plain `int` incremented without the mutex would be a data race --
    // undefined behaviour that -DEF_SANITIZE=thread would flag.)
    EXPECT_EQ(atomicCount, kThreads * kIters);
    EXPECT_EQ(mutexCount, kThreads * kIters);
}

TEST(StopToken, JthreadCooperativeCancellation) {
    std::atomic<int> ticks{0};
    std::atomic<bool> sawStop{false};
    {
        std::jthread worker([&](std::stop_token st) {
            while (!st.stop_requested()) {
                ++ticks;
                std::this_thread::sleep_for(1ms);
            }
            sawStop = true;
        });
        while (ticks < 3) std::this_thread::yield();
    }  // ~jthread calls request_stop() then join()
    EXPECT_TRUE(sawStop);
}
