#ifndef METRICS_MANAGER_H
#define METRICS_MANAGER_H

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <memory>
#include <string>
#include <chrono>
#include <mutex>
#include <unordered_map>

class MetricsManager {
public:
    static MetricsManager& instance() {
        static MetricsManager instance;
        return instance;
    }

    void start(const std::string& address = "0.0.0.0:9090") {
        // Always create a registry so metrics can be used even if the HTTP exposer fails.
        registry_ = std::make_shared<prometheus::Registry>();
        try {
            exposer_ = std::make_unique<prometheus::Exposer>(address);
            exposer_->RegisterCollectable(registry_);
        } catch (const std::exception&) {
            // If binding the metrics HTTP server fails (e.g., port in use), disable the exposer
            // but still keep the registry so unit tests and non-server contexts can proceed.
            exposer_.reset();
        }
        
        // Initialize order counters with symbol label
        auto& order_family = prometheus::BuildCounter()
            .Name("matching_engine_orders_total")
            .Help("Total number of orders processed")
            .Register(*registry_);
            
        orders_total_family_ = &order_family;
        
        // Initialize rate limiter metrics
        auto& rate_limit_family = prometheus::BuildCounter()
            .Name("rate_limiter_total")
            .Help("Rate limiter statistics")
            .Register(*registry_);
            
        rate_limit_family_ = &rate_limit_family;
        
        // Rate limiter gauge for current tokens
        auto& token_gauge_family = prometheus::BuildGauge()
            .Name("rate_limiter_tokens")
            .Help("Current token bucket levels")
            .Register(*registry_);
            
        token_gauge_family_ = &token_gauge_family;
        
        // Initialize symbol-specific gauges
        auto& symbol_gauge_family = prometheus::BuildGauge()
            .Name("symbol_stats")
            .Help("Per-symbol statistics")
            .Register(*registry_);
        symbol_gauge_family_ = &symbol_gauge_family;
        
        // Initialize latency histograms
        auto& latency_family = prometheus::BuildHistogram()
            .Name("matching_engine_latency_microseconds")
            .Help("Order processing latency in microseconds")
            .Register(*registry_);
            
        order_latency_ = &latency_family.Add(
            {{"type", "order_processing"}},
            std::vector<double>{10, 50, 100, 250, 500, 1000, 2500, 5000}
        );
        
        // Initialize gauges
        auto& pool_family = prometheus::BuildGauge()
            .Name("memory_pool_usage")
            .Help("Memory pool utilization")
            .Register(*registry_);
            
        order_pool_capacity_ = &pool_family.Add({{"type", "order_capacity"}});
        order_pool_used_ = &pool_family.Add({{"type", "order_used"}});
        
        // Book depth gauges
        auto& book_family = prometheus::BuildGauge()
            .Name("order_book_depth")
            .Help("Order book depth by symbol")
            .Register(*registry_);
            
        book_depth_gauge_ = &book_family.Add({{"type", "total_orders"}});

    // System metrics gauges
    system_metrics_family_ = &prometheus::BuildGauge()
            .Name("system_metrics")
            .Help("System metrics such as CPU, memory, threads")
            .Register(*registry_);
    }

    // Per-symbol counter increments
    void incrementOrdersReceived(const std::string& symbol) {
        getOrCreateCounter(orders_total_family_, "received", symbol)->Increment();
    }
    
    void incrementOrdersMatched(const std::string& symbol) {
        getOrCreateCounter(orders_total_family_, "matched", symbol)->Increment();
    }
    
    void incrementOrdersCancelled(const std::string& symbol) {
        getOrCreateCounter(orders_total_family_, "cancelled", symbol)->Increment();
    }
    
    // Rate limiter metrics
    void incrementRateLimiterAllowed(const std::string& symbol) {
        getOrCreateCounter(rate_limit_family_, "allowed", symbol)->Increment();
    }
    
    void incrementRateLimiterRejected(const std::string& symbol) {
        getOrCreateCounter(rate_limit_family_, "rejected", symbol)->Increment();
    }
    
    void setRateLimiterTokens(const std::string& symbol, double tokens) {
        getOrCreateGauge(token_gauge_family_, "current", symbol)->Set(tokens);
    }
    
    // Symbol-specific metrics
    void updateSymbolPrice(const std::string& symbol, double price, const std::string& side) {
        getOrCreateGauge(symbol_gauge_family_, side + "_price", symbol)->Set(price);
    }
    
    void updateSymbolVolume(const std::string& symbol, double volume) {
        getOrCreateGauge(symbol_gauge_family_, "volume_24h", symbol)->Set(volume);
    }
    
    void updateSymbolTrades(const std::string& symbol, size_t count) {
        getOrCreateGauge(symbol_gauge_family_, "trades_24h", symbol)->Set(count);
    }

    // Latency tracking
    void observeOrderLatency(double microseconds) {
        order_latency_->Observe(microseconds);
    }

    // Pool metrics
    void updatePoolMetrics(size_t capacity, size_t used) {
        order_pool_capacity_->Set(capacity);
        order_pool_used_->Set(used);
    }

    // Book depth
    void updateBookDepth(const std::string& symbol, size_t depth) {
        book_depth_gauge_->Set(depth);
    }

    // System metrics (used by SystemMetrics collector)
    void setSystemMetric(const std::string& name, double value) {
        getOrCreateSystemGauge(name)->Set(value);
    }

    // RAII latency tracker
    class LatencyTracker {
    public:
        LatencyTracker(prometheus::Histogram* histogram)
            : histogram_(histogram), start_(std::chrono::high_resolution_clock::now()) {}
            
        ~LatencyTracker() {
            auto duration = std::chrono::high_resolution_clock::now() - start_;
            auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
            histogram_->Observe(static_cast<double>(microseconds));
        }
        
    private:
        prometheus::Histogram* histogram_;
        std::chrono::time_point<std::chrono::high_resolution_clock> start_;
    };

    // Get a latency tracker
    LatencyTracker trackOrderLatency() {
        return LatencyTracker(order_latency_);
    }

    // Stop and clean up the exposer/registry (useful during shutdown or tests)
    void stop() {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            if (exposer_) {
                exposer_.reset();
            }
            if (registry_) {
                registry_.reset();
            }
        } catch (...) {
            // swallow
        }
    }

private:
    MetricsManager() = default;
    ~MetricsManager() = default;
    MetricsManager(const MetricsManager&) = delete;
    MetricsManager& operator=(const MetricsManager&) = delete;

    // Helper methods for creating/accessing metrics
    prometheus::Counter* getOrCreateCounter(
        prometheus::Family<prometheus::Counter>* family,
        const std::string& type,
        const std::string& symbol
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = type + "_" + symbol;
        auto it = counters_.find(key);
        if (it == counters_.end()) {
            auto& counter = family->Add({
                {"type", type},
                {"symbol", symbol}
            });
            counters_[key] = &counter;
            return &counter;
        }
        return it->second;
    }

    prometheus::Gauge* getOrCreateGauge(
        prometheus::Family<prometheus::Gauge>* family,
        const std::string& type,
        const std::string& symbol
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = type + "_" + symbol;
        auto it = gauges_.find(key);
        if (it == gauges_.end()) {
            auto& gauge = family->Add({
                {"type", type},
                {"symbol", symbol}
            });
            gauges_[key] = &gauge;
            return &gauge;
        }
        return it->second;
    }

    prometheus::Gauge* getOrCreateSystemGauge(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string key = "system_" + name;
        auto it = gauges_.find(key);
        if (it == gauges_.end()) {
            auto& gauge = system_metrics_family_->Add({
                {"type", name}
            });
            gauges_[key] = &gauge;
            return &gauge;
        }
        return it->second;
    }

    std::unique_ptr<prometheus::Exposer> exposer_;
    std::shared_ptr<prometheus::Registry> registry_;
    std::mutex mutex_;

    // Metric families
    prometheus::Family<prometheus::Counter>* orders_total_family_;
    prometheus::Family<prometheus::Counter>* rate_limit_family_;
    prometheus::Family<prometheus::Gauge>* token_gauge_family_;
    prometheus::Family<prometheus::Gauge>* symbol_gauge_family_;
    prometheus::Family<prometheus::Gauge>* system_metrics_family_;
    prometheus::Histogram* order_latency_;
    prometheus::Gauge* order_pool_capacity_;
    prometheus::Gauge* order_pool_used_;
    prometheus::Gauge* book_depth_gauge_;

    // Metric caches
    std::unordered_map<std::string, prometheus::Counter*> counters_;
    std::unordered_map<std::string, prometheus::Gauge*> gauges_;
};

#endif // METRICS_MANAGER_H