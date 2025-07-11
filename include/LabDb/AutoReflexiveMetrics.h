#pragma once

#include <chrono>

namespace LabDb {
//-----------------------------------------------------------------------------
// Auto-reflexive metrics for operation monitoring
//-----------------------------------------------------------------------------
struct AutoReflexiveMetrics {
    std::chrono::milliseconds operation_time_ms{0};
    uint64_t items_processed{0};
    double cache_hit_ratio{0.0};
    uint32_t tid_allocations{0};
    uint64_t memory_usage_kb{0};
};

} // namespace LabDb