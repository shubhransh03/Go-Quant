#include <gtest/gtest.h>
#include "engine/order_book.h"

class OrderBookTest : public ::testing::Test {
protected:
    OrderBook order_book;

    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};

TEST_F(OrderBookTest, AddOrder) {
    Order order1 = {1, 100.0, 10, OrderType::LIMIT, Side::BUY};
    order_book.addOrder(order1);
    EXPECT_EQ(order_book.getOrderCount(), 1);
}

TEST_F(OrderBookTest, ModifyOrder) {
    Order order1 = {1, 100.0, 10, OrderType::LIMIT, Side::BUY};
    order_book.addOrder(order1);
    
    Order modified_order = {1, 101.0, 10, OrderType::LIMIT, Side::BUY};
    order_book.modifyOrder(modified_order);
    
    EXPECT_EQ(order_book.getBestBidPrice(), 101.0);
}

TEST_F(OrderBookTest, CancelOrder) {
    Order order1 = {1, 100.0, 10, OrderType::LIMIT, Side::BUY};
    order_book.addOrder(order1);
    
    order_book.cancelOrder(1);
    EXPECT_EQ(order_book.getOrderCount(), 0);
}

TEST_F(OrderBookTest, GetBestBidAndAsk) {
    Order buy_order = {1, 100.0, 10, OrderType::LIMIT, Side::BUY};
    Order sell_order = {2, 101.0, 10, OrderType::LIMIT, Side::SELL};
    
    order_book.addOrder(buy_order);
    order_book.addOrder(sell_order);
    
    EXPECT_EQ(order_book.getBestBidPrice(), 100.0);
    EXPECT_EQ(order_book.getBestAskPrice(), 101.0);
}