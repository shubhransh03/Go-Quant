// Lightweight Prometheus stub for IntelliSense
#pragma once

// Keep independent of std headers so it parses when opened directly.
namespace prometheus {

class Registry; // fwd decl

class Gauge {
public:
    Gauge() = default;
    explicit Gauge(double) {}  // Support initialization with value
    void Set(double) {}
    void Increment() {}
    void Increment(double) {}
    void Decrement() {}
    void Decrement(double) {}
    double Value() const { return 0.0; }
};

template<typename T>
class Family {
public:
    T& Add(...) { static T inst; return inst; }
};

template<typename T>
class Builder {
public:
    Builder& Name(const char*) { return *this; }
    Builder& Help(const char*) { return *this; }
    Family<T>& Register(Registry&) { static Family<T> fam; return fam; }
};

inline Builder<Gauge> BuildGauge() { return {}; }

} // namespace prometheus
