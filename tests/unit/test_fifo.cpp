#include <gtest/gtest.h>
#include <vector>
#include "engine/matching_engine.h"

class FIFOTest : public ::testing::Test {
  protected:
    MatchingEngine engine;
    const std::string SYMBOL = "FIFO-SYM";

    std::shared_ptr<Order> makeOrder(const std::string &id, Order::Side side, Order::Type type, double price, double qty) {
        return std::make_shared<Order>(id, SYMBOL, side, type, price, qty);
    }
};

TEST_F(FIFOTest, PriceLevelFIFO) {
    // Two makers at same price in time order: sell1 then sell2
    engine.submitOrder(makeOrder("sell1", Order::Side::SELL, Order::Type::LIMIT, 100.0, 1.0));
    engine.submitOrder(makeOrder("sell2", Order::Side::SELL, Order::Type::LIMIT, 100.0, 2.0));

    std::vector<Trade> trades;
    engine.subscribeToTrades(SYMBOL, [&](const Trade &t) { trades.push_back(t); });

    // Taker market buy for 2.5 should fill sell1 completely then sell2
    engine.submitOrder(makeOrder("buy1", Order::Side::BUY, Order::Type::MARKET, 0.0, 2.5));

    ASSERT_GE(trades.size(), 2);
    EXPECT_EQ(trades[0].makerOrderId, "sell1");
    EXPECT_DOUBLE_EQ(trades[0].quantity, 1.0);
    EXPECT_EQ(trades[1].makerOrderId, "sell2");
    EXPECT_DOUBLE_EQ(trades[1].quantity, 1.5);
}

TEST_F(FIFOTest, NoTradeThroughLevelByLevel) {
    // Create two price levels: best 100 qty1, next 101 qty2
    engine.submitOrder(makeOrder("s1", Order::Side::SELL, Order::Type::LIMIT, 100.0, 1.0));
    engine.submitOrder(makeOrder("s2", Order::Side::SELL, Order::Type::LIMIT, 101.0, 2.0));

    std::vector<Trade> trades;
    engine.subscribeToTrades(SYMBOL, [&](const Trade &t) { trades.push_back(t); });

    // Market buy for 2.5 should take 1.0 at 100 then 1.5 at 101 (no skipping)
    engine.submitOrder(makeOrder("b1", Order::Side::BUY, Order::Type::MARKET, 0.0, 2.5));

    ASSERT_EQ(trades.size(), 2);
    EXPECT_DOUBLE_EQ(trades[0].price, 100.0);
    EXPECT_DOUBLE_EQ(trades[0].quantity, 1.0);
    EXPECT_DOUBLE_EQ(trades[1].price, 101.0);
    EXPECT_DOUBLE_EQ(trades[1].quantity, 1.5);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
