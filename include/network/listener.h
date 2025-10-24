#ifndef LISTENER_H
#define LISTENER_H

#include <memory>
#include <string>
#include <thread>
#include "engine/matching_engine.h"

class Listener {
public:
    // Construct with a reference to the matching engine and an optional port
    Listener(MatchingEngine &engine, unsigned short port = 8080);
    ~Listener();

    // Start the WebSocket listener in background
    bool startListening();
    // Stop the listener and join background threads
    void stop();

    // Process events (non-blocking); kept for compatibility with existing main loop
    void processEvents();

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

#endif // LISTENER_H