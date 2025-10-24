#include <iostream>
#include "engine/matching_engine.h"
#include "network/listener.h"
#include "utils/logging.h"

int main() {
    // Initialize logging
    utils::init_logging("matching_engine.log");

    // Create the matching engine
    MatchingEngine matchingEngine;

    // Start metrics and system metrics explicitly in the application
    MetricsManager::instance().start();
    SystemMetrics::instance().start();

    // Set up network listener
    Listener listener(matchingEngine, 8080);
    if (!listener.startListening()) {
        std::cerr << "Failed to start network listener." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Matching engine is running..." << std::endl;

    // Main event loop
    while (true) {
        listener.processEvents();
    }

    return EXIT_SUCCESS;
}