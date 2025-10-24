#ifndef RATE_LIMITER_H
#define RATE_LIMITER_H

#include <chrono>
#include <unordered_map>
#include <mutex>
#include <string>
#include "metrics_manager.h"

// Token bucket rate limiter with metrics tracking
class RateLimiter {
public:
    struct Limit {
        size_t maxBurst;    // Maximum burst size
        double tokensPerSec; // Token refill rate per second
        
        Limit(size_t burst, double rate) 
            : maxBurst(burst), tokensPerSec(rate) {}
    };

    RateLimiter(const std::string& name, size_t maxBurst, double tokensPerSec)
        : name_(name), limit_(maxBurst, tokensPerSec), tokens_(maxBurst),
          lastRefill_(std::chrono::steady_clock::now()) {}

    bool tryConsume(size_t tokens = 1) {
        std::lock_guard<std::mutex> lock(mutex_);
        refillTokens();

        if (tokens_ >= tokens) {
            tokens_ -= tokens;
            MetricsManager::instance().incrementRateLimiterAllowed(name_);
            return true;
        }

        MetricsManager::instance().incrementRateLimiterRejected(name_);
        return false;
    }

    double getTokens() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return tokens_;
    }

private:
    void refillTokens() {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<double>(now - lastRefill_).count();
        
        tokens_ = std::min(
            static_cast<double>(limit_.maxBurst),
            tokens_ + duration * limit_.tokensPerSec
        );
        
        lastRefill_ = now;
        
        // Update metrics
        MetricsManager::instance().setRateLimiterTokens(name_, tokens_);
    }

    std::string name_;
    Limit limit_;
    double tokens_;
    std::chrono::steady_clock::time_point lastRefill_;
    mutable std::mutex mutex_;
};

// Rate limiter manager for multiple symbols
class RateLimiterManager {
public:
    static RateLimiterManager& instance() {
        static RateLimiterManager instance;
        return instance;
    }

    void addSymbol(const std::string& symbol, 
                  size_t ordersPerSecond = 1000,
                  size_t maxBurst = 2000) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (limiters_.find(symbol) == limiters_.end()) {
            // Construct RateLimiter in-place to avoid copying non-copyable members (mutex)
            limiters_.try_emplace(symbol, symbol, maxBurst, static_cast<double>(ordersPerSecond));
        }
    }

    bool tryAcceptOrder(const std::string& symbol) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = limiters_.find(symbol);
        if (it == limiters_.end()) {
            addSymbol(symbol);
            it = limiters_.find(symbol);
        }
        return it->second.tryConsume();
    }

private:
    RateLimiterManager() = default;
    ~RateLimiterManager() = default;
    RateLimiterManager(const RateLimiterManager&) = delete;
    RateLimiterManager& operator=(const RateLimiterManager&) = delete;

    std::unordered_map<std::string, RateLimiter> limiters_;
    std::mutex mutex_;
};

#endif // RATE_LIMITER_H