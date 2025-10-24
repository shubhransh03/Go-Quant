#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <chrono>
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

            // Persist a compact view using public API (top levels with aggregated quantities)
            // Choose a reasonable depth; callers can adjust by post-processing if needed.
            constexpr size_t DEPTH = 100;
            auto bids = orderBook.getTopBids(DEPTH);
            auto asks = orderBook.getTopAsks(DEPTH);
            j["bids"] = json::array();
            for (const auto &p : bids) j["bids"].push_back({{"price", p.first}, {"quantity", p.second}});
            j["asks"] = json::array();
            for (const auto &p : asks) j["asks"].push_back({{"price", p.first}, {"quantity", p.second}});

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

            auto orderBook = std::make_unique<OrderBook>(j["symbol"].get<std::string>());

            // Recreate a simple representation: one LIMIT order per level with aggregated quantity
            int idx = 0;
            if (j.contains("bids")) {
                for (const auto &lv : j["bids"]) {
                    std::string id = "B_" + std::to_string(idx++);
                    auto order = std::make_shared<Order>(id, orderBook->getSymbol(), Order::Side::BUY, Order::Type::LIMIT,
                                                         lv["price"].get<double>(), lv["quantity"].get<double>());
                    orderBook->addOrder(order);
                }
            }
            idx = 0;
            if (j.contains("asks")) {
                for (const auto &lv : j["asks"]) {
                    std::string id = "A_" + std::to_string(idx++);
                    auto order = std::make_shared<Order>(id, orderBook->getSymbol(), Order::Side::SELL, Order::Type::LIMIT,
                                                         lv["price"].get<double>(), lv["quantity"].get<double>());
                    orderBook->addOrder(order);
                }
            }

            return orderBook;
        } catch (const std::exception &e) {
            // Log error
            return nullptr;
        }
    }
};

#endif // PERSISTENCE_H