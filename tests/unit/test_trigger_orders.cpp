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
    stop->setId("STOP1");
    engine.submitOrder(stop);

    EXPECT_EQ(engine.getTriggerOrderCount(SYM), 1);

    // Submit a buy market order that will trade through and produce a last trade price >= 100
    auto taker = engine.createOrder(SYM, Order::Side::BUY, Order::Type::MARKET, 0.0, 1.0);
    engine.submitOrder(taker);

    // After trade, stop-loss should not trigger because it was a sell stop below the last trade price
    // Now submit a sell that moves price down to trigger stop
    auto aggressiveSell = engine.createOrder(SYM, Order::Side::SELL, Order::Type::LIMIT, 98.0, 1.0);
    engine.submitOrder(aggressiveSell);

    // Now last trade price should be <= 98 and stop-loss should have been activated (converted)
    // Allow some operations to run; check trigger orders count (should be 0)
    EXPECT_EQ(engine.getTriggerOrderCount(SYM), 0);

    engine.stopWAL();

    // Replay WAL into a fresh engine to verify persistence of past submits (we expect no trigger orders left)
    MatchingEngine engine2;
    bool ok = engine2.replayWAL(walPath);
    EXPECT_TRUE(ok);

    // After replay, trigger orders for symbol should be 0 (previous stop got activated)
    EXPECT_EQ(engine2.getTriggerOrderCount(SYM), 0);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
