#pragma once
#include <vector>
#include <map>
#include <string>

namespace prometheus {

class Histogram {
public:
    Histogram() = default;
    void Observe(double) {}
};

template<typename T>
class Family {
public:
    Histogram* Add(const std::map<std::string, std::string>&, const std::vector<double>&) {
        static Histogram inst;
        return &inst;
    }
};

inline Family<Histogram>& BuildHistogram() {
    static Family<Histogram> fam;
    return fam;
}

} // namespace prometheus
