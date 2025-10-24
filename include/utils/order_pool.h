#ifndef ORDER_POOL_H
#define ORDER_POOL_H

#include "memory_pool.h"
#include "../engine/order_book.h"
#include "../engine/matching_algorithm.h"
#include <unordered_map>

class OrderPool {
public:
    static OrderPool& instance() {
        static OrderPool instance;
        return instance;
    }

    // Allocate a new order
    template<typename... Args>
    std::shared_ptr<Order> createOrder(Args&&... args) {
        return pool_.allocate(std::forward<Args>(args)...);
    }

    // Note: MarketDataUpdate and Trade pooling removed to avoid tight coupling.

    // Stats for monitoring
    struct PoolStats {
        size_t orderCapacity;
        size_t orderAvailable;
    size_t marketDataCapacity;   // deprecated
    size_t marketDataAvailable;  // deprecated
    size_t tradeCapacity;        // deprecated
    size_t tradeAvailable;       // deprecated
    };

    PoolStats getStats() const {
        return {
            pool_.capacity(),
            pool_.available(),
            0,
            0,
            0,
            0
        };
    }

private:
    OrderPool() = default;
    ~OrderPool() = default;
    OrderPool(const OrderPool&) = delete;
    OrderPool& operator=(const OrderPool&) = delete;

    // Memory pools for different types
    MemoryPool<Order, 10000> pool_;
};

#endif // ORDER_POOL_H