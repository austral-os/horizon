#include <views/DisplayView/LabwcAdapter.hpp>
#include <horizon/Logger.hpp>
#include <sstream>
#include <cstdlib>

namespace horizon::preferences
{
    void LabwcAdapter::apply_configs(const std::vector<MonitorConfig>& configs)
    {
        LOG_INFO << "[ADAPTER] Applying configs for Labwc...";

        // Check if wlr-randr exists
        if (std::system("which wlr-randr > /dev/null 2>&1") != 0) {
            LOG_ERROR << "[ADAPTER] wlr-randr not found! Resolution changes in Labwc require wlr-randr.";
            return;
        }
        
        for (const auto& config : configs)
        {
            std::stringstream ss;
            ss << "wlr-randr --output " << config.name;
            
            if (config.enabled) {
                // Some versions of wlr-randr are picky about the refresh rate format.
                // We'll try just the dimensions first as it's more compatible.
                ss << " --mode " << config.width << "x" << config.height;
                // If you want to specify refresh, it often needs to be exact match (e.g. 59.940)
                // so we skip it for now to increase chance of success.
                ss << " --pos " << config.x << "," << config.y;
                
                // Rotation mapping
                std::string rot = "normal";
                if (config.rotation == 90) rot = "90";
                else if (config.rotation == 180) rot = "180";
                else if (config.rotation == 270) rot = "270";
                ss << " --transform " << rot;
            } else {
                ss << " --off";
            }

            LOG_INFO << "[ADAPTER] Executing: " << ss.str();
            int ret = std::system(ss.str().c_str());
            if (ret != 0) {
                LOG_ERROR << "[ADAPTER] Failed to apply config for " << config.name;
            }
        }
    }
}
