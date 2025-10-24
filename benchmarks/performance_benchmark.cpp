#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <vector>
#include "engine/matching_engine.h"

class BenchmarkResults {
  public:
    double avgOrderLatency;
    double p95OrderLatency;
    double p99OrderLatency;
    double throughput;
    double bboUpdateLatency;
    double tradeGenerationLatency;

    void print() const {
        std::cout << std::fixed << std::setprecision(3) << "\nBenchmark Results:\n"
                  << "==================\n"
                  << "Order Processing:\n"
                  << "  Average Latency: " << avgOrderLatency << " μs\n"
                  << "  95th Percentile: " << p95OrderLatency << " μs\n"
                  << "  99th Percentile: " << p99OrderLatency << " μs\n"
                  << "  Throughput: " << throughput << " orders/sec\n"
                  << "\nMarket Data:\n"
                  << "  BBO Update Latency: " << bboUpdateLatency << " μs\n"
                  << "  Trade Generation Latency: " << tradeGenerationLatency << " μs\n";
    }
};

class MatchingEngineBenchmark {
  public:
    MatchingEngineBenchmark() { engine = std::make_unique<MatchingEngine>(); }

    BenchmarkResults runBenchmark(int numOrders = 100000) {
        std::vector<std::chrono::nanoseconds> orderLatencies;
        std::vector<std::chrono::nanoseconds> bboLatencies;
        std::vector<std::chrono::nanoseconds> tradeLatencies;

        // Create some initial liquidity
        setupLiquidity();

        // Prepare random orders
        std::vector<std::shared_ptr<Order>> orders = generateRandomOrders(numOrders);

        // Measure order processing
        auto startTime = std::chrono::high_resolution_clock::now();

        for (const auto &order : orders) {
            auto orderStart = std::chrono::high_resolution_clock::now();
            engine->submitOrder(order);
            auto orderEnd = std::chrono::high_resolution_clock::now();
            orderLatencies.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(orderEnd - orderStart));
        }

        auto endTime = std::chrono::high_resolution_clock::now();

        // Calculate results
        BenchmarkResults results;
        results.avgOrderLatency = calculateAverage(orderLatencies) / 1000.0; // Convert to microseconds
        results.p95OrderLatency = calculatePercentile(orderLatencies, 0.95) / 1000.0;
        results.p99OrderLatency = calculatePercentile(orderLatencies, 0.99) / 1000.0;

        auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        results.throughput = (numOrders * 1000.0) / totalDuration.count();

        return results;
    }

  private:
    std::unique_ptr<MatchingEngine> engine;

    void setupLiquidity() {
        // Add some initial orders to create liquidity
        for (int i = 0; i < 1000; ++i) {
            // Add buy orders
            engine->submitOrder(std::make_shared<Order>("INIT_BUY_" + std::to_string(i), "BTC-USDT", Order::Side::BUY,
                                                        Order::Type::LIMIT, 50000.0 - i * 10.0, 1.0));

            // Add sell orders
            engine->submitOrder(std::make_shared<Order>("INIT_SELL_" + std::to_string(i), "BTC-USDT", Order::Side::SELL,
                                                        Order::Type::LIMIT, 50000.0 + i * 10.0, 1.0));
        }
    }

    std::vector<std::shared_ptr<Order>> generateRandomOrders(int count) {
        std::vector<std::shared_ptr<Order>> orders;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> priceDist(49000.0, 51000.0);
        std::uniform_real_distribution<> quantityDist(0.1, 2.0);
        std::uniform_int_distribution<> sideDist(0, 1);
        std::uniform_int_distribution<> typeDist(0, 3);

        for (int i = 0; i < count; ++i) {
            auto side = sideDist(gen) == 0 ? Order::Side::BUY : Order::Side::SELL;
            auto type = static_cast<Order::Type>(typeDist(gen));

            orders.push_back(std::make_shared<Order>("BENCH_" + std::to_string(i), "BTC-USDT", side, type,
                                                     priceDist(gen), quantityDist(gen)));
        }

        return orders;
    }

    double calculateAverage(const std::vector<std::chrono::nanoseconds> &latencies) {
        if (latencies.empty()) return 0.0;
        int64_t sum = 0;
        for (const auto &lat : latencies) { sum += lat.count(); }
        return static_cast<double>(sum) / latencies.size();
    }

    double calculatePercentile(std::vector<std::chrono::nanoseconds> latencies, double percentile) {
        if (latencies.empty()) return 0.0;
        std::sort(latencies.begin(), latencies.end());
        size_t index = static_cast<size_t>(percentile * (latencies.size() - 1));
        return static_cast<double>(latencies[index].count());
    }
};

int main() {
    MatchingEngineBenchmark benchmark;
    auto results = benchmark.runBenchmark();
    results.print();
    return 0;
}