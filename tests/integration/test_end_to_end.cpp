#include <gtest/gtest.h>
#include "engine/matching_engine.h"
#include "network/session.h"

class MatchingEngineEndToEndTest : public ::testing::Test {
protected:
    MatchingEngine* matchingEngine;
    Session* session;

    void SetUp() override {
        matchingEngine = new MatchingEngine();
        session = new Session();
        matchingEngine->initialize();
    }

    void TearDown() override {
        delete session;
        delete matchingEngine;
    }
};

TEST_F(MatchingEngineEndToEndTest, TestOrderSubmissionAndMatching) {
    // Simulate order submission
    Order order1 = {1, "BTC/USD", 1000, 1, OrderType::LIMIT, Side::BUY};
    Order order2 = {2, "BTC/USD", 1000, 1, OrderType::LIMIT, Side::SELL};

    matchingEngine->submitOrder(order1);
    matchingEngine->submitOrder(order2);

    // Check if the order book has the orders
    EXPECT_EQ(matchingEngine->getOrderBook("BTC/USD").getOrderCount(), 2);

    // Execute matching
    matchingEngine->matchOrders();

    // Verify that the order was matched
    EXPECT_EQ(matchingEngine->getOrderBook("BTC/USD").getOrderCount(), 0);
}

TEST_F(MatchingEngineEndToEndTest, TestMarketDataDissemination) {
    // Simulate order submission
    Order order = {1, "ETH/USD", 2000, 1, OrderType::LIMIT, Side::BUY};
    matchingEngine->submitOrder(order);

    // Check market data dissemination
    MarketData marketData = matchingEngine->getMarketData("ETH/USD");
    EXPECT_EQ(marketData.bestBid, 2000);
    EXPECT_EQ(marketData.bestAsk, 0); // Assuming no sell orders yet
}

TEST_F(MatchingEngineEndToEndTest, TestTradeExecutionDataGeneration) {
    // Simulate order submission
    Order buyOrder = {1, "LTC/USD", 150, 1, OrderType::LIMIT, Side::BUY};
    Order sellOrder = {2, "LTC/USD", 150, 1, OrderType::LIMIT, Side::SELL};

    matchingEngine->submitOrder(buyOrder);
    matchingEngine->submitOrder(sellOrder);
    matchingEngine->matchOrders();

    // Check trade execution data
    auto trades = matchingEngine->getTradeExecutionData();
    EXPECT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].price, 150);
    EXPECT_EQ(trades[0].quantity, 1);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}