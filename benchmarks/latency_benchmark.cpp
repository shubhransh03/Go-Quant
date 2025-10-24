#include <iostream>
#include <chrono>
#include <vector>
#include "engine/matching_engine.h"
#include "utils/metrics.h"

void benchmarkLatency(MatchingEngine& engine, int numOrders) {
    std::vector<Order> orders;
    for (int i = 0; i < numOrders; ++i) {
        orders.emplace_back(Order{i, "BUY", 100 + i, i}); // Sample orders
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
    engine.initialize();

    int numOrders = 1000; // Example number of orders to benchmark
    benchmarkLatency(engine, numOrders);

    return 0;
}