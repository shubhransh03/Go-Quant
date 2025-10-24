#include <gtest/gtest.h>
#include "engine/matching_engine.h"

TEST(BBOTest, BBOAfterAddModifyCancel) {
    MatchingEngine engine;
    const std::string SYM = "BBO-SYM";

    // Initially empty
    auto md0 = engine.getMarketData(SYM);
    EXPECT_DOUBLE_EQ(md0.bestBidPrice, 0.0);
    EXPECT_DOUBLE_EQ(md0.bestAskPrice, 0.0);
    EXPECT_TRUE(md0.bids.empty());
    EXPECT_TRUE(md0.asks.empty());

    // Add bids and asks
    engine.submitOrder(std::make_shared<Order>("b1", SYM, Order::Side::BUY, Order::Type::LIMIT, 99.0, 2.0));
    engine.submitOrder(std::make_shared<Order>("b2", SYM, Order::Side::BUY, Order::Type::LIMIT, 100.0, 1.0));
    engine.submitOrder(std::make_shared<Order>("a1", SYM, Order::Side::SELL, Order::Type::LIMIT, 101.0, 3.0));

    auto md1 = engine.getMarketData(SYM);
    EXPECT_DOUBLE_EQ(md1.bestBidPrice, 100.0);
    EXPECT_DOUBLE_EQ(md1.bestBidQuantity, 1.0);
    EXPECT_DOUBLE_EQ(md1.bestAskPrice, 101.0);
    EXPECT_DOUBLE_EQ(md1.bestAskQuantity, 3.0);

    // Modify reduces top bid quantity
    ASSERT_TRUE(engine.modifyOrder("b2", 0.5));
    auto md2 = engine.getMarketData(SYM);
    EXPECT_DOUBLE_EQ(md2.bestBidPrice, 100.0);
    EXPECT_DOUBLE_EQ(md2.bestBidQuantity, 0.5);

    // Cancel top bid, next level should become best bid
    ASSERT_TRUE(engine.cancelOrder("b2"));
    auto md3 = engine.getMarketData(SYM);
    EXPECT_DOUBLE_EQ(md3.bestBidPrice, 99.0);
    EXPECT_DOUBLE_EQ(md3.bestBidQuantity, 2.0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
