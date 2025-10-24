// Lightweight Prometheus stub for IntelliSense
#pragma once

// Keep independent of std headers so it parses when opened directly.
namespace prometheus {

class Registry; // fwd decl
template <typename T> class Family; // declared in family.h stub

class Histogram {
public:
    Histogram() = default;
    void Observe(double) {}
};

template<typename T>
class Builder {
public:
    Builder& Name(const char*) { return *this; }
    Builder& Help(const char*) { return *this; }
    Family<T>& Register(Registry&) { static Family<T> fam; return fam; }
};

inline Builder<Histogram> BuildHistogram() { return {}; }

} // namespace prometheus
