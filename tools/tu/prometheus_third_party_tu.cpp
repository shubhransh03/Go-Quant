// TU to provide compile flags for third_party Prometheus stubs when opened directly.
// This ensures compile_commands.json has an entry that references these headers with
// the correct include paths and C++ standard library search paths.

#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/family.h>
#include <prometheus/registry.h>
#include <prometheus/exposer.h>

int _prometheus_third_party_anchor = 0;
