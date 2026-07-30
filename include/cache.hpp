#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace cachecraft {

struct Statistics {
    std::size_t accesses{0};
    std::size_t hits{0};
    std::size_t misses{0};
    std::size_t evictions{0};
    double hit_rate() const;
};

class Cache {
public:
    Cache(std::size_t capacity_bytes, std::size_t block_bytes, std::size_t ways);
    bool access(std::uint64_t address);
    const Statistics& statistics() const { return stats_; }
    std::size_t set_count() const { return sets_.size(); }

private:
    struct Line {
        bool valid{false};
        std::uint64_t tag{0};
        std::uint64_t last_used{0};
    };

    std::size_t block_bytes_;
    std::size_t ways_;
    std::uint64_t clock_{0};
    Statistics stats_;
    std::vector<std::vector<Line>> sets_;
};

}  // namespace cachecraft
