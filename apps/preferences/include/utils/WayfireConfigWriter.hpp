#pragma once
#include <views/DisplayView/IDisplayAdapter.hpp>
#include <vector>

namespace horizon::preferences
{
    /**
     * @class WayfireConfigWriter
     * @brief Helper for updating Wayfire config files from monitor configurations.
     */
    class WayfireConfigWriter
    {
    public:
        /**
         * @brief Updates wayfire.ini with the provided monitor settings.
         * @param configs The list of monitor configurations.
         */
        static void update_monitor_config(const std::vector<MonitorConfig>& configs);
    };
}
