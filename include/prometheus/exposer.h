#pragma once
#include <string>
#include <memory>

namespace prometheus {

class Exposer {
public:
    Exposer(const std::string&) {}
    void RegisterCollectable(std::shared_ptr<void>) {}
};

} // namespace prometheus
