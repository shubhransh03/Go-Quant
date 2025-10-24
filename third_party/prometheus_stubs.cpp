// No-op stub implementations for Prometheus metrics (testing only)
#include <map>
#include <memory>
#include <string>
#include <vector>

// This provides minimal linking stubs so tests can link without prometheus-cpp library

namespace prometheus {

// Counter
class Counter {
public:
    Counter() {}
    void Increment() {}
    void Increment(double) {}
    double Value() const { return 0.0; }
};

// Gauge
class Gauge {
public:
    Gauge() {}
    explicit Gauge(double) {}
    void Set(double) {}
    void Increment() {}
    void Increment(double) {}
    void Decrement() {}
    void Decrement(double) {}
    double Value() const { return 0.0; }
};

// Histogram
class Histogram {
public:
    Histogram(const std::vector<double>&) {}
    void Observe(double) {}
};

// Registry
enum class InsertBehavior { ReturnExisting };
class Registry {
public:
    Registry(InsertBehavior = InsertBehavior::ReturnExisting) {}
};

// Exposer
class Exposer {
public:
    Exposer(const std::string&, unsigned long = 0, const void* = nullptr) {}
    ~Exposer() {}
    template<typename T>
    void RegisterCollectable(const T&, const std::string& = "") {}
};

// Family template instantiations
template<typename T>
class Family {
public:
    T& Add(const std::map<std::string, std::string>&, std::unique_ptr<T>) {
        static T instance;
        return instance;
    }
};

// Explicit template instantiations for linkage
template class Family<Counter>;
template class Family<Gauge>;
template class Family<Histogram>;

template Counter& Family<Counter>::Add(const std::map<std::string, std::string>&, std::unique_ptr<Counter>);
template Gauge& Family<Gauge>::Add(const std::map<std::string, std::string>&, std::unique_ptr<Gauge>);
template Histogram& Family<Histogram>::Add(const std::map<std::string, std::string>&, std::unique_ptr<Histogram>);

} // namespace prometheus
