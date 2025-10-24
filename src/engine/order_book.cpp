#include "engine/order_book.h"
#include <algorithm>
#include <mutex>
#include <stdexcept>
#include <nlohmann/json.hpp>
using json = nlohmann::json;

OrderBook::OrderBook(const std::string &symbol) : symbol(symbol) {}

void OrderBook::addOrder(std::shared_ptr<Order> order) {
    std::lock_guard<std::recursive_mutex> lock(mutex);

    double p = order->getPrice();
    if (order->getSide() == Order::Side::BUY) {
        auto it = buyLevels.find(p);
        if (it == buyLevels.end()) {
            it = buyLevels.emplace(p, PriceLevel(p)).first;
        }
        auto &level = it->second;
        level.orders.push_back(order);
        level.totalQuantity += order->getQuantity();
    } else {
        auto it = sellLevels.find(p);
        if (it == sellLevels.end()) {
            it = sellLevels.emplace(p, PriceLevel(p)).first;
        }
        auto &level = it->second;
        level.orders.push_back(order);
        level.totalQuantity += order->getQuantity();
    }
    orderMap[order->getId()] = order;
}

void OrderBook::modifyOrder(const std::string &orderId, double newQuantity) {
    std::lock_guard<std::recursive_mutex> lock(mutex);

    auto orderIt = orderMap.find(orderId);
    if (orderIt == orderMap.end()) { throw std::runtime_error("Order not found"); }

    auto order = orderIt->second;
    auto oldQuantity = order->getQuantity();
    double p2 = order->getPrice();
    if (order->getSide() == Order::Side::BUY) {
        auto it = buyLevels.find(p2);
        if (it == buyLevels.end()) return; // nothing to modify
        auto &level = it->second;
        level.totalQuantity = level.totalQuantity - oldQuantity + newQuantity;
        order->modifyQuantity(newQuantity);
        if (level.totalQuantity <= 0) { buyLevels.erase(p2); }
    } else {
        auto it = sellLevels.find(p2);
        if (it == sellLevels.end()) return; // nothing to modify
        auto &level = it->second;
        level.totalQuantity = level.totalQuantity - oldQuantity + newQuantity;
        order->modifyQuantity(newQuantity);
        if (level.totalQuantity <= 0) { sellLevels.erase(p2); }
    }
}

bool OrderBook::cancelOrder(const std::string &orderId) {
    std::lock_guard<std::recursive_mutex> lock(mutex);

    auto orderIt = orderMap.find(orderId);
    if (orderIt == orderMap.end()) { return false; }

    auto order = orderIt->second;
    double p3 = order->getPrice();
    if (order->getSide() == Order::Side::BUY) {
        auto it = buyLevels.find(p3);
        if (it == buyLevels.end()) { orderMap.erase(orderId); return true; }
        auto &level = it->second;
        level.totalQuantity -= order->getQuantity();
        level.orders.remove_if([&orderId](const auto &o) { return o->getId() == orderId; });
        if (level.orders.empty()) { buyLevels.erase(it); }
    } else {
        auto it = sellLevels.find(p3);
        if (it == sellLevels.end()) { orderMap.erase(orderId); return true; }
        auto &level = it->second;
        level.totalQuantity -= order->getQuantity();
        level.orders.remove_if([&orderId](const auto &o) { return o->getId() == orderId; });
        if (level.orders.empty()) { sellLevels.erase(it); }
    }

    orderMap.erase(orderId);
    return true;
}

double OrderBook::getBestBidPrice() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return buyLevels.empty() ? 0.0 : buyLevels.begin()->first;
}

double OrderBook::getBestAskPrice() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return sellLevels.empty() ? 0.0 : sellLevels.begin()->first;
}

double OrderBook::getBestBidQuantity() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return buyLevels.empty() ? 0.0 : buyLevels.begin()->second.totalQuantity;
}

double OrderBook::getBestAskQuantity() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return sellLevels.empty() ? 0.0 : sellLevels.begin()->second.totalQuantity;
}

std::vector<std::pair<double, double>> OrderBook::getTopBids(size_t levels) const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    std::vector<std::pair<double, double>> result;

    for (const auto &level : buyLevels) {
        if (result.size() >= levels) break;
        result.emplace_back(level.first, level.second.totalQuantity);
    }

    return result;
}

std::vector<std::pair<double, double>> OrderBook::getTopAsks(size_t levels) const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    std::vector<std::pair<double, double>> result;

    for (const auto &level : sellLevels) {
        if (result.size() >= levels) break;
        result.emplace_back(level.first, level.second.totalQuantity);
    }

    return result;
}

std::vector<std::shared_ptr<Order>> OrderBook::getMatchingOrders(const Order &incomingOrder) const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    std::vector<std::shared_ptr<Order>> matches;

    double remainingQty = incomingOrder.getQuantity();
    if (incomingOrder.getSide() == Order::Side::BUY) {
        for (const auto &level : sellLevels) {
            if (incomingOrder.getType() != Order::Type::MARKET) {
                if (level.first > incomingOrder.getPrice()) { break; }
            }
            for (const auto &order : level.second.orders) {
                if (remainingQty <= 0) break;
                matches.push_back(order);
                remainingQty -= order->getQuantity();
            }
        }
    } else {
        for (const auto &level : buyLevels) {
            if (incomingOrder.getType() != Order::Type::MARKET) {
                if (level.first < incomingOrder.getPrice()) { break; }
            }
            for (const auto &order : level.second.orders) {
                if (remainingQty <= 0) break;
                matches.push_back(order);
                remainingQty -= order->getQuantity();
            }
        }
    }

    return matches;
}

bool OrderBook::hasMatchingOrders(const Order &order) const {
    std::lock_guard<std::recursive_mutex> lock(mutex);

    // Market orders match if any opposite-side liquidity exists
    if (order.getType() == Order::Type::MARKET) {
        if (order.getSide() == Order::Side::BUY) return !sellLevels.empty();
        return !buyLevels.empty();
    }

    if (order.getSide() == Order::Side::BUY) {
        return !sellLevels.empty() && sellLevels.begin()->first <= order.getPrice();
    } else {
        return !buyLevels.empty() && buyLevels.begin()->first >= order.getPrice();
    }
}

size_t OrderBook::getOrderCount() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return orderMap.size();
}

double OrderBook::decreaseOrderQuantity(const std::string &orderId, double amount) {
    std::lock_guard<std::recursive_mutex> lock(mutex);

    auto it = orderMap.find(orderId);
    if (it == orderMap.end()) return 0.0;

    auto order = it->second;
    double oldQty = order->getQuantity();
    double newQty = oldQty - amount;
    if (newQty < 0) newQty = 0.0;

    // Adjust price level total
    if (order->getSide() == Order::Side::BUY) {
        auto lvlIt = buyLevels.find(order->getPrice());
        if (lvlIt != buyLevels.end()) {
            lvlIt->second.totalQuantity -= (oldQty - newQty);
            if (lvlIt->second.totalQuantity < 0) lvlIt->second.totalQuantity = 0;
            if (newQty <= 0.0) {
                lvlIt->second.orders.remove_if([&orderId](const auto &o) { return o->getId() == orderId; });
            }
            if (lvlIt->second.orders.empty()) { buyLevels.erase(lvlIt); }
        }
    } else {
        auto lvlIt = sellLevels.find(order->getPrice());
        if (lvlIt != sellLevels.end()) {
            lvlIt->second.totalQuantity -= (oldQty - newQty);
            if (lvlIt->second.totalQuantity < 0) lvlIt->second.totalQuantity = 0;
            if (newQty <= 0.0) {
                lvlIt->second.orders.remove_if([&orderId](const auto &o) { return o->getId() == orderId; });
            }
            if (lvlIt->second.orders.empty()) { sellLevels.erase(lvlIt); }
        }
    }

    if (newQty <= 0.0) {
        orderMap.erase(it);
    } else {
        order->modifyQuantity(newQty);
    }

    return newQty;
}

std::string OrderBook::toJson() const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    json j;
    j["symbol"] = symbol;
    j["bids"] = json::array();
    j["asks"] = json::array();

    for (const auto &lvl : buyLevels) {
        json level;
        level["price"] = lvl.first;
        level["totalQuantity"] = lvl.second.totalQuantity;
        level["orders"] = json::array();
        for (const auto &o : lvl.second.orders) {
            json oj;
            oj["id"] = o->getId();
            oj["side"] = (o->getSide() == Order::Side::BUY) ? "buy" : "sell";
            oj["type"] = static_cast<int>(o->getType());
            oj["price"] = o->getPrice();
            oj["quantity"] = o->getQuantity();
            oj["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(o->getTimestamp().time_since_epoch()).count();
            level["orders"].push_back(oj);
        }
        j["bids"].push_back(level);
    }

    for (const auto &lvl : sellLevels) {
        json level;
        level["price"] = lvl.first;
        level["totalQuantity"] = lvl.second.totalQuantity;
        level["orders"] = json::array();
        for (const auto &o : lvl.second.orders) {
            json oj;
            oj["id"] = o->getId();
            oj["side"] = (o->getSide() == Order::Side::BUY) ? "buy" : "sell";
            oj["type"] = static_cast<int>(o->getType());
            oj["price"] = o->getPrice();
            oj["quantity"] = o->getQuantity();
            oj["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(o->getTimestamp().time_since_epoch()).count();
            level["orders"].push_back(oj);
        }
        j["asks"].push_back(level);
    }

    return j.dump();
}

void OrderBook::fromJson(const std::string &jsonStr) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    json j = json::parse(jsonStr);
    // Clear existing
    buyLevels.clear();
    sellLevels.clear();
    orderMap.clear();

    if (j.contains("bids")) {
        for (const auto &lvl : j["bids"]) {
            double price = lvl["price"].get<double>();
            PriceLevel pl(price);
            pl.totalQuantity = lvl.value("totalQuantity", 0.0);
            for (const auto &oj : lvl["orders"]) {
                std::string id = oj["id"].get<std::string>();
                std::string side = oj.value("side", "buy");
                Order::Side s = (side == "buy") ? Order::Side::BUY : Order::Side::SELL;
                Order::Type t = static_cast<Order::Type>(oj.value("type", static_cast<int>(Order::Type::LIMIT)));
                double priceVal = oj.value("price", price);
                double quantity = oj.value("quantity", 0.0);
                auto order = std::make_shared<Order>(id, symbol, s, t, priceVal, quantity);
                pl.orders.push_back(order);
                orderMap[id] = order;
            }
            buyLevels.emplace(price, std::move(pl));
        }
    }

    if (j.contains("asks")) {
        for (const auto &lvl : j["asks"]) {
            double price = lvl["price"].get<double>();
            PriceLevel pl(price);
            pl.totalQuantity = lvl.value("totalQuantity", 0.0);
            for (const auto &oj : lvl["orders"]) {
                std::string id = oj["id"].get<std::string>();
                std::string side = oj.value("side", "sell");
                Order::Side s = (side == "buy") ? Order::Side::BUY : Order::Side::SELL;
                Order::Type t = static_cast<Order::Type>(oj.value("type", static_cast<int>(Order::Type::LIMIT)));
                double priceVal = oj.value("price", price);
                double quantity = oj.value("quantity", 0.0);
                auto order = std::make_shared<Order>(id, symbol, s, t, priceVal, quantity);
                pl.orders.push_back(order);
                orderMap[id] = order;
            }
            sellLevels.emplace(price, std::move(pl));
        }
    }
}

bool OrderBook::hasOrder(const std::string &orderId) const {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    return orderMap.find(orderId) != orderMap.end();
}