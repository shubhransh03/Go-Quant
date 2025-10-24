#pragma once
#include <map>
#include <string>

namespace prometheus {

template <typename T>
class Family {
public:
    T& Add(const std::map<std::string, std::string>&) {
        static T inst;
        return inst;
    }
};

} // namespace prometheus
