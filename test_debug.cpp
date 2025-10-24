#include <iostream>
#include "engine/matching_engine.h"

int main() {
    std::cout << "Creating engine..." << std::endl;
    MatchingEngine engine;
    std::cout << "Engine created" << std::endl;
    
    std::cout << "Creating order..." << std::endl;
    auto order = std::make_shared<Order>("test1", "SYM", Order::Side::BUY, Order::Type::LIMIT, 100.0, 1.0);
    std::cout << "Order created" << std::endl;
    
    std::cout << "About to submit..." << std::endl;
    try {
        engine.submitOrder(order);
        std::cout << "Submit succeeded" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Submit failed: " << e.what() << std::endl;
    }
    
    return 0;
}
