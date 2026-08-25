#pragma once
#include "cache.hpp"
#include "trace.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace cachecraft {

struct WorkloadSummary {
    std::size_t records{0};
    std::size_t expanded_accesses{0};
    std::size_t reads{0};
    std::size_t writes{0};
    std::size_t instructions{0};
    std::size_t unique_blocks{0};
    std::size_t sequential_transitions{0};
    double sequential_fraction{0.0};
    double average_absolute_stride{0.0};
};

struct ExperimentResult {
    CacheConfig config;
    Statistics stats;
    double amat{0.0};
    double memory_transactions_per_access{0.0};
    double bytes_fetched_per_access{0.0};
};

WorkloadSummary summarize_workload(const std::vector<TraceRecord>& records, std::size_t block_bytes = 64);
ExperimentResult run_experiment(const std::vector<TraceRecord>& records, const CacheConfig& config);
std::vector<ExperimentResult> sweep(const std::vector<TraceRecord>& records, const std::vector<CacheConfig>& configs);
std::vector<ExperimentResult> pareto_frontier(const std::vector<ExperimentResult>& results);
std::string experiments_to_csv(const std::vector<ExperimentResult>& results);
std::string experiment_to_json(const ExperimentResult& result);

}  // namespace cachecraft
