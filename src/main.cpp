#include "cache.hpp"
#include "trace.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Options {
    cachecraft::CacheConfig config;
    std::string trace_file;
    bool json{false};
    bool verbose{false};
    bool compare{false};
    std::size_t dump_set{static_cast<std::size_t>(-1)};
};

void usage() {
    std::cerr
        << "CacheCraft 2.0\n"
        << "Usage:\n"
        << "  cachecraft_cli <capacity> <block> <ways> <trace>\n"
        << "  cachecraft_cli --trace FILE [--capacity N] [--block N] [--ways N]\n"
        << "                 [--policy lru|fifo|random] [--write-policy write-back|write-through]\n"
        << "                 [--allocation write-allocate|no-write-allocate] [--json] [--verbose]\n"
        << "                 [--compare] [--dump-set N] [--hit-latency N] [--miss-penalty N]\n";
}

Options parse_options(int argc, char** argv) {
    Options options;
    if (argc == 5 && argv[1][0] != '-') {
        options.config.capacity_bytes = std::stoull(argv[1]);
        options.config.block_bytes = std::stoull(argv[2]);
        options.config.ways = std::stoull(argv[3]);
        options.trace_file = argv[4];
        return options;
    }
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        auto value = [&](const std::string& name) -> std::string {
            if (index + 1 >= argc) throw std::invalid_argument("missing value for " + name);
            return argv[++index];
        };
        if (arg == "--trace") options.trace_file = value(arg);
        else if (arg == "--capacity") options.config.capacity_bytes = std::stoull(value(arg));
        else if (arg == "--block") options.config.block_bytes = std::stoull(value(arg));
        else if (arg == "--ways") options.config.ways = std::stoull(value(arg));
        else if (arg == "--policy") options.config.replacement = cachecraft::replacement_policy_from_string(value(arg));
        else if (arg == "--write-policy") options.config.write_policy = cachecraft::write_policy_from_string(value(arg));
        else if (arg == "--allocation") options.config.allocation_policy = cachecraft::allocation_policy_from_string(value(arg));
        else if (arg == "--hit-latency") options.config.hit_latency = std::stod(value(arg));
        else if (arg == "--miss-penalty") options.config.miss_penalty = std::stod(value(arg));
        else if (arg == "--seed") options.config.random_seed = static_cast<std::uint32_t>(std::stoul(value(arg)));
        else if (arg == "--dump-set") options.dump_set = std::stoull(value(arg));
        else if (arg == "--json") options.json = true;
        else if (arg == "--verbose") options.verbose = true;
        else if (arg == "--compare") options.compare = true;
        else if (arg == "--help" || arg == "-h") { usage(); std::exit(0); }
        else throw std::invalid_argument("unknown option: " + arg);
    }
    if (options.trace_file.empty()) throw std::invalid_argument("--trace is required");
    return options;
}

std::vector<cachecraft::TraceRecord> load_trace(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("unable to open trace file: " + path);
    return cachecraft::read_trace(input);
}

void simulate(cachecraft::Cache& cache, const std::vector<cachecraft::TraceRecord>& records, bool verbose) {
    for (const auto& record : records) {
        const auto first_block = record.address / cache.config().block_bytes;
        const auto last_block = (record.address + record.size - 1) / cache.config().block_bytes;
        for (auto block = first_block; block <= last_block; ++block) {
            const auto address = block * cache.config().block_bytes;
            const auto result = cache.access(address, record.type);
            if (verbose) {
                std::cout << "line " << record.line_number << " " << cachecraft::to_string(record.type)
                          << " 0x" << std::hex << address << std::dec << " "
                          << (result.hit ? "hit" : "miss") << " set=" << result.set_index
                          << " way=" << result.way << " miss_type=" << cachecraft::to_string(result.miss_type)
                          << (result.writeback ? " writeback" : "") << "\n";
            }
        }
    }
}

void print_human(const cachecraft::Cache& cache) {
    const auto& config = cache.config();
    const auto& stats = cache.statistics();
    std::cout << "CacheCraft 2.0 simulation\n"
              << "Configuration: " << config.capacity_bytes << " B, " << config.block_bytes << " B blocks, "
              << config.ways << "-way, " << cachecraft::to_string(config.replacement) << ", "
              << cachecraft::to_string(config.write_policy) << ", " << cachecraft::to_string(config.allocation_policy) << "\n"
              << "Sets: " << cache.set_count() << " | lines: " << cache.line_count() << "\n"
              << "Accesses: " << stats.accesses << " (reads " << stats.reads << ", writes " << stats.writes
              << ", instructions " << stats.instructions << ")\n"
              << "Hits: " << stats.hits << " | misses: " << stats.misses << " | evictions: " << stats.evictions << "\n"
              << "Misses: compulsory " << stats.compulsory_misses << ", conflict " << stats.conflict_misses
              << ", capacity " << stats.capacity_misses << "\n"
              << "Memory reads: " << stats.memory_reads << " | memory writes: " << stats.memory_writes
              << " | writebacks: " << stats.writebacks << "\n"
              << "Bytes fetched: " << stats.bytes_fetched << "\n"
              << "Hit rate: " << std::fixed << std::setprecision(2) << stats.hit_rate() * 100.0 << "%\n"
              << "AMAT: " << stats.average_memory_access_time(config.hit_latency, config.miss_penalty) << " cycles\n";
}

void print_json(const cachecraft::Cache& cache) {
    const auto& config = cache.config();
    const auto& s = cache.statistics();
    std::cout << std::fixed << std::setprecision(6)
              << "{\n"
              << "  \"config\": {\"capacity_bytes\": " << config.capacity_bytes << ", \"block_bytes\": " << config.block_bytes
              << ", \"ways\": " << config.ways << ", \"sets\": " << cache.set_count()
              << ", \"replacement\": \"" << cachecraft::to_string(config.replacement) << "\", \"write_policy\": \""
              << cachecraft::to_string(config.write_policy) << "\", \"allocation_policy\": \""
              << cachecraft::to_string(config.allocation_policy) << "\"},\n"
              << "  \"statistics\": {\"accesses\": " << s.accesses << ", \"hits\": " << s.hits << ", \"misses\": " << s.misses
              << ", \"hit_rate\": " << s.hit_rate() << ", \"evictions\": " << s.evictions << ", \"writebacks\": " << s.writebacks
              << ", \"memory_reads\": " << s.memory_reads << ", \"memory_writes\": " << s.memory_writes
              << ", \"compulsory_misses\": " << s.compulsory_misses << ", \"conflict_misses\": " << s.conflict_misses
              << ", \"capacity_misses\": " << s.capacity_misses << ", \"amat\": "
              << s.average_memory_access_time(config.hit_latency, config.miss_penalty) << "}\n}\n";
}

void compare_policies(cachecraft::CacheConfig config, const std::vector<cachecraft::TraceRecord>& records) {
    std::cout << "policy,hits,misses,hit_rate,evictions,writebacks,amat\n";
    for (const auto policy : {cachecraft::ReplacementPolicy::LRU, cachecraft::ReplacementPolicy::FIFO, cachecraft::ReplacementPolicy::Random}) {
        config.replacement = policy;
        cachecraft::Cache cache(config);
        simulate(cache, records, false);
        const auto& s = cache.statistics();
        std::cout << cachecraft::to_string(policy) << ',' << s.hits << ',' << s.misses << ',' << s.hit_rate() << ','
                  << s.evictions << ',' << s.writebacks << ','
                  << s.average_memory_access_time(config.hit_latency, config.miss_penalty) << '\n';
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const auto options = parse_options(argc, argv);
        const auto records = load_trace(options.trace_file);
        if (options.compare) {
            compare_policies(options.config, records);
            return 0;
        }
        cachecraft::Cache cache(options.config);
        simulate(cache, records, options.verbose);
        if (options.json) print_json(cache);
        else print_human(cache);
        if (options.dump_set != static_cast<std::size_t>(-1)) {
            const auto lines = cache.set_state(options.dump_set);
            for (std::size_t way = 0; way < lines.size(); ++way) {
                const auto& line = lines[way];
                std::cout << "set " << options.dump_set << " way " << way << ": valid=" << line.valid
                          << " dirty=" << line.dirty << " tag=0x" << std::hex << line.tag << std::dec
                          << " last_used=" << line.last_used << " inserted=" << line.inserted_at << '\n';
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        usage();
        return 1;
    }
    return 0;
}
