#pragma once
#include "trace.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace cachecraft {
std::vector<TraceRecord> sequential_trace(std::size_t count, std::uint64_t start = 0, std::size_t stride = 64, AccessType type = AccessType::Read);
std::vector<TraceRecord> strided_trace(std::size_t count, std::uint64_t start, std::size_t stride, AccessType type = AccessType::Read);
std::vector<TraceRecord> hotset_trace(std::size_t count, std::size_t hot_blocks, std::size_t block_bytes = 64, double hot_probability = 0.9, std::uint32_t seed = 42);
}  // namespace cachecraft
