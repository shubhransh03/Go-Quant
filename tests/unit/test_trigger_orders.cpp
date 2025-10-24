#include <gtest/gtest.h>
#include "engine/matching_engine.h"

TEST(TriggerOrdersTest, StopLossActivationAndPersistence) {
    MatchingEngine engine;
    const std::string SYM = "TRG-1";

    // Start WAL to a temp file
    std::string walPath = "test_wal_trigger.log";
    engine.startWAL(walPath);

    // Place some resting orders to allow trades
    auto o1 = engine.createOrder(SYM, Order::Side::SELL, Order::Type::LIMIT, 100.0, 1.0);
    engine.submitOrder(o1);

    // Submit a stop-loss sell order that should NOT activate yet
    auto stop = engine.createOrder(SYM, Order::Side::SELL, Order::Type::STOP_LOSS, 99.0, 1.0);
    engine.submitOrder(stop);

    EXPECT_EQ(engine.getTriggerOrderCount(SYM), 1);

    // Submit a buy market order that will trade at 100
    auto taker = engine.createOrder(SYM, Order::Side::BUY, Order::Type::MARKET, 0.0, 1.0);
    engine.submitOrder(taker);

    // After trade at 100, stop-loss should not trigger (sell stop triggers when price <= 99)
    EXPECT_EQ(engine.getTriggerOrderCount(SYM), 1);

    // To trigger the stop, we need a trade at or below 99
    // First place a buy order at 98
    auto buy98 = engine.createOrder(SYM, Order::Side::BUY, Order::Type::LIMIT, 98.0, 1.0);
    engine.submitOrder(buy98);

    // Now submit a sell that will trade at 98
    auto sell98 = engine.createOrder(SYM, Order::Side::SELL, Order::Type::LIMIT, 98.0, 1.0);
    engine.submitOrder(sell98);

    // Now last trade price is 98 <= 99, so stop-loss should have been activated
    EXPECT_EQ(engine.getTriggerOrderCount(SYM), 0);

    engine.stopWAL();

    // Replay WAL into a fresh engine to verify persistence
    MatchingEngine engine2;
    bool ok = engine2.replayWAL(walPath);
    EXPECT_TRUE(ok);

    // After replay, trigger orders should be 0 (stop got activated during original run)
    EXPECT_EQ(engine2.getTriggerOrderCount(SYM), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
