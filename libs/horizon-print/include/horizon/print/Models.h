#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <optional>

namespace horizon::print {

enum class JobState {
    QUEUED,
    RUNNING,
    DONE,
    ERROR
};

enum class PrinterSource {
    Discovered,
    Installed
};

using PrinterId = std::string;
using JobId = std::string;

struct Printer {
    PrinterId id;
    std::string name;
    std::string uri;
    PrinterSource source;
};

enum class PrintOption {
    MediaType,
    Resolution,
    ColorMode,
    Sides,
    PageRanges
};

struct PrintConfig {
    std::string ppdPath;
    std::optional<int> copies;
    std::optional<bool> duplex;
    double paper_width_mm{0.0};
    double paper_height_mm{0.0};
    // Keys defined strongly to avoid typos
    std::unordered_map<PrintOption, std::string> extra;
};

struct PrintDocument {
    std::vector<uint8_t> data;
    std::string mime_type;
    std::string filename;
    std::string title;

    // Minimal validation to prevent passing completely broken buffers
    bool isValid() const {
        if (data.size() < 5) return false;
        // Basic %PDF- magic number check
        return data[0] == '%' && data[1] == 'P' && data[2] == 'D' && data[3] == 'F' && data[4] == '-';
    }
};

} // namespace horizon::print
