#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "engine/matching_engine.h"
#include "network/session.h"

int main() {
    // Initialize the matching engine
    MatchingEngine engine;

    // Start the engine
    if (!engine.initialize()) {
        std::cerr << "Failed to initialize the matching engine." << std::endl;
        return EXIT_FAILURE;
    }

    // Set up a network listener
    Listener listener(engine);
    if (!listener.start(8080)) {
        std::cerr << "Failed to start the listener on port 8080." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Matching engine is running. Listening for connections..." << std::endl;

    // Main event loop
    while (true) {
        listener.acceptConnections();
        sleep(1); // Sleep to prevent busy waiting
    }

    // Cleanup (not reached in this example)
    listener.stop();
    engine.shutdown();

    return EXIT_SUCCESS;
}