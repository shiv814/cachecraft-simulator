#include "cache.hpp"

#include <algorithm>
#include <stdexcept>

namespace cachecraft {

double Statistics::hit_rate() const {
    return accesses == 0 ? 0.0 : static_cast<double>(hits) / static_cast<double>(accesses);
}

Cache::Cache(std::size_t capacity_bytes, std::size_t block_bytes, std::size_t ways)
    : block_bytes_(block_bytes), ways_(ways) {
    if (capacity_bytes == 0 || block_bytes == 0 || ways == 0) {
        throw std::invalid_argument("capacity, block size, and ways must be positive");
    }
    if (capacity_bytes % (block_bytes * ways) != 0) {
        throw std::invalid_argument("capacity must be divisible by block size multiplied by ways");
    }
    const auto set_count = capacity_bytes / (block_bytes * ways);
    sets_.assign(set_count, std::vector<Line>(ways));
}

bool Cache::access(std::uint64_t address) {
    ++clock_;
    ++stats_.accesses;
    const auto block = address / block_bytes_;
    const auto set_index = static_cast<std::size_t>(block % sets_.size());
    const auto tag = block / sets_.size();
    auto& set = sets_[set_index];

    for (auto& line : set) {
        if (line.valid && line.tag == tag) {
            line.last_used = clock_;
            ++stats_.hits;
            return true;
        }
    }

    ++stats_.misses;
    auto victim = std::find_if(set.begin(), set.end(), [](const Line& line) { return !line.valid; });
    if (victim == set.end()) {
        victim = std::min_element(set.begin(), set.end(), [](const Line& left, const Line& right) {
            return left.last_used < right.last_used;
        });
        ++stats_.evictions;
    }
    victim->valid = true;
    victim->tag = tag;
    victim->last_used = clock_;
    return false;
}

}  // namespace cachecraft
