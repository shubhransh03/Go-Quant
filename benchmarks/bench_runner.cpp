#include <iostream>
#include <chrono>
#include <vector>
#include <memory>
#include "engine/matching_engine.h"

int main(int argc, char **argv) {
    MatchingEngine engine;
    const std::string SYM = "BENCH-1";

    // Create initial liquidity
    for (int i = 0; i < 500; ++i) {
        auto b = engine.createOrder(SYM, Order::Side::BUY, Order::Type::LIMIT, 100.0 - i * 0.1, 1.0);
        engine.submitOrder(b);
        auto a = engine.createOrder(SYM, Order::Side::SELL, Order::Type::LIMIT, 100.0 + i * 0.1, 1.0);
        engine.submitOrder(a);
    }

    const int NUM = 10000;
    std::vector<double> latencies;
    latencies.reserve(NUM);

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < NUM; ++i) {
        auto o = engine.createOrder(SYM, (i % 2 == 0) ? Order::Side::BUY : Order::Side::SELL,
                                    Order::Type::MARKET, 0.0, 0.5);
        auto t0 = std::chrono::high_resolution_clock::now();
        engine.submitOrder(o);
        auto t1 = std::chrono::high_resolution_clock::now();
        latencies.push_back(std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
    }
    auto end = std::chrono::high_resolution_clock::now();

    double sum = 0.0;
    for (double v : latencies) sum += v;
    double avg = sum / latencies.size();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "Benchmark run: " << NUM << " market orders\n";
    std::cout << "Total time: " << totalMs << " ms\n";
    std::cout << "Avg latency per order: " << avg << " us\n";

    return 0;
}
