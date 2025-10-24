#include <gtest/gtest.h>
#include <memory>
#include "engine/order_book.h"

class OrderBookTest : public ::testing::Test {
protected:
    OrderBook order_book;

    OrderBookTest() : order_book("AAPL") {}

    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(OrderBookTest, AddOrder) {
    auto order1 = std::make_shared<Order>("1", "AAPL", Order::Side::BUY, Order::Type::LIMIT, 100.0, 10);
    order_book.addOrder(order1);
    EXPECT_EQ(order_book.getOrderCount(), 1);
}

TEST_F(OrderBookTest, ModifyOrder) {
    auto order1 = std::make_shared<Order>("1", "AAPL", Order::Side::BUY, Order::Type::LIMIT, 100.0, 10);
    order_book.addOrder(order1);
    
    // Modify the order quantity to 15
    order_book.modifyOrder("1", 15.0);
    
    EXPECT_EQ(order_book.getBestBidPrice(), 100.0);
    EXPECT_TRUE(order_book.hasOrder("1"));
}

TEST_F(OrderBookTest, CancelOrder) {
    auto order1 = std::make_shared<Order>("1", "AAPL", Order::Side::BUY, Order::Type::LIMIT, 100.0, 10);
    order_book.addOrder(order1);
    
    order_book.cancelOrder("1");
    EXPECT_EQ(order_book.getOrderCount(), 0);
}

TEST_F(OrderBookTest, GetBestBidAndAsk) {
    auto buy_order = std::make_shared<Order>("1", "AAPL", Order::Side::BUY, Order::Type::LIMIT, 100.0, 10);
    auto sell_order = std::make_shared<Order>("2", "AAPL", Order::Side::SELL, Order::Type::LIMIT, 101.0, 10);
    
    order_book.addOrder(buy_order);
    order_book.addOrder(sell_order);
    
    EXPECT_EQ(order_book.getBestBidPrice(), 100.0);
    EXPECT_EQ(order_book.getBestAskPrice(), 101.0);
}