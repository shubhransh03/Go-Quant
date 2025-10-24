// Lightweight Prometheus stub for IntelliSense and testing without linking prometheus-cpp
#pragma once
#include <memory>

namespace prometheus {

// Forward declarations
class Collectable;

class Exposer {
public:
    explicit Exposer(const std::string&, unsigned long = 0, const void* = nullptr) {}
    template<typename T>
    void RegisterCollectable(const T&, const std::string& = "") {}
};

} // namespace prometheus
