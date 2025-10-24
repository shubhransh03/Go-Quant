#include "engine/matching_algorithm.h"
#include "engine/order_book.h"
#include <algorithm>
#include <stdexcept>
#include <vector>

std::vector<Trade> MatchingAlgorithm::processOrder(OrderBook &orderBook, const std::shared_ptr<Order> &order) {
    // Ensure atomicity per order against this order book
    auto lock = orderBook.acquireLock();
    switch (order->getType()) {
    case Order::Type::MARKET: return processMarketOrder(orderBook, order);
    case Order::Type::LIMIT: return processLimitOrder(orderBook, order);
    case Order::Type::IOC: return processIOCOrder(orderBook, order);
    case Order::Type::FOK: return processFOKOrder(orderBook, order);
    default: throw std::invalid_argument("Unknown order type");
    }
}

std::vector<Trade> MatchingAlgorithm::processMarketOrder(OrderBook &orderBook,
                                                         const std::shared_ptr<Order> &order) {
    std::vector<Trade> trades;
    double remainingQty = order->getQuantity();

    while (remainingQty > 0) {
        auto matchingOrders = orderBook.getMatchingOrders(*order);
        if (matchingOrders.empty()) break;

        for (const auto &matchingOrder : matchingOrders) {
            if (remainingQty <= 0) break;
            double tradeQty = std::min(remainingQty, matchingOrder->getQuantity());
            trades.push_back(executeTrade(orderBook, matchingOrder, order, tradeQty));
            remainingQty -= tradeQty;
        }
    }
    return trades;
}

std::vector<Trade> MatchingAlgorithm::processLimitOrder(OrderBook &orderBook, const std::shared_ptr<Order> &order) {
    std::vector<Trade> trades;
    double remainingQty = order->getQuantity();

    while (remainingQty > 0 && orderBook.hasMatchingOrders(*order)) {
        auto matchingOrders = orderBook.getMatchingOrders(*order);
        if (matchingOrders.empty()) break;

        for (const auto &matchingOrder : matchingOrders) {
            if (remainingQty <= 0) break;
            double tradeQty = std::min(remainingQty, matchingOrder->getQuantity());
            trades.push_back(executeTrade(orderBook, matchingOrder, order, tradeQty));
            remainingQty -= tradeQty;
        }
    }

    if (remainingQty > 0) {
        order->modifyQuantity(remainingQty);
        orderBook.addOrder(order);
    }
    return trades;
}

std::vector<Trade> MatchingAlgorithm::processIOCOrder(OrderBook &orderBook, const std::shared_ptr<Order> &order) {
    std::vector<Trade> trades;
    double remainingQty = order->getQuantity();

    while (remainingQty > 0 && orderBook.hasMatchingOrders(*order)) {
        auto matchingOrders = orderBook.getMatchingOrders(*order);
        if (matchingOrders.empty()) break;

        for (const auto &matchingOrder : matchingOrders) {
            if (remainingQty <= 0) break;
            double tradeQty = std::min(remainingQty, matchingOrder->getQuantity());
            trades.push_back(executeTrade(orderBook, matchingOrder, order, tradeQty));
            remainingQty -= tradeQty;
        }
    }
    // Remainder cancels automatically
    return trades;
}

std::vector<Trade> MatchingAlgorithm::processFOKOrder(OrderBook &orderBook, const std::shared_ptr<Order> &order) {
    // Check fillability first
    double totalAvailableQty = 0.0;
    auto matchingOrders = orderBook.getMatchingOrders(*order);
    for (const auto &mo : matchingOrders) {
        totalAvailableQty += mo->getQuantity();
        if (totalAvailableQty >= order->getQuantity()) break;
    }
    if (totalAvailableQty < order->getQuantity()) return {};

    std::vector<Trade> trades;
    double remainingQty = order->getQuantity();
    for (const auto &mo : matchingOrders) {
        if (remainingQty <= 0) break;
        double tradeQty = std::min(remainingQty, mo->getQuantity());
        trades.push_back(executeTrade(orderBook, mo, order, tradeQty));
        remainingQty -= tradeQty;
    }
    return trades;
}

Trade MatchingAlgorithm::executeTrade(OrderBook &orderBook, const std::shared_ptr<Order> &makerOrder,
                                      const std::shared_ptr<Order> &takerOrder, double quantity) {
    // Price-time priority: execute at maker price
    const double execPrice = makerOrder->getPrice();
    orderBook.decreaseOrderQuantity(makerOrder->getId(), quantity);
    orderBook.decreaseOrderQuantity(takerOrder->getId(), quantity);

    Trade t(makerOrder->getSymbol(), execPrice, quantity, makerOrder->getId(),
            takerOrder->getId(), takerOrder->getSide() == Order::Side::BUY ? "buy" : "sell");

    // Use FeeModel if provided, otherwise fallback to defaults
    if (feeModel_) {
        auto calc = feeModel_->calculateFees(makerOrder->getSymbol(), execPrice, quantity);
        // FeeModel::calculateFees returns absolute amounts for maker/taker fees
        // Maker rebate is expressed via negative makerFee in schedule; we assign makerFee as (makerFee - makerRebate)
        t.makerFee = calc.makerFee - calc.makerRebate;
        t.takerFee = calc.takerFee;
    } else {
        constexpr double makerRate = 0.001; // 0.1%
        constexpr double takerRate = 0.002; // 0.2%
        t.makerFee = execPrice * quantity * makerRate;
        t.takerFee = execPrice * quantity * takerRate;
    }
    return t;
}