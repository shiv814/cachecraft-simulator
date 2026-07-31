#include "trace.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

namespace cachecraft {
namespace {

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

AccessType parse_type(std::string token) {
    std::transform(token.begin(), token.end(), token.begin(), [](unsigned char ch) {
        return static_cast<char>(std::toupper(ch));
    });
    if (token == "R" || token == "READ" || token == "L") return AccessType::Read;
    if (token == "W" || token == "WRITE" || token == "S") return AccessType::Write;
    if (token == "I" || token == "IFETCH") return AccessType::Instruction;
    throw std::invalid_argument("unknown access type: " + token);
}

}  // namespace

std::optional<TraceRecord> parse_trace_line(const std::string& raw, std::size_t line_number) {
    auto line = trim(raw);
    if (line.empty() || line[0] == '#') return std::nullopt;
    const auto comment = line.find('#');
    if (comment != std::string::npos) line = trim(line.substr(0, comment));
    std::replace(line.begin(), line.end(), ',', ' ');

    std::stringstream parser(line);
    std::string first;
    std::string address_token;
    std::size_t size = 1;
    parser >> first;
    if (first.empty()) return std::nullopt;

    AccessType type = AccessType::Read;
    if (first == "R" || first == "r" || first == "W" || first == "w" || first == "I" || first == "i" ||
        first == "READ" || first == "read" || first == "WRITE" || first == "write") {
        type = parse_type(first);
        parser >> address_token;
    } else {
        address_token = first;
    }
    if (address_token.empty()) throw std::invalid_argument("missing address on trace line " + std::to_string(line_number));
    if (parser >> size) {
        if (size == 0) throw std::invalid_argument("access size must be positive on trace line " + std::to_string(line_number));
    }

    std::size_t consumed = 0;
    const auto address = std::stoull(address_token, &consumed, 0);
    if (consumed != address_token.size()) throw std::invalid_argument("invalid address on trace line " + std::to_string(line_number));
    return TraceRecord{type, address, size, raw, line_number};
}

std::vector<TraceRecord> read_trace(std::istream& input) {
    std::vector<TraceRecord> records;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        auto record = parse_trace_line(line, line_number);
        if (record) records.push_back(*record);
    }
    return records;
}

}  // namespace cachecraft
