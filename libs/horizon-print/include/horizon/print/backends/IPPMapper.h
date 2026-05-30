#pragma once

#include <horizon/print/Models.h>
#include <string>
#include <map>

namespace horizon::print::backends {

class IPPMapper {
private:
    static std::string mapOptionKey(PrintOption option) {
        switch (option) {
            case PrintOption::MediaType: return "media";
            case PrintOption::Resolution: return "resolution";
            case PrintOption::ColorMode: return "print-color-mode";
            case PrintOption::Sides: return "sides";
            case PrintOption::PageRanges: return "page-ranges";
            default: return "unknown-option";
        }
    }

public:
    // Convierte PrintConfig en pares de strings que cupsAddOption puede consumir directamente
    static std::map<std::string, std::string> toCUPSOptions(const PrintConfig& config) {
        std::map<std::string, std::string> cupsOptions;

        if (config.copies.has_value()) {
            cupsOptions["copies"] = std::to_string(config.copies.value());
        }

        if (config.duplex.has_value()) {
            cupsOptions["sides"] = config.duplex.value() ? "two-sided-long-edge" : "one-sided";
        }
        
        if (!config.paper_size.empty()) {
            cupsOptions["media"] = config.paper_size;
        }

        switch (config.orientation) {
            case Orientation::Portrait: cupsOptions["orientation-requested"] = "3"; break;
            case Orientation::Landscape: cupsOptions["orientation-requested"] = "4"; break;
        }

        switch (config.quality) {
            case PrintQuality::Draft: cupsOptions["print-quality"] = "3"; break;
            case PrintQuality::Normal: cupsOptions["print-quality"] = "4"; break;
            case PrintQuality::High: cupsOptions["print-quality"] = "5"; break;
        }
        
        if (!config.page_ranges.empty()) {
            cupsOptions["page-ranges"] = config.page_ranges;
        }
        
        if (config.page_set != "all") {
            cupsOptions["page-set"] = config.page_set;
        }

        for (const auto& [option, value] : config.extra) {
            std::string key = mapOptionKey(option);
            if (key != "unknown-option") {
                cupsOptions[key] = value;
            }
        }

        return cupsOptions;
    }
};

} // namespace horizon::print::backends
