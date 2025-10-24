#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include "engine/fee_model.h"
#include "engine/matching_engine.h"

class AdvancedOrderTest : public ::testing::Test {
  protected:
    MatchingEngine engine;
    const std::string TEST_SYMBOL = "BTC-USDT";
    FeeModel feeModel;

    void SetUp() override {
        // Set up fee model
        feeModel.setFeeSchedule(TEST_SYMBOL, FeeSchedule(-0.0002, 0.0005)); // -0.02% maker, 0.05% taker
        engine.setFeeModel(&feeModel);
    }

    std::shared_ptr<Order> createOrder(const std::string &id, Order::Side side, Order::Type type, double price,
                                       double quantity) {
        auto order = std::make_shared<Order>(id, TEST_SYMBOL, side, type, price, quantity);
        return order;
    }
};

TEST_F(AdvancedOrderTest, TestStopLossOrder) {
    // Create initial market price by adding trades
    engine.submitOrder(createOrder("sell1", Order::Side::SELL, Order::Type::LIMIT, 100.0, 1.0));
    engine.submitOrder(createOrder("buy1", Order::Side::BUY, Order::Type::LIMIT, 100.0, 1.0));

    // Place a stop loss order that should trigger when price goes below 95
    // For stop orders, the price field represents the trigger price
    auto stopLoss = createOrder("stop1", Order::Side::SELL, Order::Type::STOP_LOSS, 95.0, 1.0);
    engine.submitOrder(stopLoss);

    // Price is still above stop price, order shouldn't execute
    EXPECT_EQ(engine.getOrderCount(TEST_SYMBOL), 1);

    // Create a trade that pushes price below stop price
    engine.submitOrder(createOrder("sell2", Order::Side::SELL, Order::Type::LIMIT, 94.0, 1.0));
    engine.submitOrder(createOrder("buy2", Order::Side::BUY, Order::Type::LIMIT, 94.0, 1.0));

    // Stop loss should have triggered
    EXPECT_EQ(engine.getOrderCount(TEST_SYMBOL), 0);
}

TEST_F(AdvancedOrderTest, TestStopLimitOrder) {
    // Set up initial market price
    engine.submitOrder(createOrder("sell1", Order::Side::SELL, Order::Type::LIMIT, 100.0, 1.0));
    engine.submitOrder(createOrder("buy1", Order::Side::BUY, Order::Type::LIMIT, 100.0, 1.0));

    // Place stop limit order: trigger at 95 (price field is the trigger price for stop orders)
    auto stopLimit = createOrder("stop1", Order::Side::SELL, Order::Type::STOP_LIMIT, 95.0, 1.0);
    engine.submitOrder(stopLimit);

    // Verify order not yet triggered
    EXPECT_EQ(engine.getOrderCount(TEST_SYMBOL), 1);

    // Push price below stop price
    engine.submitOrder(createOrder("sell2", Order::Side::SELL, Order::Type::LIMIT, 94.5, 1.0));
    engine.submitOrder(createOrder("buy2", Order::Side::BUY, Order::Type::LIMIT, 94.5, 1.0));

    // Stop limit should have triggered and placed a limit order
    auto orderBook = engine.getMarketData(TEST_SYMBOL);
    EXPECT_EQ(orderBook.asks[0].first, 94.0);
}

TEST_F(AdvancedOrderTest, TestTakeProfitOrder) {
    // Set up initial market price
    engine.submitOrder(createOrder("sell1", Order::Side::SELL, Order::Type::LIMIT, 100.0, 1.0));
    engine.submitOrder(createOrder("buy1", Order::Side::BUY, Order::Type::LIMIT, 100.0, 1.0));

    // Place take profit order that triggers at 110 (price field is the trigger price)
    auto takeProfit = createOrder("tp1", Order::Side::SELL, Order::Type::TAKE_PROFIT, 110.0, 1.0);
    engine.submitOrder(takeProfit);

    // Verify order not triggered
    EXPECT_EQ(engine.getOrderCount(TEST_SYMBOL), 1);

    // Push price above take profit level
    engine.submitOrder(createOrder("sell2", Order::Side::SELL, Order::Type::LIMIT, 111.0, 1.0));
    engine.submitOrder(createOrder("buy2", Order::Side::BUY, Order::Type::LIMIT, 111.0, 1.0));

    // Take profit should have triggered
    EXPECT_EQ(engine.getOrderCount(TEST_SYMBOL), 0);
}

TEST_F(AdvancedOrderTest, TestFeeCalculation) {
    // Place maker order
    auto makerOrder = createOrder("sell1", Order::Side::SELL, Order::Type::LIMIT, 100.0, 1.0);
    engine.submitOrder(makerOrder);

    std::vector<Trade> trades;
    engine.subscribeToTrades(TEST_SYMBOL, [&](const Trade &trade) { trades.push_back(trade); });

    // Submit taker order
    auto takerOrder = createOrder("buy1", Order::Side::BUY, Order::Type::MARKET, 0.0, 1.0);
    engine.submitOrder(takerOrder);

    ASSERT_EQ(trades.size(), 1);
    const auto &trade = trades[0];

    // Verify fees
    EXPECT_EQ(trade.makerFee, -0.02); // 0.02% rebate
    EXPECT_EQ(trade.takerFee, 0.05);  // 0.05% fee
    EXPECT_EQ(trade.price * trade.quantity * 0.0005, trade.takerFee);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}