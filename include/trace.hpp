#pragma once

#include "cache.hpp"

#include <cstdint>
#include <istream>
#include <optional>
#include <string>
#include <vector>

namespace cachecraft {

struct TraceRecord {
    AccessType type{AccessType::Read};
    std::uint64_t address{0};
    std::size_t size{1};
    std::string source;
    std::size_t line_number{0};
};

std::optional<TraceRecord> parse_trace_line(const std::string& line, std::size_t line_number = 0);
std::vector<TraceRecord> read_trace(std::istream& input);

}  // namespace cachecraft
