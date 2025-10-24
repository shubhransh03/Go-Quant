#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include "engine/matching_engine.h"

class MatchingEngineTest : public ::testing::Test {
  protected:
    MatchingEngine engine;
    const std::string TEST_SYMBOL = "BTC-USDT";

    void SetUp() override {}
    void TearDown() override {}

    std::shared_ptr<Order> createOrder(const std::string &id, Order::Side side, Order::Type type, double price,
                                       double quantity) {
        return std::make_shared<Order>(id, TEST_SYMBOL, side, type, price, quantity);
    }
};

TEST_F(MatchingEngineTest, TestLimitOrderMatching) {
    // Submit a sell limit order
    auto sellOrder = createOrder("sell1", Order::Side::SELL, Order::Type::LIMIT, 100.0, 1.0);
    engine.submitOrder(sellOrder);

    // Submit a matching buy limit order
    auto buyOrder = createOrder("buy1", Order::Side::BUY, Order::Type::LIMIT, 100.0, 1.0);

    // Set up trade callback to verify the trade
    bool tradeReceived = false;
    engine.subscribeToTrades(TEST_SYMBOL, [&](const Trade &trade) {
        EXPECT_EQ(trade.price, 100.0);
        EXPECT_EQ(trade.quantity, 1.0);
        EXPECT_EQ(trade.makerOrderId, "sell1");
        EXPECT_EQ(trade.takerOrderId, "buy1");
        tradeReceived = true;
    });

    engine.submitOrder(buyOrder);

    // Verify trade was executed
    EXPECT_TRUE(tradeReceived);
    EXPECT_EQ(engine.getOrderCount(TEST_SYMBOL), 0);
}

TEST_F(MatchingEngineTest, TestMarketOrderExecution) {
    // Submit some limit orders to create liquidity
    engine.submitOrder(createOrder("sell1", Order::Side::SELL, Order::Type::LIMIT, 100.0, 1.0));
    engine.submitOrder(createOrder("sell2", Order::Side::SELL, Order::Type::LIMIT, 101.0, 1.0));

    // Submit a market buy order
    auto marketBuy = createOrder("buy1", Order::Side::BUY, Order::Type::MARKET, 0.0, 1.5);

    std::vector<Trade> trades;
    engine.subscribeToTrades(TEST_SYMBOL, [&](const Trade &trade) { trades.push_back(trade); });

    engine.submitOrder(marketBuy);

    // Verify trades
    ASSERT_EQ(trades.size(), 2);
    EXPECT_EQ(trades[0].price, 100.0);
    EXPECT_EQ(trades[0].quantity, 1.0);
    EXPECT_EQ(trades[1].price, 101.0);
    EXPECT_EQ(trades[1].quantity, 0.5);
}

TEST_F(MatchingEngineTest, TestIOCOrderPartialFill) {
    // Create some liquidity
    engine.submitOrder(createOrder("sell1", Order::Side::SELL, Order::Type::LIMIT, 100.0, 1.0));

    // Submit IOC order for more quantity than available
    auto iocBuy = createOrder("buy1", Order::Side::BUY, Order::Type::IOC, 100.0, 2.0);

    std::vector<Trade> trades;
    engine.subscribeToTrades(TEST_SYMBOL, [&](const Trade &trade) { trades.push_back(trade); });

    engine.submitOrder(iocBuy);

    // Verify partial fill
    ASSERT_EQ(trades.size(), 1);
    EXPECT_EQ(trades[0].quantity, 1.0);
    EXPECT_EQ(engine.getOrderCount(TEST_SYMBOL), 0);
}

TEST_F(MatchingEngineTest, TestFOKOrderNoPartialFill) {
    // Create some liquidity
    engine.submitOrder(createOrder("sell1", Order::Side::SELL, Order::Type::LIMIT, 100.0, 1.0));

    // Submit FOK order for more quantity than available
    auto fokBuy = createOrder("buy1", Order::Side::BUY, Order::Type::FOK, 100.0, 2.0);

    std::vector<Trade> trades;
    engine.subscribeToTrades(TEST_SYMBOL, [&](const Trade &trade) { trades.push_back(trade); });

    engine.submitOrder(fokBuy);

    // Verify no trades occurred
    EXPECT_TRUE(trades.empty());
    EXPECT_EQ(engine.getOrderCount(TEST_SYMBOL), 1); // Only the original sell order
}

TEST_F(MatchingEngineTest, TestOrderCancellation) {
    auto order = createOrder("sell1", Order::Side::SELL, Order::Type::LIMIT, 100.0, 1.0);
    engine.submitOrder(order);

    EXPECT_TRUE(engine.cancelOrder("sell1"));
    EXPECT_EQ(engine.getOrderCount(TEST_SYMBOL), 0);
}

TEST_F(MatchingEngineTest, TestMarketDataDissemination) {
    // Set up market data callback
    int callbackCount = 0;
    int snapshotCount = 0;
    int incrementCount = 0;
    bool foundBothSides = false;
    
    engine.subscribeToMarketData(TEST_SYMBOL, [&](const MarketDataUpdate &update) {
        EXPECT_EQ(update.symbol, TEST_SYMBOL);
        callbackCount++;
        
        if (update.type == MarketDataUpdate::Type::SNAPSHOT) {
            snapshotCount++;
            std::cout << "SNAPSHOT: bids=" << update.bids.size() 
                      << " asks=" << update.asks.size() << std::endl;
            // First snapshot should have 1 bid (buy @ 99)
            if (snapshotCount == 1) {
                EXPECT_EQ(update.bids.size(), 1);
                EXPECT_EQ(update.asks.size(), 0);
            }
        } else if (update.type == MarketDataUpdate::Type::INCREMENT) {
            incrementCount++;
            std::cout << "INCREMENT: bidsChanges=" << update.bidsChanges.size() 
                      << " asksChanges=" << update.asksChanges.size() << std::endl;
            // Second update should be incremental with ask changes
            if (incrementCount == 1) {
                EXPECT_GT(update.asksChanges.size(), 0);  // Should have ask additions
                // BBO should show both sides now
                EXPECT_GT(update.bestBidPrice, 0);
                EXPECT_GT(update.bestAskPrice, 0);
                foundBothSides = true;
            }
        }
    });

    // Submit orders to both sides
    engine.submitOrder(createOrder("buy1", Order::Side::BUY, Order::Type::LIMIT, 99.0, 1.0));
    engine.submitOrder(createOrder("sell1", Order::Side::SELL, Order::Type::LIMIT, 100.0, 1.0));

    // Verify market data updates were received
    EXPECT_EQ(callbackCount, 2);  // 1 snapshot + 1 increment
    EXPECT_EQ(snapshotCount, 1);
    EXPECT_EQ(incrementCount, 1);
    EXPECT_TRUE(foundBothSides);

    // Verify order book state
    auto marketData = engine.getMarketData(TEST_SYMBOL);
    ASSERT_EQ(marketData.bids.size(), 1);
    ASSERT_EQ(marketData.asks.size(), 1);
    EXPECT_EQ(marketData.bids[0].first, 99.0);
    EXPECT_EQ(marketData.asks[0].first, 100.0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}