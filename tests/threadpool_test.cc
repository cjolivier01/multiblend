#include <gtest/gtest.h>
#include <atomic>

#include "src/threadpool.h"

TEST(ThreadpoolTest, QueuesAndExecutesTasks) {
    Threadpool* tp = Threadpool::GetInstance(2);
    ASSERT_NE(tp, nullptr);
    std::atomic<int> counter{0};

    const int kTasks = 20;
    for (int i = 0; i < kTasks; ++i) {
        tp->Queue([&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
    }

    tp->Wait();
    EXPECT_EQ(counter.load(std::memory_order_relaxed), kTasks);
}

