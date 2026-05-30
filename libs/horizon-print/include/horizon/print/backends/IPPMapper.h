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
