#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include "engine/matching_engine.h"

void benchmarkLatency(MatchingEngine& engine, int numOrders) {
    std::vector<std::shared_ptr<Order>> orders;
    for (int i = 0; i < numOrders; ++i) {
        orders.push_back(std::make_shared<Order>(
            "ORDER_" + std::to_string(i),
            "BTC-USDT",
            Order::Side::BUY,
            Order::Type::LIMIT,
            50000.0 + i,
            1.0
        ));
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (const auto& order : orders) {
        engine.submitOrder(order);
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> latency = end - start;

    std::cout << "Processed " << numOrders << " orders in " 
              << latency.count() << " microseconds." << std::endl;
    std::cout << "Latency per order: " 
              << latency.count() / numOrders << " microseconds." << std::endl;
}

int main(int argc, char* argv[]) {
    MatchingEngine engine;

    int numOrders = 1000; // Example number of orders to benchmark
    benchmarkLatency(engine, numOrders);

    return 0;
}