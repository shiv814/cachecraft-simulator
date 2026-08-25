#include "synthetic.hpp"

#include <random>
#include <stdexcept>

namespace cachecraft {

std::vector<TraceRecord> sequential_trace(
    std::size_t count,
    std::uint64_t start,
    std::size_t stride,
    AccessType type) {
    return strided_trace(count, start, stride, type);
}

std::vector<TraceRecord> strided_trace(
    std::size_t count,
    std::uint64_t start,
    std::size_t stride,
    AccessType type) {
    if (stride == 0) {
        throw std::invalid_argument("stride must be positive");
    }

    std::vector<TraceRecord> out;
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        out.push_back(TraceRecord{
            type,
            start + static_cast<std::uint64_t>(i * stride),
            1,
            "synthetic",
            i + 1,
        });
    }
    return out;
}

std::vector<TraceRecord> hotset_trace(
    std::size_t count,
    std::size_t hot_blocks,
    std::size_t block_bytes,
    double hot_probability,
    std::uint32_t seed) {
    if (hot_blocks == 0 || block_bytes == 0) {
        throw std::invalid_argument("hot_blocks and block_bytes must be positive");
    }
    if (hot_probability < 0.0 || hot_probability > 1.0) {
        throw std::invalid_argument("hot_probability must be 0..1");
    }

    std::mt19937 rng(seed);
    std::bernoulli_distribution choose_hot(hot_probability);
    std::uniform_int_distribution<std::size_t> hot(0, hot_blocks - 1);
    std::uniform_int_distribution<std::size_t> cold(hot_blocks, hot_blocks * 16 - 1);

    std::vector<TraceRecord> out;
    out.reserve(count);
    for (std::size_t i = 0; i < count; ++i) {
        const auto block = choose_hot(rng) ? hot(rng) : cold(rng);
        out.push_back(TraceRecord{
            AccessType::Read,
            static_cast<std::uint64_t>(block * block_bytes),
            1,
            "hotset",
            i + 1,
        });
    }
    return out;
}

}  // namespace cachecraft
