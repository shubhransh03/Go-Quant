// Lightweight Prometheus stub for IntelliSense
#pragma once

// Provide two modes: a super-light IntelliSense mode that avoids std headers,
// and a typed mode (for non-IntelliSense) that uses std::map/std::vector.

#if defined(__INTELLISENSE__) || defined(__clangd__)
namespace prometheus {
template <typename T>
class Family {
public:
    // Use variadic template to accept any argument combination
    template<typename... Args>
    T& Add(Args&&...) { static T inst; return inst; }
};
} // namespace prometheus
#else
#include <map>
#include <string>
#include <vector>
namespace prometheus {
template <typename T>
class Family {
public:
    T& Add(const std::map<std::string, std::string>&) { static T inst; return inst; }
    T& Add(const std::map<std::string, std::string>&, const std::vector<double>&) { static T inst; return inst; }
};
} // namespace prometheus
#endif
