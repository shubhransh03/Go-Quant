// Tiny translation unit to provide compile flags for IntelliSense when opening headers directly.
// This file is not linked into any binary; it's only compiled as an OBJECT target.

#include "utils/metrics_manager.h"
#include "utils/rate_limiter.h"
#include "utils/memory_pool.h"
#include "engine/matching_engine.h"
#include "engine/order_book.h"
#include "engine/matching_algorithm.h"
#include "engine/persistence.h"
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/family.h>
#include <prometheus/registry.h>
#include <prometheus/exposer.h>

int _headers_tu_anchor = 0;
