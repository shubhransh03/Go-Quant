// Prometheus family shim for IntelliSense only; forward to real library in builds.
#pragma once

#if defined(__INTELLISENSE__) || defined(__clangd__)
#include <initializer_list>
#include <utility>
#include <vector>
#include <string>
#include <map>
namespace prometheus {
template <typename T>
class Family {
public:
    // Overload for map constructor from initializer_list
    T& Add(const std::map<std::string, std::string>&) {
        static T inst;
        return inst;
    }
    
    // Overload for initializer_list that will convert to map
    T& Add(std::initializer_list<std::pair<const char*, const char*>>) {
        static T inst;
        return inst;
    }
    
    // Overload for two arguments (labels + buckets for histograms)
    T& Add(const std::map<std::string, std::string>&, const std::vector<double>&) {
        static T inst;
        return inst;
    }
    
    T& Add(std::initializer_list<std::pair<const char*, const char*>>, const std::vector<double>&) {
        static T inst;
        return inst;
    }
    
    // Fallback variadic template for other cases
    template<typename... Args>
    T& Add(Args&&... args) {
        static T inst;
        return inst;
    }
};
}
#else
#include_next <prometheus/family.h>
#endif
