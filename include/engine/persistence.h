#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include "order_book.h"

using json = nlohmann::json;

class OrderBookPersistence {
  public:
    // Save order book state to file
    static bool saveOrderBook(const std::string &symbol, const OrderBook &orderBook, const std::string &filepath) {
        try {
            json j;
            j["symbol"] = symbol;
            j["timestamp"] = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

            // Save buy orders
            json buyOrders = json::array();
            for (const auto &level : orderBook.getBuyLevels()) {
                for (const auto &order : level.second.orders) {
                    buyOrders.push_back({{"id", order->getId()},
                                         {"price", order->getPrice()},
                                         {"quantity", order->getQuantity()},
                                         {"timestamp", std::chrono::system_clock::to_time_t(order->getTimestamp())}});
                }
            }
            j["buy_orders"] = buyOrders;

            // Save sell orders
            json sellOrders = json::array();
            for (const auto &level : orderBook.getSellLevels()) {
                for (const auto &order : level.second.orders) {
                    sellOrders.push_back({{"id", order->getId()},
                                          {"price", order->getPrice()},
                                          {"quantity", order->getQuantity()},
                                          {"timestamp", std::chrono::system_clock::to_time_t(order->getTimestamp())}});
                }
            }
            j["sell_orders"] = sellOrders;

            // Write to file
            std::ofstream file(filepath);
            file << j.dump(4);
            return true;
        } catch (const std::exception &e) {
            // Log error
            return false;
        }
    }

    // Load order book state from file
    static std::unique_ptr<OrderBook> loadOrderBook(const std::string &filepath) {
        try {
            std::ifstream file(filepath);
            json j;
            file >> j;

            auto orderBook = std::make_unique<OrderBook>(j["symbol"]);

            // Load buy orders
            for (const auto &orderData : j["buy_orders"]) {
                auto order = std::make_shared<Order>(orderData["id"], j["symbol"], Order::Side::BUY, Order::Type::LIMIT,
                                                     orderData["price"], orderData["quantity"]);
                orderBook->addOrder(order);
            }

            // Load sell orders
            for (const auto &orderData : j["sell_orders"]) {
                auto order = std::make_shared<Order>(orderData["id"], j["symbol"], Order::Side::SELL,
                                                     Order::Type::LIMIT, orderData["price"], orderData["quantity"]);
                orderBook->addOrder(order);
            }

            return orderBook;
        } catch (const std::exception &e) {
            // Log error
            return nullptr;
        }
    }
};

#endif // PERSISTENCE_H