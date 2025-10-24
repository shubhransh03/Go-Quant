// Lightweight Prometheus stub for IntelliSense
#pragma once

// Keep this header independent of std headers so it parses even when opened directly.
namespace prometheus {
class Registry; // fwd decl
class Counter { public: void Increment(double = 1.0) {} double Value() const { return 0.0; } };
template<typename T> class Family { public: T& Add(...) { static T inst; return inst; } };
template<typename T> class Builder { public: Builder& Name(const char*) { return *this; } Builder& Help(const char*) { return *this; } Family<T>& Register(Registry&) { static Family<T> fam; return fam; } };
inline Builder<Counter> BuildCounter() { return {}; }
} // namespace prometheus
