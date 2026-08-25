#include "reuse.hpp"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace cachecraft {
ReuseDistanceSummary analyze_reuse_distance(const std::vector<TraceRecord>& records, std::size_t block_bytes) {
    if (block_bytes == 0) throw std::invalid_argument("block_bytes must be positive");
    std::vector<std::uint64_t> sequence;
    for (const auto& record : records) {
        const auto size = std::max<std::size_t>(record.size, 1); const auto first = record.address / block_bytes; const auto last = (record.address + size - 1) / block_bytes;
        for (auto block = first; block <= last; ++block) sequence.push_back(block);
    }
    ReuseDistanceSummary summary; summary.accesses = sequence.size();
    std::unordered_map<std::uint64_t, std::size_t> last_position;
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        const auto block = sequence[i]; const auto found = last_position.find(block);
        if (found == last_position.end()) { ++summary.cold_accesses; }
        else { std::unordered_set<std::uint64_t> distinct; for (std::size_t j = found->second + 1; j < i; ++j) distinct.insert(sequence[j]); summary.distances.push_back(distinct.size()); }
        last_position[block] = i;
    }
    summary.reused_accesses = summary.distances.size(); summary.cold_fraction = summary.accesses == 0 ? 0.0 : static_cast<double>(summary.cold_accesses) / summary.accesses;
    if (!summary.distances.empty()) {
        std::sort(summary.distances.begin(), summary.distances.end());
        const auto total = std::accumulate(summary.distances.begin(), summary.distances.end(), 0.0); summary.average_distance = total / summary.distances.size(); summary.median_distance = summary.distances[summary.distances.size()/2];
        const auto p95_index = static_cast<std::size_t>(0.95 * static_cast<double>(summary.distances.size()-1)); summary.p95_distance = summary.distances[p95_index]; summary.maximum_distance = summary.distances.back();
    }
    return summary;
}
}  // namespace cachecraft
