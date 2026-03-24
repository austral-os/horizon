#pragma once
#include <string>
#include <vector>

namespace horizon::preferences
{
    /**
     * @struct MonitorConfig
     * @brief Represents the desired configuration for a single monitor.
     */
    struct MonitorConfig
    {
        std::string name;   // Output name (e.g., "eDP-1", "HDMI-A-1")
        int x;              // X position in logical coordinates
        int y;              // Y position in logical coordinates
        int width;          // Resolution width
        int height;         // Resolution height
        float refresh;      // Refresh rate in Hz
        bool enabled;       // Whether the monitor is active
        int rotation;       // 0, 90, 180, 270
    };

    /**
     * @class IDisplayAdapter
     * @brief Interface for applying display settings across different compositors.
     */
    class IDisplayAdapter
    {
    public:
        virtual ~IDisplayAdapter() = default;

        /**
         * @brief Apply the provided monitor configurations to the system.
         * @param configs Vector of monitor configurations.
         */
        virtual void apply_configs(const std::vector<MonitorConfig>& configs) = 0;
    };
}
