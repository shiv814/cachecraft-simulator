#include "analysis.hpp"
#include "reuse.hpp"
#include "synthetic.hpp"

#include <cstdlib>
#include <iostream>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::cerr << "CHECK failed: " #x << " at " << __FILE__ << ':' << __LINE__ << '\n'; return 1; } } while (0)

int main() {
    using namespace cachecraft;

    const auto seq = sequential_trace(8, 0, 64);
    const auto summary = summarize_workload(seq, 64);
    CHECK(summary.records == 8);
    CHECK(summary.unique_blocks == 8);
    CHECK(summary.sequential_fraction > 0.99);

    const auto reuse_trace = std::vector<TraceRecord>{
        {AccessType::Read, 0, 1, "", 1},
        {AccessType::Read, 64, 1, "", 2},
        {AccessType::Read, 0, 1, "", 3},
        {AccessType::Read, 128, 1, "", 4},
        {AccessType::Read, 0, 1, "", 5},
    };
    const auto reuse = analyze_reuse_distance(reuse_trace, 64);
    CHECK(reuse.cold_accesses == 3);
    CHECK(reuse.reused_accesses == 2);
    CHECK(reuse.maximum_distance == 1);

    const auto a = hotset_trace(100, 4, 64, 1.0, 7);
    const auto b = hotset_trace(100, 4, 64, 1.0, 7);
    CHECK(a.size() == b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK(a[i].address == b[i].address);
    }

    CacheConfig small;
    small.capacity_bytes = 256;
    small.block_bytes = 64;
    small.ways = 1;
    const auto experiment = run_experiment(seq, small);
    CHECK(experiment.stats.accesses == 8);
    CHECK(experiment.stats.misses == 8);
    CHECK(experiment.amat > small.hit_latency);
    CHECK(experiment_to_json(experiment).find("miss_rate") != std::string::npos);

    auto large = small;
    large.capacity_bytes = 1024;
    const auto results = sweep(seq, {small, large});
    CHECK(results.size() == 2);
    CHECK(!experiments_to_csv(results).empty());
    CHECK(!pareto_frontier(results).empty());

    std::cout << "CacheCraft v3 analysis tests passed\n";
    return 0;
}
