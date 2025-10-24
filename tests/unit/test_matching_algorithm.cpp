#include <gtest/gtest.h>
#include <memory>
#include "engine/matching_algorithm.h"
#include "engine/order_book.h"

class MatchingAlgorithmTest : public ::testing::Test {
protected:
    OrderBook orderBook;
    MatchingAlgorithm matchingAlgorithm;

    MatchingAlgorithmTest() : orderBook("AAPL") {}

    void SetUp() override {
        // Initialize order book and matching algorithm if needed
    }

    void TearDown() override {
        // Clean up resources if needed
    }
};

TEST_F(MatchingAlgorithmTest, TestPriceTimePriority) {
    // Add test cases to verify price-time priority matching
    auto buyOrder = std::make_shared<Order>("1", "AAPL", Order::Side::BUY, Order::Type::LIMIT, 100.0, 10);
    auto sellOrder = std::make_shared<Order>("2", "AAPL", Order::Side::SELL, Order::Type::LIMIT, 99.0, 5);

    orderBook.addOrder(buyOrder);
    orderBook.addOrder(sellOrder);

    // Process the sell order against existing buy orders
    auto trades = matchingAlgorithm.processOrder(orderBook, sellOrder);

    EXPECT_GT(trades.size(), 0);
    if (trades.size() > 0) {
        EXPECT_EQ(trades[0].price, 100.0); // Should match at the buy order price
    }
}

TEST_F(MatchingAlgorithmTest, TestInternalOrderProtection) {
    // Add test cases to verify internal order protection
    auto order1 = std::make_shared<Order>("1", "AAPL", Order::Side::BUY, Order::Type::LIMIT, 100.0, 10);
    auto order2 = std::make_shared<Order>("2", "AAPL", Order::Side::SELL, Order::Type::LIMIT, 100.0, 5);

    orderBook.addOrder(order1);
    
    // Process the sell order against the buy order
    auto trades = matchingAlgorithm.processOrder(orderBook, order2);

    EXPECT_GT(trades.size(), 0);
    if (trades.size() > 0) {
        EXPECT_EQ(trades[0].price, 100.0);
        EXPECT_EQ(trades[0].quantity, 5.0);
    }
    // Check that order1 has remaining quantity of 5
    EXPECT_TRUE(orderBook.hasOrder("1"));
}

TEST_F(MatchingAlgorithmTest, TestOrderCancellation) {
    // Add test cases to verify order cancellation functionality
    auto order = std::make_shared<Order>("1", "AAPL", Order::Side::BUY, Order::Type::LIMIT, 100.0, 10);
    orderBook.addOrder(order);
    orderBook.cancelOrder("1");

    EXPECT_FALSE(orderBook.hasOrder("1")); // Ensure order is canceled
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}