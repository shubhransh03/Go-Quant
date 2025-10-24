#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_engine.h"
#include "network/session.h"

class MatchingEngineEndToEndTest : public ::testing::Test {
protected:
    MatchingEngine* matchingEngine;
    Session* session;

    void SetUp() override {
        matchingEngine = new MatchingEngine();
        session = new Session(1); // Session requires an ID
        // No initialize() method - constructor handles initialization
    }

    void TearDown() override {
        delete session;
        delete matchingEngine;
    }
};

TEST_F(MatchingEngineEndToEndTest, TestOrderSubmissionAndMatching) {
    const std::string symbol = "BTC/USD";
    
    // Simulate order submission using correct API
    auto order1 = std::make_shared<Order>("1", symbol, Order::Side::BUY, Order::Type::LIMIT, 1000.0, 1.0);
    auto order2 = std::make_shared<Order>("2", symbol, Order::Side::SELL, Order::Type::LIMIT, 1000.0, 1.0);

    matchingEngine->submitOrder(order1);
    matchingEngine->submitOrder(order2);

    // Check if orders were processed (they should match immediately)
    // After matching, order count should be 0 as they fully matched
    EXPECT_EQ(matchingEngine->getOrderCount(symbol), 0);

    // Verify trades were generated
    auto trades = matchingEngine->getRecentTrades(symbol);
    EXPECT_GT(trades.size(), 0);
}

TEST_F(MatchingEngineEndToEndTest, TestMarketDataDissemination) {
    const std::string symbol = "ETH/USD";
    
    // Simulate order submission
    auto order = std::make_shared<Order>("1", symbol, Order::Side::BUY, Order::Type::LIMIT, 2000.0, 1.0);
    matchingEngine->submitOrder(order);

    // Check market data dissemination
    auto marketData = matchingEngine->getMarketData(symbol);
    EXPECT_EQ(marketData.bestBidPrice, 2000.0);
    EXPECT_EQ(marketData.bestAskPrice, 0.0); // Assuming no sell orders yet
}

TEST_F(MatchingEngineEndToEndTest, TestTradeExecutionDataGeneration) {
    const std::string symbol = "LTC/USD";
    
    // Simulate order submission
    auto buyOrder = std::make_shared<Order>("1", symbol, Order::Side::BUY, Order::Type::LIMIT, 150.0, 1.0);
    auto sellOrder = std::make_shared<Order>("2", symbol, Order::Side::SELL, Order::Type::LIMIT, 150.0, 1.0);

    matchingEngine->submitOrder(buyOrder);
    matchingEngine->submitOrder(sellOrder);

    // Check trade execution data
    auto trades = matchingEngine->getRecentTrades(symbol);
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 150.0);
    EXPECT_EQ(trades[0].quantity, 1.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}