#include "eigen_fun/thread_safe_queue.hpp"

#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <numeric>
#include <thread>
#include <vector>

using ef::threading::ThreadSafeQueue;

TEST(ThreadSafeQueue, FifoOrderSingleThreaded) {
    ThreadSafeQueue<int> q;
    for (int i = 0; i < 5; ++i) q.push(i);
    for (int i = 0; i < 5; ++i) EXPECT_EQ(q.pop(), i);
    EXPECT_FALSE(q.tryPop().has_value());
}

TEST(ThreadSafeQueue, SupportsMoveOnlyTypes) {
    ThreadSafeQueue<std::unique_ptr<int>> q;
    q.push(std::make_unique<int>(7));
    auto p = q.pop();
    ASSERT_TRUE(p.has_value());
    EXPECT_EQ(**p, 7);
}

TEST(ThreadSafeQueue, CloseWakesBlockedConsumers) {
    ThreadSafeQueue<int> q;
    std::atomic<int> exited{0};
    {
        std::vector<std::jthread> consumers;
        for (int i = 0; i < 4; ++i)
            consumers.emplace_back([&] {
                while (q.pop()) {
                }
                ++exited;
            });
        q.close();  // without this the jthread destructors would deadlock
    }
    EXPECT_EQ(exited, 4);
    EXPECT_FALSE(q.push(1));
}

TEST(ThreadSafeQueue, ClosedQueueIsDrainedBeforeReturningNullopt) {
    ThreadSafeQueue<int> q;
    q.push(1);
    q.push(2);
    q.close();
    EXPECT_EQ(q.pop(), 1);
    EXPECT_EQ(q.pop(), 2);
    EXPECT_EQ(q.pop(), std::nullopt);
}

TEST(ThreadSafeQueue, ManyProducersManyConsumersLoseNothing) {
    constexpr int kProducers = 4, kConsumers = 4, kPerProducer = 10'000;
    ThreadSafeQueue<int> q;
    std::atomic<long long> consumedSum{0};
    std::atomic<int> consumedCount{0};

    {
        std::vector<std::jthread> consumers;
        for (int c = 0; c < kConsumers; ++c)
            consumers.emplace_back([&] {
                while (auto v = q.pop()) {
                    consumedSum.fetch_add(*v, std::memory_order_relaxed);
                    consumedCount.fetch_add(1, std::memory_order_relaxed);
                }
            });

        {
            std::vector<std::jthread> producers;
            for (int p = 0; p < kProducers; ++p)
                producers.emplace_back([&, p] {
                    for (int i = 0; i < kPerProducer; ++i) q.push(p * kPerProducer + i);
                });
        }  // all producers joined
        q.close();
    }  // all consumers joined

    constexpr long long n = kProducers * kPerProducer;
    EXPECT_EQ(consumedCount, n);
    EXPECT_EQ(consumedSum, n * (n - 1) / 2);
}
