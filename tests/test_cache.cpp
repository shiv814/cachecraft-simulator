#include "cache.hpp"
#include "trace.hpp"

#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace cachecraft;

#define CHECK(condition) do { if (!(condition)) { std::cerr << "Check failed at " << __FILE__ << ":" << __LINE__ << ": " #condition << "\n"; std::exit(1); } } while (false)

void test_lru_and_miss_classification() {
    Cache cache(CacheConfig{64, 16, 2});
    CHECK(!cache.access(0x00).hit);       // compulsory
    CHECK(cache.access(0x00).hit);
    CHECK(!cache.access(0x20).hit);       // compulsory, same set
    CHECK(!cache.access(0x40).hit);       // compulsory and eviction
    const auto revisit = cache.access(0x00);
    CHECK(!revisit.hit);
    CHECK(revisit.miss_type == MissType::Conflict);
    const auto& stats = cache.statistics();
    CHECK(stats.accesses == 5);
    CHECK(stats.hits == 1);
    CHECK(stats.compulsory_misses == 3);
    CHECK(stats.conflict_misses == 1);
    CHECK(stats.evictions == 2);
    CHECK(std::abs(stats.hit_rate() - 0.2) < 0.0001);
}

void test_write_back_and_write_through() {
    CacheConfig wb{32, 16, 1};
    wb.write_policy = WritePolicy::WriteBack;
    Cache write_back(wb);
    write_back.access(0x00, AccessType::Write);
    CHECK(write_back.set_state(0)[0].dirty);
    const auto evict = write_back.access(0x20, AccessType::Read);
    CHECK(evict.writeback);
    CHECK(write_back.statistics().writebacks == 1);

    CacheConfig wt = wb;
    wt.write_policy = WritePolicy::WriteThrough;
    Cache write_through(wt);
    write_through.access(0x00, AccessType::Write);
    CHECK(!write_through.set_state(0)[0].dirty);
    CHECK(write_through.statistics().memory_writes == 1);
}

void test_no_write_allocate() {
    CacheConfig config{64, 16, 2};
    config.allocation_policy = AllocationPolicy::NoWriteAllocate;
    Cache cache(config);
    const auto miss = cache.access(0x00, AccessType::Write);
    CHECK(!miss.hit && !miss.allocated);
    CHECK(cache.statistics().memory_writes == 1);
    CHECK(!cache.access(0x00, AccessType::Read).hit);
}

void test_fifo_differs_from_lru() {
    CacheConfig lru_config{32, 16, 2};
    lru_config.replacement = ReplacementPolicy::LRU;
    Cache lru(lru_config);
    lru.access(0x00); lru.access(0x10); lru.access(0x00); lru.access(0x20);
    CHECK(lru.access(0x00).hit);

    auto fifo_config = lru_config;
    fifo_config.replacement = ReplacementPolicy::FIFO;
    Cache fifo(fifo_config);
    fifo.access(0x00); fifo.access(0x10); fifo.access(0x00); fifo.access(0x20);
    CHECK(!fifo.access(0x00).hit);
}

void test_trace_parser() {
    std::stringstream stream("# example\nR 0x10 4\nW,0x20,8\n0x30\nI 64\n");
    const auto records = read_trace(stream);
    CHECK(records.size() == 4);
    CHECK(records[0].type == AccessType::Read && records[0].address == 0x10 && records[0].size == 4);
    CHECK(records[1].type == AccessType::Write && records[1].address == 0x20);
    CHECK(records[2].type == AccessType::Read && records[2].address == 0x30);
    CHECK(records[3].type == AccessType::Instruction && records[3].address == 64);
}

void test_hierarchy_and_validation() {
    CacheConfig l1{64, 16, 2};
    CacheConfig l2{256, 16, 4};
    TwoLevelHierarchy hierarchy(l1, l2);
    auto first = hierarchy.access(0x100);
    CHECK(!first.l1.hit && first.l2_accessed && !first.l2.hit);
    auto second = hierarchy.access(0x100);
    CHECK(second.l1.hit && !second.l2_accessed);

    bool failed = false;
    try { Cache invalid(CacheConfig{63, 16, 2}); }
    catch (const std::invalid_argument&) { failed = true; }
    CHECK(failed);
}

int main() {
    test_lru_and_miss_classification();
    test_write_back_and_write_through();
    test_no_write_allocate();
    test_fifo_differs_from_lru();
    test_trace_parser();
    test_hierarchy_and_validation();
    std::cout << "All CacheCraft 2.0 tests passed\n";
    return 0;
}
