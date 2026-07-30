#include "cache.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>

int main() {
    cachecraft::Cache cache(64, 16, 2);
    assert(!cache.access(0x00));
    assert(cache.access(0x00));
    assert(!cache.access(0x20));
    assert(!cache.access(0x40));
    const auto stats = cache.statistics();
    assert(stats.accesses == 4);
    assert(stats.hits == 1);
    assert(stats.misses == 3);
    assert(stats.evictions == 1);
    assert(std::abs(stats.hit_rate() - 0.25) < 0.0001);

    bool failed = false;
    try {
        cachecraft::Cache invalid(63, 16, 2);
    } catch (const std::invalid_argument&) {
        failed = true;
    }
    assert(failed);
    std::cout << "All CacheCraft tests passed\n";
    return 0;
}
