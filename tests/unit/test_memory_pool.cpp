#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include "utils/memory_pool.h"
#include "utils/order_pool.h"

class MemoryPoolTest : public ::testing::Test {
protected:
    static constexpr size_t NUM_ITERATIONS = 100000;
    static constexpr size_t NUM_THREADS = 4;
    
    // Helper to measure time
    template<typename F>
    double measureTimeMillis(F&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
};

TEST_F(MemoryPoolTest, CompareWithStandardAllocation) {
    // Test standard allocation
    double stdTime = measureTimeMillis([&]() {
        std::vector<std::shared_ptr<Order>> orders;
        orders.reserve(NUM_ITERATIONS);
        
        for (size_t i = 0; i < NUM_ITERATIONS; ++i) {
            orders.push_back(std::make_shared<Order>(
                "O" + std::to_string(i),
                "BTC-USDT",
                Order::Side::BUY,
                Order::Type::LIMIT,
                50000.0,
                1.0
            ));
        }
    });
    
    // Test pool allocation
    double poolTime = measureTimeMillis([&]() {
        std::vector<std::shared_ptr<Order>> orders;
        orders.reserve(NUM_ITERATIONS);
        
        for (size_t i = 0; i < NUM_ITERATIONS; ++i) {
            orders.push_back(OrderPool::instance().createOrder(
                "O" + std::to_string(i),
                "BTC-USDT",
                Order::Side::BUY,
                Order::Type::LIMIT,
                50000.0,
                1.0
            ));
        }
    });
    
    std::cout << "Standard allocation time: " << stdTime << "ms\n"
              << "Pool allocation time: " << poolTime << "ms\n"
              << "Speedup: " << (stdTime / poolTime) << "x\n";
              
    EXPECT_LT(poolTime, stdTime);  // Pool should be faster
}

TEST_F(MemoryPoolTest, ConcurrentAllocation) {
    std::vector<std::thread> threads;
    std::atomic<size_t> totalAllocations{0};
    
    auto worker = [&]() {
        for (size_t i = 0; i < NUM_ITERATIONS / NUM_THREADS; ++i) {
            auto order = OrderPool::instance().createOrder(
                "O" + std::to_string(i),
                "BTC-USDT",
                Order::Side::BUY,
                Order::Type::LIMIT,
                50000.0,
                1.0
            );
            if (order) {
                totalAllocations++;
            }
        }
    };
    
    double concurrentTime = measureTimeMillis([&]() {
        for (size_t i = 0; i < NUM_THREADS; ++i) {
            threads.emplace_back(worker);
        }
        
        for (auto& t : threads) {
            t.join();
        }
    });
    
    std::cout << "Concurrent allocation time: " << concurrentTime << "ms\n"
              << "Allocations per second: " << (totalAllocations / (concurrentTime / 1000.0)) << "\n";
              
    EXPECT_EQ(totalAllocations, NUM_ITERATIONS);
}

TEST_F(MemoryPoolTest, RingBufferPerformance) {
    RingBuffer<MarketDataUpdate, 1024> buffer;
    std::atomic<size_t> pushed{0};
    std::atomic<size_t> popped{0};
    
    // Test concurrent push/pop operations
    std::thread producer([&]() {
        for (size_t i = 0; i < NUM_ITERATIONS; ++i) {
            MarketDataUpdate update;
            update.seqNum = i;
            if (buffer.push(update)) {
                pushed++;
            }
        }
    });
    
    std::thread consumer([&]() {
        while (popped < NUM_ITERATIONS) {
            if (auto update = buffer.pop()) {
                popped++;
            }
        }
    });
    
    producer.join();
    consumer.join();
    
    std::cout << "Ring buffer: pushed " << pushed << " items, popped " << popped << " items\n";
    EXPECT_GT(pushed, 0);
    EXPECT_GT(popped, 0);
}