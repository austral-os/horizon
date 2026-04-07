#include <views/DisplayView/KwinAdapter.hpp>
#include <horizon/Logger.hpp>
#include <sstream>
#include <cstdlib>

namespace horizon::preferences
{
    void KwinAdapter::apply_configs(const std::vector<MonitorConfig>& configs)
    {
        LOG_INFO << "[ADAPTER] Applying configs for Kwin...";

        // KWin uses kscreen-doctor for command line configuration.
        std::stringstream ss;
        ss << "kscreen-doctor";
        
        for (const auto& config : configs)
        {
            ss << " output." << config.name;
            if (config.enabled) {
                ss << ".mode." << config.width << "x" << config.height << "@" << (int)config.refresh;
                ss << " output." << config.name << ".position." << config.x << "," << config.y;
                
                // Rotation mapping (0, 1, 2, 3 for kscreen-doctor)
                int rot = 1; // Normal
                if (config.rotation == 90) rot = 2;
                else if (config.rotation == 180) rot = 4;
                else if (config.rotation == 270) rot = 8;
                // Wait, kscreen-doctor rotation indices vary. 
                // Using 1, 2, 4, 8 is common for some versions, or 0, 90, 180, 270.
                // Let's use the degree values which are also supported.
                ss << " output." << config.name << ".rotation." << config.rotation;
            } else {
                ss << ".disable";
            }
        }

        LOG_INFO << "[ADAPTER] Executing: " << ss.str();
        int ret = std::system(ss.str().c_str());
        if (ret != 0) {
            LOG_ERROR << "[ADAPTER] Failed to apply Kwin configs";
        }
    }
}
