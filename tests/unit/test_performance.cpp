#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <numeric>
#include "engine/matching_engine.h"
#include "utils/rate_limiter.h"

class PerformanceTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set extremely high rate limits for performance testing (1M orders/sec, 2M burst)
        RateLimiterManager::instance().addSymbol(symbol, 1000000, 2000000);
    }
    
    MatchingEngine engine;
    const std::string symbol = "BTC-USDT";
    
    // Helper to generate orders
    std::shared_ptr<Order> createOrder(bool isBuy, double price, double qty) {
        static std::atomic<uint64_t> orderId{0};
        return std::make_shared<Order>(
            "O" + std::to_string(++orderId),
            symbol,
            isBuy ? Order::Side::BUY : Order::Side::SELL,
            Order::Type::LIMIT,
            price,
            qty
        );
    }

    // Helper to measure latency
    template<typename F>
    double measureLatencyMicros(F&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
};

// Test order submission latency
TEST_F(PerformanceTest, OrderSubmissionLatency) {
    std::vector<double> latencies;
    const int NUM_ORDERS = 1000;
    
    for (int i = 0; i < NUM_ORDERS; i++) {
        auto order = createOrder(true, 50000.0 + i, 1.0);
        double latency = measureLatencyMicros([&]() {
            engine.submitOrder(order);
        });
        latencies.push_back(latency);
    }
    
    // Calculate statistics
    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    double mean = sum / latencies.size();
    
    std::sort(latencies.begin(), latencies.end());
    double median = latencies[latencies.size() / 2];
    double p99 = latencies[static_cast<size_t>(latencies.size() * 0.99)];
    
    // Log results
    std::cout << "Order Submission Latency (microseconds):\n"
              << "Mean: " << mean << "\n"
              << "Median: " << median << "\n"
              << "99th percentile: " << p99 << "\n";
              
    // Assert performance requirements
    EXPECT_LT(median, 100.0); // Median latency should be under 100 microseconds
    EXPECT_LT(p99, 500.0);    // 99th percentile should be under 500 microseconds
}

// Test market data update latency
TEST_F(PerformanceTest, MarketDataLatency) {
    std::vector<double> latencies;
    const int NUM_UPDATES = 1000;
    
    std::atomic<int> updateCount{0};
    engine.subscribeToMarketData(symbol, [&](const MarketDataUpdate& update) {
        updateCount++;
    });
    
    for (int i = 0; i < NUM_UPDATES; i++) {
        int startCount = updateCount;
        auto order = createOrder(true, 50000.0 + i, 1.0);
        
        double latency = measureLatencyMicros([&]() {
            engine.submitOrder(order);
            while (updateCount == startCount) {
                std::this_thread::yield();
            }
        });
        latencies.push_back(latency);
    }
    
    double sum = std::accumulate(latencies.begin(), latencies.end(), 0.0);
    double mean = sum / latencies.size();
    std::sort(latencies.begin(), latencies.end());
    double median = latencies[latencies.size() / 2];
    
    std::cout << "Market Data Update Latency (microseconds):\n"
              << "Mean: " << mean << "\n"
              << "Median: " << median << "\n";
              
    EXPECT_LT(median, 50.0); // Market data updates should be under 50 microseconds
}

// Test throughput
TEST_F(PerformanceTest, OrderThroughput) {
    const int DURATION_SECONDS = 5;
    const int NUM_THREADS = 4;
    std::atomic<int> orderCount{0};
    std::vector<std::thread> threads;
    
    auto worker = [&]() {
        auto end = std::chrono::steady_clock::now() + std::chrono::seconds(DURATION_SECONDS);
        while (std::chrono::steady_clock::now() < end) {
            auto order = createOrder(true, 50000.0, 1.0);
            engine.submitOrder(order);
            orderCount++;
        }
    };
    
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(worker);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start).count();
    
    double ordersPerSecond = static_cast<double>(orderCount) / duration;
    std::cout << "Throughput: " << ordersPerSecond << " orders/second\n";
    
    EXPECT_GT(ordersPerSecond, 10000.0); // Should handle >10K orders/second
}

// Test concurrent modifications
TEST_F(PerformanceTest, ConcurrentModifications) {
    const int NUM_THREADS = 4;
    const int ORDERS_PER_THREAD = 1000;
    std::atomic<int> successCount{0};
    std::vector<std::thread> threads;
    std::vector<std::string> orderIds;
    std::mutex idMutex;
    
    // Pre-populate some orders and track their IDs
    for (int i = 0; i < 100; i++) {
        auto order = createOrder(true, 50000.0 + i, 1.0);
        orderIds.push_back(order->getId());
        engine.submitOrder(order);
    }
    
    auto worker = [&]() {
        for (int i = 0; i < ORDERS_PER_THREAD; i++) {
            // Random operation: add, modify, or cancel
            int op = rand() % 3;
            if (op == 0) {
                // Add new order
                auto order = createOrder(true, 50000.0 + rand() % 1000, 1.0);
                {
                    std::lock_guard<std::mutex> lock(idMutex);
                    orderIds.push_back(order->getId());
                }
                engine.submitOrder(order);
                successCount++;
            } else if (op == 1 && !orderIds.empty()) {
                // Try to modify a random existing order
                std::string orderId;
                {
                    std::lock_guard<std::mutex> lock(idMutex);
                    if (!orderIds.empty()) {
                        orderId = orderIds[rand() % orderIds.size()];
                    }
                }
                if (!orderId.empty() && engine.modifyOrder(orderId, 2.0)) {
                    successCount++;
                }
            } else if (op == 2 && !orderIds.empty()) {
                // Try to cancel a random existing order
                std::string orderId;
                {
                    std::lock_guard<std::mutex> lock(idMutex);
                    if (!orderIds.empty()) {
                        orderId = orderIds[rand() % orderIds.size()];
                    }
                }
                if (!orderId.empty() && engine.cancelOrder(orderId)) {
                    successCount++;
                }
            }
        }
    };
    
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.emplace_back(worker);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::cout << "Successful concurrent operations: " << successCount << "\n";
    // More realistic expectation: at least 30% success (many operations on same orders will conflict)
    EXPECT_GT(successCount, NUM_THREADS * ORDERS_PER_THREAD * 0.3);
}