#include <views/DisplayView/WayfireAdapter.hpp>
#include <horizon/Logger.hpp>
#include <sstream>
#include <cstdlib>

namespace horizon::preferences
{
    void WayfireAdapter::apply_configs(const std::vector<MonitorConfig>& configs)
    {
        LOG_INFO << "[ADAPTER] Applying configs for Wayfire...";

        // Wayfire is wlroots-based and supports wlr-randr.
        // Similar to Labwc for now.
        for (const auto& config : configs)
        {
            std::stringstream ss;
            ss << "wlr-randr --output " << config.name;
            
            if (config.enabled) {
                ss << " --mode " << config.width << "x" << config.height << "@" << config.refresh;
                ss << " --pos " << config.x << "," << config.y;
                
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
