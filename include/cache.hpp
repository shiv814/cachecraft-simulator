#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

namespace cachecraft {

enum class AccessType { Read, Write, Instruction };
enum class ReplacementPolicy { LRU, FIFO, Random };
enum class WritePolicy { WriteBack, WriteThrough };
enum class AllocationPolicy { WriteAllocate, NoWriteAllocate };
enum class MissType { None, Compulsory, Conflict, Capacity };

std::string to_string(AccessType value);
std::string to_string(ReplacementPolicy value);
std::string to_string(WritePolicy value);
std::string to_string(AllocationPolicy value);
std::string to_string(MissType value);
ReplacementPolicy replacement_policy_from_string(const std::string& value);
WritePolicy write_policy_from_string(const std::string& value);
AllocationPolicy allocation_policy_from_string(const std::string& value);

struct CacheConfig {
    std::size_t capacity_bytes{32768};
    std::size_t block_bytes{64};
    std::size_t ways{8};
    ReplacementPolicy replacement{ReplacementPolicy::LRU};
    WritePolicy write_policy{WritePolicy::WriteBack};
    AllocationPolicy allocation_policy{AllocationPolicy::WriteAllocate};
    double hit_latency{1.0};
    double miss_penalty{100.0};
    std::uint32_t random_seed{42};
};

struct Statistics {
    std::size_t accesses{0};
    std::size_t reads{0};
    std::size_t writes{0};
    std::size_t instructions{0};
    std::size_t hits{0};
    std::size_t misses{0};
    std::size_t evictions{0};
    std::size_t writebacks{0};
    std::size_t memory_reads{0};
    std::size_t memory_writes{0};
    std::size_t compulsory_misses{0};
    std::size_t conflict_misses{0};
    std::size_t capacity_misses{0};
    std::size_t bytes_fetched{0};

    double hit_rate() const;
    double miss_rate() const;
    double average_memory_access_time(double hit_latency, double miss_penalty) const;
};

struct AccessResult {
    bool hit{false};
    bool allocated{false};
    bool evicted{false};
    bool writeback{false};
    std::size_t set_index{0};
    std::size_t way{0};
    std::uint64_t tag{0};
    MissType miss_type{MissType::None};
};

struct LineView {
    bool valid{false};
    bool dirty{false};
    std::uint64_t tag{0};
    std::uint64_t last_used{0};
    std::uint64_t inserted_at{0};
};

class Cache {
public:
    explicit Cache(CacheConfig config);
    Cache(std::size_t capacity_bytes, std::size_t block_bytes, std::size_t ways);

    AccessResult access(std::uint64_t address, AccessType type = AccessType::Read);
    void reset();

    const CacheConfig& config() const { return config_; }
    const Statistics& statistics() const { return stats_; }
    std::size_t set_count() const { return sets_.size(); }
    std::size_t line_count() const { return config_.capacity_bytes / config_.block_bytes; }
    std::vector<LineView> set_state(std::size_t set_index) const;

private:
    struct Line {
        bool valid{false};
        bool dirty{false};
        std::uint64_t tag{0};
        std::uint64_t last_used{0};
        std::uint64_t inserted_at{0};
    };

    std::size_t select_victim(const std::vector<Line>& set);
    MissType classify_miss(std::uint64_t block);
    void update_shadow(std::uint64_t block);
    void validate_config() const;

    CacheConfig config_;
    std::uint64_t clock_{0};
    Statistics stats_;
    std::vector<std::vector<Line>> sets_;
    std::unordered_set<std::uint64_t> seen_blocks_;
    std::vector<std::uint64_t> shadow_lru_;
    std::mt19937 random_;
};

struct HierarchyResult {
    AccessResult l1;
    AccessResult l2;
    bool l2_accessed{false};
};

class TwoLevelHierarchy {
public:
    TwoLevelHierarchy(CacheConfig l1_config, CacheConfig l2_config);
    HierarchyResult access(std::uint64_t address, AccessType type = AccessType::Read);
    const Cache& l1() const { return l1_; }
    const Cache& l2() const { return l2_; }

private:
    Cache l1_;
    Cache l2_;
};

}  // namespace cachecraft
