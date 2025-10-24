#include "engine/matching_engine.h"
#include "engine/matching_algorithm.h"
#include "engine/order_book.h"
#include "utils/logging.h"
#include "utils/rate_limiter.h"
#include "utils/order_pool.h"
#include "utils/metrics_manager.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;
namespace fs = std::filesystem;

void MatchingEngine::submitOrder(std::shared_ptr<Order> order) {
    const std::string &symbol = order->getSymbol();

    // Rate limit per symbol
    if (!RateLimiterManager::instance().tryAcceptOrder(symbol)) {
        throw std::runtime_error("Rate limit exceeded for symbol: " + symbol);
    }

    // Track latency for end-to-end processing
    auto latencyTracker = MetricsManager::instance().trackOrderLatency();

    std::lock_guard<std::mutex> lock(mutex);
    metric_orders_received++;
    MetricsManager::instance().incrementOrdersReceived(symbol);

    auto &book = getOrCreateOrderBook(symbol);
    std::vector<Trade> trades = matchingAlgorithm.processOrder(book, order);

    if (!trades.empty()) {
        MetricsManager::instance().incrementOrdersMatched(symbol);
    }
    metric_trades_executed += trades.size();

    // Update book depth metric
    MetricsManager::instance().updateBookDepth(symbol, book.getOrderCount());

    // Publish trades and market data
    for (const auto &t : trades) {
        publishTrade(t);
    }

    // Pool metrics
    auto stats = OrderPool::instance().getStats();
    MetricsManager::instance().updatePoolMetrics(stats.orderCapacity, stats.orderCapacity - stats.orderAvailable);

    publishMarketDataUpdate(symbol);
}

bool MatchingEngine::cancelOrder(const std::string &orderId) {
    std::lock_guard<std::mutex> lock(mutex);

    for (auto &[symbol, orderBook] : orderBooks) {
        if (orderBook->cancelOrder(orderId)) {
            ++metric_orders_cancelled;
            publishMarketDataUpdate(symbol);
            return true;
        }
    }

    return false;
}

bool MatchingEngine::modifyOrder(const std::string &orderId, double newQuantity) {
    std::lock_guard<std::mutex> lock(mutex);

    for (auto &[symbol, orderBook] : orderBooks) {
        try {
            orderBook->modifyOrder(orderId, newQuantity);
            publishMarketDataUpdate(symbol);
            return true;
        } catch (const std::runtime_error &) { continue; }
    }

    return false;
}

MarketDataUpdate MatchingEngine::getMarketData(const std::string &symbol, size_t depth) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = orderBooks.find(symbol);
    if (it == orderBooks.end()) {
        MarketDataUpdate empty;
        empty.symbol = symbol;
        empty.timestamp = std::chrono::system_clock::now();
        empty.type = MarketDataUpdate::Type::SNAPSHOT;
        empty.seqNum = 0;
        empty.bestBidPrice = 0;
        empty.bestBidQuantity = 0;
        empty.bestAskPrice = 0;
        empty.bestAskQuantity = 0;
        return empty;
    }

    const auto &orderBook = it->second;
    MarketDataUpdate update;
    update.symbol = symbol;
    update.timestamp = std::chrono::system_clock::now();
    update.bids = orderBook->getTopBids(depth);
    update.asks = orderBook->getTopAsks(depth);
    update.type = MarketDataUpdate::Type::SNAPSHOT;
    update.seqNum = 0;
    update.bestBidPrice = orderBook->getBestBidPrice();
    update.bestBidQuantity = orderBook->getBestBidQuantity();
    update.bestAskPrice = orderBook->getBestAskPrice();
    update.bestAskQuantity = orderBook->getBestAskQuantity();

    return update;
}

std::vector<Trade> MatchingEngine::getRecentTrades(const std::string &symbol, size_t count) const {
    std::lock_guard<std::mutex> lock(mutex);

    auto it = tradeHistory.find(symbol);
    if (it == tradeHistory.end()) { return {}; }

    const auto &dq = it->second;
    std::vector<Trade> out;
    const size_t num = std::min(count, dq.size());
    out.reserve(num);
    for (size_t i = dq.size() - num; i < dq.size(); ++i) {
        out.push_back(dq[i]);
    }
    return out;
}

void MatchingEngine::subscribeToMarketData(const std::string &symbol, MarketDataCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    marketDataSubscribers[symbol].push_back(callback);
}

void MatchingEngine::subscribeToTrades(const std::string &symbol, TradeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    tradeSubscribers[symbol].push_back(callback);
}

bool MatchingEngine::hasSymbol(const std::string &symbol) const {
    std::lock_guard<std::mutex> lock(mutex);
    return orderBooks.find(symbol) != orderBooks.end();
}

size_t MatchingEngine::getOrderCount(const std::string &symbol) const {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = orderBooks.find(symbol);
    return it == orderBooks.end() ? 0 : it->second->getOrderCount();
}

void MatchingEngine::publishMarketDataUpdate(const std::string &symbol) {
    auto update = getMarketData(symbol);

    auto it = marketDataSubscribers.find(symbol);
    if (it != marketDataSubscribers.end()) {
        for (const auto &callback : it->second) { callback(update); }
    }
}

void MatchingEngine::publishTrade(const Trade &trade) {
    // Store in trade history
    auto &dq = tradeHistory[trade.symbol];
    dq.push_back(trade);
    if (dq.size() > 1000) { dq.pop_front(); }

    // Notify subscribers
    auto it = tradeSubscribers.find(trade.symbol);
    if (it != tradeSubscribers.end()) {
        for (const auto &callback : it->second) { callback(trade); }
    }
    // WAL: record trade
    if (walEnabled && !replaying) {
        json entry;
        entry["type"] = "trade";
        entry["trade"] = {
            {"trade_id", trade.tradeId},
            {"symbol", trade.symbol},
            {"price", trade.price},
            {"quantity", trade.quantity},
            {"maker_order_id", trade.makerOrderId},
            {"taker_order_id", trade.takerOrderId},
            {"aggressor_side", trade.aggressorSide},
            {"maker_fee", trade.makerFee},
            {"taker_fee", trade.takerFee}
        };
        walStream << entry.dump() << "\n";
        walStream.flush();
    }
    // Metrics
    ++metric_trades_executed;
}

std::string MatchingEngine::getMetricsJSON() const {
    std::lock_guard<std::mutex> lock(mutex);
    json j;
    j["orders_received"] = metric_orders_received.load();
    j["orders_cancelled"] = metric_orders_cancelled.load();
    j["trades_executed"] = metric_trades_executed.load();
    j["symbols_tracked"] = orderBooks.size();
    return j.dump();
}

bool MatchingEngine::startWAL(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex);
    try {
        walStream.open(path, std::ios::app);
        if (!walStream.is_open()) return false;
        walEnabled = true;
        return true;
    } catch (...) { return false; }
}

void MatchingEngine::stopWAL() {
    std::lock_guard<std::mutex> lock(mutex);
    walEnabled = false;
    if (walStream.is_open()) walStream.close();
}

bool MatchingEngine::replayWAL(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex);
    try {
        std::ifstream ifs(path);
        if (!ifs.is_open()) return false;
        replaying = true;
        std::string line;
        while (std::getline(ifs, line)) {
            if (line.empty()) continue;
            auto j = json::parse(line);
            std::string type = j.value("type", "");
            if (type == "submit") {
                auto oj = j["order"];
                std::string id = oj.value("id", "");
                std::string symbol = oj.value("symbol", "");
                std::string side = oj.value("side", "buy");
                int ot = oj.value("order_type", static_cast<int>(Order::Type::LIMIT));
                double price = oj.value("price", 0.0);
                double quantity = oj.value("quantity", 0.0);
                Order::Side s = (side == "buy") ? Order::Side::BUY : Order::Side::SELL;
                Order::Type t = static_cast<Order::Type>(ot);
                auto order = std::make_shared<Order>(id, symbol, s, t, price, quantity);
                // During replay, do not WAL again
                matchingAlgorithm.processOrder(getOrCreateOrderBook(symbol), order);
            } else if (type == "cancel") {
                std::string orderId = j["order_id"].get<std::string>();
                cancelOrder(orderId);
            } else if (type == "trade") {
                // Trades are produced by matching; no-op on replay or could be used for audit logs
            }
        }
        replaying = false;
        return true;
    } catch (const std::exception &e) {
        utils::log_error(std::string("Failed to replay WAL: ") + e.what());
        replaying = false;
        return false;
    }
}

OrderBook &MatchingEngine::getOrCreateOrderBook(const std::string &symbol) {
    auto it = orderBooks.find(symbol);
    if (it == orderBooks.end()) {
        auto [newIt, inserted] = orderBooks.emplace(symbol, std::make_unique<OrderBook>(symbol));
        return *newIt->second;
    }
    return *it->second;
}

bool MatchingEngine::saveState(const std::string &path) const {
    std::lock_guard<std::mutex> lock(mutex);
    try {
        if (!fs::exists(path)) fs::create_directories(path);
        for (const auto &p : orderBooks) {
            const auto &symbol = p.first;
            const auto &book = p.second;
            std::string jsonStr = book->toJson();
            std::string filename = (fs::path(path) / (symbol + ".json")).string();
            std::ofstream ofs(filename);
            ofs << jsonStr;
            ofs.close();
        }
        return true;
    } catch (const std::exception &e) {
        utils::log_error(std::string("Failed to save state: ") + e.what());
        return false;
    }
}

bool MatchingEngine::loadState(const std::string &path) {
    std::lock_guard<std::mutex> lock(mutex);
    try {
        if (!fs::exists(path) || !fs::is_directory(path)) return false;
        for (const auto &entry : fs::directory_iterator(path)) {
            if (!entry.is_regular_file()) continue;
            auto filename = entry.path().filename().string();
            if (entry.path().extension() != ".json") continue;
            auto symbol = entry.path().stem().string();
            std::ifstream ifs(entry.path());
            std::stringstream ss;
            ss << ifs.rdbuf();
            std::string jsonStr = ss.str();
            auto &book = getOrCreateOrderBook(symbol);
            book.fromJson(jsonStr);
        }
        return true;
    } catch (const std::exception &e) {
        utils::log_error(std::string("Failed to load state: ") + e.what());
        return false;
    }
}