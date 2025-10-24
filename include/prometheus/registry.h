// Prometheus registry shim for IntelliSense only; forward to real library in builds.
#pragma once

#if defined(__INTELLISENSE__) || defined(__clangd__)
#include <memory>
namespace prometheus {
class Registry { };
}
#else
#include_next <prometheus/registry.h>
#endif
