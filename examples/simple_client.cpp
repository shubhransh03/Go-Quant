#include <iostream>
#include <string>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "engine/matching_engine.h"
#include "network/listener.h"

int main() {
    // Initialize the matching engine
    MatchingEngine engine;

    // Set up and start a network listener on port 8080
    Listener listener(engine, 8080);
    if (!listener.startListening()) {
        std::cerr << "Failed to start the listener on port 8080." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Matching engine is running. Listening for connections..." << std::endl;

    // Main event loop (process events in background, optionally do periodic tasks)
    while (true) {
        listener.processEvents();
        sleep(1); // Sleep to prevent busy waiting
    }

    // Cleanup (not reached in this example)
    listener.stop();

    return EXIT_SUCCESS;
}