#pragma once
#include <map>
#include <string>

namespace prometheus {

class Gauge {
public:
    Gauge() = default;
    void Set(double) {}
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

inline Family<Gauge>& BuildGauge() {
    static Family<Gauge> fam;
    return fam;
}

} // namespace prometheus
