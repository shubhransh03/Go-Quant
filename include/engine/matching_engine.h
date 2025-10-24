#ifndef MATCHING_ENGINE_H
#define MATCHING_ENGINE_H

#include <memory>
#include <vector>
#include "matching_algorithm.h"
#include "order_book.h"

#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include <atomic>
#include <fstream>

#include "../utils/metrics_manager.h"
#include "../utils/system_metrics.h"
#include "../utils/rate_limiter.h"
#include "../utils/ring_buffer.h"
#include "../utils/order_pool.h"

struct MarketDataUpdate {
    std::string symbol;
    std::chrono::system_clock::time_point timestamp;
    // BBO
    double bestBidPrice;
    double bestBidQuantity;
    double bestAskPrice;
    double bestAskQuantity;
    // Full depth
    std::vector<std::pair<double, double>> bids;
    std::vector<std::pair<double, double>> asks;
  // Incremental per-level changes (for INCREMENT updates)
  enum class ChangeOp { ADD = 0, UPDATE = 1, REMOVE = 2 };
  struct LevelChange {
    ChangeOp op;
    double price;
    double quantity; // ignored for REMOVE
  };
  std::vector<LevelChange> bidsChanges;
  std::vector<LevelChange> asksChanges;
  // Previous sequence number (for gap detection on client side)
  uint64_t prevSeqNum{0};
  bool gap{false};
    // Update type
    enum class Type { SNAPSHOT, INCREMENT } type;
    uint64_t seqNum;  // Sequence number for ordering updates
};

class MatchingEngine {
  public:
  MatchingEngine() = default; // Metrics and system metrics should be started by the application, not implicitly here
    
  ~MatchingEngine() = default;

    // Order operations
        // Order submission helpers using the pool
    std::shared_ptr<Order> createOrder(const std::string &symbol, Order::Side side,
                                     Order::Type type, double price, double quantity) {
        return OrderPool::instance().createOrder(
            generateOrderId(), symbol, side, type, price, quantity);
    }

    void submitOrder(std::shared_ptr<Order> order);
    bool cancelOrder(const std::string &orderId);
    bool modifyOrder(const std::string &orderId, double newQuantity);

    // Market data operations
    MarketDataUpdate getMarketData(const std::string &symbol, size_t depth = 10) const;
    std::vector<Trade> getRecentTrades(const std::string &symbol, size_t count = 100) const;

    // Market data subscription
    using MarketDataCallback = std::function<void(const MarketDataUpdate &)>;
    using TradeCallback = std::function<void(const Trade &)>;

    void subscribeToMarketData(const std::string &symbol, MarketDataCallback callback);
    void subscribeToTrades(const std::string &symbol, TradeCallback callback);

  // Fee model
  void setFeeModel(FeeModel *fm) { matchingAlgorithm.setFeeModel(fm); }

    // Utility functions
    bool hasSymbol(const std::string &symbol) const;
    size_t getOrderCount(const std::string &symbol) const;
  // Expose trigger order counts for testing/persistence checks
  size_t getTriggerOrderCount(const std::string &symbol) const;

    // Metrics
    std::string getMetricsJSON() const;

  // Write-Ahead Log (WAL) for durability
  bool startWAL(const std::string &path);
  void stopWAL();
  bool replayWAL(const std::string &path);

  // Persistence
  bool saveState(const std::string &path) const;
  bool loadState(const std::string &path);

  private:
    // Internal data structures
    std::unordered_map<std::string, std::unique_ptr<OrderBook>> orderBooks;
    MatchingAlgorithm matchingAlgorithm;

    // Market data dissemination
    std::unordered_map<std::string, std::vector<MarketDataCallback>> marketDataSubscribers;
    std::unordered_map<std::string, std::vector<TradeCallback>> tradeSubscribers;

    // Lock-free buffers for market data and trades
    std::unordered_map<std::string, std::unique_ptr<RingBuffer<MarketDataUpdate, 1024>>> marketDataBuffers;
    std::unordered_map<std::string, std::unique_ptr<RingBuffer<Trade, 1024>>> tradeBuffers;
  // Last published market snapshot per-symbol for incremental updates
  std::unordered_map<std::string, MarketDataUpdate> lastMarketSnapshot;

  // Trigger orders (stop/limit/take-profit) stored until activation
  std::unordered_map<std::string, std::vector<std::shared_ptr<Order>>> triggerOrders;

    // Trade history (limited size, circular buffer)
  std::unordered_map<std::string, std::deque<Trade>> tradeHistory;

    // Order ID generation
    std::atomic<uint64_t> orderIdCounter_{0};
    std::string generateOrderId() {
        return "O" + std::to_string(++orderIdCounter_);
    }

  // Sequence numbers per-symbol for market data and trades
  std::unordered_map<std::string, uint64_t> marketDataSeqs;
  std::unordered_map<std::string, uint64_t> tradeSeqs;

    // Internal helper functions
    void publishMarketDataUpdate(const std::string &symbol);
    void publishTrade(const Trade &trade);
    OrderBook &getOrCreateOrderBook(const std::string &symbol);
    void checkTriggers(const std::string &symbol, double lastTradePrice);
    
    // Unlocked versions for internal use (assume mutex already held)
    bool cancelOrderUnlocked(const std::string &orderId, std::string &affectedSymbol);
    bool modifyOrderUnlocked(const std::string &orderId, double newQuantity, std::string &affectedSymbol);

    // Thread safety
    mutable std::mutex mutex;

  // WAL state
  mutable std::ofstream walStream;
  bool walEnabled{false};
  bool replaying{false};
  // Runtime metrics
  std::atomic<uint64_t> metric_orders_received{0};
  std::atomic<uint64_t> metric_orders_cancelled{0};
  std::atomic<uint64_t> metric_orders_matched{0};
  std::atomic<uint64_t> metric_trades_executed{0};
};

#endif // MATCHING_ENGINE_H