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

    // If this is a trigger order (stop/stop-limit/take-profit), store it until activation
    if (order->getType() == Order::Type::STOP_LOSS || order->getType() == Order::Type::STOP_LIMIT || order->getType() == Order::Type::TAKE_PROFIT) {
        // WAL: record submission of trigger order
        if (walEnabled && !replaying) {
            json entry;
            entry["type"] = "submit";
            entry["order"] = {
                {"id", order->getId()},
                {"symbol", order->getSymbol()},
                {"side", order->getSide() == Order::Side::BUY ? "buy" : "sell"},
                {"order_type", static_cast<int>(order->getType())},
                {"price", order->getPrice()},
                {"quantity", order->getQuantity()}
            };
            walStream << entry.dump() << "\n";
            walStream.flush();
        }

        triggerOrders[symbol].push_back(order);
        return;
    }

    // WAL: record submission (so replay can reconstruct state)
    if (walEnabled && !replaying) {
        json entry;
        entry["type"] = "submit";
        entry["order"] = {
            {"id", order->getId()},
            {"symbol", order->getSymbol()},
            {"side", order->getSide() == Order::Side::BUY ? "buy" : "sell"},
            {"order_type", static_cast<int>(order->getType())},
            {"price", order->getPrice()},
            {"quantity", order->getQuantity()}
        };
        walStream << entry.dump() << "\n";
        walStream.flush();
    }

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
            // WAL: record cancel
            if (walEnabled && !replaying) {
                json entry;
                entry["type"] = "cancel";
                entry["order_id"] = orderId;
                walStream << entry.dump() << "\n";
                walStream.flush();
            }

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
            // WAL: record modify (quantity change)
            if (walEnabled && !replaying) {
                json entry;
                entry["type"] = "modify";
                entry["order_id"] = orderId;
                entry["new_quantity"] = newQuantity;
                walStream << entry.dump() << "\n";
                walStream.flush();
            }

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

size_t MatchingEngine::getTriggerOrderCount(const std::string &symbol) const {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = triggerOrders.find(symbol);
    if (it == triggerOrders.end()) return 0;
    return it->second.size();
}

void MatchingEngine::publishMarketDataUpdate(const std::string &symbol) {
    auto update = getMarketData(symbol);
    // Compute incremental diff vs last snapshot (per-level ops)
    std::lock_guard<std::mutex> lock(mutex);
    uint64_t &seq = marketDataSeqs[symbol];
    seq++;
    update.seqNum = seq;
    update.timestamp = std::chrono::system_clock::now();

    auto it = lastMarketSnapshot.find(symbol);
    if (it == lastMarketSnapshot.end()) {
        // First time, publish full snapshot
        update.type = MarketDataUpdate::Type::SNAPSHOT;
        lastMarketSnapshot[symbol] = update;

        auto sit = marketDataSubscribers.find(symbol);
        if (sit != marketDataSubscribers.end()) {
            for (const auto &callback : sit->second) { callback(update); }
        }
        return;
    }

    const auto &prev = it->second;
    // If top-of-book unchanged, but deeper levels changed, clients still need delta. Compute diffs.
    auto computeChanges = [](const std::vector<std::pair<double,double>> &oldV,
                             const std::vector<std::pair<double,double>> &newV) {
        std::vector<MarketDataUpdate::LevelChange> changes;
        // Build price->qty maps for quick lookups
        std::unordered_map<double,double> oldMap;
        for (const auto &lv : oldV) oldMap[lv.first] = lv.second;
        std::unordered_map<double,double> newMap;
        for (const auto &lv : newV) newMap[lv.first] = lv.second;

        // Detect adds and updates
        for (const auto &nv : newMap) {
            auto itOld = oldMap.find(nv.first);
            if (itOld == oldMap.end()) {
                MarketDataUpdate::LevelChange c; c.op = MarketDataUpdate::ChangeOp::ADD; c.price = nv.first; c.quantity = nv.second;
                changes.push_back(c);
            } else if (itOld->second != nv.second) {
                MarketDataUpdate::LevelChange c; c.op = MarketDataUpdate::ChangeOp::UPDATE; c.price = nv.first; c.quantity = nv.second;
                changes.push_back(c);
            }
        }

        // Detect removals
        for (const auto &ov : oldMap) {
            if (newMap.find(ov.first) == newMap.end()) {
                MarketDataUpdate::LevelChange c; c.op = MarketDataUpdate::ChangeOp::REMOVE; c.price = ov.first; c.quantity = 0.0;
                changes.push_back(c);
            }
        }

        return changes;
    };

    auto bidsChanges = computeChanges(prev.bids, update.bids);
    auto asksChanges = computeChanges(prev.asks, update.asks);

    if (bidsChanges.empty() && asksChanges.empty()) {
        // No significant change
        return;
    }

    MarketDataUpdate inc;
    inc.symbol = symbol;
    inc.timestamp = update.timestamp;
    inc.seqNum = update.seqNum;
    inc.prevSeqNum = prev.seqNum;
    inc.gap = (inc.prevSeqNum + 1) != inc.seqNum; // client can detect gap
    inc.type = MarketDataUpdate::Type::INCREMENT;
    inc.bestBidPrice = update.bestBidPrice;
    inc.bestBidQuantity = update.bestBidQuantity;
    inc.bestAskPrice = update.bestAskPrice;
    inc.bestAskQuantity = update.bestAskQuantity;
    inc.bidsChanges = std::move(bidsChanges);
    inc.asksChanges = std::move(asksChanges);

    lastMarketSnapshot[symbol] = update;

    auto sit = marketDataSubscribers.find(symbol);
    if (sit != marketDataSubscribers.end()) {
        for (const auto &cb : sit->second) cb(inc);
    }
}

// Check and activate any trigger orders for the given symbol using last trade price / BBO
void MatchingEngine::checkTriggers(const std::string &symbol, double lastTradePrice) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = triggerOrders.find(symbol);
    if (it == triggerOrders.end()) return;

    auto &vec = it->second;
    std::vector<std::shared_ptr<Order>> remaining;
    for (auto &ord : vec) {
        bool triggered = false;
        if (ord->getType() == Order::Type::STOP_LOSS) {
            if (ord->getSide() == Order::Side::SELL) {
                if (lastTradePrice <= ord->getPrice()) triggered = true;
            } else {
                if (lastTradePrice >= ord->getPrice()) triggered = true;
            }
        } else if (ord->getType() == Order::Type::TAKE_PROFIT) {
            if (ord->getSide() == Order::Side::SELL) {
                if (lastTradePrice >= ord->getPrice()) triggered = true;
            } else {
                if (lastTradePrice <= ord->getPrice()) triggered = true;
            }
        } else if (ord->getType() == Order::Type::STOP_LIMIT) {
            // Trigger the limit when last trade crosses price
            if (ord->getSide() == Order::Side::SELL) {
                if (lastTradePrice <= ord->getPrice()) triggered = true;
            } else {
                if (lastTradePrice >= ord->getPrice()) triggered = true;
            }
        }

        if (triggered) {
            // activate: create an active order
            if (ord->getType() == Order::Type::STOP_LIMIT) {
                // create a LIMIT order at same price
                auto act = std::make_shared<Order>(ord->getId(), ord->getSymbol(), ord->getSide(), Order::Type::LIMIT, ord->getPrice(), ord->getQuantity());
                matchingAlgorithm.processOrder(getOrCreateOrderBook(symbol), act);
            } else {
                // create a MARKET order
                auto act = std::make_shared<Order>(ord->getId(), ord->getSymbol(), ord->getSide(), Order::Type::MARKET, 0.0, ord->getQuantity());
                matchingAlgorithm.processOrder(getOrCreateOrderBook(symbol), act);
            }

            // Record activation in WAL
            if (walEnabled && !replaying) {
                json entry;
                entry["type"] = "activated";
                entry["order_id"] = ord->getId();
                entry["symbol"] = symbol;
                walStream << entry.dump() << "\n";
                walStream.flush();
            }
        } else {
            remaining.push_back(ord);
        }
    }

    triggerOrders[symbol] = std::move(remaining);
}

void MatchingEngine::publishTrade(const Trade &trade) {
    std::lock_guard<std::mutex> lock(mutex);

    // Stamp per-symbol trade seqNum
    uint64_t &tseq = tradeSeqs[trade.symbol];
    tseq++;
    Trade t = trade; // make a copy we can modify
    t.seqNum = tseq;
    t.timestamp = std::chrono::system_clock::now();

    // Store in trade history
    auto &dq = tradeHistory[t.symbol];
    dq.push_back(t);
    if (dq.size() > 1000) { dq.pop_front(); }

    // Notify subscribers
    auto it = tradeSubscribers.find(t.symbol);
    if (it != tradeSubscribers.end()) {
        for (const auto &callback : it->second) { callback(t); }
    }

    // WAL: record trade
    if (walEnabled && !replaying) {
        json entry;
        entry["type"] = "trade";
        entry["trade"] = {
            {"trade_id", t.tradeId},
            {"symbol", t.symbol},
            {"price", t.price},
            {"quantity", t.quantity},
            {"maker_order_id", t.makerOrderId},
            {"taker_order_id", t.takerOrderId},
            {"aggressor_side", t.aggressorSide},
            {"maker_fee", t.makerFee},
            {"taker_fee", t.takerFee},
            {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(t.timestamp.time_since_epoch()).count()},
            {"seqNum", t.seqNum}
        };
        walStream << entry.dump() << "\n";
        walStream.flush();
    }

    // Metrics
    ++metric_trades_executed;

    // After trade, check triggers for this symbol using trade price
    checkTriggers(t.symbol, t.price);
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
                // If this is a trigger order, store it; otherwise apply to book if missing
                OrderBook &book = getOrCreateOrderBook(symbol);
                if (t == Order::Type::STOP_LOSS || t == Order::Type::STOP_LIMIT || t == Order::Type::TAKE_PROFIT) {
                    triggerOrders[symbol].push_back(order);
                } else {
                    // Idempotency: skip submit if the order already exists in the book
                    if (!book.hasOrder(id)) {
                        matchingAlgorithm.processOrder(book, order);
                    }
                }
            } else if (type == "cancel") {
                std::string orderId = j["order_id"].get<std::string>();
                cancelOrder(orderId);
            } else if (type == "modify") {
                std::string orderId = j.value("order_id", "");
                double newQty = j.value("new_quantity", 0.0);
                // Apply modify during replay if order exists; ignore if not
                try {
                    // modifyOrder will throw if order not found; catch and ignore for replay
                    modifyOrder(orderId, newQty);
                } catch (...) {
                    // ignore
                }
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