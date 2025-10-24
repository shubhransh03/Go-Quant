#include <iostream>
#include <chrono>
#include <vector>
#include "engine/matching_engine.h"

class ThroughputBenchmark {
public:
    ThroughputBenchmark(MatchingEngine& engine) : engine(engine) {}

    void run(int numOrders) {
        std::vector<Order> orders = generateOrders(numOrders);
        auto start = std::chrono::high_resolution_clock::now();

        for (const auto& order : orders) {
            engine.submitOrder(order);
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        double throughput = numOrders / duration.count();

        std::cout << "Throughput: " << throughput << " orders per second" << std::endl;
    }

private:
    MatchingEngine& engine;

    std::vector<Order> generateOrders(int numOrders) {
        std::vector<Order> orders;
        for (int i = 0; i < numOrders; ++i) {
            orders.emplace_back(Order{"BTC/USD", 1.0, 50000.0, OrderType::LIMIT, OrderSide::BUY});
        }
        return orders;
    }
};

int main() {
    MatchingEngine engine;
    engine.initialize();

    ThroughputBenchmark benchmark(engine);
    benchmark.run(10000); // Adjust the number of orders as needed

    return 0;
}