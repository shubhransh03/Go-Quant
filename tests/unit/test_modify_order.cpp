#include <gtest/gtest.h>
#include "engine/matching_engine.h"

TEST(ModifyOrderTest, ModifyReducesQuantityAndUpdatesBook) {
    MatchingEngine engine;
    const std::string SYM = "MOD-SYM";

    auto o = std::make_shared<Order>("ord1", SYM, Order::Side::BUY, Order::Type::LIMIT, 50.0, 5.0);
    engine.submitOrder(o);
    EXPECT_EQ(engine.getOrderCount(SYM), 1);

    bool modified = engine.modifyOrder("ord1", 2.0);
    EXPECT_TRUE(modified);

    // Ensure order count remains 1 and top bid quantity decreased
    EXPECT_EQ(engine.getOrderCount(SYM), 1);
    auto md = engine.getMarketData(SYM);
    if (!md.bids.empty()) {
        EXPECT_DOUBLE_EQ(md.bids[0].second, 2.0);
    }
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
