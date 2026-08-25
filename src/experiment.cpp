#include "analysis.hpp"
#include "reuse.hpp"
#include "synthetic.hpp"
#include <iostream>
#include <vector>
using namespace cachecraft;
int main() {
    const auto trace = hotset_trace(10000, 64, 64, 0.92, 42);
    const auto workload = summarize_workload(trace, 64);
    std::cerr << "workload records=" << workload.records << " unique_blocks=" << workload.unique_blocks << " sequential=" << workload.sequential_fraction << '\n';
    const auto reuse = analyze_reuse_distance(trace, 64);
    std::cerr << "reuse cold_fraction=" << reuse.cold_fraction << " p95=" << reuse.p95_distance << '\n';
    std::vector<CacheConfig> configs;
    for (std::size_t capacity : {4096u, 8192u, 16384u, 32768u}) for (std::size_t ways : {1u, 2u, 4u, 8u}) { CacheConfig config; config.capacity_bytes=capacity; config.block_bytes=64; config.ways=ways; config.replacement=ReplacementPolicy::LRU; configs.push_back(config); }
    const auto results = sweep(trace, configs);
    std::cout << experiments_to_csv(results);
    std::cerr << "pareto configurations=" << pareto_frontier(results).size() << '\n';
    return 0;
}
