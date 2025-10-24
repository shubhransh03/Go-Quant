#pragma once
#include <map>
#include <string>

namespace prometheus {

class Counter {
public:
    Counter() = default;
    void Increment(double val = 1.0) {}
    double Value() const { return 0.0; }
};

template<typename T>
class Family {
public:
    T& Add(const std::map<std::string, std::string>&) {
        static T inst;
        return inst;
    }
};

inline Family<Counter>& BuildCounter() {
    static Family<Counter> fam;
    return fam;
}

} // namespace prometheus
