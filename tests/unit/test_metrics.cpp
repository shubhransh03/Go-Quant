#include <gtest/gtest.h>
#include "utils/metrics_manager.h"
#include "engine/matching_engine.h"
#include <thread>
#include <chrono>

class MetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start metrics server on a different port for testing
        MetricsManager::instance().start("0.0.0.0:9091");
    }
};

TEST_F(MetricsTest, BasicMetricsRecording) {
    MatchingEngine engine;
    const std::string symbol = "BTC-USDT";
    
    // Create and submit some orders
    auto buyOrder = engine.createOrder(
        symbol,
        Order::Side::BUY,
        Order::Type::LIMIT,
        50000.0,
        1.0
    );
    
    auto sellOrder = engine.createOrder(
        symbol,
        Order::Side::SELL,
        Order::Type::LIMIT,
        50000.0,
        0.5
    );
    
    // Submit orders and let them match
    engine.submitOrder(buyOrder);
    engine.submitOrder(sellOrder);
    
    // Cancel remaining buy order
    engine.cancelOrder(buyOrder->getId());
    
    // Give metrics time to update
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Get metrics via prometheus client
    auto metrics = engine.getMetricsJSON();
    
    // Verify presence of key metrics
    EXPECT_TRUE(metrics.find("orders_received") != std::string::npos);
    EXPECT_TRUE(metrics.find("orders_matched") != std::string::npos);
    EXPECT_TRUE(metrics.find("orders_cancelled") != std::string::npos);
    EXPECT_TRUE(metrics.find("order_latency") != std::string::npos);
}

TEST_F(MetricsTest, LatencyTracking) {
    MatchingEngine engine;
    const std::string symbol = "BTC-USDT";
    
    // Create orders for a stress test
    const int NUM_ORDERS = 1000;
    std::vector<std::shared_ptr<Order>> orders;
    
    for (int i = 0; i < NUM_ORDERS; i++) {
        orders.push_back(engine.createOrder(
            symbol,
            Order::Side::BUY,
            Order::Type::LIMIT,
            50000.0 + i,
            1.0
        ));
    }
    
    // Submit orders and measure latency
    auto start = std::chrono::high_resolution_clock::now();
    
    for (const auto& order : orders) {
        engine.submitOrder(order);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calculate average latency
    double avgLatency = duration.count() / static_cast<double>(NUM_ORDERS);
    
    std::cout << "Average latency: " << avgLatency << " microseconds\n";
    
    // Verify metrics were recorded
    auto metrics = engine.getMetricsJSON();
    EXPECT_TRUE(metrics.find("order_latency_microseconds") != std::string::npos);
}

TEST_F(MetricsTest, MemoryPoolMetrics) {
    MatchingEngine engine;
    const std::string symbol = "BTC-USDT";
    
    // Get initial pool stats
    auto initialStats = OrderPool::instance().getStats();
    
    // Create and submit many orders to stress the pool
    const int NUM_ORDERS = 5000;
    std::vector<std::shared_ptr<Order>> orders;
    
    for (int i = 0; i < NUM_ORDERS; i++) {
        orders.push_back(engine.createOrder(
            symbol,
            Order::Side::BUY,
            Order::Type::LIMIT,
            50000.0 + i,
            1.0
        ));
    }
    
    // Get pool stats after allocation
    auto afterAllocationStats = OrderPool::instance().getStats();
    
    // Verify pool grew
    EXPECT_GT(afterAllocationStats.orderCapacity, initialStats.orderCapacity);
    EXPECT_LT(afterAllocationStats.orderAvailable, initialStats.orderAvailable);
    
    // Submit orders
    for (const auto& order : orders) {
        engine.submitOrder(order);
    }
    
    // Give metrics time to update
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify metrics were recorded
    auto metrics = engine.getMetricsJSON();
    EXPECT_TRUE(metrics.find("memory_pool_usage") != std::string::npos);
}

TEST_F(MetricsTest, BookDepthMetrics) {
    MatchingEngine engine;
    const std::string symbol = "BTC-USDT";
    
    // Add orders at different price levels
    for (int i = 0; i < 10; i++) {
        auto buyOrder = engine.createOrder(
            symbol,
            Order::Side::BUY,
            Order::Type::LIMIT,
            50000.0 - i,
            1.0
        );
        
        auto sellOrder = engine.createOrder(
            symbol,
            Order::Side::SELL,
            Order::Type::LIMIT,
            50000.0 + i,
            1.0
        );
        
        engine.submitOrder(buyOrder);
        engine.submitOrder(sellOrder);
    }
    
    // Give metrics time to update
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify book depth metrics
    auto metrics = engine.getMetricsJSON();
    EXPECT_TRUE(metrics.find("order_book_depth") != std::string::npos);
}