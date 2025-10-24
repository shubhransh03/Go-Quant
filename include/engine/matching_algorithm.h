#ifndef MATCHING_ALGORITHM_H
#define MATCHING_ALGORITHM_H

#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include "order_book.h"
#include "fee_model.h"

class Trade {
  public:
    std::string tradeId;
    std::string symbol;
    double price;
    double quantity;
    double makerFee;
    double takerFee;
    std::string makerOrderId;
    std::string takerOrderId;
    std::string aggressorSide;  // "buy" or "sell"
    std::chrono::system_clock::time_point timestamp;
    uint64_t seqNum;  // Sequence number for trade feed ordering

    Trade(const std::string &symbol, double price, double quantity, const std::string &makerOrderId,
          const std::string &takerOrderId, const std::string &aggressorSide)
        : symbol(symbol), price(price), quantity(quantity), makerOrderId(makerOrderId), takerOrderId(takerOrderId),
          aggressorSide(aggressorSide), timestamp(std::chrono::system_clock::now()), makerFee(0.0), takerFee(0.0) {
        // Generate unique trade ID
        tradeId = generateTradeId();
    }

  private:
    static std::string generateTradeId() {
        static std::atomic<uint64_t> counter(0);
        return "TRD" + std::to_string(++counter);
    }
};

class MatchingAlgorithm {
  public:
    MatchingAlgorithm() = default;

    void setFeeModel(FeeModel *fm) { feeModel_ = fm; }

    // Main matching function for new orders
    std::vector<Trade> processOrder(OrderBook &orderBook, const std::shared_ptr<Order> &order);

  private:
    // Helper functions for different order types
    std::vector<Trade> processMarketOrder(OrderBook &orderBook, const std::shared_ptr<Order> &order);
    std::vector<Trade> processLimitOrder(OrderBook &orderBook, const std::shared_ptr<Order> &order);
    std::vector<Trade> processIOCOrder(OrderBook &orderBook, const std::shared_ptr<Order> &order);
    std::vector<Trade> processFOKOrder(OrderBook &orderBook, const std::shared_ptr<Order> &order);

    // Trade execution helper
  // Execute a trade and update the order book state. Returns the Trade record.
  Trade executeTrade(OrderBook &orderBook, const std::shared_ptr<Order> &makerOrder,
             const std::shared_ptr<Order> &takerOrder, double quantity);

  private:
    FeeModel *feeModel_{nullptr};
};

#endif // MATCHING_ALGORITHM_H