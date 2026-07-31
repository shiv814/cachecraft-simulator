#include "cache.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace cachecraft {
namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool is_power_of_two(std::size_t value) {
    return value > 0 && (value & (value - 1)) == 0;
}

}  // namespace

std::string to_string(AccessType value) {
    switch (value) {
        case AccessType::Read: return "read";
        case AccessType::Write: return "write";
        case AccessType::Instruction: return "instruction";
    }
    return "unknown";
}

std::string to_string(ReplacementPolicy value) {
    switch (value) {
        case ReplacementPolicy::LRU: return "lru";
        case ReplacementPolicy::FIFO: return "fifo";
        case ReplacementPolicy::Random: return "random";
    }
    return "unknown";
}

std::string to_string(WritePolicy value) {
    return value == WritePolicy::WriteBack ? "write-back" : "write-through";
}

std::string to_string(AllocationPolicy value) {
    return value == AllocationPolicy::WriteAllocate ? "write-allocate" : "no-write-allocate";
}

std::string to_string(MissType value) {
    switch (value) {
        case MissType::None: return "none";
        case MissType::Compulsory: return "compulsory";
        case MissType::Conflict: return "conflict";
        case MissType::Capacity: return "capacity";
    }
    return "unknown";
}

ReplacementPolicy replacement_policy_from_string(const std::string& value) {
    const auto cleaned = lower(value);
    if (cleaned == "lru") return ReplacementPolicy::LRU;
    if (cleaned == "fifo") return ReplacementPolicy::FIFO;
    if (cleaned == "random") return ReplacementPolicy::Random;
    throw std::invalid_argument("replacement policy must be lru, fifo, or random");
}

WritePolicy write_policy_from_string(const std::string& value) {
    const auto cleaned = lower(value);
    if (cleaned == "write-back" || cleaned == "wb") return WritePolicy::WriteBack;
    if (cleaned == "write-through" || cleaned == "wt") return WritePolicy::WriteThrough;
    throw std::invalid_argument("write policy must be write-back or write-through");
}

AllocationPolicy allocation_policy_from_string(const std::string& value) {
    const auto cleaned = lower(value);
    if (cleaned == "write-allocate" || cleaned == "wa") return AllocationPolicy::WriteAllocate;
    if (cleaned == "no-write-allocate" || cleaned == "nwa") return AllocationPolicy::NoWriteAllocate;
    throw std::invalid_argument("allocation policy must be write-allocate or no-write-allocate");
}

double Statistics::hit_rate() const {
    return accesses == 0 ? 0.0 : static_cast<double>(hits) / static_cast<double>(accesses);
}

double Statistics::miss_rate() const {
    return accesses == 0 ? 0.0 : static_cast<double>(misses) / static_cast<double>(accesses);
}

double Statistics::average_memory_access_time(double hit_latency, double miss_penalty) const {
    return hit_latency + miss_rate() * miss_penalty;
}

Cache::Cache(CacheConfig config) : config_(config), random_(config.random_seed) {
    validate_config();
    const auto count = config_.capacity_bytes / (config_.block_bytes * config_.ways);
    sets_.assign(count, std::vector<Line>(config_.ways));
    shadow_lru_.reserve(line_count());
}

Cache::Cache(std::size_t capacity_bytes, std::size_t block_bytes, std::size_t ways)
    : Cache(CacheConfig{capacity_bytes, block_bytes, ways}) {}

void Cache::validate_config() const {
    if (config_.capacity_bytes == 0 || config_.block_bytes == 0 || config_.ways == 0) {
        throw std::invalid_argument("capacity, block size, and ways must be positive");
    }
    if (!is_power_of_two(config_.block_bytes)) {
        throw std::invalid_argument("block size must be a power of two");
    }
    if (config_.capacity_bytes % (config_.block_bytes * config_.ways) != 0) {
        throw std::invalid_argument("capacity must be divisible by block size multiplied by ways");
    }
    if (config_.hit_latency < 0 || config_.miss_penalty < 0) {
        throw std::invalid_argument("latencies cannot be negative");
    }
}

void Cache::reset() {
    clock_ = 0;
    stats_ = {};
    seen_blocks_.clear();
    shadow_lru_.clear();
    random_.seed(config_.random_seed);
    for (auto& set : sets_) {
        for (auto& line : set) line = {};
    }
}

MissType Cache::classify_miss(std::uint64_t block) {
    if (seen_blocks_.find(block) == seen_blocks_.end()) return MissType::Compulsory;
    const auto shadow_hit = std::find(shadow_lru_.begin(), shadow_lru_.end(), block) != shadow_lru_.end();
    return shadow_hit ? MissType::Conflict : MissType::Capacity;
}

void Cache::update_shadow(std::uint64_t block) {
    auto existing = std::find(shadow_lru_.begin(), shadow_lru_.end(), block);
    if (existing != shadow_lru_.end()) shadow_lru_.erase(existing);
    shadow_lru_.insert(shadow_lru_.begin(), block);
    if (shadow_lru_.size() > line_count()) shadow_lru_.pop_back();
    seen_blocks_.insert(block);
}

std::size_t Cache::select_victim(const std::vector<Line>& set) {
    const auto invalid = std::find_if(set.begin(), set.end(), [](const Line& line) { return !line.valid; });
    if (invalid != set.end()) return static_cast<std::size_t>(std::distance(set.begin(), invalid));
    if (config_.replacement == ReplacementPolicy::Random) {
        std::uniform_int_distribution<std::size_t> distribution(0, set.size() - 1);
        return distribution(random_);
    }
    const auto selected = std::min_element(set.begin(), set.end(), [&](const Line& left, const Line& right) {
        const auto left_value = config_.replacement == ReplacementPolicy::LRU ? left.last_used : left.inserted_at;
        const auto right_value = config_.replacement == ReplacementPolicy::LRU ? right.last_used : right.inserted_at;
        return left_value < right_value;
    });
    return static_cast<std::size_t>(std::distance(set.begin(), selected));
}

AccessResult Cache::access(std::uint64_t address, AccessType type) {
    ++clock_;
    ++stats_.accesses;
    if (type == AccessType::Read) ++stats_.reads;
    if (type == AccessType::Write) ++stats_.writes;
    if (type == AccessType::Instruction) ++stats_.instructions;

    const auto block = address / config_.block_bytes;
    const auto set_index = static_cast<std::size_t>(block % sets_.size());
    const auto tag = block / sets_.size();
    auto& set = sets_[set_index];

    for (std::size_t way = 0; way < set.size(); ++way) {
        auto& line = set[way];
        if (line.valid && line.tag == tag) {
            ++stats_.hits;
            line.last_used = clock_;
            if (type == AccessType::Write) {
                if (config_.write_policy == WritePolicy::WriteBack) line.dirty = true;
                else ++stats_.memory_writes;
            }
            update_shadow(block);
            return {true, false, false, false, set_index, way, tag, MissType::None};
        }
    }

    ++stats_.misses;
    const auto miss_type = classify_miss(block);
    if (miss_type == MissType::Compulsory) ++stats_.compulsory_misses;
    if (miss_type == MissType::Conflict) ++stats_.conflict_misses;
    if (miss_type == MissType::Capacity) ++stats_.capacity_misses;
    update_shadow(block);

    if (type == AccessType::Write && config_.allocation_policy == AllocationPolicy::NoWriteAllocate) {
        ++stats_.memory_writes;
        return {false, false, false, false, set_index, 0, tag, miss_type};
    }

    ++stats_.memory_reads;
    stats_.bytes_fetched += config_.block_bytes;
    const auto way = select_victim(set);
    auto& victim = set[way];
    const bool evicted = victim.valid;
    const bool writeback = victim.valid && victim.dirty;
    if (evicted) ++stats_.evictions;
    if (writeback) {
        ++stats_.writebacks;
        ++stats_.memory_writes;
    }
    victim.valid = true;
    victim.dirty = type == AccessType::Write && config_.write_policy == WritePolicy::WriteBack;
    victim.tag = tag;
    victim.last_used = clock_;
    victim.inserted_at = clock_;
    if (type == AccessType::Write && config_.write_policy == WritePolicy::WriteThrough) {
        ++stats_.memory_writes;
    }
    return {false, true, evicted, writeback, set_index, way, tag, miss_type};
}

std::vector<LineView> Cache::set_state(std::size_t set_index) const {
    if (set_index >= sets_.size()) throw std::out_of_range("set index is out of range");
    std::vector<LineView> result;
    result.reserve(sets_[set_index].size());
    for (const auto& line : sets_[set_index]) {
        result.push_back({line.valid, line.dirty, line.tag, line.last_used, line.inserted_at});
    }
    return result;
}

TwoLevelHierarchy::TwoLevelHierarchy(CacheConfig l1_config, CacheConfig l2_config)
    : l1_(l1_config), l2_(l2_config) {
    if (l2_config.capacity_bytes < l1_config.capacity_bytes) {
        throw std::invalid_argument("L2 capacity must be at least L1 capacity");
    }
}

HierarchyResult TwoLevelHierarchy::access(std::uint64_t address, AccessType type) {
    auto l1_result = l1_.access(address, type);
    if (l1_result.hit) return {l1_result, {}, false};
    auto l2_result = l2_.access(address, type == AccessType::Write ? AccessType::Read : type);
    return {l1_result, l2_result, true};
}

}  // namespace cachecraft
