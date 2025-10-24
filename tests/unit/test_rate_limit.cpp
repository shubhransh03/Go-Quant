#include <gtest/gtest.h>
#include "utils/rate_limiter.h"
#include "engine/matching_engine.h"
#include <thread>
#include <chrono>
#include <vector>

class RateLimitTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsManager::instance().start("0.0.0.0:9092");
    }
};

TEST_F(RateLimitTest, BasicRateLimit) {
    const std::string symbol = "BTC-USDT";
    const size_t RATE_LIMIT = 100;  // 100 orders per second
    const size_t BURST_LIMIT = 200;
    
    RateLimiter limiter(symbol, BURST_LIMIT, RATE_LIMIT);
    
    // Should allow burst up to limit
    for (size_t i = 0; i < BURST_LIMIT; ++i) {
        EXPECT_TRUE(limiter.tryConsume());
    }
    
    // Next request should fail
    EXPECT_FALSE(limiter.tryConsume());
    
    // Wait for 1 second to refill
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Should now have RATE_LIMIT new tokens
    for (size_t i = 0; i < RATE_LIMIT; ++i) {
        EXPECT_TRUE(limiter.tryConsume());
    }
}

TEST_F(RateLimitTest, MultiSymbolRateLimit) {
    MatchingEngine engine;
    const size_t NUM_ORDERS = 1000;
    const std::vector<std::string> symbols = {"BTC-USDT", "ETH-USDT", "SOL-USDT"};
    
    // Submit orders to different symbols
    size_t accepted = 0;
    size_t rejected = 0;
    
    for (size_t i = 0; i < NUM_ORDERS; ++i) {
        const std::string& symbol = symbols[i % symbols.size()];
        
        try {
            auto order = engine.createOrder(
                symbol,
                Order::Side::BUY,
                Order::Type::LIMIT,
                50000.0,
                1.0
            );
            engine.submitOrder(order);
            accepted++;
        } catch (const std::runtime_error& e) {
            rejected++;
        }
    }
    
    std::cout << "Accepted orders: " << accepted << "\n"
              << "Rejected orders: " << rejected << "\n";
              
    // Verify some orders were rate limited
    EXPECT_GT(rejected, 0);
}

TEST_F(RateLimitTest, RateLimitRecovery) {
    const std::string symbol = "BTC-USDT";
    const size_t RATE_LIMIT = 10;  // 10 orders per second
    const size_t BURST_LIMIT = 20;
    
    RateLimiter limiter(symbol, BURST_LIMIT, RATE_LIMIT);
    
    // Consume all tokens
    for (size_t i = 0; i < BURST_LIMIT; ++i) {
        EXPECT_TRUE(limiter.tryConsume());
    }
    
    // Wait for partial recovery
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Should have ~5 new tokens (half second at 10 tokens/sec)
    size_t recovered = 0;
    for (size_t i = 0; i < BURST_LIMIT; ++i) {
        if (limiter.tryConsume()) {
            recovered++;
        }
    }
    
    EXPECT_GT(recovered, 0);
    EXPECT_LE(recovered, RATE_LIMIT / 2 + 1);  // Allow for slight timing variance
}

TEST_F(RateLimitTest, PerSymbolMetrics) {
    MatchingEngine engine;
    const std::string symbol = "BTC-USDT";
    
    // Create matching orders
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
        1.0
    );
    
    // Submit orders to generate metrics
    engine.submitOrder(buyOrder);
    engine.submitOrder(sellOrder);
    
    // Wait for metrics to update
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Get metrics
    auto metrics = engine.getMetricsJSON();
    
    // Verify symbol-specific metrics exist
    EXPECT_TRUE(metrics.find(symbol) != std::string::npos);
    EXPECT_TRUE(metrics.find("rate_limiter_tokens") != std::string::npos);
    EXPECT_TRUE(metrics.find("symbol_stats") != std::string::npos);
}