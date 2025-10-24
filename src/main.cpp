#include <iostream>
#include "engine/matching_engine.h"
#include "network/listener.h"
#include "utils/logging.h"
#include <csignal>
#include <condition_variable>
#include <atomic>

static std::atomic<bool> g_running{true};
static std::condition_variable g_cv;
static std::mutex g_cv_mutex;

void handle_signal(int) {
    g_running = false;
    g_cv.notify_one();
}

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

    // Set up signal handling for graceful shutdown
    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    // Wait until signalled
    std::unique_lock<std::mutex> lk(g_cv_mutex);
    g_cv.wait(lk, [] { return !g_running.load(); });

    // Begin shutdown
    std::cout << "Shutting down..." << std::endl;
    listener.stop();
    SystemMetrics::instance().stop();
    MetricsManager::instance().stop();

    return EXIT_SUCCESS;
}