#include "analysis.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace cachecraft {
namespace {
std::vector<std::uint64_t> touched_addresses(const TraceRecord& record, std::size_t block_bytes) {
    const auto size = std::max<std::size_t>(record.size, 1);
    const std::uint64_t first_block = record.address / block_bytes;
    const std::uint64_t last_address = record.address + static_cast<std::uint64_t>(size - 1);
    const std::uint64_t last_block = last_address / block_bytes;
    std::vector<std::uint64_t> addresses;
    addresses.reserve(static_cast<std::size_t>(last_block - first_block + 1));
    for (std::uint64_t block = first_block; block <= last_block; ++block) addresses.push_back(block * block_bytes);
    return addresses;
}

double miss_rate_of(const ExperimentResult& result) {
    return result.stats.accesses == 0 ? 0.0 : static_cast<double>(result.stats.misses) / static_cast<double>(result.stats.accesses);
}
}

WorkloadSummary summarize_workload(const std::vector<TraceRecord>& records, std::size_t block_bytes) {
    if (block_bytes == 0) throw std::invalid_argument("block_bytes must be positive");
    WorkloadSummary summary; summary.records = records.size();
    std::unordered_set<std::uint64_t> blocks;
    bool have_previous = false; std::uint64_t previous = 0; long double stride_sum = 0.0; std::size_t stride_count = 0;
    for (const auto& record : records) {
        if (record.type == AccessType::Read) ++summary.reads; else if (record.type == AccessType::Write) ++summary.writes; else ++summary.instructions;
        for (auto address : touched_addresses(record, block_bytes)) {
            ++summary.expanded_accesses; const auto block = address / block_bytes; blocks.insert(block);
            if (have_previous) { const auto stride = address > previous ? address - previous : previous - address; stride_sum += static_cast<long double>(stride); ++stride_count; if (stride == block_bytes) ++summary.sequential_transitions; }
            previous = address; have_previous = true;
        }
    }
    summary.unique_blocks = blocks.size();
    summary.sequential_fraction = stride_count == 0 ? 0.0 : static_cast<double>(summary.sequential_transitions) / static_cast<double>(stride_count);
    summary.average_absolute_stride = stride_count == 0 ? 0.0 : static_cast<double>(stride_sum / stride_count);
    return summary;
}

ExperimentResult run_experiment(const std::vector<TraceRecord>& records, const CacheConfig& config) {
    Cache cache(config);
    for (const auto& record : records) for (auto address : touched_addresses(record, config.block_bytes)) cache.access(address, record.type);
    ExperimentResult result; result.config = config; result.stats = cache.statistics(); result.amat = result.stats.average_memory_access_time(config.hit_latency, config.miss_penalty);
    if (result.stats.accesses != 0) {
        result.memory_transactions_per_access = static_cast<double>(result.stats.memory_reads + result.stats.memory_writes) / static_cast<double>(result.stats.accesses);
        result.bytes_fetched_per_access = static_cast<double>(result.stats.bytes_fetched) / static_cast<double>(result.stats.accesses);
    }
    return result;
}

std::vector<ExperimentResult> sweep(const std::vector<TraceRecord>& records, const std::vector<CacheConfig>& configs) {
    std::vector<ExperimentResult> results; results.reserve(configs.size());
    for (const auto& config : configs) results.push_back(run_experiment(records, config));
    return results;
}

std::vector<ExperimentResult> pareto_frontier(const std::vector<ExperimentResult>& results) {
    std::vector<ExperimentResult> frontier;
    for (std::size_t i = 0; i < results.size(); ++i) {
        bool dominated = false;
        for (std::size_t j = 0; j < results.size() && !dominated; ++j) {
            if (i == j) continue;
            const auto& a = results[j]; const auto& b = results[i];
            const bool no_worse = a.config.capacity_bytes <= b.config.capacity_bytes && miss_rate_of(a) <= miss_rate_of(b) + 1e-12 && a.memory_transactions_per_access <= b.memory_transactions_per_access + 1e-12;
            const bool better = a.config.capacity_bytes < b.config.capacity_bytes || miss_rate_of(a) < miss_rate_of(b) - 1e-12 || a.memory_transactions_per_access < b.memory_transactions_per_access - 1e-12;
            dominated = no_worse && better;
        }
        if (!dominated) frontier.push_back(results[i]);
    }
    std::sort(frontier.begin(), frontier.end(), [](const auto& a, const auto& b) { return a.config.capacity_bytes < b.config.capacity_bytes; });
    return frontier;
}

std::string experiments_to_csv(const std::vector<ExperimentResult>& results) {
    std::ostringstream out; out << "capacity_bytes,block_bytes,ways,replacement,write_policy,allocation_policy,accesses,hits,misses,miss_rate,amat,memory_tx_per_access,bytes_fetched_per_access\n";
    out << std::fixed << std::setprecision(6);
    for (const auto& r : results) out << r.config.capacity_bytes << ',' << r.config.block_bytes << ',' << r.config.ways << ',' << to_string(r.config.replacement) << ',' << to_string(r.config.write_policy) << ',' << to_string(r.config.allocation_policy) << ',' << r.stats.accesses << ',' << r.stats.hits << ',' << r.stats.misses << ',' << miss_rate_of(r) << ',' << r.amat << ',' << r.memory_transactions_per_access << ',' << r.bytes_fetched_per_access << '\n';
    return out.str();
}

std::string experiment_to_json(const ExperimentResult& r) {
    std::ostringstream out; out << std::fixed << std::setprecision(6);
    out << "{\"capacity_bytes\":" << r.config.capacity_bytes << ",\"block_bytes\":" << r.config.block_bytes << ",\"ways\":" << r.config.ways << ",\"replacement\":\"" << to_string(r.config.replacement) << "\",\"accesses\":" << r.stats.accesses << ",\"hits\":" << r.stats.hits << ",\"misses\":" << r.stats.misses << ",\"miss_rate\":" << miss_rate_of(r) << ",\"amat\":" << r.amat << ",\"memory_transactions_per_access\":" << r.memory_transactions_per_access << "}";
    return out.str();
}
}  // namespace cachecraft
