#pragma once
#include "trace.hpp"
#include <cstddef>
#include <vector>

namespace cachecraft {
struct ReuseDistanceSummary {
    std::size_t accesses{0}; std::size_t cold_accesses{0}; std::size_t reused_accesses{0};
    double cold_fraction{0.0}; double average_distance{0.0}; double median_distance{0.0}; double p95_distance{0.0}; std::size_t maximum_distance{0};
    std::vector<std::size_t> distances;
};
ReuseDistanceSummary analyze_reuse_distance(const std::vector<TraceRecord>& records, std::size_t block_bytes = 64);
}  // namespace cachecraft
