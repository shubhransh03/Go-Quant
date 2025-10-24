#include <gtest/gtest.h>
#include <filesystem>
#include "engine/matching_engine.h"

static std::string make_temp_wal_path() {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    auto tmp = std::filesystem::temp_directory_path() / ("goquant_wal_mod_" + std::to_string(now) + ".log");
    return tmp.string();
}

TEST(WALModifyTest, ModifyIsLoggedAndReplayed) {
    const std::string SYMBOL = "WAL-MOD";
    std::string wal = make_temp_wal_path();

    // Engine 1: start WAL and submit + modify
    MatchingEngine engine1;
    ASSERT_TRUE(engine1.startWAL(wal));

    auto o1 = std::make_shared<Order>("m1", SYMBOL, Order::Side::BUY, Order::Type::LIMIT, 10.0, 5.0);
    engine1.submitOrder(o1);

    // Reduce quantity to 2.0
    ASSERT_TRUE(engine1.modifyOrder("m1", 2.0));

    // Snapshot before stopping WAL
    auto md1 = engine1.getMarketData(SYMBOL);
    engine1.stopWAL();

    // Engine 2: replay WAL
    MatchingEngine engine2;
    ASSERT_TRUE(engine2.replayWAL(wal));

    auto md2 = engine2.getMarketData(SYMBOL);

    // Validate top bid level reflects modified quantity
    ASSERT_FALSE(md2.bids.empty());
    EXPECT_DOUBLE_EQ(md1.bids.front().first, md2.bids.front().first);
    EXPECT_DOUBLE_EQ(md2.bids.front().second, 2.0);

    // cleanup
    std::error_code ec;
    std::filesystem::remove(wal, ec);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
