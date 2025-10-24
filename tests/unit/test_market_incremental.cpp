#include <gtest/gtest.h>
#include "engine/matching_engine.h"

TEST(MarketIncrementalTest, SnapshotThenIncrement) {
    MatchingEngine engine;
    const std::string SYM = "MKT-INC";

    std::vector<MarketDataUpdate> updates;
    engine.subscribeToMarketData(SYM, [&](const MarketDataUpdate &u) { updates.push_back(u); });

    // First orders produce a snapshot
    engine.submitOrder(std::make_shared<Order>("b1", SYM, Order::Side::BUY, Order::Type::LIMIT, 100.0, 1.0));
    engine.submitOrder(std::make_shared<Order>("a1", SYM, Order::Side::SELL, Order::Type::LIMIT, 101.0, 1.0));

    ASSERT_GE(updates.size(), 1);
    EXPECT_EQ(updates[0].type, MarketDataUpdate::Type::SNAPSHOT);

    // Second change (modify order) should produce an INCREMENT
    engine.modifyOrder("b1", 0.5);
    // allow callbacks
    ASSERT_GE(updates.size(), 2);
    bool foundInc = false;
    for (size_t i = 1; i < updates.size(); ++i) {
        if (updates[i].type == MarketDataUpdate::Type::INCREMENT) { foundInc = true; break; }
    }
    EXPECT_TRUE(foundInc);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
