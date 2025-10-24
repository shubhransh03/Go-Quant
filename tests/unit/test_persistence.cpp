#include <gtest/gtest.h>
#include <filesystem>
#include "engine/matching_engine.h"

TEST(PersistenceTest, SaveLoadRoundtrip) {
    const std::string SYMBOL = "SAVELOAD";
    auto tmpdir = std::filesystem::temp_directory_path() / ("goquant_state_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count()));
    std::string path = tmpdir.string();

    MatchingEngine engine1;
    engine1.submitOrder(std::make_shared<Order>("s1", SYMBOL, Order::Side::BUY, Order::Type::LIMIT, 50.0, 1.0));
    engine1.submitOrder(std::make_shared<Order>("s2", SYMBOL, Order::Side::SELL, Order::Type::LIMIT, 60.0, 2.0));

    ASSERT_TRUE(engine1.saveState(path));

    MatchingEngine engine2;
    ASSERT_TRUE(engine2.loadState(path));

    EXPECT_EQ(engine1.getOrderCount(SYMBOL), engine2.getOrderCount(SYMBOL));

    // cleanup
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
