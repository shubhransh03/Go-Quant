#include <gtest/gtest.h>
#include "engine/matching_algorithm.h"
#include "engine/order_book.h"

class MatchingAlgorithmTest : public ::testing::Test {
protected:
    OrderBook orderBook;
    MatchingAlgorithm matchingAlgorithm;

    void SetUp() override {
        // Initialize order book and matching algorithm if needed
    }

    void TearDown() override {
        // Clean up resources if needed
    }
};

TEST_F(MatchingAlgorithmTest, TestPriceTimePriority) {
    // Add test cases to verify price-time priority matching
    Order buyOrder = {1, "BUY", 100.0, 10}; // Example order
    Order sellOrder = {2, "SELL", 99.0, 5}; // Example order

    orderBook.addOrder(buyOrder);
    orderBook.addOrder(sellOrder);

    auto matchedOrders = matchingAlgorithm.match(orderBook);

    EXPECT_EQ(matchedOrders.size(), 1);
    EXPECT_EQ(matchedOrders[0].price, 99.0);
}

TEST_F(MatchingAlgorithmTest, TestInternalOrderProtection) {
    // Add test cases to verify internal order protection
    Order order1 = {1, "BUY", 100.0, 10};
    Order order2 = {2, "SELL", 100.0, 5};

    orderBook.addOrder(order1);
    orderBook.addOrder(order2);

    auto matchedOrders = matchingAlgorithm.match(orderBook);

    EXPECT_EQ(matchedOrders.size(), 1);
    EXPECT_EQ(matchedOrders[0].price, 100.0);
    EXPECT_EQ(orderBook.getOrderQuantity(order1.id), 5); // Check remaining quantity
}

TEST_F(MatchingAlgorithmTest, TestOrderCancellation) {
    // Add test cases to verify order cancellation functionality
    Order order = {1, "BUY", 100.0, 10};
    orderBook.addOrder(order);
    orderBook.cancelOrder(order.id);

    EXPECT_EQ(orderBook.getOrderQuantity(order.id), 0); // Ensure order is canceled
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}