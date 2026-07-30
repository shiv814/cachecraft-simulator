#include "cache.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "Usage: cachecraft_cli <capacity_bytes> <block_bytes> <ways> <trace_file>\n";
        return 1;
    }
    try {
        cachecraft::Cache cache(std::stoull(argv[1]), std::stoull(argv[2]), std::stoull(argv[3]));
        std::ifstream trace(argv[4]);
        if (!trace) {
            std::cerr << "Unable to open trace file\n";
            return 2;
        }
        std::string line;
        while (std::getline(trace, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::uint64_t address = 0;
            std::stringstream parser(line);
            parser >> std::hex >> address;
            if (!parser.fail()) cache.access(address);
        }
        const auto& stats = cache.statistics();
        std::cout << "Accesses: " << stats.accesses << "\n"
                  << "Hits: " << stats.hits << "\n"
                  << "Misses: " << stats.misses << "\n"
                  << "Evictions: " << stats.evictions << "\n"
                  << "Hit rate: " << std::fixed << std::setprecision(2) << stats.hit_rate() * 100.0 << "%\n";
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 3;
    }
    return 0;
}
