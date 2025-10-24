// Prometheus gauge shim for IntelliSense only; forward to real library in builds.
#pragma once

#if defined(__INTELLISENSE__) || defined(__clangd__)
#include "family.h"
namespace prometheus {
class Gauge { public: void Set(double) {} double Value() const { return 0.0; } };

class Registry; // fwd decl for builder

template<typename T>
class Builder {
public:
	Builder& Name(const std::string&) { return *this; }
	Builder& Help(const std::string&) { return *this; }
	Family<T>& Register(Registry&) { static Family<T> fam; return fam; }
};

inline Builder<Gauge> BuildGauge() { return {}; }
}
#else
#include_next <prometheus/gauge.h>
#endif
