#include <gtest/gtest.h>
#include <filesystem>
#include <chrono>
#include <string>
#include "engine/matching_engine.h"

using namespace std::chrono_literals;

static std::string make_temp_wal_path() {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    auto tmp = std::filesystem::temp_directory_path() / ("goquant_wal_" + std::to_string(now) + ".log");
    return tmp.string();
}

TEST(WALTest, ReplayReconstructsOrderBook) {
    const std::string SYMBOL = "TEST-SYM";
    std::string wal = make_temp_wal_path();

    // Engine 1: start WAL and submit some orders
    MatchingEngine engine1;
    ASSERT_TRUE(engine1.startWAL(wal));

    auto o1 = std::make_shared<Order>("o1", SYMBOL, Order::Side::BUY, Order::Type::LIMIT, 100.0, 1.5);
    auto o2 = std::make_shared<Order>("o2", SYMBOL, Order::Side::SELL, Order::Type::LIMIT, 101.0, 2.0);

    engine1.submitOrder(o1);
    engine1.submitOrder(o2);

    // keep snapshot of state
    auto md1 = engine1.getMarketData(SYMBOL);
    size_t count1 = engine1.getOrderCount(SYMBOL);

    engine1.stopWAL();

    // Engine 2: replay WAL and compare state
    MatchingEngine engine2;
    ASSERT_TRUE(engine2.replayWAL(wal));

    auto md2 = engine2.getMarketData(SYMBOL);
    size_t count2 = engine2.getOrderCount(SYMBOL);

    EXPECT_EQ(count1, count2);
    EXPECT_EQ(md1.bids.size(), md2.bids.size());
    EXPECT_EQ(md1.asks.size(), md2.asks.size());
    if (!md1.bids.empty() && !md2.bids.empty()) {
        EXPECT_DOUBLE_EQ(md1.bids[0].first, md2.bids[0].first);
        EXPECT_DOUBLE_EQ(md1.bids[0].second, md2.bids[0].second);
    }
    if (!md1.asks.empty() && !md2.asks.empty()) {
        EXPECT_DOUBLE_EQ(md1.asks[0].first, md2.asks[0].first);
        EXPECT_DOUBLE_EQ(md1.asks[0].second, md2.asks[0].second);
    }

    // cleanup
    std::error_code ec;
    std::filesystem::remove(wal, ec);
}

TEST(WALTest, ReplayAppliesCancels) {
    const std::string SYMBOL = "TEST-SYM2";
    std::string wal = make_temp_wal_path();

    MatchingEngine engine1;
    ASSERT_TRUE(engine1.startWAL(wal));

    auto o1 = std::make_shared<Order>("o1", SYMBOL, Order::Side::BUY, Order::Type::LIMIT, 200.0, 3.0);
    engine1.submitOrder(o1);
    ASSERT_TRUE(engine1.cancelOrder("o1"));

    engine1.stopWAL();

    MatchingEngine engine2;
    ASSERT_TRUE(engine2.replayWAL(wal));

    EXPECT_EQ(engine2.getOrderCount(SYMBOL), 0);

    // cleanup
    std::error_code ec;
    std::filesystem::remove(wal, ec);
}

TEST(WALTest, ReplayIdempotent) {
    const std::string SYMBOL = "IDEMP";
    std::string wal = make_temp_wal_path();

    MatchingEngine engine1;
    ASSERT_TRUE(engine1.startWAL(wal));

    engine1.submitOrder(std::make_shared<Order>("a1", SYMBOL, Order::Side::BUY, Order::Type::LIMIT, 10.0, 1.0));
    engine1.submitOrder(std::make_shared<Order>("a2", SYMBOL, Order::Side::SELL, Order::Type::LIMIT, 11.0, 2.0));
    engine1.stopWAL();

    MatchingEngine engine2;
    ASSERT_TRUE(engine2.replayWAL(wal));
    // Replay again; should not duplicate orders
    ASSERT_TRUE(engine2.replayWAL(wal));

    EXPECT_EQ(engine2.getOrderCount(SYMBOL), 2);

    // cleanup
    std::error_code ec;
    std::filesystem::remove(wal, ec);
}

TEST(WALTest, LogsTrades) {
    const std::string SYMBOL = "WAL-TRADE";
    std::string wal = make_temp_wal_path();

    MatchingEngine engine;
    ASSERT_TRUE(engine.startWAL(wal));

    // Create opposite orders to generate a trade
    engine.submitOrder(std::make_shared<Order>("m1", SYMBOL, Order::Side::SELL, Order::Type::LIMIT, 10.0, 1.0));
    engine.submitOrder(std::make_shared<Order>("t1", SYMBOL, Order::Side::BUY, Order::Type::LIMIT, 10.0, 1.0));

    engine.stopWAL();

    // Read WAL file and look for a trade entry
    std::ifstream ifs(wal);
    ASSERT_TRUE(ifs.is_open());
    bool foundTrade = false;
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.find("\"type\":\"trade\"") != std::string::npos) { foundTrade = true; break; }
    }
    ifs.close();

    EXPECT_TRUE(foundTrade);

    std::error_code ec;
    std::filesystem::remove(wal, ec);
}

TEST(WALTest, ReplayAppliesModify) {
    const std::string SYMBOL = "MOD-TEST";
    std::string wal = make_temp_wal_path();

    MatchingEngine engine1;
    ASSERT_TRUE(engine1.startWAL(wal));

    // Submit order and then modify it
    auto o1 = std::make_shared<Order>("mod1", SYMBOL, Order::Side::BUY, Order::Type::LIMIT, 100.0, 5.0);
    engine1.submitOrder(o1);
    ASSERT_TRUE(engine1.modifyOrder("mod1", 3.0)); // Reduce quantity to 3.0

    auto md1 = engine1.getMarketData(SYMBOL);
    engine1.stopWAL();

    // Replay and verify modified quantity
    MatchingEngine engine2;
    ASSERT_TRUE(engine2.replayWAL(wal));

    auto md2 = engine2.getMarketData(SYMBOL);
    ASSERT_EQ(md1.bids.size(), md2.bids.size());
    if (!md1.bids.empty() && !md2.bids.empty()) {
        EXPECT_DOUBLE_EQ(md1.bids[0].second, 3.0); // Original modified quantity
        EXPECT_DOUBLE_EQ(md2.bids[0].second, 3.0); // Replayed quantity
    }

    std::error_code ec;
    std::filesystem::remove(wal, ec);
}

TEST(WALTest, ComplexReplaySubmitCancelModify) {
    const std::string SYMBOL = "COMPLEX";
    std::string wal = make_temp_wal_path();

    MatchingEngine engine1;
    ASSERT_TRUE(engine1.startWAL(wal));

    // Complex sequence: submit multiple, cancel some, modify others
    engine1.submitOrder(std::make_shared<Order>("c1", SYMBOL, Order::Side::BUY, Order::Type::LIMIT, 100.0, 1.0));
    engine1.submitOrder(std::make_shared<Order>("c2", SYMBOL, Order::Side::BUY, Order::Type::LIMIT, 99.0, 2.0));
    engine1.submitOrder(std::make_shared<Order>("c3", SYMBOL, Order::Side::SELL, Order::Type::LIMIT, 101.0, 1.5));
    
    engine1.cancelOrder("c2"); // Cancel second buy
    engine1.modifyOrder("c1", 0.5); // Modify first buy quantity
    
    engine1.submitOrder(std::make_shared<Order>("c4", SYMBOL, Order::Side::SELL, Order::Type::LIMIT, 102.0, 3.0));

    auto md1 = engine1.getMarketData(SYMBOL);
    size_t count1 = engine1.getOrderCount(SYMBOL);
    engine1.stopWAL();

    // Replay and verify exact state
    MatchingEngine engine2;
    ASSERT_TRUE(engine2.replayWAL(wal));

    auto md2 = engine2.getMarketData(SYMBOL);
    size_t count2 = engine2.getOrderCount(SYMBOL);

    EXPECT_EQ(count1, count2); // Should have 3 orders (c1 modified, c3, c4)
    EXPECT_EQ(count1, 3);
    
    // Verify BBO matches
    EXPECT_DOUBLE_EQ(md1.bestBidPrice, md2.bestBidPrice);
    EXPECT_DOUBLE_EQ(md1.bestBidQuantity, md2.bestBidQuantity);
    EXPECT_DOUBLE_EQ(md1.bestAskPrice, md2.bestAskPrice);
    EXPECT_DOUBLE_EQ(md1.bestAskQuantity, md2.bestAskQuantity);

    std::error_code ec;
    std::filesystem::remove(wal, ec);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
