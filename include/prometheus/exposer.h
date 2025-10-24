// Prometheus exposer shim for IntelliSense only; forward to real library in builds.
#pragma once

#if defined(__INTELLISENSE__) || defined(__clangd__)
#include <memory>
#include <string>
namespace prometheus {
class Exposer {
public:
    explicit Exposer(const std::string&) {}
    template <typename T>
    void RegisterCollectable(const std::shared_ptr<T>&) {}
};
}
#else
#include_next <prometheus/exposer.h>
#endif
